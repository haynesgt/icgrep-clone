
#include "LZ4Generator.h"

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PrettyStackTrace.h>

#include <cc/cc_compiler.h>

#include <lz4FrameDecoder.h>
#include <kernels/streamset.h>
#include <kernels/cc_kernel.h>
#include <kernels/s2p_kernel.h>
#include <kernels/p2s_kernel.h>
#include <kernels/source_kernel.h>
#include <kernels/stdout_kernel.h>
#include <kernels/lz4/lz4_generate_deposit_stream.h>
#include <kernels/kernel_builder.h>
#include <kernels/deletion.h>
#include <kernels/swizzle.h>
#include <kernels/pdep_kernel.h>
#include <kernels/lz4/lz4_multiple_pdep_kernel.h>
#include <kernels/lz4/lz4_match_copy_kernel.h>
#include <kernels/lz4/lz4_swizzled_match_copy_kernel.h>
#include <kernels/lz4/lz4_block_decoder.h>
#include <kernels/lz4/lz4_index_builder.h>

namespace re { class CC; }

using namespace llvm;
using namespace parabix;
using namespace kernel;

LZ4Generator::LZ4Generator():pxDriver("lz4d") {

}

MainFunctionType LZ4Generator::getMainFunc() {
    return reinterpret_cast<MainFunctionType>(pxDriver.getMain());
}



void LZ4Generator::generateExtractOnlyPipeline(const std::string& outputFile) {
    auto & iBuilder = pxDriver.getBuilder();
    this->generateMainFunc(iBuilder);

    StreamSetBuffer * const DecompressedByteStream = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 8), this->getDecompressedBufferBlocks());

    // GeneratePipeline
    this->generateLoadByteStreamAndBitStream(iBuilder);


    this->generateExtractAndDepositMarkers(iBuilder);


    auto swizzle = this->generateSwizzleExtractData(iBuilder);


    // Produce unswizzled bit streams
    StreamSetBuffer * extractedbits = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(8), this->getInputBufferBlocks());
    Kernel * unSwizzleK = pxDriver.addKernelInstance<SwizzleGenerator>(iBuilder, 8, 1, 2);

    pxDriver.makeKernelCall(unSwizzleK, {swizzle.first, swizzle.second}, {extractedbits});


    Kernel * p2sK = pxDriver.addKernelInstance<P2SKernel>(iBuilder);
    pxDriver.makeKernelCall(p2sK, {extractedbits}, {DecompressedByteStream});

    // --------------------------------------------------------
    // End
    Kernel * outK = pxDriver.addKernelInstance<FileSink>(iBuilder, 8);

    outK->setInitialArguments({iBuilder->GetString(outputFile)});
    pxDriver.makeKernelCall(outK, {DecompressedByteStream}, {});

    pxDriver.generatePipelineIR();
    pxDriver.deallocateBuffers();

    iBuilder->CreateRetVoid();

    pxDriver.finalizeObject();
}

void LZ4Generator::generateExtractAndDepositOnlyPipeline(const std::string &outputFile) {
    auto & iBuilder = pxDriver.getBuilder();
    this->generateMainFunc(iBuilder);

    StreamSetBuffer * const DecompressedByteStream = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 8), this->getDecompressedBufferBlocks());

    // GeneratePipeline
    this->generateLoadByteStreamAndBitStream(iBuilder);
    this->generateExtractAndDepositMarkers(iBuilder);

    auto swizzle = this->generateSwizzleExtractData(iBuilder);

    StreamSetBuffer * depositedSwizzle0 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);
    StreamSetBuffer * depositedSwizzle1 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);

    Kernel * multiplePdepK = pxDriver.addKernelInstance<LZ4MultiplePDEPkernel>(iBuilder, 4, 2, 4);
    pxDriver.makeKernelCall(multiplePdepK, {DepositMarker, swizzle.first, swizzle.second}, {depositedSwizzle0, depositedSwizzle1});

    // Produce unswizzled bit streams
    StreamSetBuffer * extractedbits = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(8), this->getInputBufferBlocks());
    Kernel * unSwizzleK = pxDriver.addKernelInstance<SwizzleGenerator>(iBuilder, 8, 1, 2);
    pxDriver.makeKernelCall(unSwizzleK, {depositedSwizzle0, depositedSwizzle1}, {extractedbits});

//    pxDriver.makeKernelCall(unSwizzleK, {u16Swizzle0, u16Swizzle1}, {extractedbits});

    Kernel * p2sK = pxDriver.addKernelInstance<P2SKernel>(iBuilder);
    pxDriver.makeKernelCall(p2sK, {extractedbits}, {DecompressedByteStream});

    // --------------------------------------------------------
    // End
    Kernel * outK = pxDriver.addKernelInstance<FileSink>(iBuilder, 8);
    outK->setInitialArguments({iBuilder->GetString(outputFile)});
    pxDriver.makeKernelCall(outK, {DecompressedByteStream}, {});

    pxDriver.generatePipelineIR();
    pxDriver.deallocateBuffers();

    iBuilder->CreateRetVoid();

    pxDriver.finalizeObject();
}

void LZ4Generator::generatePipeline(const std::string& outputFile) {
    auto & iBuilder = pxDriver.getBuilder();
    this->generateMainFunc(iBuilder);

    StreamSetBuffer * const DecompressedByteStream = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 8), this->getDecompressedBufferBlocks());

    // GeneratePipeline
    this->generateLoadByteStreamAndBitStream(iBuilder);
    this->generateExtractAndDepositMarkers(iBuilder);

    auto swizzle = this->generateSwizzleExtractData(iBuilder);

    StreamSetBuffer * depositedSwizzle0 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);
    StreamSetBuffer * depositedSwizzle1 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);

    Kernel * multiplePdepK = pxDriver.addKernelInstance<LZ4MultiplePDEPkernel>(iBuilder, 4, 2, 4);
    pxDriver.makeKernelCall(multiplePdepK, {DepositMarker, swizzle.first, swizzle.second}, {depositedSwizzle0, depositedSwizzle1});


    StreamSetBuffer * matchCopiedSwizzle0 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);
    StreamSetBuffer * matchCopiedSwizzle1 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);

    Kernel * swizzledMatchCopyK = pxDriver.addKernelInstance<LZ4SwizzledMatchCopyKernel>(iBuilder, 4, 2, 4);
    swizzledMatchCopyK->setInitialArguments({fileSize});
    pxDriver.makeKernelCall(swizzledMatchCopyK, {M0_Start, M0_End, Match_Offset, depositedSwizzle0, depositedSwizzle1}, {matchCopiedSwizzle0, matchCopiedSwizzle1});


    // Produce unswizzled bit streams
    StreamSetBuffer * extractedbits = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(8), this->getInputBufferBlocks());
    Kernel * unSwizzleK = pxDriver.addKernelInstance<SwizzleGenerator>(iBuilder, 8, 1, 2);
    pxDriver.makeKernelCall(unSwizzleK, {matchCopiedSwizzle0, matchCopiedSwizzle1}, {extractedbits});


    Kernel * p2sK = pxDriver.addKernelInstance<P2SKernel>(iBuilder);
    pxDriver.makeKernelCall(p2sK, {extractedbits}, {DecompressedByteStream});

    // --------------------------------------------------------
    // End
    Kernel * outK = pxDriver.addKernelInstance<FileSink>(iBuilder, 8);
    outK->setInitialArguments({iBuilder->GetString(outputFile)});
    pxDriver.makeKernelCall(outK, {DecompressedByteStream}, {});

    pxDriver.generatePipelineIR();
    pxDriver.deallocateBuffers();

    iBuilder->CreateRetVoid();

    pxDriver.finalizeObject();
}

void LZ4Generator::generateMainFunc(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) {
    Module * M = iBuilder->getModule();
    Type * const sizeTy = iBuilder->getSizeTy();
    Type * const boolTy = iBuilder->getIntNTy(sizeof(bool) * 8);
    Type * const voidTy = iBuilder->getVoidTy();
    Type * const inputType = iBuilder->getInt8PtrTy();

    Function * const main = cast<Function>(M->getOrInsertFunction("Main", voidTy, inputType, sizeTy, sizeTy, boolTy, nullptr));
    main->setCallingConv(CallingConv::C);
    Function::arg_iterator args = main->arg_begin();
    inputStream = &*(args++);
    inputStream->setName("input");

    headerSize = &*(args++);
    headerSize->setName("headerSize");

    fileSize = &*(args++);
    fileSize->setName("fileSize");

    hasBlockChecksum = &*(args++);
    hasBlockChecksum->setName("hasBlockChecksum");

    iBuilder->SetInsertPoint(BasicBlock::Create(M->getContext(), "entry", main, 0));
}

void LZ4Generator::generateLoadByteStreamAndBitStream(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) {
    ByteStream = pxDriver.addBuffer<SourceBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 8));
    BasisBits = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(8, 1), this->getInputBufferBlocks());

    kernel::Kernel * sourceK = pxDriver.addKernelInstance<MemorySourceKernel>(iBuilder, iBuilder->getInt8PtrTy());
    sourceK->setInitialArguments({inputStream, fileSize});
    pxDriver.makeKernelCall(sourceK, {}, {ByteStream});
    Kernel * s2pk = pxDriver.addKernelInstance<S2PKernel>(iBuilder, /*aligned = */ true);
    s2pk->addAttribute(MustConsumeAll());
    pxDriver.makeKernelCall(s2pk, {ByteStream}, {BasisBits});
}

void LZ4Generator::generateExtractAndDepositMarkers(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) {
    //// Decode Block Information
    StreamSetBuffer * const BlockData_IsCompressed = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 8), this->getInputBufferBlocks());
    StreamSetBuffer * const BlockData_BlockStart = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());
    StreamSetBuffer * const BlockData_BlockEnd = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());

    //// Generate Helper Markers Extenders, FX, XF
    StreamSetBuffer * const Extenders = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 1), this->getInputBufferBlocks());

    Kernel * extenderK = pxDriver.addKernelInstance<ParabixCharacterClassKernelBuilder>(iBuilder, "extenders", std::vector<re::CC *>{re::makeCC(0xFF)}, 8);
    extenderK->addAttribute(MustConsumeAll());
    pxDriver.makeKernelCall(extenderK, {BasisBits}, {Extenders});


    Kernel * blockDecoderK = pxDriver.addKernelInstance<LZ4BlockDecoderNewKernel>(iBuilder);
    blockDecoderK->setInitialArguments({iBuilder->CreateTrunc(hasBlockChecksum, iBuilder->getInt1Ty()), headerSize, fileSize});
    pxDriver.makeKernelCall(blockDecoderK, {ByteStream}, {BlockData_IsCompressed, BlockData_BlockStart, BlockData_BlockEnd});

    //// Generate Extract/Deposit Markers, M0_Start, M0_End, MatchOffset

    M0_Start = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());
    M0_End = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());

    //TODO handle uncompressed part
    StreamSetBuffer * const UncompressedStartPos = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());
    StreamSetBuffer * const UncompressedLength = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());
    StreamSetBuffer * const UncompressedOutputPos = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());

    DeletionMarker = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 1), this->getInputBufferBlocks());
    StreamSetBuffer * const M0Marker = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 1), this->getDecompressedBufferBlocks());
    DepositMarker = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 1), this->getDecompressedBufferBlocks());
    Match_Offset = pxDriver.addBuffer<CircularBuffer>(iBuilder, iBuilder->getStreamSetTy(1, 64), this->getInputBufferBlocks());

    Kernel* Lz4IndexBuilderK = pxDriver.addKernelInstance<LZ4IndexBuilderKernel>(iBuilder);
    Lz4IndexBuilderK->setInitialArguments({fileSize});
    pxDriver.makeKernelCall(
            Lz4IndexBuilderK,
            {
                    ByteStream,
                    Extenders,

                    // Block Data
                    BlockData_IsCompressed,
                    BlockData_BlockStart,
                    BlockData_BlockEnd
            }, {
                    //Uncompressed Data
                    UncompressedStartPos,
                    UncompressedLength,
                    UncompressedOutputPos,

                    DeletionMarker,
                    M0_Start,
                    M0_End,
                    Match_Offset,
                    M0Marker
            });

    Kernel * generateDepositK = pxDriver.addKernelInstance<LZ4GenerateDepositStreamKernel>(iBuilder);
    pxDriver.makeKernelCall(generateDepositK, {M0Marker}, {DepositMarker});

}

std::pair<StreamSetBuffer*, StreamSetBuffer*> LZ4Generator::generateSwizzleExtractData(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) {
    StreamSetBuffer * u16Swizzle0 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);
    StreamSetBuffer * u16Swizzle1 = pxDriver.addBuffer<SwizzledCopybackBuffer>(iBuilder, iBuilder->getStreamSetTy(4), this->getInputBufferBlocks(), 1);

    Kernel * delK = pxDriver.addKernelInstance<SwizzledDeleteByPEXTkernel>(iBuilder, 64, 8);
    pxDriver.makeKernelCall(delK, {DeletionMarker, BasisBits}, {u16Swizzle0, u16Swizzle1});
    return std::make_pair(u16Swizzle0, u16Swizzle1);
}

int LZ4Generator::get4MbBufferBlocks() {
    return 4 * 1024 * 1024 / codegen::BlockSize;
}

int LZ4Generator::getInputBufferBlocks() {
    return this->get4MbBufferBlocks() * 2;
}
int LZ4Generator::getDecompressedBufferBlocks() {
    return this->get4MbBufferBlocks() * 2;
}




// Kernel Pipeline
