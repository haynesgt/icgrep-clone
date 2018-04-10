/*
 *  Copyright (c) 2014 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#ifndef PE_ZEROES_H
#define PE_ZEROES_H

#include <pablo/pabloAST.h>

namespace pablo {

class Zeroes final : public PabloAST {
    friend class PabloBlock;
    friend class PabloKernel;
public:
    static inline bool classof(const PabloAST * expr) {
        assert (expr);
        return expr->getClassTypeId() == ClassTypeId::Zeroes;
    }
    static inline bool classof(const void *) {
        return false;
    }
    virtual ~Zeroes() {
    }
    inline bool operator==(const Zeroes &) const {
        return true;
    }
    virtual bool operator==(const PabloAST & other) const {
        return llvm::isa<Zeroes>(other);
    }
protected:
    Zeroes(llvm::Type * const type, Allocator & allocator)
    : PabloAST(ClassTypeId::Zeroes, type, allocator) {
    }
};

}

#endif // PE_ZEROES_H


