#include "p2s_kernel.h"
//#include "llvm/IR/Constant.h"      // for Constant
//#include "llvm/IR/Constants.h"     // for ConstantInt
//#include "llvm/IR/DerivedTypes.h"  // for PointerType, VectorType
//#include "llvm/IR/Function.h"      // for Function, Function::arg_iterator
//#include <llvm/IR/Module.h>
#include <kernels/streamset.h>
#include <kernels/kernel_builder.h>

namespace llvm { class Value; }

using namespace llvm;
using namespace parabix;

namespace kernel{
	
void p2s_step(const std::unique_ptr<KernelBuilder> & iBuilder, Value * p0, Value * p1, Value * hi_mask, unsigned shift, Value * &s1, Value * &s0) {
    Value * t0 = iBuilder->simd_if(1, hi_mask, p0, iBuilder->simd_srli(16, p1, shift));
    Value * t1 = iBuilder->simd_if(1, hi_mask, iBuilder->simd_slli(16, p0, shift), p1);
    s1 = iBuilder->esimd_mergeh(8, t1, t0);
    s0 = iBuilder->esimd_mergel(8, t1, t0);
}

inline void p2s(const std::unique_ptr<KernelBuilder> & iBuilder, Value * p[], Value * s[]) {
    Value * bit00004444[2];
    Value * bit22226666[2];
    Value * bit11115555[2];
    Value * bit33337777[2];
    p2s_step(iBuilder, p[0], p[4], iBuilder->simd_himask(8), 4, bit00004444[1], bit00004444[0]);
    p2s_step(iBuilder, p[1], p[5], iBuilder->simd_himask(8), 4, bit11115555[1], bit11115555[0]);
    p2s_step(iBuilder, p[2], p[6], iBuilder->simd_himask(8), 4, bit22226666[1], bit22226666[0]);
    p2s_step(iBuilder, p[3], p[7], iBuilder->simd_himask(8), 4, bit33337777[1], bit33337777[0]);
    Value * bit00224466[4];
    Value * bit11335577[4];
    for (unsigned j = 0; j<2; j++) {
        p2s_step(iBuilder, bit00004444[j], bit22226666[j],iBuilder->simd_himask(4), 2, bit00224466[2*j+1], bit00224466[2*j]);
        p2s_step(iBuilder, bit11115555[j], bit33337777[j],iBuilder->simd_himask(4), 2, bit11335577[2*j+1], bit11335577[2*j]);
    }
    for (unsigned j = 0; j<4; j++) {
        p2s_step(iBuilder, bit00224466[j], bit11335577[j], iBuilder->simd_himask(2), 1, s[2*j+1], s[2*j]);
    }
}
    		
void P2SKernel::generateDoBlockMethod(const std::unique_ptr<KernelBuilder> & iBuilder) {
    Value * p_bitblock[8];
    for (unsigned i = 0; i < 8; i++) {
        p_bitblock[i] = iBuilder->loadInputStreamBlock("basisBits", iBuilder->getInt32(i));
    }
    Value * s_bytepack[8];
    p2s(iBuilder, p_bitblock, s_bytepack);
    for (unsigned j = 0; j < 8; ++j) {
        iBuilder->storeOutputStreamPack("byteStream", iBuilder->getInt32(0), iBuilder->getInt32(j), s_bytepack[j]);
    }
}

void P2SKernelWithCompressedOutput::generateDoBlockMethod(const std::unique_ptr<KernelBuilder> & iBuilder) {
    IntegerType * i32 = iBuilder->getInt32Ty();
    PointerType * bitBlockPtrTy = PointerType::get(iBuilder->getBitBlockType(), 0);

    Value * basisBits[8];
    for (unsigned i = 0; i < 8; i++) {
        basisBits[i] = iBuilder->loadInputStreamBlock("basisBits", iBuilder->getInt32(i));
    }
    Value * bytePack[8];
    p2s(iBuilder, basisBits, bytePack);

    unsigned units_per_register = iBuilder->getBitBlockWidth()/8;
    Value * delCountBlock_ptr = iBuilder->getInputStreamBlockPtr("deletionCounts", iBuilder->getInt32(0));
    Value * unit_counts = iBuilder->fwCast(units_per_register, iBuilder->CreateBlockAlignedLoad(delCountBlock_ptr));

    Value * output_ptr = iBuilder->getOutputStreamBlockPtr("byteStream", iBuilder->getInt32(0));
    output_ptr = iBuilder->CreatePointerCast(output_ptr, iBuilder->getInt8PtrTy());
    Value * offset = iBuilder->getInt32(0);
    for (unsigned j = 0; j < 8; ++j) {
        iBuilder->CreateStore(bytePack[j], iBuilder->CreateBitCast(iBuilder->CreateGEP(output_ptr, offset), bitBlockPtrTy));
        offset = iBuilder->CreateZExt(iBuilder->CreateExtractElement(unit_counts, iBuilder->getInt32(j)), i32);
    }

    Value * unitsGenerated = iBuilder->getProducedItemCount("byteStream"); // units generated to buffer
    unitsGenerated = iBuilder->CreateAdd(unitsGenerated, iBuilder->CreateZExt(offset, iBuilder->getSizeTy()));
    iBuilder->setProducedItemCount("byteStream", unitsGenerated);
}

void P2S16Kernel::generateDoBlockMethod(const std::unique_ptr<KernelBuilder> & iBuilder) {
    Value * hi_input[8];
    for (unsigned j = 0; j < 8; ++j) {
        hi_input[j] = iBuilder->loadInputStreamBlock("basisBits", iBuilder->getInt32(j));
    }
    Value * hi_bytes[8];
    p2s(iBuilder, hi_input, hi_bytes);    
    Value * lo_input[8];
    for (unsigned j = 0; j < 8; ++j) {
        lo_input[j] = iBuilder->loadInputStreamBlock("basisBits", iBuilder->getInt32(j + 8));
    }
    Value * lo_bytes[8];
    p2s(iBuilder, lo_input, lo_bytes);   
    for (unsigned j = 0; j < 8; ++j) {
        Value * merge0 = iBuilder->bitCast(iBuilder->esimd_mergel(8, hi_bytes[j], lo_bytes[j]));
        Value * merge1 = iBuilder->bitCast(iBuilder->esimd_mergeh(8, hi_bytes[j], lo_bytes[j]));
        iBuilder->storeOutputStreamPack("i16Stream", iBuilder->getInt32(0), iBuilder->getInt32(2 * j), merge0);
        iBuilder->storeOutputStreamPack("i16Stream", iBuilder->getInt32(0), iBuilder->getInt32(2 * j + 1), merge1);
    }
}
        
void P2S16KernelWithCompressedOutput::generateDoBlockMethod(const std::unique_ptr<KernelBuilder> & iBuilder) {
    IntegerType * i32Ty = iBuilder->getInt32Ty();
    PointerType * int16PtrTy = iBuilder->getInt16Ty()->getPointerTo();
    PointerType * bitBlockPtrTy = iBuilder->getBitBlockType()->getPointerTo();
    ConstantInt * stride = iBuilder->getSize(iBuilder->getStride());

    Value * hi_input[8];
    for (unsigned j = 0; j < 8; ++j) {
        hi_input[j] = iBuilder->loadInputStreamBlock("basisBits", iBuilder->getInt32(j));
    }
    Value * hi_bytes[8];
    p2s(iBuilder, hi_input, hi_bytes);

    Value * lo_input[8];
    for (unsigned j = 0; j < 8; ++j) {
        lo_input[j] = iBuilder->loadInputStreamBlock("basisBits", iBuilder->getInt32(j + 8));
    }
    Value * lo_bytes[8];
    p2s(iBuilder, lo_input, lo_bytes);

    Value * delCountBlock_ptr = iBuilder->getInputStreamBlockPtr("deletionCounts", iBuilder->getInt32(0));
    Value * unit_counts = iBuilder->fwCast(iBuilder->getBitBlockWidth() / 16, iBuilder->CreateBlockAlignedLoad(delCountBlock_ptr));


    Value * u16_output_ptr = iBuilder->getOutputStreamBlockPtr("i16Stream", iBuilder->getInt32(0));
    u16_output_ptr = iBuilder->CreatePointerCast(u16_output_ptr, int16PtrTy);
    Value * i16UnitsGenerated = iBuilder->getProducedItemCount("i16Stream"); // units generated to buffer
    u16_output_ptr = iBuilder->CreateGEP(u16_output_ptr, iBuilder->CreateURem(i16UnitsGenerated, stride));

    Value * offset = ConstantInt::get(i32Ty, 0);

    for (unsigned j = 0; j < 8; ++j) {
        Value * merge0 = iBuilder->bitCast(iBuilder->esimd_mergel(8, hi_bytes[j], lo_bytes[j]));
        iBuilder->CreateAlignedStore(merge0, iBuilder->CreateBitCast(iBuilder->CreateGEP(u16_output_ptr, offset), bitBlockPtrTy), 1);
        offset = iBuilder->CreateZExt(iBuilder->CreateExtractElement(unit_counts, iBuilder->getInt32(2 * j)), i32Ty);

        Value * merge1 = iBuilder->bitCast(iBuilder->esimd_mergeh(8, hi_bytes[j], lo_bytes[j]));
        iBuilder->CreateAlignedStore(merge1, iBuilder->CreateBitCast(iBuilder->CreateGEP(u16_output_ptr, offset), bitBlockPtrTy), 1);
        offset = iBuilder->CreateZExt(iBuilder->CreateExtractElement(unit_counts, iBuilder->getInt32(2 * j + 1)), i32Ty);
    }    
    Value * i16UnitsFinal = iBuilder->CreateAdd(i16UnitsGenerated, iBuilder->CreateZExt(offset, iBuilder->getSizeTy()));
    iBuilder->setProducedItemCount("i16Stream", i16UnitsFinal);
}

P2SKernel::P2SKernel(const std::unique_ptr<kernel::KernelBuilder> & iBuilder)
: BlockOrientedKernel("p2s",
              {Binding{iBuilder->getStreamSetTy(8, 1), "basisBits"}},
              {Binding{iBuilder->getStreamSetTy(1, 8), "byteStream"}},
              {}, {}, {}) {
}

P2SKernelWithCompressedOutput::P2SKernelWithCompressedOutput(const std::unique_ptr<kernel::KernelBuilder> & iBuilder)
: BlockOrientedKernel("p2s_compress",
              {Binding{iBuilder->getStreamSetTy(8, 1), "basisBits"}, Binding{iBuilder->getStreamSetTy(1, 1), "deletionCounts"}},
                      {Binding{iBuilder->getStreamSetTy(1, 8), "byteStream", MaxRatio(1)}},
              {}, {}, {}) {
}

P2S16Kernel::P2S16Kernel(const std::unique_ptr<kernel::KernelBuilder> & iBuilder)
: BlockOrientedKernel("p2s_16",
              {Binding{iBuilder->getStreamSetTy(16, 1), "basisBits"}},
              {Binding{iBuilder->getStreamSetTy(1, 16), "i16Stream"}},
              {}, {}, {}) {
}


P2S16KernelWithCompressedOutput::P2S16KernelWithCompressedOutput(const std::unique_ptr<kernel::KernelBuilder> & b)
: BlockOrientedKernel("p2s_16_compress",
              {Binding{b->getStreamSetTy(16, 1), "basisBits"}, Binding{b->getStreamSetTy(1, 1), "deletionCounts"}},
              {Binding{b->getStreamSetTy(1, 16), "i16Stream", MaxRatio(1)}},
              {},
              {},
              {}) {

}
    
    
}
