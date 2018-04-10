#include "re_name_gather.h"
#include <re/re_name.h>
#include <re/re_alt.h>
#include <re/re_cc.h>
#include <re/re_seq.h>
#include <re/re_rep.h>
#include <re/re_range.h>
#include <re/re_diff.h>
#include <re/re_intersect.h>
#include <re/re_assertion.h>
#include <re/re_group.h>
#include <re/re_analysis.h>
#include <re/re_memoizer.hpp>
#include <cc/alphabet.h>
#include <UCD/ucd_compiler.hpp>
#include <UCD/resolve_properties.h>
#include <boost/container/flat_set.hpp>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
namespace re {
    
    
void gatherUnicodeProperties (RE * re, std::set<Name *> & nameSet) {
    if (isa<Name>(re)) {
        if (cast<Name>(re)->getType() == Name::Type::UnicodeProperty) {
            nameSet.emplace(cast<Name>(re));
        }
    } else if (isa<Seq>(re)) {
        for (RE * item : *cast<Seq>(re)) {
            gatherUnicodeProperties(item, nameSet);
        }
    } else if (isa<Alt>(re)) {
        for (RE * item : *cast<Alt>(re)) {
            gatherUnicodeProperties(item, nameSet);
        }
    } else if (isa<Rep>(re)) {
        gatherUnicodeProperties(cast<Rep>(re)->getRE(), nameSet);
    } else if (isa<Assertion>(re)) {
        gatherUnicodeProperties(cast<Assertion>(re)->getAsserted(), nameSet);
    } else if (Range * rg = dyn_cast<Range>(re)) {
        gatherUnicodeProperties(rg->getLo(), nameSet);
        gatherUnicodeProperties(rg->getHi(), nameSet);
    } else if (isa<Diff>(re)) {
        gatherUnicodeProperties(cast<Diff>(re)->getLH(), nameSet);
        gatherUnicodeProperties(cast<Diff>(re)->getRH(), nameSet);
    } else if (isa<Intersect>(re)) {
        gatherUnicodeProperties(cast<Intersect>(re)->getLH(), nameSet);
        gatherUnicodeProperties(cast<Intersect>(re)->getRH(), nameSet);
    } else if (isa<Group>(re)) {
        gatherUnicodeProperties(cast<Group>(re)->getRE(), nameSet);
    }
}

}
