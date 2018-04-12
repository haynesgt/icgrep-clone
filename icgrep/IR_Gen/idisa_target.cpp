/*
 *  Copyright (c) 2016 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 */

#include "idisa_target.h"
#include <toolchain/toolchain.h>
#include <IR_Gen/idisa_sse_builder.h>
#include <IR_Gen/idisa_avx_builder.h>
#include <IR_Gen/idisa_i64_builder.h>
#include <IR_Gen/idisa_nvptx_builder.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/ErrorHandling.h>
#include <kernels/kernel_builder.h>

using namespace kernel;

namespace IDISA {
    
KernelBuilder * GetIDISA_Builder(llvm::LLVMContext & C) {
    const bool hasAVX2 = AVX2_available();
    if (LLVM_LIKELY(codegen::BlockSize == 0)) {  // No BlockSize override: use processor SIMD width
        codegen::BlockSize = hasAVX2 ? 256 : 128;
    }
    else if (((codegen::BlockSize & (codegen::BlockSize - 1)) != 0) || (codegen::BlockSize < 64)) {
        llvm::report_fatal_error("BlockSize must be a power of 2 and >=64");
    }
    if (codegen::BlockSize >= 256) {
        if (hasAVX2) {
            return new KernelBuilderImpl<IDISA_AVX2_Builder>(C, codegen::BlockSize, codegen::BlockSize);
        }
    } else if (codegen::BlockSize == 64) {
        return new KernelBuilderImpl<IDISA_I64_Builder>(C, codegen::BlockSize, codegen::BlockSize);
    }
    return new KernelBuilderImpl<IDISA_SSE2_Builder>(C, codegen::BlockSize, codegen::BlockSize);
}

KernelBuilder * GetIDISA_GPU_Builder(llvm::LLVMContext & C) {
    return new KernelBuilderImpl<IDISA_NVPTX20_Builder>(C, 64, 64 * 64);
}

}
