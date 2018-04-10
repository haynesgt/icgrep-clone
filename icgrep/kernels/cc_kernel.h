/*
 *  Copyright (c) 2016 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 */
#ifndef CC_KERNEL_H
#define CC_KERNEL_H

#include <pablo/pablo_kernel.h>

namespace IDISA { class IDISA_Builder; }
namespace re { class CC; }

namespace kernel {

class DirectCharacterClassKernelBuilder final : public pablo::PabloKernel {
public:    
    DirectCharacterClassKernelBuilder(const std::unique_ptr<KernelBuilder> & b, std::string ccSetName, std::vector<re::CC *> charClasses);
protected:
    void generatePabloMethod() override;
private:
    const std::vector<re::CC *> mCharClasses;
};

class ParabixCharacterClassKernelBuilder final : public pablo::PabloKernel {
public:
    ParabixCharacterClassKernelBuilder(const std::unique_ptr<KernelBuilder> & b, std::string ccSetName, const std::vector<re::CC *> & charClasses, unsigned codeUnitWidth);
protected:
    void generatePabloMethod() override;
private:
    const std::vector<re::CC *> mCharClasses;
};

}
#endif
