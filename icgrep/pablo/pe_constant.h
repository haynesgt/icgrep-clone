#ifndef PE_CONSTANT_H
#define PE_CONSTANT_H

#include <pablo/pabloAST.h>

namespace pablo {

class Constant final : public PabloAST {
    friend class PabloBlock;
public:
    static inline bool classof(const PabloAST * e) {
        return e->getClassTypeId() == ClassTypeId::Ones;
    }
    static inline bool classof(const void *) {
        return false;
    }
    virtual ~Constant() {
    }
    inline bool operator==(const Ones &) const {
        return true;
    }
    virtual bool operator==(const PabloAST & other) const {
        return isa<Ones>(other);
    }
protected:
    Constant(Type * type, const PabloAST * const value, Allocator & allocator)
    : PabloAST(ClassTypeId::Ones, type, allocators)
    , mValue(value) {
    }
private:

    const PabloAST * const mValue;
};

}

#endif // PE_CONSTANT_H
