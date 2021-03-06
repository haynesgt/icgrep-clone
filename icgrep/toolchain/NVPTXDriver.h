/*
 *  Copyright (c) 2017 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#ifndef NVPTXDRIVER_H
#define NVPTXDRIVER_H

#include "driver.h"
#include <toolchain/object_cache.h>

#define PTX_CACHE_NAMESPACE "ptx"

class NVPTXDriver final : public Driver {
    friend class CBuilder;
public:
    NVPTXDriver(std::string && moduleName);

    ~NVPTXDriver();

    void generatePipelineIR() override;
    
    void makeKernelCall(kernel::Kernel * kb, const std::vector<parabix::StreamSetBuffer *> & inputs, const std::vector<parabix::StreamSetBuffer *> & outputs) override;

    std::string getPTXFilename();

    void finalizeObject() override;

    void finalizeObject(int REi) override;

    bool hasExternalFunction(const llvm::StringRef /* functionName */) const override { return false; }

    void * getMain() override; // "main" exists until the driver is deleted

private:

    llvm::Function * addLinkFunction(llvm::Module * mod, llvm::StringRef name, llvm::FunctionType * type, void * functionPtr) const override;

    ParabixObjectCache *                                    mCache;

};

#endif
