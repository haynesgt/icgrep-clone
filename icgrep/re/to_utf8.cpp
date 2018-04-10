/*
 *  Copyright (c) 2017 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#include <re/to_utf8.h>
#include <UCD/unicode_set.h>
#include <UCD/UTF.h>
#include <cc/alphabet.h>
#include <re/re_name.h>
#include <re/re_start.h>
#include <re/re_end.h>
#include <re/re_cc.h>
#include <re/re_seq.h>
#include <re/re_alt.h>
#include <re/re_rep.h>
#include <re/re_diff.h>
#include <re/re_intersect.h>
#include <re/re_assertion.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>

using namespace llvm;

namespace re {
    
static RE * rangeCodeUnits(codepoint_t lo, codepoint_t hi, unsigned index, const unsigned lgth){
    const codepoint_t hunit = UTF<8>::nthCodeUnit(hi, index);
    const codepoint_t lunit = UTF<8>::nthCodeUnit(lo, index);
    if (index == lgth) {
        return makeByte(lunit, hunit);
    }
    else if (hunit == lunit) {
        return makeSeq({makeByte(hunit), rangeCodeUnits(lo, hi, index + 1, lgth)});
    }
    else {
        const unsigned suffix_mask = (static_cast<unsigned>(1) << ((lgth - index) * 6)) - 1;
        if ((hi & suffix_mask) != suffix_mask) {
            const unsigned hi_floor = (~suffix_mask) & hi;
            return makeAlt({rangeCodeUnits(hi_floor, hi, index, lgth), rangeCodeUnits(lo, hi_floor - 1, index, lgth)});
        }
        else if ((lo & suffix_mask) != 0) {
            const unsigned low_ceil = lo | suffix_mask;
            return makeAlt({rangeCodeUnits(low_ceil + 1, hi, index, lgth), rangeCodeUnits(lo, low_ceil, index, lgth)});
        }
        else {
            return makeSeq({makeByte(lunit, hunit), rangeCodeUnits(lo, hi, index + 1, lgth)});
        }
    }
}

static RE * rangeToUTF8(codepoint_t lo, codepoint_t hi) {
    const auto min_lgth = UTF<8>::encoded_length(lo);
    const auto max_lgth = UTF<8>::encoded_length(hi);
    if (min_lgth < max_lgth) {
        const auto m = UTF<8>::max_codepoint_of_length(min_lgth);
        return makeAlt({rangeToUTF8(lo, m), rangeToUTF8(m + 1, hi)});
    }
    else {
        return rangeCodeUnits(lo, hi, 1, max_lgth);
    }
}

RE * toUTF8(RE * r) {
    if (isa<Name>(r) || isa<Start>(r) || isa<End>(r)) {
        return r;
    } else if (const CC * cc = dyn_cast<CC>(r)) {
        if (cc->getAlphabet() != &cc::Unicode) return r;
        std::vector<RE *> alt;
        for (const interval_t & i : *cc) {
            alt.push_back(rangeToUTF8(lo_codepoint(i), hi_codepoint(i)));
        }
        return makeAlt(alt.begin(), alt.end());
    } else if (Alt * alt = dyn_cast<Alt>(r)) {
        std::vector<RE *> list;
        list.reserve(alt->size());
        for (RE * a : *alt) {
            list.push_back(toUTF8(a));
        }
        return makeAlt(list.begin(), list.end());
    } else if (Seq * seq = dyn_cast<Seq>(r)) {
        std::vector<RE *> list;
        list.reserve(seq->size());
        for (RE * s : *seq) {
            list.push_back(toUTF8(s));
        }
        return makeSeq(list.begin(), list.end());
    } else if (Assertion * a = dyn_cast<Assertion>(r)) {
        return makeAssertion(toUTF8(a->getAsserted()), a->getKind(), a->getSense());
    } else if (Rep * rep = dyn_cast<Rep>(r)) {
        RE * expr = toUTF8(rep->getRE());
        return makeRep(expr, rep->getLB(), rep->getUB());
    } else if (Diff * diff = dyn_cast<Diff>(r)) {
        return makeDiff(toUTF8(diff->getLH()), toUTF8(diff->getRH()));
    } else if (Intersect * e = dyn_cast<Intersect>(r)) {
        return makeIntersect(toUTF8(e->getLH()), toUTF8(e->getRH()));
    }
    llvm_unreachable("unexpected RE type given to toUTF8");
    return nullptr;
}

}

