#ifndef SOURCE_KERNEL_H
#define SOURCE_KERNEL_H

#include "kernel.h"
namespace kernel { class KernelBuilder; }

namespace kernel {

/* The MMapSourceKernel is a simple wrapper for an external MMap file buffer.
   The doSegment method of this kernel feeds one segment at a time to a
   pipeline. */

class MMapSourceKernel final : public SegmentOrientedKernel {
public:
    MMapSourceKernel(const std::unique_ptr<kernel::KernelBuilder> & iBuilder, unsigned blocksPerSegment = 1, unsigned codeUnitWidth = 8);
    bool isCachable() const override { return true; }
    bool hasSignature() const override { return false; }
protected:
    void linkExternalMethods(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
    void generateInitializeMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
    void generateDoSegmentMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
    void generateFinalizeMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
protected:
    const unsigned          mSegmentBlocks;
    const unsigned          mCodeUnitWidth;
    llvm::Function *        mFileSizeFunction;
};

class ReadSourceKernel final : public SegmentOrientedKernel {
public:
    ReadSourceKernel(const std::unique_ptr<kernel::KernelBuilder> & iBuilder, unsigned blocksPerSegment = 1, unsigned codeUnitWidth = 8);
    bool isCachable() const override { return true; }
    bool hasSignature() const override { return false; }
protected:
    void generateInitializeMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
    void generateDoSegmentMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
    void generateFinalizeMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
private:
    unsigned mSegmentBlocks;
    unsigned mCodeUnitWidth;
};

class MemorySourceKernel final : public SegmentOrientedKernel {
public:
    MemorySourceKernel(const std::unique_ptr<kernel::KernelBuilder> & iBuilder, llvm::Type * type, unsigned blocksPerSegment = 1, unsigned codeUnitWidth = 8);
    bool isCachable() const override { return true; }
    bool hasSignature() const override { return false; }
protected:
    void generateInitializeMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
    void generateDoSegmentMethod(const std::unique_ptr<kernel::KernelBuilder> & iBuilder) override;
private:
    unsigned mSegmentBlocks;
    unsigned mCodeUnitWidth;
};

}

#endif // SOURCE_KERNEL_H
