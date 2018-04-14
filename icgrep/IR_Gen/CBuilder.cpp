/*
 *  Copyright (c) 2016 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#include "CBuilder.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/TypeBuilder.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Format.h>
#include <toolchain/toolchain.h>
#include <toolchain/driver.h>
#include <set>
#include <thread>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#if defined(__i386__)
typedef uint32_t unw_word_t;
#else
typedef uint64_t unw_word_t;
#endif
#if defined(HAS_MACH_VM_TYPES)
#include <mach/vm_types.h>
extern void _thread_stack_pcs(vm_address_t *buffer, unsigned max, unsigned *nb, unsigned skip);
static_assert(sizeof(vm_address_t) == sizeof(uintptr_t), "");
#elif defined(HAS_LIBUNWIND)
#define UNW_LOCAL_ONLY
#include <libunwind.h>
static_assert(sizeof(unw_word_t) <= sizeof(uintptr_t), "");
#elif defined(HAS_EXECINFO)
#include <execinfo.h>
static_assert(sizeof(void *) == sizeof(uintptr_t), "");
#endif

using namespace llvm;


Value * CBuilder::CreateURem(Value * number, Value * divisor, const Twine &Name) {
    if (ConstantInt * c = dyn_cast<ConstantInt>(divisor)) {
        uint64_t d = c->getZExtValue();
        if ((d & (d - 1)) == 0) { // is a power of 2 or 0
            if (d > 0) return CreateAnd(number, ConstantInt::get(divisor->getType(), d - 1), Name);
        }
    }
    return Insert(BinaryOperator::CreateURem(number, divisor), Name);
}

Value * CBuilder::CreateUDiv(Value * number, Value * divisor, const Twine &Name) {
    if (ConstantInt * c = dyn_cast<ConstantInt>(divisor)) {
        uint64_t d = c->getZExtValue();
        if ((d & (d - 1)) == 0) { // is a power of 2 or 0
            if (d > 1) return CreateLShr(number, ConstantInt::get(divisor->getType(), std::log2(d)), Name);
            else if (d == 1) return number;
        }
    }
    return Insert(BinaryOperator::CreateUDiv(number, divisor), Name);
}

Value * CBuilder::CreateUDivCeil(Value * number, Value * divisor, const Twine &Name) {
    if (ConstantInt * c = dyn_cast<ConstantInt>(divisor)) {
        uint64_t d = c->getZExtValue();
        if ((d & (d - 1)) == 0) { // is a power of 2 or 0
            if (d > 1) {
                Value * n = CreateAdd(number, ConstantInt::get(divisor->getType(), d - 1));
                return CreateLShr(n, ConstantInt::get(divisor->getType(), std::log2(d)), Name);
            }
            else if (d == 1) return number;
        }
    }
    return CreateUDiv(CreateAdd(number, CreateSub(divisor, ConstantInt::get(divisor->getType(), 1))), divisor, Name);
}

Value * CBuilder::CreateRoundUp(Value * number, Value * divisor, const Twine &Name) {
    return CreateMul(CreateUDivCeil(number, divisor), divisor, Name);
}

Value * CBuilder::CreateOpenCall(Value * filename, Value * oflag, Value * mode) {
    Module * const m = getModule();
    Function * openFn = m->getFunction("open");
    if (openFn == nullptr) {
        IntegerType * int32Ty = getInt32Ty();
        PointerType * int8PtrTy = getInt8PtrTy();
        openFn = cast<Function>(m->getOrInsertFunction("open",
                                                         int32Ty, int8PtrTy, int32Ty, int32Ty, nullptr));
    }
    return CreateCall(openFn, {filename, oflag, mode});
}

// ssize_t write(int fildes, const void *buf, size_t nbyte);
Value * CBuilder::CreateWriteCall(Value * fileDescriptor, Value * buf, Value * nbyte) {
    PointerType * voidPtrTy = getVoidPtrTy();
    Module * const m = getModule();
    Function * write = m->getFunction("write");
    if (write == nullptr) {
        IntegerType * sizeTy = getSizeTy();
        IntegerType * int32Ty = getInt32Ty();
        write = cast<Function>(m->getOrInsertFunction("write",
                                                        AttributeSet().addAttribute(getContext(), 2U, Attribute::NoAlias),
                                                        sizeTy, int32Ty, voidPtrTy, sizeTy, nullptr));
    }
    buf = CreatePointerCast(buf, voidPtrTy);
    return CreateCall(write, {fileDescriptor, buf, nbyte});
}

Value * CBuilder::CreateReadCall(Value * fileDescriptor, Value * buf, Value * nbyte) {
    PointerType * voidPtrTy = getVoidPtrTy();
    Module * const m = getModule();
    Function * readFn = m->getFunction("read");
    if (readFn == nullptr) {
        IntegerType * sizeTy = getSizeTy();
        IntegerType * int32Ty = getInt32Ty();
        readFn = cast<Function>(m->getOrInsertFunction("read",
                                                         AttributeSet().addAttribute(getContext(), 2U, Attribute::NoAlias),
                                                         sizeTy, int32Ty, voidPtrTy, sizeTy, nullptr));
    }
    buf = CreatePointerCast(buf, voidPtrTy);
    return CreateCall(readFn, {fileDescriptor, buf, nbyte});
}

Value * CBuilder::CreateCloseCall(Value * fileDescriptor) {
    Module * const m = getModule();
    Function * closeFn = m->getFunction("close");
    if (closeFn == nullptr) {
        IntegerType * int32Ty = getInt32Ty();
        FunctionType * fty = FunctionType::get(int32Ty, {int32Ty}, true);
        closeFn = Function::Create(fty, Function::ExternalLinkage, "close", m);
    }
    return CreateCall(closeFn, fileDescriptor);
}

Value * CBuilder::CreateUnlinkCall(Value * path) {
    Module * const m = getModule();
    Function * unlinkFunc = m->getFunction("unlink");
    if (unlinkFunc == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), {getInt8PtrTy()}, false);
        unlinkFunc = Function::Create(fty, Function::ExternalLinkage, "unlink", m);
        unlinkFunc->setCallingConv(CallingConv::C);
    }
    return CreateCall(unlinkFunc, path);
}

Value * CBuilder::CreateMkstempCall(Value * ftemplate) {
    Module * const m = getModule();
    Function * mkstempFn = m->getFunction("mkstemp");
    if (mkstempFn == nullptr) {
        mkstempFn = cast<Function>(m->getOrInsertFunction("mkstemp", getInt32Ty(), getInt8PtrTy(), nullptr));
    }
    return CreateCall(mkstempFn, ftemplate);
}

Value * CBuilder::CreateStrlenCall(Value * str) {
    Module * const m = getModule();
    Function * strlenFn = m->getFunction("strlen");
    if (strlenFn == nullptr) {
        strlenFn = cast<Function>(m->getOrInsertFunction("strlen", getSizeTy(), getInt8PtrTy(), nullptr));
    }
    return CreateCall(strlenFn, str);
}

Function * CBuilder::GetPrintf() {
    Module * const m = getModule();
    Function * printf = m->getFunction("printf");
    if (printf == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), {getInt8PtrTy()}, true);
        printf = Function::Create(fty, Function::ExternalLinkage, "printf", m);
        printf->addAttribute(1, Attribute::NoAlias);
    }
    return printf;
}

Function * CBuilder::GetDprintf() {
    Module * const m = getModule();
    Function * dprintf = m->getFunction("dprintf");
    if (dprintf == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), {getInt32Ty(), getInt8PtrTy()}, true);
        dprintf = Function::Create(fty, Function::ExternalLinkage, "dprintf", m);
    }
    return dprintf;
}

void CBuilder::CallPrintInt(const std::string & name, Value * const value) {
    Module * const m = getModule();
    Constant * printRegister = m->getFunction("PrintInt");
    IntegerType * int64Ty = getInt64Ty();
    if (LLVM_UNLIKELY(printRegister == nullptr)) {
        FunctionType *FT = FunctionType::get(getVoidTy(), { getInt8PtrTy(), int64Ty }, false);
        Function * function = Function::Create(FT, Function::InternalLinkage, "PrintInt", m);
        auto arg = function->arg_begin();
        std::string out = "%-40s = %" PRIx64 "\n";
        BasicBlock * entry = BasicBlock::Create(getContext(), "entry", function);
        IRBuilder<> builder(entry);
        std::vector<Value *> args;
        args.push_back(GetString(out.c_str()));
        Value * const name = &*(arg++);
        name->setName("name");
        args.push_back(name);
        Value * value = &*arg;
        value->setName("value");
        args.push_back(value);
        builder.CreateCall(GetPrintf(), args);
        builder.CreateRetVoid();

        printRegister = function;
    }
    Value * num = nullptr;
    if (value->getType()->isPointerTy()) {
        num = CreatePtrToInt(value, int64Ty);
    } else {
        num = CreateZExtOrBitCast(value, int64Ty);
    }
    assert (num->getType()->isIntegerTy());
    CreateCall(printRegister, {GetString(name.c_str()), num});
}

void CBuilder::CallPrintIntToStderr(const std::string & name, Value * const value) {
    Module * const m = getModule();
    Constant * printRegister = m->getFunction("PrintIntToStderr");
    if (LLVM_UNLIKELY(printRegister == nullptr)) {
        FunctionType *FT = FunctionType::get(getVoidTy(), { PointerType::get(getInt8Ty(), 0), getSizeTy() }, false);
        Function * function = Function::Create(FT, Function::InternalLinkage, "PrintIntToStderr", m);
        auto arg = function->arg_begin();
        std::string out = "%-40s = %" PRIx64 "\n";
        BasicBlock * entry = BasicBlock::Create(getContext(), "entry", function);
        IRBuilder<> builder(entry);
        std::vector<Value *> args;
        args.push_back(getInt32(STDERR_FILENO));
        args.push_back(GetString(out.c_str()));
        Value * const name = &*(arg++);
        name->setName("name");
        args.push_back(name);
        Value * value = &*arg;
        value->setName("value");
        args.push_back(value);
        builder.CreateCall(GetDprintf(), args);
        builder.CreateRetVoid();

        printRegister = function;
    }
    Value * num = nullptr;
    if (value->getType()->isPointerTy()) {
        num = CreatePtrToInt(value, getSizeTy());
    } else {
        num = CreateZExtOrBitCast(value, getSizeTy());
    }
    assert (num->getType()->isIntegerTy());
    CreateCall(printRegister, {GetString(name.c_str()), num});
}

void CBuilder::CallPrintMsgToStderr(const std::string & message) {
    Module * const m = getModule();
    Constant * printMsg = m->getFunction("PrintMsgToStderr");
    if (LLVM_UNLIKELY(printMsg == nullptr)) {
        FunctionType *FT = FunctionType::get(getVoidTy(), { PointerType::get(getInt8Ty(), 0) }, false);
        Function * function = Function::Create(FT, Function::InternalLinkage, "PrintMsgToStderr", m);
        auto arg = function->arg_begin();
        std::string out = "%s\n";
        BasicBlock * entry = BasicBlock::Create(getContext(), "entry", function);
        IRBuilder<> builder(entry);
        std::vector<Value *> args;
        args.push_back(getInt32(STDERR_FILENO));
        args.push_back(GetString(out));
        Value * const msg = &*(arg++);
        msg->setName("msg");
        args.push_back(msg);
        builder.CreateCall(GetDprintf(), args);
        builder.CreateRetVoid();

        printMsg = function;
    }
    CreateCall(printMsg, {GetString(message.c_str())});
}

Value * CBuilder::CreateMalloc(Value * size) {
    Module * const m = getModule();
    IntegerType * const sizeTy = getSizeTy();    
    Function * f = m->getFunction("malloc");
    if (f == nullptr) {
        PointerType * const voidPtrTy = getVoidPtrTy();
        FunctionType * fty = FunctionType::get(voidPtrTy, {sizeTy}, false);
        f = Function::Create(fty, Function::ExternalLinkage, "malloc", m);
        f->setCallingConv(CallingConv::C);
        f->setDoesNotAlias(0);
    }
    size = CreateZExtOrTrunc(size, sizeTy);
    CallInst * const ptr = CreateCall(f, size);
    CreateAssert(ptr, "CreateMalloc: returned null pointer");
    return ptr;
}

llvm::Value * CBuilder::CreateCacheAlignedMalloc(llvm::Value * size) {
    const auto alignment = getCacheAlignment();
    if (LLVM_LIKELY(isa<Constant>(size))) {
        Constant * const align = ConstantInt::get(size->getType(), alignment, false);
        Constant * offset = ConstantExpr::getURem(cast<Constant>(size), align);
        if (!offset->isNullValue()) {
            size = ConstantExpr::getAdd(cast<Constant>(size), ConstantExpr::getSub(align, offset));
        }
    }
    return CreateAlignedMalloc(size, alignment);
}

Value * CBuilder::CreateAlignedMalloc(Value * size, const unsigned alignment) {
    if (LLVM_UNLIKELY((alignment & (alignment - 1)) != 0)) {
        report_fatal_error("CreateAlignedMalloc: alignment must be a power of 2");
    }
    Module * const m = getModule();
    IntegerType * const sizeTy = getSizeTy();
    PointerType * const voidPtrTy = getVoidPtrTy();

    size = CreateZExtOrTrunc(size, sizeTy);
    ConstantInt * const align = ConstantInt::get(sizeTy, alignment);
    if (codegen::EnableAsserts) {
        CreateAssertZero(CreateURem(size, align), "CreateAlignedMalloc: size must be an integral multiple of alignment.");
    }
    Value * ptr = nullptr;
    if (hasAlignedAlloc()) {
        Function * f = m->getFunction("aligned_alloc");
        if (LLVM_UNLIKELY(f == nullptr)) {
            FunctionType * const fty = FunctionType::get(voidPtrTy, {sizeTy, sizeTy}, false);
            f = Function::Create(fty, Function::ExternalLinkage, "aligned_alloc", m);
            f->setCallingConv(CallingConv::C);
            f->setDoesNotAlias(0);
        }
        ptr = CreateCall(f, {align, size});
    } else if (hasPosixMemalign()) {
        Function * f = m->getFunction("posix_memalign");
        if (LLVM_UNLIKELY(f == nullptr)) {
            FunctionType * const fty = FunctionType::get(getInt32Ty(), {voidPtrTy->getPointerTo(), sizeTy, sizeTy}, false);
            f = Function::Create(fty, Function::ExternalLinkage, "posix_memalign", m);
            f->setCallingConv(CallingConv::C);
            f->setDoesNotAlias(0);
            f->setDoesNotAlias(1);
        }
        Value * handle = CreateAlloca(voidPtrTy);
        CallInst * success = CreateCall(f, {handle, align, size});
        if (codegen::EnableAsserts) {
            CreateAssertZero(success, "CreateAlignedMalloc: posix_memalign reported bad allocation");
        }
        ptr = CreateLoad(handle);
    } else {
        report_fatal_error("stdlib.h does not contain either aligned_alloc or posix_memalign");
    }
    CreateAssert(ptr, "CreateAlignedMalloc: returned null pointer.");
    return ptr;
}

inline bool CBuilder::hasAlignedAlloc() const {
    return mDriver && mDriver->hasExternalFunction("aligned_alloc");
}


inline bool CBuilder::hasPosixMemalign() const {
    return mDriver && mDriver->hasExternalFunction("posix_memalign");
}

Value * CBuilder::CreateRealloc(Value * const ptr, Value * const size) {
    Module * const m = getModule();
    IntegerType * const sizeTy = getSizeTy();
    PointerType * const voidPtrTy = getVoidPtrTy();
    Function * f = m->getFunction("realloc");
    if (f == nullptr) {
        FunctionType * fty = FunctionType::get(voidPtrTy, {voidPtrTy, sizeTy}, false);
        f = Function::Create(fty, Function::ExternalLinkage, "realloc", m);
        f->setCallingConv(CallingConv::C);
        f->setDoesNotAlias(0);
        f->setDoesNotAlias(1);
    }
    CallInst * const ci = CreateCall(f, {CreatePointerCast(ptr, voidPtrTy), CreateZExtOrTrunc(size, sizeTy)});
    return CreatePointerCast(ci, ptr->getType());
}

void CBuilder::CreateFree(Value * const ptr) {
    assert (ptr->getType()->isPointerTy());
    Module * const m = getModule();
    Type * const voidPtrTy =  getVoidPtrTy();
    Function * f = m->getFunction("free");
    if (f == nullptr) {
        FunctionType * fty = FunctionType::get(getVoidTy(), {voidPtrTy}, false);
        f = Function::Create(fty, Function::ExternalLinkage, "free", m);
        f->setCallingConv(CallingConv::C);
    }
    CreateCall(f, CreatePointerCast(ptr, voidPtrTy));
}

Value * CBuilder::CreateAnonymousMMap(Value * size) {
    PointerType * const voidPtrTy = getVoidPtrTy();
    IntegerType * const intTy = getInt32Ty();
    IntegerType * const sizeTy = getSizeTy();
    size = CreateZExtOrTrunc(size, sizeTy);
    ConstantInt * const prot =  ConstantInt::get(intTy, PROT_READ | PROT_WRITE);
    ConstantInt * const flags =  ConstantInt::get(intTy, MAP_PRIVATE | MAP_ANON);
    ConstantInt * const fd =  ConstantInt::get(intTy, -1);
    Constant * const offset = ConstantInt::get(sizeTy, 0);
    return CreateMMap(ConstantPointerNull::getNullValue(voidPtrTy), size, prot, flags, fd, offset);
}

Value * CBuilder::CreateFileSourceMMap(Value * fd, Value * size) {
    PointerType * const voidPtrTy = getVoidPtrTy();
    IntegerType * const intTy = getInt32Ty();
    fd = CreateZExtOrTrunc(fd, intTy);
    IntegerType * const sizeTy = getSizeTy();
    size = CreateZExtOrTrunc(size, sizeTy);
    ConstantInt * const prot =  ConstantInt::get(intTy, PROT_READ);
    ConstantInt * const flags =  ConstantInt::get(intTy, MAP_PRIVATE);
    Constant * const offset = ConstantInt::get(sizeTy, 0);       
    return CreateMMap(ConstantPointerNull::getNullValue(voidPtrTy), size, prot, flags, fd, offset);
}

Value * CBuilder::CreateMMap(Value * const addr, Value * size, Value * const prot, Value * const flags, Value * const fd, Value * const offset) {
    Module * const m = getModule();
    Function * fMMap = m->getFunction("mmap");
    if (LLVM_UNLIKELY(fMMap == nullptr)) {
        PointerType * const voidPtrTy = getVoidPtrTy();
        IntegerType * const intTy = getInt32Ty();
        IntegerType * const sizeTy = getSizeTy();
        FunctionType * fty = FunctionType::get(voidPtrTy, {voidPtrTy, sizeTy, intTy, intTy, intTy, sizeTy}, false);
        fMMap = Function::Create(fty, Function::ExternalLinkage, "mmap", m);
    }
    Value * ptr = CreateCall(fMMap, {addr, size, prot, flags, fd, offset});
    if (codegen::EnableAsserts) {
        DataLayout DL(m);
        IntegerType * const intTy = getIntPtrTy(DL);
        Value * success = CreateICmpNE(CreatePtrToInt(addr, intTy), ConstantInt::getAllOnesValue(intTy)); // MAP_FAILED = -1
        CreateAssert(success, "CreateMMap: mmap failed to allocate memory");
    }
    return ptr;
}

/**
 * @brief CBuilder::CreateMAdvise
 * @param addr
 * @param length
 * @param advice
 *
 * Note: this funcition can fail if a kernel resource was temporarily unavailable. Test if this is more than a simple hint and handle accordingly.
 *
 *  ADVICE_NORMAL
 *      No special treatment. This is the default.
 *  ADVICE_RANDOM
 *      Expect page references in random order. (Hence, read ahead may be less useful than normally.)
 *  ADVICE_SEQUENTIAL
 *      Expect page references in sequential order. (Hence, pages in the given range can be aggressively read ahead, and may be freed
 *      soon after they are accessed.)
 *  ADVICE_WILLNEED
 *      Expect access in the near future. (Hence, it might be a good idea to read some pages ahead.)
 *  ADVICE_DONTNEED
 *      Do not expect access in the near future. (For the time being, the application is finished with the given range, so the kernel
 *      can free resources associated with it.) Subsequent accesses of pages in this range will succeed, but will result either in
 *      reloading of the memory contents from the underlying mapped file (see mmap(2)) or zero-fill-on-demand pages for mappings
 *      without an underlying file.
 *
 * @return Value indicating success (0) or failure (-1).
 */
Value * CBuilder::CreateMAdvise(Value * addr, Value * length, Advice advice) {
    Triple T(mTriple);
    Value * result = nullptr;
    if (T.isOSLinux() || T.isOSDarwin()) {
        Module * const m = getModule();
        IntegerType * const intTy = getInt32Ty();
        IntegerType * const sizeTy = getSizeTy();
        PointerType * const voidPtrTy = getVoidPtrTy();
        Function * MAdviseFunc = m->getFunction("madvise");
        if (LLVM_UNLIKELY(MAdviseFunc == nullptr)) {
            FunctionType * fty = FunctionType::get(intTy, {voidPtrTy, sizeTy, intTy}, false);
            MAdviseFunc = Function::Create(fty, Function::ExternalLinkage, "madvise", m);
        }
        addr = CreatePointerCast(addr, voidPtrTy);
        length = CreateZExtOrTrunc(length, sizeTy);
        int madv_flag = 0;
        switch (advice) {
            case Advice::ADVICE_NORMAL:
                madv_flag = MADV_NORMAL; break;
            case Advice::ADVICE_RANDOM:
                madv_flag = MADV_RANDOM; break;
            case Advice::ADVICE_SEQUENTIAL:
                madv_flag = MADV_SEQUENTIAL; break;
            case Advice::ADVICE_WILLNEED:
                madv_flag = MADV_WILLNEED; break;
            case Advice::ADVICE_DONTNEED:
                madv_flag = MADV_DONTNEED; break;
        }
        result = CreateCall(MAdviseFunc, {addr, length, ConstantInt::get(intTy, madv_flag)});
    }
    return result;
}

#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE	1
#endif

Value * CBuilder::CreateMRemap(Value * addr, Value * oldSize, Value * newSize) {
    Triple T(mTriple);
    Value * ptr = nullptr;
    if (T.isOSLinux()) {
        Module * const m = getModule();
        DataLayout DL(m);
        PointerType * const voidPtrTy = getVoidPtrTy();
        IntegerType * const sizeTy = getSizeTy();
        IntegerType * const intTy = getIntPtrTy(DL);
        Function * fMRemap = m->getFunction("mremap");
        if (LLVM_UNLIKELY(fMRemap == nullptr)) {
            FunctionType * fty = FunctionType::get(voidPtrTy, {voidPtrTy, sizeTy, sizeTy, intTy}, false);
            fMRemap = Function::Create(fty, Function::ExternalLinkage, "mremap", m);
        }
        addr = CreatePointerCast(addr, voidPtrTy);
        oldSize = CreateZExtOrTrunc(oldSize, sizeTy);
        newSize = CreateZExtOrTrunc(newSize, sizeTy);
        ConstantInt * const flags = ConstantInt::get(intTy, MREMAP_MAYMOVE);
        ptr = CreateCall(fMRemap, {addr, oldSize, newSize, flags});
        if (codegen::EnableAsserts) {
            Value * success = CreateICmpNE(CreatePtrToInt(addr, intTy), ConstantInt::getAllOnesValue(intTy)); // MAP_FAILED = -1
            CreateAssert(success, "CreateMRemap: mremap failed to allocate memory");
        }
    } else { // no OS mremap support
        ptr = CreateAnonymousMMap(newSize);
        CreateMemCpy(ptr, addr, oldSize, getpagesize());
        CreateMUnmap(addr, oldSize);
    }
    return ptr;
}

Value * CBuilder::CreateMUnmap(Value * addr, Value * len) {
    IntegerType * const sizeTy = getSizeTy();
    PointerType * const voidPtrTy = getVoidPtrTy();
    Module * const m = getModule();
    Function * munmapFunc = m->getFunction("munmap");
    if (LLVM_UNLIKELY(munmapFunc == nullptr)) {
        FunctionType * const fty = FunctionType::get(sizeTy, {voidPtrTy, sizeTy}, false);
        munmapFunc = Function::Create(fty, Function::ExternalLinkage, "munmap", m);
    }
    len = CreateZExtOrTrunc(len, sizeTy);
    if (codegen::EnableAsserts) {
        DataLayout DL(getModule());
        IntegerType * const intPtrTy = getIntPtrTy(DL);
        CreateAssert(len, "CreateMUnmap: length cannot be 0");
        Value * const addrValue = CreatePtrToInt(addr, intPtrTy);
        Value * const pageOffset = CreateURem(addrValue, ConstantInt::get(intPtrTy, getpagesize()));
        CreateAssertZero(pageOffset, "CreateMUnmap: addr must be a multiple of the page size");
        Value * const boundCheck = CreateICmpULT(addrValue, CreateSub(ConstantInt::getAllOnesValue(intPtrTy), CreateZExtOrTrunc(len, intPtrTy)));
        CreateAssert(boundCheck, "CreateMUnmap: addresses in [addr, addr+len) are outside the valid address space range");
    }
    addr = CreatePointerCast(addr, voidPtrTy);
    return CreateCall(munmapFunc, {addr, len});
}

PointerType * CBuilder::getVoidPtrTy() const {
    return TypeBuilder<void *, true>::get(getContext());
}

LoadInst * CBuilder::CreateAtomicLoadAcquire(Value * ptr) {
    const auto alignment = ptr->getType()->getPointerElementType()->getPrimitiveSizeInBits() / 8;
    LoadInst * inst = CreateAlignedLoad(ptr, alignment);
    inst->setOrdering(AtomicOrdering::Acquire);
    return inst;
    
}

StoreInst * CBuilder::CreateAtomicStoreRelease(Value * val, Value * ptr) {
    const auto alignment = ptr->getType()->getPointerElementType()->getPrimitiveSizeInBits() / 8;
    StoreInst * inst = CreateAlignedStore(val, ptr, alignment);
    inst->setOrdering(AtomicOrdering::Release);
    return inst;
}

PointerType * CBuilder::getFILEptrTy() {
    if (mFILEtype == nullptr) {
        mFILEtype = StructType::create(getContext(), "struct._IO_FILE");
    }
    return mFILEtype->getPointerTo();
}

Value * CBuilder::CreateFOpenCall(Value * filename, Value * mode) {
    Module * const m = getModule();
    Function * fOpenFunc = m->getFunction("fopen");
    if (fOpenFunc == nullptr) {
        FunctionType * fty = FunctionType::get(getFILEptrTy(), {getInt8Ty()->getPointerTo(), getInt8Ty()->getPointerTo()}, false);
        fOpenFunc = Function::Create(fty, Function::ExternalLinkage, "fopen", m);
        fOpenFunc->setCallingConv(CallingConv::C);
    }
    return CreateCall(fOpenFunc, {filename, mode});
}

Value * CBuilder::CreateFReadCall(Value * ptr, Value * size, Value * nitems, Value * stream) {
    Module * const m = getModule();
    Function * fReadFunc = m->getFunction("fread");
    PointerType * const voidPtrTy = getVoidPtrTy();
    if (fReadFunc == nullptr) {
        IntegerType * const sizeTy = getSizeTy();
        FunctionType * fty = FunctionType::get(sizeTy, {voidPtrTy, sizeTy, sizeTy, getFILEptrTy()}, false);
        fReadFunc = Function::Create(fty, Function::ExternalLinkage, "fread", m);
        fReadFunc->setCallingConv(CallingConv::C);
    }
    ptr = CreatePointerCast(ptr, voidPtrTy);
    return CreateCall(fReadFunc, {ptr, size, nitems, stream});
}

Value * CBuilder::CreateFWriteCall(Value * ptr, Value * size, Value * nitems, Value * stream) {
    Module * const m = getModule();
    Function * fWriteFunc = m->getFunction("fwrite");
    PointerType * const voidPtrTy = getVoidPtrTy();
    if (fWriteFunc == nullptr) {
        IntegerType * const sizeTy = getSizeTy();
        FunctionType * fty = FunctionType::get(sizeTy, {voidPtrTy, sizeTy, sizeTy, getFILEptrTy()}, false);
        fWriteFunc = Function::Create(fty, Function::ExternalLinkage, "fwrite", m);
        fWriteFunc->setCallingConv(CallingConv::C);
    }
    ptr = CreatePointerCast(ptr, voidPtrTy);
    return CreateCall(fWriteFunc, {ptr, size, nitems, stream});
}

Value * CBuilder::CreateFCloseCall(Value * stream) {
    Module * const m = getModule();
    Function * fCloseFunc = m->getFunction("fclose");
    if (fCloseFunc == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), {getFILEptrTy()}, false);
        fCloseFunc = Function::Create(fty, Function::ExternalLinkage, "fclose", m);
        fCloseFunc->setCallingConv(CallingConv::C);
    }
    return CreateCall(fCloseFunc, {stream});
}

Value * CBuilder::CreateRenameCall(Value * oldName, Value * newName) {
    Module * const m = getModule();
    Function * renameFunc = m->getFunction("rename");
    if (renameFunc == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), {getInt8PtrTy(), getInt8PtrTy()}, false);
        renameFunc = Function::Create(fty, Function::ExternalLinkage, "rename", m);
        renameFunc->setCallingConv(CallingConv::C);
    }
    return CreateCall(renameFunc, {oldName, newName});
}

Value * CBuilder::CreateRemoveCall(Value * path) {
    Module * const m = getModule();
    Function * removeFunc = m->getFunction("remove");
    if (removeFunc == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), {getInt8PtrTy()}, false);
        removeFunc = Function::Create(fty, Function::ExternalLinkage, "remove", m);
        removeFunc->setCallingConv(CallingConv::C);
    }
    return CreateCall(removeFunc, {path});
}

Value * CBuilder::CreatePThreadCreateCall(Value * thread, Value * attr, Function * start_routine, Value * arg) {
    Module * const m = getModule();
    Type * const voidPtrTy = getVoidPtrTy();
    Function * pthreadCreateFunc = m->getFunction("pthread_create");
    if (pthreadCreateFunc == nullptr) {
        Type * pthreadTy = getSizeTy();
        FunctionType * funVoidPtrVoidTy = FunctionType::get(getVoidTy(), {getVoidPtrTy()}, false);
        FunctionType * fty = FunctionType::get(getInt32Ty(), {pthreadTy->getPointerTo(), voidPtrTy, funVoidPtrVoidTy->getPointerTo(), voidPtrTy}, false);
        pthreadCreateFunc = Function::Create(fty, Function::ExternalLinkage, "pthread_create", m);
        pthreadCreateFunc->setCallingConv(CallingConv::C);
    }
    return CreateCall(pthreadCreateFunc, {thread, attr, start_routine, CreatePointerCast(arg, voidPtrTy)});
}

Value * CBuilder::CreatePThreadYield() {
    Module * const m = getModule();
    Function * f = m->getFunction("pthread_yield");
    if (f == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), false);
        f = Function::Create(fty, Function::ExternalLinkage, "pthread_yield", m);
        f->setCallingConv(CallingConv::C);
    }
    return CreateCall(f);
}

Value * CBuilder::CreatePThreadExitCall(Value * value_ptr) {
    Module * const m = getModule();
    Function * pthreadExitFunc = m->getFunction("pthread_exit");
    if (pthreadExitFunc == nullptr) {
        FunctionType * fty = FunctionType::get(getVoidTy(), {getVoidPtrTy()}, false);
        pthreadExitFunc = Function::Create(fty, Function::ExternalLinkage, "pthread_exit", m);
        pthreadExitFunc->addFnAttr(Attribute::NoReturn);
        pthreadExitFunc->setCallingConv(CallingConv::C);
    }
    CallInst * exitThread = CreateCall(pthreadExitFunc, {value_ptr});
    exitThread->setDoesNotReturn();
    return exitThread;
}

Value * CBuilder::CreatePThreadJoinCall(Value * thread, Value * value_ptr){
    Module * const m = getModule();
    Function * pthreadJoinFunc = m->getFunction("pthread_join");
    if (pthreadJoinFunc == nullptr) {
        FunctionType * fty = FunctionType::get(getInt32Ty(), {getSizeTy(), getVoidPtrTy()->getPointerTo()}, false);
        pthreadJoinFunc = Function::Create(fty, Function::ExternalLinkage, "pthread_join", m);
        pthreadJoinFunc->setCallingConv(CallingConv::C);
    }
    return CreateCall(pthreadJoinFunc, {thread, value_ptr});
}

void __report_failure(const char * msg, const uintptr_t * trace, const uint32_t n) {
    raw_fd_ostream out(STDERR_FILENO, false);
    if (trace) {
        SmallVector<char, 4096> tmp;
        raw_svector_ostream trace_string(tmp);
        for (uint32_t i = 0; i < n; ++i) {
            const auto pc = trace[i];
            trace_string << format_hex(pc, 16) << "   ";
            const auto len = codegen::ProgramName.length() + 32;
            char cmd[len];
            snprintf(cmd, len,"addr2line -fpCe %s %p", codegen::ProgramName.data(), reinterpret_cast<void *>(pc));
            FILE * f = popen(cmd, "r");
            if (f) {
                char buffer[1024] = {0};
                while(fgets(buffer, sizeof(buffer), f)) {
                    trace_string << buffer;
                }
                pclose(f);
            }
        }
        out.changeColor(raw_fd_ostream::WHITE, true);
        out << "Compilation Stacktrace:\n";
        out.resetColor();
        out << trace_string.str();
    }
    out.changeColor(raw_fd_ostream::WHITE, true);
    out << "Assertion `" << msg << "' failed.\n";
    out.resetColor();
    out.flush();

}

#if defined(HAS_MACH_VM_TYPES)

/*
 * Copyright (c) 1999, 2007 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include <pthread.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <stdlib.h>

#if defined(__i386__) || defined(__x86_64__)
#define FP_LINK_OFFSET 1
#elif defined(__ppc__) || defined(__ppc64__)
#define FP_LINK_OFFSET 2
#else
#error  ********** Unimplemented architecture
#endif

#define	INSTACK(a)	((uintptr_t)(a) >= stackbot && (uintptr_t)(a) <= stacktop)
#if defined(__ppc__) || defined(__ppc64__) || defined(__x86_64__)
#define	ISALIGNED(a)	((((uintptr_t)(a)) & 0xf) == 0)
#elif defined(__i386__)
#define	ISALIGNED(a)	((((uintptr_t)(a)) & 0xf) == 8)
#endif

__private_extern__  __attribute__((noinline))
void
_thread_stack_pcs(vm_address_t *buffer, unsigned max, unsigned *nb, unsigned skip)
{
    void *frame, *next;
    pthread_t self = pthread_self();
    uintptr_t stacktop = (uintptr_t)(pthread_get_stackaddr_np(self));
    uintptr_t stackbot = stacktop - (uintptr_t)(pthread_get_stacksize_np(self));
    
    *nb = 0;
    
    /* make sure return address is never out of bounds */
    stacktop -= (FP_LINK_OFFSET + 1) * sizeof(void *);
    
    /*
     * The original implementation called the first_frame_address() function,
     * which returned the stack frame pointer.  The problem was that in ppc,
     * it was a leaf function, so no new stack frame was set up with
     * optimization turned on (while a new stack frame was set up without
     * optimization).  We now inline the code to get the stack frame pointer,
     * so we are consistent about the stack frame.
     */
#if defined(__i386__) || defined(__x86_64__)
    frame = __builtin_frame_address(0);
#elif defined(__ppc__) || defined(__ppc64__)
    /* __builtin_frame_address IS BROKEN IN BEAKER: RADAR #2340421 */
    __asm__ volatile("mr %0, r1" : "=r" (frame));
#endif
    if(!INSTACK(frame) || !ISALIGNED(frame))
        return;
#if defined(__ppc__) || defined(__ppc64__)
    /* back up the stack pointer up over the current stack frame */
    next = *(void **)frame;
    if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
        return;
    frame = next;
#endif
    while (skip--) {
        next = *(void **)frame;
        if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
            return;
        frame = next;
    }
    while (max--) {
        buffer[*nb] = *(vm_address_t *)(((void **)frame) + FP_LINK_OFFSET);
        (*nb)++;
        next = *(void **)frame;
        if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
            return;
        frame = next;
    }
}
#endif

void CBuilder::__CreateAssert(Value * const assertion, StringRef failureMessage) {
    if (LLVM_UNLIKELY(codegen::EnableAsserts)) {
        Module * const m = getModule();
        if (LLVM_UNLIKELY(isa<ConstantInt>(assertion))) {
            if (LLVM_UNLIKELY(cast<ConstantInt>(assertion)->isZero())) {
                report_fatal_error(failureMessage);
            } else {
                return;
            }
        }
        Type * const stackTy = TypeBuilder<uintptr_t, false>::get(getContext());
        PointerType * const stackPtrTy = stackTy->getPointerTo();
        PointerType * const int8PtrTy = getInt8PtrTy();
        Function * function = m->getFunction("assert");
        if (LLVM_UNLIKELY(function == nullptr)) {
            auto ip = saveIP();
            IntegerType * const int1Ty = getInt1Ty();
            FunctionType * fty = FunctionType::get(getVoidTy(), { int1Ty, int8PtrTy, stackPtrTy, getInt32Ty() }, false);
            function = Function::Create(fty, Function::PrivateLinkage, "assert", m);
            function->setDoesNotThrow();
            function->setDoesNotAlias(2);
            BasicBlock * const entry = BasicBlock::Create(getContext(), "", function);
            BasicBlock * const failure = BasicBlock::Create(getContext(), "", function);
            BasicBlock * const success = BasicBlock::Create(getContext(), "", function);
            auto arg = function->arg_begin();
            arg->setName("assertion");
            Value * assertion = &*arg++;
            arg->setName("msg");
            Value * msg = &*arg++;
            arg->setName("trace");
            Value * trace = &*arg++;
            arg->setName("depth");
            Value * depth = &*arg++;
            SetInsertPoint(entry);
            IRBuilder<>::CreateCondBr(assertion, success, failure);
            IRBuilder<>::SetInsertPoint(failure);
            IRBuilder<>::CreateCall(LinkFunction("__report_failure", __report_failure), { msg, trace, depth });
            CreateExit(-1);
            IRBuilder<>::CreateBr(success); // necessary to satisfy the LLVM verifier. this is never executed.
            SetInsertPoint(success);
            IRBuilder<>::CreateRetVoid();
            restoreIP(ip);
        }

        SmallVector<unw_word_t, 64> stack;
        #if defined(HAS_MACH_VM_TYPES)
        for (;;) {
            unsigned int n;
            _thread_stack_pcs(reinterpret_cast<vm_address_t *>(stack.data()), stack.capacity(), &n, 1);
            if (LLVM_UNLIKELY(n < stack.capacity() || stack[n - 1] == 0)) {
                while (n >= 1 && stack[n - 1] == 0) {
                    n -= 1;
                }
                stack.set_size(n);
                break;
            }
            stack.reserve(n * 2);
        }
        #elif defined(HAS_LIBUNWIND)
        unw_context_t context;
        // Initialize cursor to current frame for local unwinding.
        unw_getcontext(&context);
        unw_cursor_t cursor;
        unw_init_local(&cursor, &context);
        // Unwind frames one by one, going up the frame stack.
        while (unw_step(&cursor) > 0) {
            unw_word_t pc;
            unw_get_reg(&cursor, UNW_REG_IP, &pc);
            if (pc == 0) {
                break;
            }
            stack.push_back(pc);
        }
        #elif defined(HAS_EXECINFO)
        for (;;) {
            const auto n = backtrace(reinterpret_cast<void **>(stack.data()), stack.capacity());
            if (LLVM_LIKELY(n < (int)stack.capacity())) {
                stack.set_size(n);
                break;
            }
            stack.reserve(n * 2);
        }
        #endif
        Value * trace = nullptr;
        ConstantInt * depth = nullptr;
        if (stack.empty()) {
            trace = ConstantPointerNull::get(stackPtrTy);
            depth = getInt32(0);
        } else {
            const auto n = stack.size() - 1;
            for (GlobalVariable & gv : m->getGlobalList()) {
                Type * const ty = gv.getValueType();
                if (ty->isArrayTy() && ty->getArrayElementType() == stackTy && ty->getArrayNumElements() == n) {
                    const ConstantDataArray * const array = cast<ConstantDataArray>(gv.getOperand(0));
                    bool found = true;
                    for (size_t i = 0; i < n; ++i) {
                        if (LLVM_LIKELY(array->getElementAsInteger(i) != stack[i + 1])) {
                            found = false;
                            break;
                        }
                    }
                    if (LLVM_UNLIKELY(found)) {
                        trace = &gv;
                        break;
                    }
                }
            }
            if (LLVM_LIKELY(trace == nullptr)) {
                Constant * const initializer = ConstantDataArray::get(getContext(), ArrayRef<unw_word_t>(stack.data() + 1, n));
                trace = new GlobalVariable(*m, initializer->getType(), true, GlobalVariable::InternalLinkage, initializer);
            }
            trace = CreatePointerCast(trace, stackPtrTy);
            depth = getInt32(n);
        }
        IRBuilder<>::CreateCall(function, {assertion, GetString(failureMessage), trace, depth});
    }
}

void CBuilder::CreateExit(const int exitCode) {
    Module * const m = getModule();
    Function * exit = m->getFunction("exit");
    if (LLVM_UNLIKELY(exit == nullptr)) {
        FunctionType * fty = FunctionType::get(getVoidTy(), {getInt32Ty()}, false);
        exit = Function::Create(fty, Function::ExternalLinkage, "exit", m);
        exit->setDoesNotReturn();
        exit->setDoesNotThrow();
    }
    CreateCall(exit, getInt32(exitCode));
}

BasicBlock * CBuilder::CreateBasicBlock(std::string && name) {
    return BasicBlock::Create(getContext(), name, GetInsertBlock()->getParent());
}

BranchInst * CBuilder::CreateLikelyCondBr(Value * Cond, BasicBlock * True, BasicBlock * False, const int probability) {
    MDBuilder mdb(getContext());
    if (probability < 0 || probability > 100) {
        report_fatal_error("branch weight probability must be in [0,100]");
    }
    return CreateCondBr(Cond, True, False, mdb.createBranchWeights(probability, 100 - probability));
}

Value * CBuilder::CreatePopcount(Value * bits) {
    Value * ctpopFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::ctpop, bits->getType());
    return CreateCall(ctpopFunc, bits);
}

Value * CBuilder::CreateCountForwardZeroes(Value * value) {
    Value * cttzFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::cttz, value->getType());
    return CreateCall(cttzFunc, {value, ConstantInt::getFalse(getContext())});
}

Value * CBuilder::CreateCountReverseZeroes(Value * value) {
    Value * ctlzFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::ctlz, value->getType());
    return CreateCall(ctlzFunc, {value, ConstantInt::getFalse(getContext())});
}

Value * CBuilder::CreateResetLowestBit(Value * bits) {
    return CreateAnd(bits, CreateSub(bits, ConstantInt::get(bits->getType(), 1)));
}

Value * CBuilder::CreateIsolateLowestBit(Value * bits) {
    return CreateAnd(bits, CreateNeg(bits));
}

Value * CBuilder::CreateMaskToLowestBitInclusive(Value * bits) {
    return CreateXor(bits, CreateSub(bits, ConstantInt::get(bits->getType(), 1)));
}

Value * CBuilder::CreateMaskToLowestBitExclusive(Value * bits) {
    return CreateAnd(CreateSub(bits, ConstantInt::get(bits->getType(), 1)), CreateNot(bits));
}

Value * CBuilder::CreateExtractBitField(Value * bits, Value * start, Value * length) {
    Constant * One = ConstantInt::get(bits->getType(), 1);
    return CreateAnd(CreateLShr(bits, start), CreateSub(CreateShl(One, length), One));
}

Value * CBuilder::CreateCeilLog2(Value * value) {
    IntegerType * ty = cast<IntegerType>(value->getType());
    CreateAssert(value, "CreateCeilLog2: value cannot be zero");
    Value * m = CreateCountReverseZeroes(CreateSub(value, ConstantInt::get(ty, 1)));
    return CreateSub(ConstantInt::get(m->getType(), ty->getBitWidth()), m);
}

Value * CBuilder::GetString(StringRef Str) {
    Module * const m = getModule();
    Value * ptr = m->getGlobalVariable(Str, true);
    if (ptr == nullptr) {
        ptr = CreateGlobalString(Str, Str);
    }
    Value * zero = getInt32(0);
    return CreateInBoundsGEP(ptr, { zero, zero });
}

Value * CBuilder::CreateReadCycleCounter() {
    Module * const m = getModule();
    Value * cycleCountFunc = Intrinsic::getDeclaration(m, Intrinsic::readcyclecounter);
    return CreateCall(cycleCountFunc, std::vector<Value *>({}));
}

Function * CBuilder::LinkFunction(StringRef name, FunctionType * type, void * functionPtr) const {
    assert (mDriver);
    return mDriver->addLinkFunction(getModule(), name, type, functionPtr);
}

#ifdef HAS_ADDRESS_SANITIZER

#define CHECK_ADDRESS(Ptr) \
    if (LLVM_UNLIKELY(hasAddressSanitizer())) { \
        Module * const m = getModule(); \
        PointerType * const voidPtrTy = getVoidPtrTy(); \
        IntegerType * const sizeTy = getSizeTy(); \
        Function * isPoisoned = m->getFunction("__asan_region_is_poisoned"); \
        if (LLVM_UNLIKELY(isPoisoned == nullptr)) { \
            isPoisoned = Function::Create(FunctionType::get(voidPtrTy, {voidPtrTy, sizeTy}, false), Function::ExternalLinkage, "__asan_region_is_poisoned", m); \
            isPoisoned->setCallingConv(CallingConv::C); \
            isPoisoned->setDoesNotAlias(0); \
            isPoisoned->setDoesNotAlias(1); \
        } \
        Value * const addr = CreatePointerCast(Ptr, voidPtrTy); \
        ConstantInt * const size = ConstantInt::get(sizeTy, Ptr->getType()->getPointerElementType()->getPrimitiveSizeInBits() / 8); \
        Value * check = CreateCall(isPoisoned, { addr, size }); \
        check = CreateICmpEQ(check, ConstantPointerNull::get(cast<PointerType>(isPoisoned->getReturnType()))); \
        CreateAssert(check, "Valid memory address"); \
    }

LoadInst * CBuilder::CreateLoad(Value *Ptr, const char * Name) {
    CHECK_ADDRESS(Ptr);
    return IRBuilder<>::CreateLoad(Ptr, Name);
}

LoadInst * CBuilder::CreateLoad(Value * Ptr, const Twine & Name) {
    CHECK_ADDRESS(Ptr);
    return IRBuilder<>::CreateLoad(Ptr, Name);
}

LoadInst * CBuilder::CreateLoad(Type *Ty, Value *Ptr, const Twine & Name) {
    CHECK_ADDRESS(Ptr);
    return IRBuilder<>::CreateLoad(Ty, Ptr, Name);
}

LoadInst * CBuilder::CreateLoad(Value *Ptr, bool isVolatile, const Twine & Name) {
    CHECK_ADDRESS(Ptr);
    return IRBuilder<>::CreateLoad(Ptr, isVolatile, Name);
}

StoreInst * CBuilder::CreateStore(Value * Val, Value * Ptr, bool isVolatile) {
    CHECK_ADDRESS(Ptr);
    return IRBuilder<>::CreateStore(Val, Ptr, isVolatile);
}

#undef CHECK_ADDRESS

#endif

inline bool CBuilder::hasAddressSanitizer() const {
    return codegen::EnableAsserts && mDriver && mDriver->hasExternalFunction("__asan_region_is_poisoned");
}

LoadInst * CBuilder::CreateAlignedLoad(Value * Ptr, unsigned Align, const char * Name) {
    if (codegen::EnableAsserts) {
        DataLayout DL(getModule());
        IntegerType * const intPtrTy = cast<IntegerType>(DL.getIntPtrType(Ptr->getType()));
        Value * alignmentOffset = CreateURem(CreatePtrToInt(Ptr, intPtrTy), ConstantInt::get(intPtrTy, Align));
        CreateAssertZero(alignmentOffset, "CreateAlignedLoad: pointer is misaligned");
    }
    LoadInst * LI = CreateLoad(Ptr, Name);
    LI->setAlignment(Align);
    return LI;
}

LoadInst * CBuilder::CreateAlignedLoad(Value * Ptr, unsigned Align, const Twine & Name) {
    if (codegen::EnableAsserts) {
        DataLayout DL(getModule());
        IntegerType * const intPtrTy = cast<IntegerType>(DL.getIntPtrType(Ptr->getType()));
        Value * alignmentOffset = CreateURem(CreatePtrToInt(Ptr, intPtrTy), ConstantInt::get(intPtrTy, Align));
        CreateAssertZero(alignmentOffset, "CreateAlignedLoad: pointer is misaligned");
    }
    LoadInst * LI = CreateLoad(Ptr, Name);
    LI->setAlignment(Align);
    return LI;
}

LoadInst * CBuilder::CreateAlignedLoad(Value * Ptr, unsigned Align, bool isVolatile, const Twine & Name) {
    if (codegen::EnableAsserts) {
        DataLayout DL(getModule());
        IntegerType * const intPtrTy = cast<IntegerType>(DL.getIntPtrType(Ptr->getType()));
        Value * alignmentOffset = CreateURem(CreatePtrToInt(Ptr, intPtrTy), ConstantInt::get(intPtrTy, Align));
        CreateAssertZero(alignmentOffset, "CreateAlignedLoad: pointer is misaligned");
    }
    LoadInst * LI = CreateLoad(Ptr, isVolatile, Name);
    LI->setAlignment(Align);
    return LI;
}

StoreInst * CBuilder::CreateAlignedStore(Value * Val, Value * Ptr, unsigned Align, bool isVolatile) {
    if (codegen::EnableAsserts) {
        DataLayout DL(getModule());
        IntegerType * const intPtrTy = cast<IntegerType>(DL.getIntPtrType(Ptr->getType()));
        Value * alignmentOffset = CreateURem(CreatePtrToInt(Ptr, intPtrTy), ConstantInt::get(intPtrTy, Align));
        CreateAssertZero(alignmentOffset, "CreateAlignedStore: pointer is misaligned");
    }
    StoreInst *SI = CreateStore(Val, Ptr, isVolatile);
    SI->setAlignment(Align);
    return SI;
}

CBuilder::CBuilder(LLVMContext & C)
: IRBuilder<>(C)
, mCacheLineAlignment(64)
, mSizeType(TypeBuilder<size_t, false>::get(C))
, mFILEtype(nullptr)
, mDriver(nullptr) {

}
