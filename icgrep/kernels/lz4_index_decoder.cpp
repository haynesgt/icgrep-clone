/*
 *  Copyright (c) 2017 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#include "lz4_index_decoder.h"
#include <kernels/kernel_builder.h>
 
using namespace llvm;
using namespace kernel;

#ifndef NDEBUG
#define DEBUG_RT_PRINT 1
#else
#define DEBUG_RT_PRINT 0
#endif

#define printRTDebugMsg(MSG) \
    do { if (DEBUG_RT_PRINT) iBuilder->CallPrintMsgToStderr(MSG); } while (0)

#define printRTDebugInt(NAME, X) \
    do { if (DEBUG_RT_PRINT) iBuilder->CallPrintIntToStderr(NAME, X); } while (0)

#define printGlobalPos() \
    printRTDebugInt("GlobalPos", iBuilder->CreateAdd(blockStartPos, iBuilder->CreateLoad(sOffset)))

namespace {

Value * generateBitswap(const std::unique_ptr<KernelBuilder> & iBuilder, Value * v) {
    Value * bswapFunc = Intrinsic::getDeclaration(iBuilder->getModule(),
            Intrinsic::bswap, v->getType());
    return iBuilder->CreateCall(bswapFunc, {v});
}

Value * selectMin(const std::unique_ptr<KernelBuilder> & iBuilder, Value * a, Value * b) {
    return iBuilder->CreateSelect(iBuilder->CreateICmpULT(a, b), a, b);
}

Value * createStackVar(const std::unique_ptr<KernelBuilder> & iBuilder, Type * type, StringRef name, Value * initializer = nullptr) {
    Value * var = iBuilder->CreateAlloca(type, nullptr, name);
    if (initializer) {
        iBuilder->CreateStore(initializer, var);
    } else {
        iBuilder->CreateStore(ConstantInt::get(type, 0), var);
    }
    return var;
}

void incStackVar(const std::unique_ptr<KernelBuilder> & iBuilder, Value * svar, Value * increment = nullptr) {
    Value * value = iBuilder->CreateLoad(svar);
    if (increment) {
        value = iBuilder->CreateAdd(value, increment);
    } else {
        value = iBuilder->CreateAdd(value, ConstantInt::get(value->getType(), 1));
    }
    iBuilder->CreateStore(value, svar);
}

Value * getOutputPtr(const std::unique_ptr<KernelBuilder> & iBuilder, Value * blockStartPtr, Value * offset) {
    return iBuilder->CreateGEP(
            iBuilder->CreatePointerCast(blockStartPtr, iBuilder->getInt32Ty()->getPointerTo()),
            offset
            );
}

}       // anonymouse namespace

/**
 * Get the offset within the current word.
 */
Value * LZ4IndexDecoderKernel::getWordOffset(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) {
    Value * offset = iBuilder->CreateLoad(sOffset);
    IntegerType * type = cast<IntegerType>(offset->getType());
    Constant * mask = ConstantInt::get(type, wordWidth - 1);
    return iBuilder->CreateAnd(offset, mask);
}

/**
 * Get the offset of the start of the current word.
 */
Value * LZ4IndexDecoderKernel::getWordStartOffset(const std::unique_ptr<KernelBuilder> & iBuilder) {
    Value * offset = iBuilder->CreateLoad(sOffset);
    IntegerType * type = cast<IntegerType>(offset->getType());
    Constant * mask = ConstantExpr::getNeg(ConstantInt::get(type, wordWidth));
    return iBuilder->CreateAnd(offset, mask);
}

/**
 * Load a raw byte from byteStream.
 * If offset is not provided, load the current byte by default.
 */
Value * LZ4IndexDecoderKernel::loadRawByte(const std::unique_ptr<KernelBuilder> & iBuilder, Value * offset) {
    Value * blockStartPtr = iBuilder->CreatePointerCast(
            iBuilder->getInputStreamBlockPtr("byteStream", iBuilder->getInt32(0)),
            iBuilder->getInt8PtrTy()
            );
    if (offset == nullptr) {
        offset = iBuilder->CreateLoad(sOffset);
    }
    Value * ptr = iBuilder->CreateGEP(blockStartPtr, offset);
    return iBuilder->CreateLoad(ptr);
}


/**
 * Set the current extender word up until before the offset position.
 * extender = .......  (little-endian, LSB on the right)
 * offset   =    ^
 * cleared  = ....111
 */
void LZ4IndexDecoderKernel::setExtenderUntilOffset(const std::unique_ptr<KernelBuilder> & iBuilder) {
    // Little-endian, offset counts from LSB
    // extender = extender ^ ~((1 << offset) -1)
    Value * extender = iBuilder->CreateLoad(sExtender);
    Value * wordOffset = iBuilder->CreateZExt(
            getWordOffset(iBuilder),
            iBuilder->getSizeTy()
            );
    Value * one = iBuilder->getSize(1);
    Value * mask = iBuilder->CreateSub(
            iBuilder->CreateShl(one, wordOffset),
            one);
    extender = iBuilder->CreateOr(extender, mask);
    iBuilder->CreateStore(extender, sExtender);
}


/**
 * Load the extender word at the current offset.
 * Called when we potentially reach a new word.  Usually followed by setExtenderUntilOffset.
 */
void LZ4IndexDecoderKernel::loadCurrentExtender(const std::unique_ptr<KernelBuilder> & iBuilder) {
    Value * offset = iBuilder->CreateLoad(sOffset);
    IntegerType * type = cast<IntegerType>(offset->getType());
    ConstantInt * shift = ConstantInt::get(type, std::log2(wordWidth));
    Value * shiftedOffset = iBuilder->CreateLShr(offset, shift);
    Value * extender = iBuilder->CreateExtractElement(extenders, shiftedOffset);
    iBuilder->CreateStore(extender, sExtender);
}


void LZ4IndexDecoderKernel::generateProduceOutput(const std::unique_ptr<KernelBuilder> &iBuilder) {
    Value * producedItem = iBuilder->getProducedItemCount("literalIndexes");

#ifndef NDEBUG
    iBuilder->CallPrintInt("ProducedItem", producedItem);
    // LiteralStart is adjusted to be relative to the block start, so that
    // the output can be compared against that of the reference implementation.
    Value * literalStart = iBuilder->CreateSub(iBuilder->getScalarField("LiteralStart"), iBuilder->getScalarField("LZ4BlockStart"));
    iBuilder->CallPrintInt("LiteralStart", literalStart);
    iBuilder->CallPrintInt("LiteralLength", iBuilder->getScalarField("LiteralLength"));
    iBuilder->CallPrintInt("MatchOffset", iBuilder->getScalarField("MatchOffset"));
    iBuilder->CallPrintInt("MatchLength", iBuilder->getScalarField("MatchLength"));
#endif
    printRTDebugMsg("--------------");

    Value * outputOffset = iBuilder->CreateAnd(
            iBuilder->CreateTrunc(producedItem, iBuilder->getInt32Ty()),
            iBuilder->getInt32(iBuilder->getBitBlockWidth() - 1)
            );  // producedItem % blockWidth (as blockWidth is always a power of 2)
    Value * literalStartPtr = getOutputPtr(iBuilder,
            iBuilder->getOutputStreamBlockPtr("literalIndexes", iBuilder->getInt32(0)), outputOffset);
    Value * literalLengthPtr = getOutputPtr(iBuilder,
            iBuilder->getOutputStreamBlockPtr("literalIndexes", iBuilder->getInt32(1)), outputOffset);
    Value * matchOffsetPtr = getOutputPtr(iBuilder,
            iBuilder->getOutputStreamBlockPtr("matchIndexes", iBuilder->getInt32(0)), outputOffset);
    Value * matchLengthPtr = getOutputPtr(iBuilder,
            iBuilder->getOutputStreamBlockPtr("matchIndexes", iBuilder->getInt32(1)), outputOffset);
    iBuilder->CreateStore(iBuilder->getScalarField("LiteralStart"), literalStartPtr);
    iBuilder->CreateStore(iBuilder->getScalarField("LiteralLength"), literalLengthPtr);
    iBuilder->CreateStore(iBuilder->getScalarField("MatchOffset"), matchOffsetPtr);
    iBuilder->CreateStore(iBuilder->getScalarField("MatchLength"), matchLengthPtr);
    iBuilder->setProducedItemCount("literalIndexes", iBuilder->CreateAdd(producedItem, iBuilder->getSize(1)));
    // matchIndexes has a fixed ratio of 1:1 w.r.t. literalIndexes.
}


void LZ4IndexDecoderKernel::generateDoBlockMethod(const std::unique_ptr<KernelBuilder> & iBuilder) {
    BasicBlock * entry_block = iBuilder->GetInsertBlock();
    BasicBlock * exit_block = iBuilder->CreateBasicBlock("exit");

    // %entry
    iBuilder->SetInsertPoint(entry_block);
    printRTDebugMsg("entry");
    // Global positions in the byte stream.
    Value * blockNo = iBuilder->getScalarField("BlockNo");
    blockStartPos = iBuilder->CreateMul(blockNo, iBuilder->getInt32(iBuilder->getBitBlockWidth()), "blockStartPos");
    extenders = iBuilder->CreateBitCast(
            iBuilder->loadInputStreamBlock("extenders", iBuilder->getInt32(0)),
            VectorType::get(iBuilder->getSizeTy(), iBuilder->getBitBlockWidth() / iBuilder->getSizeTy()->getBitWidth()),
            "extenders");
    // Create a series of stack variables which will be promoted by mem2reg.
    sOffset = createStackVar(iBuilder, iBuilder->getInt32Ty(), "offset");
    // tempLength has different meanings in different states.
    sTempLength = createStackVar(iBuilder, iBuilder->getInt32Ty(), "tempLength", iBuilder->getScalarField("TempLength"));
    sTempCount = createStackVar(iBuilder, iBuilder->getInt32Ty(), "tempCount", iBuilder->getScalarField("TempCount"));
    sState = createStackVar(iBuilder, iBuilder->getInt8Ty(), "state", iBuilder->getScalarField("State"));
    sExtender = createStackVar(iBuilder, iBuilder->getSizeTy(), "extender",
            iBuilder->CreateExtractElement(extenders, iBuilder->getInt32(0)));

    BasicBlock * skippingBytes = iBuilder->CreateBasicBlock("skipping_bytes");
    BasicBlock * dispatch = iBuilder->CreateBasicBlock("dispatch");

    iBuilder->CreateCondBr(
            iBuilder->CreateICmpUGT(iBuilder->getScalarField("BytesToSkip"), iBuilder->getInt32(0)),
            skippingBytes, dispatch
            );

    // %skipping_bytes
    generateSkippingBytes(iBuilder, skippingBytes, exit_block);
    // Insert point is at the end of skippingBytes.
    iBuilder->CreateBr(dispatch);

    // %dispatch
    // Indirect branching will be added to %dispatch at last.

    // %at_block_checksum
    BasicBlock * atBlockChecksum = iBuilder->CreateBasicBlock("at_block_checksum");
    generateAtBlockChecksum(iBuilder, atBlockChecksum, skippingBytes);
  
    // %at_block_size
    BasicBlock * atBlockSize = iBuilder->CreateBasicBlock("at_block_size");
    generateAtBlockSize(iBuilder, atBlockSize, skippingBytes, exit_block);

    // %at_token
    BasicBlock * atToken = iBuilder->CreateBasicBlock("at_token");
    generateAtToken(iBuilder, atToken, exit_block);

    // %extending_literal_length
    BasicBlock * extendingLiteralLen = iBuilder->CreateBasicBlock("extending_literal_length");
    generateExtendingLiteralLen(iBuilder, extendingLiteralLen, exit_block);

    // %at_literals
    BasicBlock * atLiterals = iBuilder->CreateBasicBlock("at_literals");
    generateAtLiterals(iBuilder, atLiterals);
    iBuilder->CreateBr(skippingBytes);

    // %at_first_offset
    // Note that the last sequence is incomplete and ends with literals.
    // If the whole LZ4 block is done, process the (optional) checksum.
    // Otherwise, go around to process the next sequence.
    BasicBlock * atOffset1 = iBuilder->CreateBasicBlock("at_first_offset");
    iBuilder->SetInsertPoint(atOffset1);
    Value * nowGlobalPos = iBuilder->CreateAdd(blockStartPos, iBuilder->CreateLoad(sOffset));
    BasicBlock * blockEnd_else = iBuilder->CreateBasicBlock("block_end_else");
    // Conditional branch inserted at the end of the last block.
    iBuilder->CreateUnlikelyCondBr(
            iBuilder->CreateICmpEQ(nowGlobalPos, iBuilder->getScalarField("LZ4BlockEnd")),
            atBlockChecksum, blockEnd_else
            );
    generateAtFirstOffset(iBuilder, blockEnd_else, exit_block);

    // %at_second_offset
    BasicBlock * atOffset2 = iBuilder->CreateBasicBlock("at_second_offset");
    generateAtSecondOffset(iBuilder, atOffset2, exit_block);

    // %extending_match_length
    BasicBlock * extendingMatchLen = iBuilder->CreateBasicBlock("extending_match_length");
    generateExtendingMatchLen(iBuilder, extendingMatchLen, exit_block);
    iBuilder->CreateBr(atToken);

    // Indirect branching.
    iBuilder->SetInsertPoint(dispatch);
    printRTDebugMsg("dispatch");
    // The order must comply with enum State.
    Constant * labels = ConstantVector::get(
            {BlockAddress::get(atBlockSize), BlockAddress::get(atToken), BlockAddress::get(extendingLiteralLen), BlockAddress::get(atLiterals),
             BlockAddress::get(atOffset1), BlockAddress::get(atOffset2), BlockAddress::get(extendingMatchLen), BlockAddress::get(atBlockChecksum)}
            );
    Value * target = iBuilder->CreateExtractElement(labels, iBuilder->CreateLoad(sState));
    IndirectBrInst * indirectBr = iBuilder->CreateIndirectBr(target);
    indirectBr->addDestination(atBlockSize);
    indirectBr->addDestination(atToken);
    indirectBr->addDestination(extendingLiteralLen);
    indirectBr->addDestination(atLiterals);
    indirectBr->addDestination(atOffset1);
    indirectBr->addDestination(atOffset2);
    indirectBr->addDestination(extendingMatchLen);
    indirectBr->addDestination(atBlockChecksum);

    // %exit
    iBuilder->SetInsertPoint(exit_block);
    printRTDebugMsg("exit");
    iBuilder->setScalarField("State", iBuilder->CreateLoad(sState));
    iBuilder->setScalarField("TempLength", iBuilder->CreateLoad(sTempLength));
    iBuilder->setScalarField("TempCount", iBuilder->CreateLoad(sTempCount));
    iBuilder->setScalarField("BlockNo", iBuilder->CreateAdd(blockNo, iBuilder->getInt32(1)));
    // When the kernel builder uses indirectbr, doBlock is not a separate function.
    // Hence, we branch to a new basic block and fall through instead of returning.
    BasicBlock * end_block = iBuilder->CreateBasicBlock("end_of_block");
    iBuilder->CreateBr(end_block);
    iBuilder->SetInsertPoint(end_block);
}


void LZ4IndexDecoderKernel::generateBoundaryDetection(const std::unique_ptr<KernelBuilder> & iBuilder, State state, BasicBlock * exit_block, bool updateExtenderWord) {
    if (updateExtenderWord) {
        BasicBlock * wordBoundary_then = iBuilder->CreateBasicBlock("word_boundary_then-" + StateLabels.at(state));
        BasicBlock * blockBoundary_else = iBuilder->CreateBasicBlock("block_boundary_else-" + StateLabels.at(state));
        BasicBlock * wordBoundary_cont = iBuilder->CreateBasicBlock("word_boundary_cont-" + StateLabels.at(state));
        iBuilder->CreateUnlikelyCondBr(
                iBuilder->CreateICmpEQ(getWordOffset(iBuilder), iBuilder->getInt32(0)),
                wordBoundary_then, wordBoundary_cont
                );

        iBuilder->SetInsertPoint(wordBoundary_then);
        iBuilder->CreateUnlikelyCondBr(
                iBuilder->CreateICmpEQ(iBuilder->CreateLoad(sOffset), iBuilder->getInt32(iBuilder->getBitBlockWidth())),
                exit_block, blockBoundary_else
                );

        // Reaching word boundary but not block boundary.  Update the extender word as requested.
        iBuilder->SetInsertPoint(blockBoundary_else);
        loadCurrentExtender(iBuilder);
        iBuilder->CreateBr(wordBoundary_cont);

        // Leave the insert point at the end and return.
        iBuilder->SetInsertPoint(wordBoundary_cont);
    } else {
        BasicBlock * blockBoundary_cont = iBuilder->CreateBasicBlock("block_boundary_cont-" + StateLabels.at(state));
        iBuilder->CreateUnlikelyCondBr(
                iBuilder->CreateICmpEQ(iBuilder->CreateLoad(sOffset), iBuilder->getInt32(iBuilder->getBitBlockWidth())),
                exit_block, blockBoundary_cont
                );
        // Leave the insert point at the end and return.
        iBuilder->SetInsertPoint(blockBoundary_cont);
    }
}


void LZ4IndexDecoderKernel::generateSkippingBytes(const std::unique_ptr<kernel::KernelBuilder> & iBuilder, BasicBlock * bb, BasicBlock * exit_block) {
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("skipping bytes");

    Value * remainingBytesInBlock = iBuilder->CreateSub(
            iBuilder->getInt32(iBuilder->getBitBlockWidth()), iBuilder->CreateLoad(sOffset)
            );
    Value * remainingBytesToSkip = iBuilder->getScalarField("BytesToSkip");
    Value * advanceDist = selectMin(iBuilder, remainingBytesInBlock, remainingBytesToSkip);
    remainingBytesToSkip = iBuilder->CreateSub(remainingBytesToSkip, advanceDist);
    incStackVar(iBuilder, sOffset, advanceDist);
    iBuilder->setScalarField("BytesToSkip", remainingBytesToSkip);

    generateBoundaryDetection(iBuilder, State::SKIPPING_BYTES, exit_block);
    // Falls through.
}


void LZ4IndexDecoderKernel::generateAtBlockSize(const std::unique_ptr<KernelBuilder> &iBuilder, BasicBlock * bb, BasicBlock * skippingBytes, BasicBlock * exit_block) {
    iBuilder->CreateBr(bb);
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("scanning block size");
    printGlobalPos();

    // Use tempLength to hold the block size temporarily.
    // Note that it is initially stored as big-endian (for the ease of reading) and will be "swapped" later.
    // Use tempCount as the loop counter (0..3).
    // Both variables are initialized from kernel states at %entry.

    // A do-while loop.
    BasicBlock * loopBody = iBuilder->CreateBasicBlock("blocksize_loop_body");
    BasicBlock * loopExit = iBuilder->CreateBasicBlock("blocksize_loop_exit");
    iBuilder->CreateBr(loopBody);

    iBuilder->SetInsertPoint(loopBody);
    Value * byte = loadRawByte(iBuilder);
    Value * newTempLength = iBuilder->CreateAdd(
            iBuilder->CreateShl(iBuilder->CreateLoad(sTempLength), iBuilder->getInt32(8)),
            iBuilder->CreateZExt(byte, iBuilder->getInt32Ty())
            );
    iBuilder->CreateStore(newTempLength, sTempLength);
    incStackVar(iBuilder, sTempCount);
    incStackVar(iBuilder, sOffset);
    // Stop when we read all four bytes or reach the end of the block.
    iBuilder->CreateCondBr(
            iBuilder->CreateOr(
                iBuilder->CreateICmpEQ(iBuilder->CreateLoad(sTempCount), iBuilder->getInt32(4)),
                iBuilder->CreateICmpEQ(iBuilder->CreateLoad(sOffset), iBuilder->getInt32(iBuilder->getBitBlockWidth()))
                ),
            loopExit, loopBody
            );

    iBuilder->SetInsertPoint(loopExit);
    BasicBlock * blockSizeCompleted_then = iBuilder->CreateBasicBlock("blocksize_completed_then");
    BasicBlock * blockSizeCompleted_cont = iBuilder->CreateBasicBlock("blocksize_completed_cont");
    iBuilder->CreateLikelyCondBr(
            iBuilder->CreateICmpEQ(iBuilder->CreateLoad(sTempCount), iBuilder->getInt32(4)),
            blockSizeCompleted_then, blockSizeCompleted_cont
            );

    // All four bytes of the block size are read in.
    iBuilder->SetInsertPoint(blockSizeCompleted_then);
    // Remember to swap the block size back to little-endian.
    Value * blockSize = generateBitswap(iBuilder, iBuilder->CreateLoad(sTempLength));
    Value * currentPos = iBuilder->CreateAdd(blockStartPos, iBuilder->CreateLoad(sOffset));
    iBuilder->setScalarField("LZ4BlockStart", currentPos);
    iBuilder->setScalarField("LZ4BlockEnd", iBuilder->CreateAdd(currentPos, blockSize));
    printRTDebugInt("blockSize", blockSize);

    BasicBlock * uncompressedBlock_then = iBuilder->CreateBasicBlock("uncompressed_block_then");
    BasicBlock * uncompressedBlock_else = iBuilder->CreateBasicBlock("uncompressed_block_cont");
    iBuilder->CreateUnlikelyCondBr(
            iBuilder->CreateTrunc(
                iBuilder->CreateLShr(blockSize, iBuilder->getInt32(31)),
                iBuilder->getInt1Ty()
                ),
            uncompressedBlock_then,
            uncompressedBlock_else
            );

    iBuilder->SetInsertPoint(uncompressedBlock_then);
    Value * realBlockSize = iBuilder->CreateXor(blockSize, iBuilder->getInt32(1L << 31));
    iBuilder->setScalarField("LZ4BlockEnd", iBuilder->CreateAdd(currentPos, realBlockSize));
    iBuilder->setScalarField("BytesToSkip", realBlockSize);
    iBuilder->setScalarField("LiteralStart", currentPos);
    iBuilder->setScalarField("LiteralLength", realBlockSize);
    // No need to set MatchLength/MatchOffset to 0, nor to produce output,
    // because %atBlockChecksum will do so as the last sequence.
    iBuilder->CreateStore(iBuilder->getInt8(State::AT_BLOCK_CHECKSUM), sState);
    iBuilder->CreateBr(skippingBytes);

    iBuilder->SetInsertPoint(uncompressedBlock_else);
    // Reset these temporary values for later use.
    iBuilder->CreateStore(iBuilder->getInt32(0), sTempLength);
    iBuilder->CreateStore(iBuilder->getInt32(0), sTempCount);
    iBuilder->CreateStore(iBuilder->getInt8(State::AT_TOKEN), sState);
    // A block size of 0 is the end mark of the frame. Exit.
    iBuilder->CreateUnlikelyCondBr(
            iBuilder->CreateICmpEQ(blockSize, ConstantInt::getNullValue(blockSize->getType())),
            exit_block,
            blockSizeCompleted_cont
            );

    // We could be at the boundary no matter the block size is completed or not.
    iBuilder->SetInsertPoint(blockSizeCompleted_cont);
    generateBoundaryDetection(iBuilder, State::AT_BLOCK_SIZE, exit_block);
    // Falls through to %at_token.
}


void LZ4IndexDecoderKernel::generateAtToken(const std::unique_ptr<kernel::KernelBuilder> & iBuilder, BasicBlock * bb, BasicBlock * exit_block) {
    iBuilder->CreateBr(bb);
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("reading token");

    Value * token = loadRawByte(iBuilder);
    Value * literalLen = iBuilder->CreateZExt(
        iBuilder->CreateLShr(token, iBuilder->getInt8(4)),
        iBuilder->getInt32Ty()
        );
    Value * matchLen = iBuilder->CreateZExt(
        iBuilder->CreateAnd(token, iBuilder->getInt8(0xf)),
        iBuilder->getInt32Ty()
        );
    incStackVar(iBuilder, sOffset);
    // Prepare extender word for scanning.
    loadCurrentExtender(iBuilder);
    setExtenderUntilOffset(iBuilder);
    // Store the (partial) match length to be extended later.
    iBuilder->setScalarField("MatchLength", matchLen);
    // Use tempLength to accumulate extended lengths (until at_literals).
    iBuilder->CreateStore(literalLen, sTempLength);
    iBuilder->CreateStore(iBuilder->getInt8(State::EXTENDING_LITERAL_LENGTH), sState);

    generateBoundaryDetection(iBuilder, State::AT_TOKEN, exit_block);
    // Falls through to %extending_literal_length.
}


void LZ4IndexDecoderKernel::generateExtendingLiteralLen(const std::unique_ptr<KernelBuilder> & iBuilder, BasicBlock * bb, BasicBlock * exit_block) {
    iBuilder->CreateBr(bb);
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("extending literal len");

    Value * wordOffset = getWordOffset(iBuilder);
    Value * blockOffset = getWordStartOffset(iBuilder);
    Value * literalLen = iBuilder->CreateLoad(sTempLength);
    Value * literalExtEnd = iBuilder->CreateTrunc(
                iBuilder->CreateCountForwardZeroes(iBuilder->CreateNot(iBuilder->CreateLoad(sExtender))),
                iBuilder->getInt32Ty());
    printRTDebugInt("wordOffset", wordOffset);
    printRTDebugInt("literalExtEnd", literalExtEnd);
    // number of extender = literalExtEnd - wordOffset
    Value * numExtenders = iBuilder->CreateSub(literalExtEnd, wordOffset);
    Value * literalExtReachBoundary =
            iBuilder->CreateICmpEQ(literalExtEnd, iBuilder->getInt32(wordWidth));
    // There are literalExtEnd forward zeroes, we load bytes[literalExtEnd]
    // which is the first non-extender.  If literalExtEnd == 64, we force the
    // load index to be 0 to avoid out-of-bound access, and lastByte will be 0.
    Value * loadOffset = iBuilder->CreateSelect(literalExtReachBoundary,
            ConstantInt::getNullValue(literalExtEnd->getType()),
            literalExtEnd);
    Value * lastByte = iBuilder->CreateSelect(literalExtReachBoundary,
            iBuilder->getInt8(0),
            loadRawByte(iBuilder, iBuilder->CreateAdd(blockOffset, loadOffset)));
    Value * literalLenExted = iBuilder->CreateICmpUGE(literalLen, iBuilder->getInt32(0xf));
    literalLen = iBuilder->CreateSelect(literalLenExted,
            iBuilder->CreateAdd(
                literalLen,
                iBuilder->CreateAdd(
                    iBuilder->CreateMul(numExtenders, iBuilder->getInt32(0xff)),
                    iBuilder->CreateZExt(lastByte, iBuilder->getInt32Ty())
                    )
                ),      // literalLen + numExtenders * 255
            literalLen);
    wordOffset = iBuilder->CreateSelect(literalLenExted,
            literalExtEnd,
            wordOffset);
    // If lastByte is truly the last length byte, we need to advance the cursor by 1.
    wordOffset = iBuilder->CreateSelect(
            iBuilder->CreateAnd(literalLenExted, iBuilder->CreateNot(literalExtReachBoundary)),
            iBuilder->CreateAdd(wordOffset, iBuilder->getInt32(1)),
            wordOffset
            );
    iBuilder->CreateStore(literalLen, sTempLength);
    iBuilder->CreateStore(iBuilder->CreateAdd(blockOffset, wordOffset), sOffset);
    Value * unfinished = iBuilder->CreateAnd(literalExtReachBoundary, literalLenExted);
    Value * newState = iBuilder->CreateSelect(unfinished,
            iBuilder->getInt8(State::EXTENDING_LITERAL_LENGTH),
            iBuilder->getInt8(State::AT_LITERALS));
    iBuilder->CreateStore(newState, sState);

    generateBoundaryDetection(iBuilder, State::EXTENDING_LITERAL_LENGTH, exit_block, true);
    BasicBlock * cont_block = iBuilder->CreateBasicBlock("finished_" + StateLabels.at(State::EXTENDING_LITERAL_LENGTH));
    // Insert point is still in wordBoundary block now.
    // See if there are still more extenders.
    iBuilder->CreateUnlikelyCondBr(unfinished, bb, cont_block);

    iBuilder->SetInsertPoint(cont_block);
    // Falls through to %at_literals.
}


void LZ4IndexDecoderKernel::generateAtLiterals(const std::unique_ptr<KernelBuilder> & iBuilder, BasicBlock * bb) {
    iBuilder->CreateBr(bb);
    iBuilder->SetInsertPoint(bb);

    iBuilder->setScalarField("LiteralStart", iBuilder->CreateAdd(blockStartPos, iBuilder->CreateLoad(sOffset)));
    iBuilder->setScalarField("LiteralLength", iBuilder->CreateLoad(sTempLength));
    iBuilder->setScalarField("BytesToSkip", iBuilder->CreateLoad(sTempLength));
    iBuilder->CreateStore(iBuilder->getInt8(State::AT_FIRST_OFFSET), sState);

    // No boundary detection here as we do not advance the cursor.
    // Control flow will be redirected to %skipping_bytes later.
}


void LZ4IndexDecoderKernel::generateAtFirstOffset(const std::unique_ptr<KernelBuilder> &iBuilder, BasicBlock * bb, BasicBlock * exit_block) {
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("reading first offset");

    Value * byte = iBuilder->CreateZExt(loadRawByte(iBuilder), iBuilder->getInt32Ty());
    // Use tempLength to store partial offset.
    iBuilder->CreateStore(byte, sTempLength);
    incStackVar(iBuilder, sOffset);
    iBuilder->CreateStore(iBuilder->getInt8(State::AT_SECOND_OFFSET), sState);

    generateBoundaryDetection(iBuilder, State::AT_FIRST_OFFSET, exit_block);
    // Falls through to %at_second_offset.
}


void LZ4IndexDecoderKernel::generateAtSecondOffset(const std::unique_ptr<KernelBuilder> & iBuilder, BasicBlock * bb, BasicBlock * exit_block) {
    iBuilder->CreateBr(bb);
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("reading second offset");

    Value * byte1 = iBuilder->CreateLoad(sTempLength);
    Value * byte2 = iBuilder->CreateZExt(loadRawByte(iBuilder), iBuilder->getInt32Ty());
    Value * offset = iBuilder->CreateAdd(
            iBuilder->CreateShl(byte2, iBuilder->getInt32(8)),
            byte1
            );
    iBuilder->setScalarField("MatchOffset", offset);
    incStackVar(iBuilder, sOffset);
    // Prepare extender word and tempLength for extending.
    loadCurrentExtender(iBuilder);
    setExtenderUntilOffset(iBuilder);
    iBuilder->CreateStore(iBuilder->getScalarField("MatchLength"), sTempLength);
    iBuilder->CreateStore(iBuilder->getInt8(State::EXTENDING_MATCH_LENGTH), sState);

    generateBoundaryDetection(iBuilder, State::AT_SECOND_OFFSET, exit_block);
    // Falls through to %extending_match_length.
}


void LZ4IndexDecoderKernel::generateExtendingMatchLen(const std::unique_ptr<KernelBuilder> & iBuilder, BasicBlock * bb, BasicBlock * exit_block) {
    iBuilder->CreateBr(bb);
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("extending match length");
    printGlobalPos();
    printRTDebugInt("rawbyte", loadRawByte(iBuilder));
    printRTDebugInt("extword", iBuilder->CreateLoad(sExtender));

    Value * wordOffset = getWordOffset(iBuilder);
    Value * blockOffset = getWordStartOffset(iBuilder);
    Value * matchLen = iBuilder->CreateLoad(sTempLength);
    Value * matchExtEnd = iBuilder->CreateTrunc(
        iBuilder->CreateCountForwardZeroes(iBuilder->CreateNot(iBuilder->CreateLoad(sExtender))),
        iBuilder->getInt32Ty()
        );
    printRTDebugInt("wordoffset", wordOffset);
    printRTDebugInt("matchExtEnd", matchExtEnd);
    // number of extender = matchExtEnd - wordOffset
    Value * numExtenders = iBuilder->CreateSub(matchExtEnd, wordOffset);
    Value * matchExtReachBoundary = 
            iBuilder->CreateICmpEQ(matchExtEnd, iBuilder->getInt32(wordWidth));
    // There are matchExtEnd forward zeroes, we load bytes[matchExtEnd]
    // which is the first non-extender.  If matchExtEnd == 64, we force the
    // load index to be 0 to avoid out-of-bound access, and lastByte will be 0.
    Value * loadOffset = iBuilder->CreateSelect(matchExtReachBoundary,
            ConstantInt::getNullValue(matchExtEnd->getType()),
            matchExtEnd);
    Value * lastByte = iBuilder->CreateSelect(matchExtReachBoundary,
            iBuilder->getInt8(0),
            loadRawByte(iBuilder, iBuilder->CreateAdd(blockOffset, loadOffset)));
    Value * matchLenExted = iBuilder->CreateICmpUGE(matchLen, iBuilder->getInt32(0xf));
    matchLen = iBuilder->CreateSelect(matchLenExted,
            iBuilder->CreateAdd(
                matchLen,
                iBuilder->CreateAdd(
                    iBuilder->CreateMul(numExtenders, iBuilder->getInt32(0xff)),
                    iBuilder->CreateZExt(lastByte, iBuilder->getInt32Ty())
                    )
                ),      // matchLen + numExtenders * 255
            matchLen);
    wordOffset = iBuilder->CreateSelect(matchLenExted,
            matchExtEnd,
            wordOffset);
    // If lastByte is truly the last length byte, we need to advance the cursor by 1.
    wordOffset = iBuilder->CreateSelect(
            iBuilder->CreateAnd(matchLenExted, iBuilder->CreateNot(matchExtReachBoundary)),
            iBuilder->CreateAdd(wordOffset, iBuilder->getInt32(1)),
            wordOffset
            );
    iBuilder->CreateStore(matchLen, sTempLength);
    iBuilder->CreateStore(iBuilder->CreateAdd(blockOffset, wordOffset), sOffset);

    Value * unfinished = iBuilder->CreateAnd(matchExtReachBoundary, matchLenExted);
    BasicBlock * output_then = iBuilder->CreateBasicBlock("output_then");
    BasicBlock * output_cont = iBuilder->CreateBasicBlock("output_cont");
    iBuilder->CreateLikelyCondBr(
            iBuilder->CreateNot(unfinished),
            output_then, output_cont
            );
    iBuilder->SetInsertPoint(output_then);
    iBuilder->CreateStore(iBuilder->getInt8(State::AT_TOKEN), sState);
    matchLen = iBuilder->CreateAdd(matchLen, iBuilder->getInt32(4));    // Add the constant at the end.
    iBuilder->setScalarField("MatchLength", matchLen);
    generateProduceOutput(iBuilder);
    iBuilder->CreateBr(output_cont);

    iBuilder->SetInsertPoint(output_cont);
    generateBoundaryDetection(iBuilder, State::EXTENDING_MATCH_LENGTH, exit_block, true);
    BasicBlock * cont_block = iBuilder->CreateBasicBlock("finished_" + StateLabels.at(State::EXTENDING_MATCH_LENGTH));
    // Insert point is still in wordBoundary block now.
    // See if there are still more extenders.
    iBuilder->CreateUnlikelyCondBr(unfinished, bb, cont_block);

    iBuilder->SetInsertPoint(cont_block);
}


void LZ4IndexDecoderKernel::generateAtBlockChecksum(const std::unique_ptr<KernelBuilder> & iBuilder, BasicBlock * bb, BasicBlock * skippingBytes) {
    // No branch here as we have made a conditional branch outside.
    iBuilder->SetInsertPoint(bb);
    printRTDebugMsg("processing block checksum");

    // Produce the partial output (fill matchIndexes with 0).
    iBuilder->setScalarField("MatchOffset", iBuilder->getInt32(0));
    iBuilder->setScalarField("MatchLength", iBuilder->getInt32(0));
    generateProduceOutput(iBuilder);

    BasicBlock * hasChecksum_then = iBuilder->CreateBasicBlock("has_checksum_then");
    BasicBlock * hasChecksum_cont = iBuilder->CreateBasicBlock("has_checksum_cont");

    iBuilder->CreateStore(iBuilder->getInt8(State::AT_BLOCK_SIZE), sState);
    iBuilder->CreateCondBr(iBuilder->getScalarField("hasBlockChecksum"), hasChecksum_then, hasChecksum_cont);

    iBuilder->SetInsertPoint(hasChecksum_then);
    iBuilder->setScalarField("BytesToSkip", iBuilder->getInt32(4));
    iBuilder->CreateBr(skippingBytes);
    // Boundary detection will be done in skipping_bytes.

    iBuilder->SetInsertPoint(hasChecksum_cont);
    // No checksum, offset not advanced.  Falls through to the next block (block_size).
}

LZ4IndexDecoderKernel::LZ4IndexDecoderKernel(const std::unique_ptr<kernel::KernelBuilder> & iBuilder)
: BlockOrientedKernel("lz4IndexDecoder",
    // Inputs
    {Binding{iBuilder->getStreamSetTy(1, 8), "byteStream"},
     Binding{iBuilder->getStreamSetTy(1, 1), "extenders"}},
    // Outputs: literal start, literal length, match offset, match length
    {Binding{iBuilder->getStreamSetTy(2, 32), "literalIndexes", UnknownRate()},
     Binding{iBuilder->getStreamSetTy(2, 32), "matchIndexes", FixedRatio(1, 1, "literalIndexes")}},
    // Arguments
    {Binding{iBuilder->getInt1Ty(), "hasBlockChecksum"}},
    {},
    // Internal states:
    {Binding{iBuilder->getInt32Ty(), "BlockNo"},
     Binding{iBuilder->getInt8Ty(), "State"},
     Binding{iBuilder->getInt32Ty(), "LZ4BlockStart"},
     Binding{iBuilder->getInt32Ty(), "LZ4BlockEnd"},
     Binding{iBuilder->getInt32Ty(), "BytesToSkip"},
     Binding{iBuilder->getInt32Ty(), "TempLength"},
     Binding{iBuilder->getInt32Ty(), "TempCount"},
     Binding{iBuilder->getInt32Ty(), "LiteralStart"},
     Binding{iBuilder->getInt32Ty(), "LiteralLength"},
     Binding{iBuilder->getInt32Ty(), "MatchOffset"},
     Binding{iBuilder->getInt32Ty(), "MatchLength"}})
, wordWidth{iBuilder->getSizeTy()->getBitWidth()} {
    setNoTerminateAttribute(true);
}
