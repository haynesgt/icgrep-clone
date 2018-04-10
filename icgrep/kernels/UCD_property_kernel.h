/*
 *  Copyright (c) 2018 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 */
#ifndef UCD_PROPERTY_KERNEL_H
#define UCD_PROPERTY_KERNEL_H

#include <pablo/pablo_kernel.h>  // for PabloKernel

namespace re { class Name; }

namespace kernel {
    
class UnicodePropertyKernelBuilder : public pablo::PabloKernel {
public:
    UnicodePropertyKernelBuilder(const std::unique_ptr<kernel::KernelBuilder> & kb,
                                 re::Name * property_value_name);
    bool isCachable() const override { return true; }
    bool hasSignature() const override { return false; }
protected:
    void generatePabloMethod() override;
private:
    re::Name * mName;
};

}
#endif
