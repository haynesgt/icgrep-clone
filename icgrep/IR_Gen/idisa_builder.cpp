/*
 *  Copyright (c) 2016 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#include "idisa_builder.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/TypeBuilder.h>
#include <toolchain/toolchain.h>

using namespace llvm;

namespace IDISA {

VectorType * IDISA_Builder::fwVectorType(const unsigned fw) {
    return VectorType::get(getIntNTy(fw), mBitBlockWidth / fw);
}

Value * IDISA_Builder::fwCast(const unsigned fw, Value * const a) {
    VectorType * const ty = fwVectorType(fw);
    assert (a->getType()->getPrimitiveSizeInBits() == ty->getPrimitiveSizeInBits());
    return CreateBitCast(a, ty);
}

void IDISA_Builder::CallPrintRegister(const std::string & name, Value * const value) {
    Module * const m = getModule();
    Constant * printRegister = m->getFunction("PrintRegister");
    if (LLVM_UNLIKELY(printRegister == nullptr)) {
        FunctionType *FT = FunctionType::get(getVoidTy(), { PointerType::get(getInt8Ty(), 0), getBitBlockType() }, false);
        Function * function = Function::Create(FT, Function::InternalLinkage, "PrintRegister", m);
        auto arg = function->arg_begin();
        std::string tmp;
        raw_string_ostream out(tmp);
        out << "%-40s =";
        for(unsigned i = 0; i < (mBitBlockWidth / 8); ++i) {
            out << " %02x";
        }
        out << '\n';
        BasicBlock * entry = BasicBlock::Create(m->getContext(), "entry", function);
        IRBuilder<> builder(entry);
        std::vector<Value *> args;
        args.push_back(GetString(out.str().c_str()));
        Value * const name = &*(arg++);
        name->setName("name");
        args.push_back(name);
        Value * value = &*arg;
        value->setName("value");
        Type * const byteVectorType = VectorType::get(getInt8Ty(), (mBitBlockWidth / 8));
        value = builder.CreateBitCast(value, byteVectorType);
        for(unsigned i = (mBitBlockWidth / 8); i != 0; --i) {
            args.push_back(builder.CreateZExt(builder.CreateExtractElement(value, builder.getInt32(i - 1)), builder.getInt32Ty()));
        }
        builder.CreateCall(GetPrintf(), args);
        builder.CreateRetVoid();

        printRegister = function;
    }
    CreateCall(printRegister, {GetString(name.c_str()), CreateBitCast(value, mBitBlockType)});
}

Constant * IDISA_Builder::simd_himask(unsigned fw) {
    return Constant::getIntegerValue(getIntNTy(mBitBlockWidth), APInt::getSplat(mBitBlockWidth, APInt::getHighBitsSet(fw, fw/2)));
}

Constant * IDISA_Builder::simd_lomask(unsigned fw) {
    return Constant::getIntegerValue(getIntNTy(mBitBlockWidth), APInt::getSplat(mBitBlockWidth, APInt::getLowBitsSet(fw, fw/2)));
}

Value * IDISA_Builder::simd_fill(unsigned fw, Value * a) {
    if (fw < 8) report_fatal_error("Unsupported field width: simd_fill " + std::to_string(fw));
    const unsigned field_count = mBitBlockWidth/fw;
    Type * singleFieldVecTy = VectorType::get(getIntNTy(fw), 1);
    Value * aVec = CreateBitCast(a, singleFieldVecTy);
    return CreateShuffleVector(aVec, UndefValue::get(singleFieldVecTy), Constant::getNullValue(VectorType::get(getInt32Ty(), field_count)));
}

Value * IDISA_Builder::simd_add(unsigned fw, Value * a, Value * b) {
    if (fw == 1) {
        return simd_xor(a, b);
    } else if (fw < 8) {
        Constant * hi_bit_mask = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                           APInt::getSplat(mBitBlockWidth, APInt::getHighBitsSet(fw, 1)));
        Constant * lo_bit_mask = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                           APInt::getSplat(mBitBlockWidth, APInt::getLowBitsSet(fw, fw-1)));
        Value * hi_xor = simd_xor(simd_and(a, hi_bit_mask), simd_and(b, hi_bit_mask));
        Value * part_sum = simd_add(32, simd_and(a, lo_bit_mask), simd_and(b, lo_bit_mask));
        return simd_xor(part_sum, hi_xor);
    }
    return CreateAdd(fwCast(fw, a), fwCast(fw, b));
}

Value * IDISA_Builder::simd_sub(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: sub " + std::to_string(fw));
    return CreateSub(fwCast(fw, a), fwCast(fw, b));
}

Value * IDISA_Builder::simd_mult(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: mult " + std::to_string(fw));
    return CreateMul(fwCast(fw, a), fwCast(fw, b));
}

Value * IDISA_Builder::simd_eq(unsigned fw, Value * a, Value * b) {
    if (fw < 8) {
        Value * eq_bits = simd_not(simd_xor(a, b));
        if (fw == 1) return eq_bits;
        eq_bits = simd_or(simd_and(simd_srli(32, simd_and(simd_himask(2), eq_bits), 1), eq_bits),
                          simd_and(simd_slli(32, simd_and(simd_lomask(2), eq_bits), 1), eq_bits));
        if (fw == 2) return eq_bits;
        eq_bits = simd_or(simd_and(simd_srli(32, simd_and(simd_himask(4), eq_bits), 2), eq_bits),
                          simd_and(simd_slli(32, simd_and(simd_lomask(4), eq_bits), 2), eq_bits));
        return eq_bits;
    }
    return CreateSExt(CreateICmpEQ(fwCast(fw, a), fwCast(fw, b)), fwVectorType(fw));
}

Value * IDISA_Builder::simd_gt(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: gt " + std::to_string(fw));
    return CreateSExt(CreateICmpSGT(fwCast(fw, a), fwCast(fw, b)), fwVectorType(fw));
}

Value * IDISA_Builder::simd_ugt(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: ugt " + std::to_string(fw));
    return CreateSExt(CreateICmpUGT(fwCast(fw, a), fwCast(fw, b)), fwVectorType(fw));
}

Value * IDISA_Builder::simd_lt(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: lt " + std::to_string(fw));
    return CreateSExt(CreateICmpSLT(fwCast(fw, a), fwCast(fw, b)), fwVectorType(fw));
}

Value * IDISA_Builder::simd_ult(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: ult " + std::to_string(fw));
    return CreateSExt(CreateICmpULT(fwCast(fw, a), fwCast(fw, b)), fwVectorType(fw));
}

Value * IDISA_Builder::simd_max(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: max " + std::to_string(fw));
    Value * aVec = fwCast(fw, a);
    Value * bVec = fwCast(fw, b);
    return CreateSelect(CreateICmpSGT(aVec, bVec), aVec, bVec);
}

Value * IDISA_Builder::simd_umax(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: umax " + std::to_string(fw));
    Value * aVec = fwCast(fw, a);
    Value * bVec = fwCast(fw, b);
    return CreateSelect(CreateICmpUGT(aVec, bVec), aVec, bVec);
}

Value * IDISA_Builder::simd_min(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: min " + std::to_string(fw));
    Value * aVec = fwCast(fw, a);
    Value * bVec = fwCast(fw, b);
    return CreateSelect(CreateICmpSLT(aVec, bVec), aVec, bVec);
}

Value * IDISA_Builder::simd_umin(unsigned fw, Value * a, Value * b) {
    if (fw < 8) report_fatal_error("Unsupported field width: umin " + std::to_string(fw));
    Value * aVec = fwCast(fw, a);
    Value * bVec = fwCast(fw, b);
    return CreateSelect(CreateICmpULT(aVec, bVec), aVec, bVec);
}

Value * IDISA_Builder::mvmd_sll(unsigned fw, Value * value, Value * shift) {
    VectorType * const vecTy = cast<VectorType>(value->getType());
    IntegerType * const intTy = getIntNTy(vecTy->getBitWidth());
    value = CreateBitCast(value, intTy);
    shift = CreateZExtOrTrunc(CreateMul(shift, ConstantInt::get(shift->getType(), fw)), intTy);
    return CreateBitCast(CreateShl(value, shift), vecTy);
}

Value * IDISA_Builder::mvmd_srl(unsigned fw, Value * value, Value * shift) {
    VectorType * const vecTy = cast<VectorType>(value->getType());
    IntegerType * const intTy = getIntNTy(vecTy->getBitWidth());
    value = CreateBitCast(value, intTy);
    shift = CreateZExtOrTrunc(CreateMul(shift, ConstantInt::get(shift->getType(), fw)), intTy);
    return CreateBitCast(CreateLShr(value, shift), vecTy);
}

Value * IDISA_Builder::simd_slli(unsigned fw, Value * a, unsigned shift) {
    if (fw < 16) {
        Constant * value_mask = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                          APInt::getSplat(mBitBlockWidth, APInt::getLowBitsSet(fw, fw-shift)));
        return CreateShl(fwCast(32, simd_and(a, value_mask)), shift);
    }
    return CreateShl(fwCast(fw, a), shift);
}

Value * IDISA_Builder::simd_srli(unsigned fw, Value * a, unsigned shift) {
    if (fw < 16) {
        Constant * value_mask = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                          APInt::getSplat(mBitBlockWidth, APInt::getHighBitsSet(fw, fw-shift)));
        return CreateLShr(fwCast(32, simd_and(a, value_mask)), shift);
    }
    return CreateLShr(fwCast(fw, a), shift);
}

Value * IDISA_Builder::simd_srai(unsigned fw, Value * a, unsigned shift) {
    if (fw < 8) report_fatal_error("Unsupported field width: srai " + std::to_string(fw));
    return CreateAShr(fwCast(fw, a), shift);
}
    
Value * IDISA_Builder::simd_sllv(unsigned fw, Value * v, Value * shifts) {
    if (fw >= 8) return CreateShl(fwCast(fw, v), fwCast(fw, shifts));
    Value * w = v;
    for (unsigned shft_amt = 1; shft_amt < fw; shft_amt *= 2) {
        APInt bit_in_field(fw, shft_amt);
        // To simulate shift within a fw, we need to mask off the high shft_amt bits of each element.
        Constant * value_mask = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                          APInt::getSplat(mBitBlockWidth, APInt::getLowBitsSet(fw, fw-shft_amt)));
        Constant * bit_select = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                          APInt::getSplat(mBitBlockWidth, bit_in_field));
        Value * unshifted_field_mask = simd_eq(fw, simd_and(bit_select, shifts), allZeroes());
        Value * fieldsToShift = simd_and(w, simd_and(value_mask, simd_not(unshifted_field_mask)));
        w = simd_or(simd_and(w, unshifted_field_mask), simd_slli(32, fieldsToShift, shft_amt));
    }
    return w;
}

Value * IDISA_Builder::simd_srlv(unsigned fw, Value * v, Value * shifts) {
    if (fw >= 8) return CreateLShr(fwCast(fw, v), fwCast(fw, shifts));
    Value * w = v;
    for (unsigned shft_amt = 1; shft_amt < fw; shft_amt *= 2) {
        APInt bit_in_field(fw, shft_amt);
        // To simulate shift within a fw, we need to mask off the low shft_amt bits of each element.
        Constant * value_mask = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                          APInt::getSplat(mBitBlockWidth, APInt::getHighBitsSet(fw, fw-shft_amt)));
        Constant * bit_select = Constant::getIntegerValue(getIntNTy(mBitBlockWidth),
                                                          APInt::getSplat(mBitBlockWidth, bit_in_field));
        Value * unshifted_field_mask = simd_eq(fw, simd_and(bit_select, shifts), allZeroes());
        Value * fieldsToShift = simd_and(w, simd_and(value_mask, simd_not(unshifted_field_mask)));
        w = simd_or(simd_and(w, unshifted_field_mask), simd_srli(32, fieldsToShift, shft_amt));
    }
    return w;
}

Value * IDISA_Builder::simd_pext(unsigned fieldwidth, Value * v, Value * extract_mask) {
    Value * delcounts = CreateNot(extract_mask);  // initially deletion counts per 1-bit field
    Value * w = simd_and(extract_mask, v);
    for (unsigned fw = 2; fw < fieldwidth; fw = fw * 2) {
        Value * shift_fwd_field_mask = simd_lomask(fw*2);
        Value * shift_back_field_mask = simd_himask(fw*2);
        Value * shift_back_count_mask = simd_and(shift_back_field_mask, simd_lomask(fw));
        Value * shift_fwd_amts = simd_srli(fw, simd_and(shift_fwd_field_mask, delcounts), fw/2);
        Value * shift_back_amts = simd_and(shift_back_count_mask, delcounts);
        w = simd_or(simd_sllv(fw, simd_and(w, shift_fwd_field_mask), shift_fwd_amts),
                    simd_srlv(fw, simd_and(w, shift_back_field_mask), shift_back_amts));
        delcounts = simd_add(fw, simd_and(simd_lomask(fw), delcounts), simd_srli(fw, delcounts, fw/2));
    }
    // Now shift back all fw fields.
    Value * shift_back_amts = simd_and(simd_lomask(fieldwidth), delcounts);
    w = simd_srlv(fieldwidth, w, shift_back_amts);
    return w;
}

Value * IDISA_Builder::simd_pdep(unsigned fieldwidth, Value * v, Value * deposit_mask) {
    // simd_pdep is implemented by reversing the process of simd_pext.
    // First determine the deletion counts necessary for each stage of the process.
    std::vector<Value *> delcounts;
    delcounts.push_back(simd_not(deposit_mask)); // initially deletion counts per 1-bit field
    for (unsigned fw = 2; fw < fieldwidth; fw = fw * 2) {
        delcounts.push_back(simd_add(fw, simd_and(simd_lomask(fw), delcounts.back()), simd_srli(fw, delcounts.back(), fw/2)));
    }
    //
    // Now reverse the pext process.  First reverse the final shift_back.
    Value * pext_shift_back_amts = simd_and(simd_lomask(fieldwidth), delcounts.back());
    Value * w = simd_sllv(fieldwidth, v, pext_shift_back_amts);
    
    //
    // No work through the smaller field widths.
    for (unsigned fw = fieldwidth/2; fw >= 2; fw = fw/2) {
        delcounts.pop_back();
        Value * pext_shift_fwd_field_mask = simd_lomask(fw*2);
        Value * pext_shift_back_field_mask = simd_himask(fw*2);
        Value * pext_shift_back_count_mask = simd_and(pext_shift_back_field_mask, simd_lomask(fw));
        Value * pext_shift_fwd_amts = simd_srli(fw, simd_and(pext_shift_fwd_field_mask, delcounts.back()), fw/2);
        Value * pext_shift_back_amts = simd_and(pext_shift_back_count_mask, delcounts.back());
        w = simd_or(simd_srlv(fw, simd_and(w, pext_shift_fwd_field_mask), pext_shift_fwd_amts),
                    simd_sllv(fw, simd_and(w, pext_shift_back_field_mask), pext_shift_back_amts));
    }
    return w;
}

Value * IDISA_Builder::simd_bitreverse(unsigned fw, Value * a) {
    /*  Pure sequential solution too slow!
     Value * func = Intrinsic::getDeclaration(getModule(), Intrinsic::bitreverse, fwVectorType(fw));
     return CreateCall(func, fwCast(fw, a));
     */
    if (fw > 8) {
        // Reverse the bits of each byte and then use a byte shuffle to complete the job.
        Value * bitrev8 = fwCast(8, simd_bitreverse(8, a));
        const auto bytes_per_field = fw/8;
        const auto byte_count = mBitBlockWidth / 8;
        Constant * Idxs[byte_count];
        for (unsigned i = 0; i < byte_count; i += bytes_per_field) {
            for (unsigned j = 0; j < bytes_per_field; j++) {
                Idxs[i + j] = getInt32(i + bytes_per_field - j - 1);
            }
        }
        return CreateShuffleVector(bitrev8, UndefValue::get(fwVectorType(8)), ConstantVector::get({Idxs, byte_count}));
    }
    else {
        if (fw > 2) {
            a = simd_bitreverse(fw/2, a);
        }
        return simd_or(simd_srli(16, simd_and(a, simd_himask(fw)), fw/2), simd_slli(16, simd_and(a, simd_lomask(fw)), fw/2));
    }
}

Value * IDISA_Builder::simd_if(unsigned fw, Value * cond, Value * a, Value * b) {
    if (fw == 1) {
        Value * a1 = bitCast(a);
        Value * b1 = bitCast(b);
        Value * c = bitCast(cond);
        return CreateOr(CreateAnd(a1, c), CreateAnd(CreateXor(c, b1), b1));
    } else {
        if (fw < 8) report_fatal_error("Unsupported field width: simd_if " + std::to_string(fw));
        Value * aVec = fwCast(fw, a);
        Value * bVec = fwCast(fw, b);
        return CreateSelect(CreateICmpSLT(cond, mZeroInitializer), aVec, bVec);
    }
}
    
Value * IDISA_Builder::esimd_mergeh(unsigned fw, Value * a, Value * b) {    
    if (fw < 8) report_fatal_error("Unsupported field width: mergeh " + std::to_string(fw));
    const auto field_count = mBitBlockWidth / fw;
    Constant * Idxs[field_count];
    for (unsigned i = 0; i < field_count / 2; i++) {
        Idxs[2 * i] = getInt32(i + field_count / 2); // selects elements from first reg.
        Idxs[2 * i + 1] = getInt32(i + field_count / 2 + field_count); // selects elements from second reg.
    }
    return CreateShuffleVector(fwCast(fw, a), fwCast(fw, b), ConstantVector::get({Idxs, field_count}));
}

Value * IDISA_Builder::esimd_mergel(unsigned fw, Value * a, Value * b) {    
    if (fw < 8) report_fatal_error("Unsupported field width: mergel " + std::to_string(fw));
    const auto field_count = mBitBlockWidth / fw;
    Constant * Idxs[field_count];
    for (unsigned i = 0; i < field_count / 2; i++) {
        Idxs[2 * i] = getInt32(i); // selects elements from first reg.
        Idxs[2 * i + 1] = getInt32(i + field_count); // selects elements from second reg.
    }
    return CreateShuffleVector(fwCast(fw, a), fwCast(fw, b), ConstantVector::get({Idxs, field_count}));
}

Value * IDISA_Builder::esimd_bitspread(unsigned fw, Value * bitmask) {
    if (fw < 8) report_fatal_error("Unsupported field width: bitspread " + std::to_string(fw));
    const auto field_count = mBitBlockWidth / fw;
    Type * field_type = getIntNTy(fw);
    Value * spread_field = CreateBitCast(CreateZExtOrTrunc(bitmask, field_type), VectorType::get(getIntNTy(fw), 1));
    Value * undefVec = UndefValue::get(VectorType::get(getIntNTy(fw), 1));
    Value * broadcast = CreateShuffleVector(spread_field, undefVec, Constant::getNullValue(VectorType::get(getInt32Ty(), field_count)));
    Constant * bitSel[field_count];
    Constant * bitShift[field_count];
    for (unsigned i = 0; i < field_count; i++) {
        bitSel[i] = ConstantInt::get(field_type, 1 << i);
        bitShift[i] = ConstantInt::get(field_type, i);
    }
    Value * bitSelVec = ConstantVector::get({bitSel, field_count});
    Value * bitShiftVec = ConstantVector::get({bitShift, field_count});
    return CreateLShr(CreateAnd(bitSelVec, broadcast), bitShiftVec);
}

Value * IDISA_Builder::hsimd_packh(unsigned fw, Value * a, Value * b) {
    if (fw <= 8) {
        const unsigned fw_wkg = 32;
        Value * aLo = simd_srli(fw_wkg, a, fw/2);
        Value * bLo = simd_srli(fw_wkg, b, fw/2);
        return hsimd_packl(fw, aLo, bLo);
    }
    Value * aVec = fwCast(fw/2, a);
    Value * bVec = fwCast(fw/2, b);
    const auto field_count = 2 * mBitBlockWidth / fw;
    Constant * Idxs[field_count];
    for (unsigned i = 0; i < field_count; i++) {
        Idxs[i] = getInt32(2 * i + 1);
    }
    return CreateShuffleVector(aVec, bVec, ConstantVector::get({Idxs, field_count}));
}

Value * IDISA_Builder::hsimd_packl(unsigned fw, Value * a, Value * b) {
    if (fw <= 8) {
        const unsigned fw_wkg = 32;
        Value * aLo = simd_srli(fw_wkg, a, fw/2);
        Value * bLo = simd_srli(fw_wkg, b, fw/2);
        return hsimd_packl(fw*2,
                           bitCast(simd_or(simd_and(simd_himask(fw), aLo), simd_and(simd_lomask(fw), a))),
                           bitCast(simd_or(simd_and(simd_himask(fw), bLo), simd_and(simd_lomask(fw), b))));
    }
    Value * aVec = fwCast(fw/2, a);
    Value * bVec = fwCast(fw/2, b);
    const auto field_count = 2 * mBitBlockWidth / fw;
    Constant * Idxs[field_count];
    for (unsigned i = 0; i < field_count; i++) {
        Idxs[i] = getInt32(2 * i);
    }
    return CreateShuffleVector(aVec, bVec, ConstantVector::get({Idxs, field_count}));
}

Value * IDISA_Builder::hsimd_packh_in_lanes(unsigned lanes, unsigned fw, Value * a, Value * b) {
    if (fw < 16) report_fatal_error("Unsupported field width: packh_in_lanes " + std::to_string(fw));
    const unsigned fw_out = fw / 2;
    const unsigned fields_per_lane = mBitBlockWidth / (fw_out * lanes);
    const unsigned field_offset_for_b = mBitBlockWidth / fw_out;
    const unsigned field_count = mBitBlockWidth / fw_out;
    Constant * Idxs[field_count];
    for (unsigned lane = 0, j = 0; lane < lanes; lane++) {
        const unsigned first_field_in_lane = lane * fields_per_lane; // every second field
        for (unsigned i = 0; i < fields_per_lane / 2; i++) {
            Idxs[j++] = getInt32(first_field_in_lane + (2 * i) + 1);
        }
        for (unsigned i = 0; i < fields_per_lane / 2; i++) {
            Idxs[j++] = getInt32(field_offset_for_b + first_field_in_lane + (2 * i) + 1);
        }
    }
    return CreateShuffleVector(fwCast(fw_out, a), fwCast(fw_out, b), ConstantVector::get({Idxs, field_count}));
}

Value * IDISA_Builder::hsimd_packl_in_lanes(unsigned lanes, unsigned fw, Value * a, Value * b) {
    if (fw < 16) report_fatal_error("Unsupported field width: packl_in_lanes " + std::to_string(fw));
    const unsigned fw_out = fw / 2;
    const unsigned fields_per_lane = mBitBlockWidth / (fw_out * lanes);
    const unsigned field_offset_for_b = mBitBlockWidth / fw_out;
    const unsigned field_count = mBitBlockWidth / fw_out;
    Constant * Idxs[field_count];
    for (unsigned lane = 0, j = 0; lane < lanes; lane++) {
        const unsigned first_field_in_lane = lane * fields_per_lane; // every second field
        for (unsigned i = 0; i < fields_per_lane / 2; i++) {
            Idxs[j++] = getInt32(first_field_in_lane + (2 * i));
        }
        for (unsigned i = 0; i < fields_per_lane / 2; i++) {
            Idxs[j++] = getInt32(field_offset_for_b + first_field_in_lane + (2 * i));
        }
    }
    return CreateShuffleVector(fwCast(fw_out, a), fwCast(fw_out, b), ConstantVector::get({Idxs, field_count}));
}

Value * IDISA_Builder::hsimd_signmask(unsigned fw, Value * a) {
    if (fw < 8) report_fatal_error("Unsupported field width: hsimd_signmask " + std::to_string(fw));
    Value * mask = CreateICmpSLT(fwCast(fw, a), ConstantAggregateZero::get(fwVectorType(fw)));
    mask = CreateBitCast(mask, getIntNTy(mBitBlockWidth/fw));
    if (mBitBlockWidth/fw < 32) return CreateZExt(mask, getInt32Ty());
    else return mask;
}

Value * IDISA_Builder::mvmd_extract(unsigned fw, Value * a, unsigned fieldIndex) {
    if (fw < 8) report_fatal_error("Unsupported field width: mvmd_extract " + std::to_string(fw));
    return CreateExtractElement(fwCast(fw, a), getInt32(fieldIndex));
}

Value * IDISA_Builder::mvmd_insert(unsigned fw, Value * blk, Value * elt, unsigned fieldIndex) {
    if (fw < 8) report_fatal_error("Unsupported field width: mvmd_insert " + std::to_string(fw));
    return CreateInsertElement(fwCast(fw, blk), elt, getInt32(fieldIndex));
}

Value * IDISA_Builder::mvmd_slli(unsigned fw, Value * a, unsigned shift) {
    if (fw < 8) report_fatal_error("Unsupported field width: mvmd_slli " + std::to_string(fw));
    const auto field_count = mBitBlockWidth / fw;
    return mvmd_dslli(fw, a, Constant::getNullValue(fwVectorType(fw)), field_count - shift);
}

Value * IDISA_Builder::mvmd_srli(unsigned fw, Value * a, unsigned shift) {
    if (fw < 8) report_fatal_error("Unsupported field width: mvmd_srli " + std::to_string(fw));
    return mvmd_dslli(fw, Constant::getNullValue(fwVectorType(fw)), a, shift);
}

Value * IDISA_Builder::mvmd_dslli(unsigned fw, Value * a, Value * b, unsigned shift) {
    if (fw < 8) report_fatal_error("Unsupported field width: mvmd_dslli " + std::to_string(fw));
    const auto field_count = mBitBlockWidth/fw;
    Constant * Idxs[field_count];
    for (unsigned i = 0; i < field_count; i++) {
        Idxs[i] = getInt32(i + shift);
    }
    return CreateShuffleVector(fwCast(fw, b), fwCast(fw, a), ConstantVector::get({Idxs, field_count}));
}

Value * IDISA_Builder::bitblock_any(Value * a) {
    Type * iBitBlock = getIntNTy(mBitBlockWidth);
    return CreateICmpNE(CreateBitCast(a, iBitBlock),  ConstantInt::getNullValue(iBitBlock));
}

// full add producing {carryout, sum}
std::pair<Value *, Value *> IDISA_Builder::bitblock_add_with_carry(Value * a, Value * b, Value * carryin) {
    Value * carrygen = simd_and(a, b);
    Value * carryprop = simd_or(a, b);
    Value * sum = simd_add(mBitBlockWidth, simd_add(mBitBlockWidth, a, b), carryin);
    Value * carryout = CreateBitCast(simd_or(carrygen, simd_and(carryprop, CreateNot(sum))), getIntNTy(mBitBlockWidth));
    return std::pair<Value *, Value *>(bitCast(simd_srli(mBitBlockWidth, carryout, mBitBlockWidth - 1)), bitCast(sum));
}

// full shift producing {shiftout, shifted}
std::pair<Value *, Value *> IDISA_Builder::bitblock_advance(Value * a, Value * shiftin, unsigned shift) {
    Value * shiftin_bitblock = CreateBitCast(shiftin, getIntNTy(mBitBlockWidth));
    Value * a_bitblock = CreateBitCast(a, getIntNTy(mBitBlockWidth));
    Value * shifted = bitCast(CreateOr(CreateShl(a_bitblock, shift), shiftin_bitblock));
    Value * shiftout = bitCast(CreateLShr(a_bitblock, mBitBlockWidth - shift));
    return std::pair<Value *, Value *>(shiftout, shifted);
}

// full shift producing {shiftout, shifted}
std::pair<Value *, Value *> IDISA_Builder::bitblock_indexed_advance(Value * strm, Value * index_strm, Value * shiftIn, unsigned shiftAmount) {
    const unsigned bitWidth = getSizeTy()->getBitWidth();
    Type * const iBitBlock = getIntNTy(getBitBlockWidth());
    Value * const shiftVal = getSize(shiftAmount);
    Value * extracted_bits = simd_pext(bitWidth, strm, index_strm);
    Value * ix_popcounts = simd_popcount(bitWidth, index_strm);
    const auto n = getBitBlockWidth() / bitWidth;
    VectorType * const vecTy = VectorType::get(getSizeTy(), n);
    if (LLVM_LIKELY(shiftAmount < bitWidth)) {
        Value * carry = mvmd_extract(bitWidth, shiftIn, 0);
        Value * result = UndefValue::get(vecTy);
        for (unsigned i = 0; i < n; i++) {
            Value * ix_popcnt = mvmd_extract(bitWidth, ix_popcounts, i);
            Value * bits = mvmd_extract(bitWidth, extracted_bits, i);
            Value * adv = CreateOr(CreateShl(bits, shiftAmount), carry);
            // We have two cases depending on whether the popcount of the index pack is < shiftAmount or not.
            Value * popcount_small = CreateICmpULT(ix_popcnt, shiftVal);
            Value * carry_if_popcount_small =
                CreateOr(CreateShl(bits, CreateSub(shiftVal, ix_popcnt)),
                            CreateLShr(carry, ix_popcnt));
            Value * carry_if_popcount_large = CreateLShr(bits, CreateSub(ix_popcnt, shiftVal));
            carry = CreateSelect(popcount_small, carry_if_popcount_small, carry_if_popcount_large);
            result = mvmd_insert(bitWidth, result, adv, i);
        }
        Value * carryOut = mvmd_insert(bitWidth, allZeroes(), carry, 0);
        return std::pair<Value *, Value *>{bitCast(carryOut), simd_pdep(bitWidth, result, index_strm)};
    }
    else if (shiftAmount <= mBitBlockWidth) {
        // The shift amount is always greater than the popcount of the individual
        // elements that we deal with.   This simplifies some of the logic.
        Value * carry = CreateBitCast(shiftIn, iBitBlock);
        Value * result = UndefValue::get(vecTy);
        for (unsigned i = 0; i < n; i++) {
            Value * ix_popcnt = mvmd_extract(bitWidth, ix_popcounts, i);
            Value * bits = mvmd_extract(bitWidth, extracted_bits, i);  // All these bits are shifted out (appended to carry).
            result = mvmd_insert(bitWidth, result, mvmd_extract(bitWidth, carry, 0), i);
            carry = CreateLShr(carry, CreateZExt(ix_popcnt, iBitBlock)); // Remove the carry bits consumed, make room for new bits.
            carry = CreateOr(carry, CreateShl(CreateZExt(bits, iBitBlock), CreateZExt(CreateSub(shiftVal, ix_popcnt), iBitBlock)));
        }
        return std::pair<Value *, Value *>{bitCast(carry), simd_pdep(bitWidth, result, index_strm)};
    }
    else {
        // The shift amount is greater than the total popcount.   We will consume popcount
        // bits from the shiftIn value only, and produce a carry out value of the selected bits.
        Value * carry = CreateBitCast(shiftIn, iBitBlock);
        Value * result = UndefValue::get(vecTy);
        Value * carryOut = ConstantInt::getNullValue(iBitBlock);
        Value * generated = getSize(0);
        for (unsigned i = 0; i < n; i++) {
            Value * ix_popcnt = mvmd_extract(bitWidth, ix_popcounts, i);
            Value * bits = mvmd_extract(bitWidth, extracted_bits, i);  // All these bits are shifted out (appended to carry).
            result = mvmd_insert(bitWidth, result, mvmd_extract(bitWidth, carry, 0), i);
            carry = CreateLShr(carry, CreateZExt(ix_popcnt, iBitBlock)); // Remove the carry bits consumed.
            carryOut = CreateOr(carryOut, CreateShl(CreateZExt(bits, iBitBlock), CreateZExt(generated, iBitBlock)));
            generated = CreateAdd(generated, ix_popcnt);
        }
        return std::pair<Value *, Value *>{bitCast(carryOut), simd_pdep(bitWidth, result, index_strm)};
    }
}


Value * IDISA_Builder::bitblock_mask_from(Value * pos) {
    Type * const ty = getIntNTy(getBitBlockWidth());
    Constant * const ONES = ConstantInt::getAllOnesValue(ty);
    Constant * const ZEROES = ConstantInt::getNullValue(ty);
    Constant * const BIT_BLOCK_WIDTH = ConstantInt::get(pos->getType(), getBitBlockWidth());
    Value * const mask = CreateSelect(CreateICmpULT(pos, BIT_BLOCK_WIDTH), CreateShl(ONES, CreateZExt(pos, ty)), ZEROES);
    return bitCast(mask);
}

Value * IDISA_Builder::bitblock_set_bit(Value * pos) {
    Type * const ty = getIntNTy(getBitBlockWidth());
    return bitCast(CreateShl(ConstantInt::get(ty, 1), CreateZExt(pos, ty)));
}

Value * IDISA_Builder::simd_and(Value * a, Value * b) {
    return a->getType() == b->getType() ? CreateAnd(a, b) : CreateAnd(bitCast(a), bitCast(b));
}

Value * IDISA_Builder::simd_or(Value * a, Value * b) {
    return a->getType() == b->getType() ? CreateOr(a, b) : CreateOr(bitCast(a), bitCast(b));
}
    
Value * IDISA_Builder::simd_xor(Value * a, Value * b) {
    return a->getType() == b->getType() ? CreateXor(a, b) : CreateXor(bitCast(a), bitCast(b));
}

Value * IDISA_Builder::simd_not(Value * a) {
    return simd_xor(a, Constant::getAllOnesValue(a->getType()));
}

IDISA_Builder::IDISA_Builder(LLVMContext & C, unsigned vectorWidth, unsigned stride)
: CBuilder(C)
, mBitBlockWidth(vectorWidth)
, mStride(stride)
, mBitBlockType(VectorType::get(IntegerType::get(C, 64), vectorWidth / 64))
, mZeroInitializer(Constant::getNullValue(mBitBlockType))
, mOneInitializer(Constant::getAllOnesValue(mBitBlockType))
, mPrintRegisterFunction(nullptr) {

}

IDISA_Builder::~IDISA_Builder() {

}

}
