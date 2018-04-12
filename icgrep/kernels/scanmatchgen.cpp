/*
 *  Copyright (c) 2015 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 */

#include "scanmatchgen.h"
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <kernels/kernel_builder.h>

using namespace llvm;

inline static unsigned floor_log2(const unsigned v) {
    assert ("log2(0) is undefined!" && v != 0);
    return 31 - __builtin_clz(v);
}

namespace kernel {

inline std::string getGrepTypeId(const GrepType grepType) {
    switch (grepType) {
        case GrepType::Normal:
            return "N";
        case GrepType::NameExpression:
            return "E";
        case GrepType::PropertyValue:
            return "P";
        default:
            llvm_unreachable("unknown grep type!");
    }
}

void ScanMatchKernel::generateMultiBlockLogic(const std::unique_ptr<KernelBuilder> &iBuilder) {

    Module * const m = iBuilder->getModule();
    BasicBlock * const entryBlock = iBuilder->GetInsertBlock();
    BasicBlock * const initialBlock = iBuilder->CreateBasicBlock("initialBlock");
    BasicBlock * const scanWordIteration = iBuilder->CreateBasicBlock("ScanWordIteration");
    BasicBlock * const matches_test_block = iBuilder->CreateBasicBlock("matches_test_block");
    BasicBlock * const processMatchesEntry = iBuilder->CreateBasicBlock("process_matches_loop");
    BasicBlock * const prior_breaks_block = iBuilder->CreateBasicBlock("prior_breaks_block");
    BasicBlock * const loop_final_block = iBuilder->CreateBasicBlock("loop_final_block");
    BasicBlock * const processMatchesExit = iBuilder->CreateBasicBlock("matches_done_block");
    BasicBlock * const remaining_breaks_block = iBuilder->CreateBasicBlock("remaining_breaks_block");
    BasicBlock * const return_block = iBuilder->CreateBasicBlock("return_block");
    BasicBlock * const scanWordExit = iBuilder->CreateBasicBlock("ScanWordExit");
    BasicBlock * const blocksExit = iBuilder->CreateBasicBlock("blocksExit");
    IntegerType * const sizeTy = iBuilder->getSizeTy();
    const unsigned fieldCount = iBuilder->getBitBlockWidth() / sizeTy->getBitWidth();
    VectorType * const scanwordVectorType =  VectorType::get(sizeTy, fieldCount);
    Constant * blockSize = iBuilder->getSize(iBuilder->getBitBlockWidth());
    Constant * blockSizeLess1 = iBuilder->getSize(iBuilder->getBitBlockWidth() - 1);

    Function::arg_iterator args = mCurrentMethod->arg_begin();
    /* self = */ args++;
    Value * itemsToDo = &*(args++);
    Value * inputStreamAvail = &*(args++);
    Value * match_result = &*(args++);
    Value * line_break = &*(args++);
    Value * input_stream = &*(args);

    Value * blocksToDo = iBuilder->CreateUDiv(iBuilder->CreateAdd(itemsToDo, blockSizeLess1), blockSize);
    
    Value * match_result_ptr = iBuilder->CreateBitCast(match_result, scanwordVectorType->getPointerTo());
    Value * line_break_ptr = iBuilder->CreateBitCast(line_break, scanwordVectorType->getPointerTo());

    iBuilder->CreateCondBr(iBuilder->CreateICmpUGT(blocksToDo, iBuilder->getSize(0)), initialBlock, blocksExit);

    iBuilder->SetInsertPoint(initialBlock);
    PHINode * const blockBase = iBuilder->CreatePHI(iBuilder->getSizeTy(), 2);
    blockBase->addIncoming(iBuilder->getSize(0), entryBlock);
    Value * const blockNo = iBuilder->getScalarField("BlockNo");
    Value * const scanwordPos = iBuilder->CreateShl(blockNo, floor_log2(iBuilder->getBitBlockWidth()));
    Value * const lastRecordStart = iBuilder->getProcessedItemCount("InputStream");
    Value * const lastRecordNum = iBuilder->getScalarField("LineNum");
    iBuilder->CreateBr(scanWordIteration);

    iBuilder->SetInsertPoint(scanWordIteration);

        // while (phiIndex < words per stride)
        PHINode * const phiIndex = iBuilder->CreatePHI(iBuilder->getInt32Ty(), 2, "index");
        phiIndex->addIncoming(iBuilder->getInt32(0), initialBlock);
        PHINode * const phiScanwordPos = iBuilder->CreatePHI(scanwordPos->getType(), 2, "pos");
        phiScanwordPos->addIncoming(scanwordPos, initialBlock);
        PHINode * const phiLineStart = iBuilder->CreatePHI(lastRecordStart->getType(), 2, "recordstart");
        phiLineStart->addIncoming(lastRecordStart, initialBlock);
        PHINode * const phiLineNum = iBuilder->CreatePHI(lastRecordNum->getType(), 2, "recordnum");
        phiLineNum->addIncoming(lastRecordNum, initialBlock);
        Value * const matches = iBuilder->CreateLoad(iBuilder->CreateGEP(match_result_ptr, blockBase));
        Value * const linebreaks = iBuilder->CreateLoad(iBuilder->CreateGEP(line_break_ptr, blockBase));
        Value * const matchWord = iBuilder->CreateExtractElement(matches, phiIndex);
        Value * const recordBreaks = iBuilder->CreateExtractElement(linebreaks, phiIndex);

        // The match scanner works with a loop involving four variables:
        // (a) the bit stream scanword of matches marking the ends of selected records,
        // (b) the bit stream scanword of record_breaks marking the ends of all records,
        // (c) the integer lastRecordNum indicating the number of records processed so far,
        // (d) the index lastRecordStart indicating the file position of the last record.
        // We set up a loop structure, in which a set of 4 phi nodes initialize these
        // variables from either the input to the scanner or the computed values within
        // the loop body.

        iBuilder->CreateBr(matches_test_block);

        // LOOP Test Block
        iBuilder->SetInsertPoint(matches_test_block);
        PHINode * phiMatchWord = iBuilder->CreatePHI(sizeTy, 2, "matches");
        PHINode * phiRecordBreaks = iBuilder->CreatePHI(sizeTy, 2, "recordbreaks");
        PHINode * phiRecordStart = iBuilder->CreatePHI(sizeTy, 2, "recordstart");
        PHINode * phiRecordNum = iBuilder->CreatePHI(sizeTy, 2, "recordnum");
        phiMatchWord->addIncoming(matchWord, scanWordIteration);
        phiRecordBreaks->addIncoming(recordBreaks, scanWordIteration);
        phiRecordNum->addIncoming(phiLineNum, scanWordIteration);
        phiRecordStart->addIncoming(phiLineStart, scanWordIteration);
        Value * anyMatches = iBuilder->CreateICmpNE(phiMatchWord, ConstantInt::getNullValue(sizeTy));
        iBuilder->CreateCondBr(anyMatches, processMatchesEntry, processMatchesExit);

            // LOOP BODY
            // The loop body is entered if we have more matches to process.
            iBuilder->SetInsertPoint(processMatchesEntry);
            Value * prior_breaks = iBuilder->CreateAnd(iBuilder->CreateMaskToLowestBitExclusive(phiMatchWord), phiRecordBreaks);
            // Within the loop we have a conditional block that is executed if there are any prior record breaks.
            Value * prior_breaks_cond = iBuilder->CreateICmpNE(prior_breaks, ConstantInt::getNullValue(sizeTy));
            iBuilder->CreateCondBr(prior_breaks_cond, prior_breaks_block, loop_final_block);

                // PRIOR_BREAKS_BLOCK
                // If there are prior breaks, we count them and compute the record start position.
                iBuilder->SetInsertPoint(prior_breaks_block);
                Value * matchedRecordNum = iBuilder->CreateAdd(iBuilder->CreatePopcount(prior_breaks), phiRecordNum);
                Value * reverseDistance = iBuilder->CreateCountReverseZeroes(prior_breaks);
                Value * width = ConstantInt::get(sizeTy, sizeTy->getBitWidth());
                Value * priorRecordStart = iBuilder->CreateAdd(phiScanwordPos, iBuilder->CreateSub(width, reverseDistance));
                iBuilder->CreateBr(loop_final_block);

            // LOOP FINAL BLOCK
            // The prior breaks, if any have been counted.  Set up phi nodes for the recordNum
            // and recortStart depending on whether the conditional execution of prior_breaks_block.
            iBuilder->SetInsertPoint(loop_final_block);
            PHINode * matchRecordNum = iBuilder->CreatePHI(sizeTy, 2, "matchRecordNum");
            matchRecordNum->addIncoming(phiRecordNum, processMatchesEntry);
            matchRecordNum->addIncoming(matchedRecordNum, prior_breaks_block);
            phiRecordNum->addIncoming(matchRecordNum, loop_final_block);

            PHINode * matchRecordStart = iBuilder->CreatePHI(sizeTy, 2, "matchRecordStart");
            matchRecordStart->addIncoming(phiRecordStart, processMatchesEntry);
            matchRecordStart->addIncoming(priorRecordStart, prior_breaks_block);
            phiRecordStart->addIncoming(matchRecordStart, loop_final_block);
            Value * matchRecordEnd = iBuilder->CreateAdd(phiScanwordPos, iBuilder->CreateCountForwardZeroes(phiMatchWord));

            Function * const matcher = m->getFunction("matcher"); assert (matcher);
            auto args_matcher = matcher->arg_begin();
            Value * const mrn = iBuilder->CreateZExtOrTrunc(matchRecordNum, args_matcher->getType());
            Value * const mrs = iBuilder->CreateZExtOrTrunc(matchRecordStart, (++args_matcher)->getType());
            Value * const mre = iBuilder->CreateZExtOrTrunc(matchRecordEnd, (++args_matcher)->getType());
            Value * const inputStream = iBuilder->getRawInputPointer("InputStream", iBuilder->getInt32(0), iBuilder->getInt32(0));
            Value * const is = iBuilder->CreatePointerCast(inputStream, (++args_matcher)->getType());
            if (mGrepType == GrepType::Normal) {
                Value * const sz = iBuilder->CreateZExtOrTrunc(iBuilder->getBufferedSize("InputStream"), (++args_matcher)->getType());
                Value * const fi = iBuilder->CreateZExtOrTrunc(iBuilder->getScalarField("FileIdx"), (++args_matcher)->getType());
                iBuilder->CreateCall(matcher, {mrn, mrs, mre, is, sz, fi});
            } else {
                iBuilder->CreateCall(matcher, {mrn, mrs, mre, is});
            }

            Value * remaining_matches = iBuilder->CreateResetLowestBit(phiMatchWord);
            phiMatchWord->addIncoming(remaining_matches, loop_final_block);

            Value * remaining_breaks = iBuilder->CreateXor(phiRecordBreaks, prior_breaks);
            phiRecordBreaks->addIncoming(remaining_breaks, loop_final_block);

            iBuilder->CreateBr(matches_test_block);

        // LOOP EXIT/MATCHES_DONE
        iBuilder->SetInsertPoint(processMatchesExit);
        // When the matches are done, there may be additional record breaks remaining
        Value * more_breaks_cond = iBuilder->CreateICmpNE(phiRecordBreaks, ConstantInt::getNullValue(sizeTy));
        iBuilder->CreateCondBr(more_breaks_cond, remaining_breaks_block, return_block);

            // REMAINING_BREAKS_BLOCK: process remaining record breaks after all matches are processed
            iBuilder->SetInsertPoint(remaining_breaks_block);
            Value * break_count = iBuilder->CreatePopcount(phiRecordBreaks);
            Value * final_record_num = iBuilder->CreateAdd(phiRecordNum, break_count);
            Value * reverseZeroes = iBuilder->CreateCountReverseZeroes(phiRecordBreaks);
            Value * pendingLineStart = iBuilder->CreateAdd(phiScanwordPos, iBuilder->CreateSub(width, reverseZeroes));
            iBuilder->CreateBr(return_block);

        // RETURN block
        iBuilder->SetInsertPoint(return_block);
        PHINode * phiFinalRecordNum = iBuilder->CreatePHI(sizeTy, 2, "finalRecordCount");
        PHINode * phiFinalRecordStart = iBuilder->CreatePHI(sizeTy, 2, "finalRecordStart");

        phiFinalRecordNum->addIncoming(phiRecordNum, processMatchesExit);
        phiFinalRecordNum->addIncoming(final_record_num, remaining_breaks_block);
        phiLineNum->addIncoming(phiFinalRecordNum, return_block);

        phiFinalRecordStart->addIncoming(phiRecordStart, processMatchesExit);
        phiFinalRecordStart->addIncoming(pendingLineStart, remaining_breaks_block);
        phiLineStart->addIncoming(phiFinalRecordStart, return_block);

        Value * nextScanwordPos = iBuilder->CreateAdd(phiScanwordPos, ConstantInt::get(sizeTy, sizeTy->getBitWidth()));
        phiScanwordPos->addIncoming(nextScanwordPos, return_block);
        Value * nextIndex = iBuilder->CreateAdd(phiIndex, iBuilder->getInt32(1));
        phiIndex->addIncoming(nextIndex, return_block);
        iBuilder->CreateLikelyCondBr(iBuilder->CreateICmpNE(nextIndex, iBuilder->getInt32(fieldCount)), scanWordIteration, scanWordExit);

    iBuilder->SetInsertPoint(scanWordExit);
    iBuilder->setScalarField("BlockNo", iBuilder->CreateAdd(blockNo, ConstantInt::get(blockNo->getType(), 1)));
    iBuilder->setScalarField("LineNum", phiFinalRecordNum);
    iBuilder->setProcessedItemCount("InputStream", phiFinalRecordStart);
    Value * blockBaseNext = iBuilder->CreateAdd(blockBase, ConstantInt::get(iBuilder->getSizeTy(), 1));
    blockBase->addIncoming(blockBaseNext, scanWordExit);
    iBuilder->CreateLikelyCondBr(iBuilder->CreateICmpNE(blockBaseNext, blocksToDo), initialBlock, blocksExit);

    iBuilder->SetInsertPoint(blocksExit);
}

ScanMatchKernel::ScanMatchKernel(const std::unique_ptr<kernel::KernelBuilder> & b, GrepType grepType, const unsigned codeUnitWidth)
: MultiBlockKernel("scanMatch" + getGrepTypeId(grepType) + std::to_string(codeUnitWidth),
    {Binding{b->getStreamSetTy(1, 1), "matchResult"}, Binding{b->getStreamSetTy(1, 1), "lineBreak"}, Binding{b->getStreamSetTy(1, 8), "InputStream", UnknownRate()}},
    {},
    {Binding{b->getInt32Ty(), "FileIdx"}},
    {},
    {Binding{b->getSizeTy(), "BlockNo"}, Binding{b->getSizeTy(), "LineNum"}})
, mGrepType(grepType) {

}

}
