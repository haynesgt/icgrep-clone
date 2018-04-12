#include "re_simplifier.h"
#include <re/re_name.h>
#include <re/re_any.h>
#include <re/re_start.h>
#include <re/re_end.h>
#include <re/re_alt.h>
#include <re/re_cc.h>
#include <re/re_seq.h>
#include <re/re_rep.h>
#include <re/re_diff.h>
#include <re/re_intersect.h>
#include <re/re_assertion.h>
#include <re/re_analysis.h>
#include <algorithm>
#include <memory>
#include <queue>

using namespace llvm;

namespace re {

RE * RE_Simplifier::simplify(RE * re) {
    if (Alt * alt = dyn_cast<Alt>(re)) {
        std::vector<RE *> list;
        list.reserve(alt->size());
        for (RE * re : *alt) {
            list.push_back(simplify(re));
        }
        re = makeAlt(list.begin(), list.end());
    } else if (Seq * seq = dyn_cast<Seq>(re)) {
        std::vector<RE *> list;
        list.reserve(seq->size());
        for (RE * re : *seq) {
            list.push_back(simplify(re));
        }
        re = makeSeq(list.begin(), list.end());
    } else if (Assertion * a = dyn_cast<Assertion>(re)) {
        re = makeAssertion(simplify(a->getAsserted()), a->getKind(), a->getSense());
    } else if (Rep * rep = dyn_cast<Rep>(re)) {
        RE * expr = simplify(rep->getRE());
        re = makeRep(expr, rep->getLB(), rep->getUB());
    } else if (Diff * diff = dyn_cast<Diff>(re)) {
        re = makeDiff(simplify(diff->getLH()), simplify(diff->getRH()));
    } else if (Intersect * e = dyn_cast<Intersect>(re)) {
        re = makeIntersect(simplify(e->getLH()), simplify(e->getRH()));
    } 
    return re;
}

}
