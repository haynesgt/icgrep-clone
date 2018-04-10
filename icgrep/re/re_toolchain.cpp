/*
 *  Copyright (c) 2017 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#include <toolchain/toolchain.h>
#include <grep_interface.h>
#include <re/re_toolchain.h>
#include <cc/cc_compiler.h>            // for CC_Compiler
#include <llvm/Support/CommandLine.h>  // for clEnumVal, clEnumValEnd, Optio...
#include <re/re_compiler.h>            // for RE_Compiler
#include <re/re_nullable.h>            // for RE_Nullable
#include <re/re_star_normal.h>         // for RE_Star_Normal
#include <re/re_simplifier.h>          // for RE_Simplifier
#include <re/re_minimizer.h>
#include <re/re_local.h>
#include <re/printer_re.h>
#include <re/re_analysis.h>
#include <re/re_cc.h>
#include <re/casing.h>
#include <re/exclude_CC.h>
#include <re/re_name_resolve.h>
#include <re/grapheme_clusters.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ErrorHandling.h>
#include <toolchain/toolchain.h>

using namespace pablo;
using namespace llvm;

namespace re {

static cl::OptionCategory RegexOptions("Regex Toolchain Options",
                                              "These options control the regular expression transformation and compilation.");
const cl::OptionCategory * LLVM_READONLY re_toolchain_flags() {
    return &RegexOptions;
}

static cl::bits<RE_PrintFlags> 
    PrintOptions(cl::values(clEnumVal(ShowREs, "Print parsed or generated regular expressions"),
                            clEnumVal(ShowAllREs, "Print all regular expression passes"),
                            clEnumVal(ShowStrippedREs, "Print REs with nullable prefixes/suffixes removed"),
                            clEnumVal(ShowSimplifiedREs, "Print final simplified REs")
                            CL_ENUM_VAL_SENTINEL), cl::cat(RegexOptions));

static cl::bits<RE_AlgorithmFlags>
    AlgorithmOptions(cl::values(clEnumVal(DisableLog2BoundedRepetition, "disable log2 optimizations for bounded repetition of bytes"),
                              clEnumVal(DisableIfHierarchy, "disable nested if hierarchy for generated Unicode classes (not recommended)"), 
                              clEnumVal(DisableMatchStar, "disable MatchStar optimization"), 
                              clEnumVal(DisableUnicodeMatchStar, "disable Unicode MatchStar optimization"),
                              clEnumVal(DisableUnicodeLineBreak, "disable Unicode line breaks - use LF only")
                              CL_ENUM_VAL_SENTINEL), cl::cat(RegexOptions));

bool LLVM_READONLY AlgorithmOptionIsSet(RE_AlgorithmFlags flag) {
    return AlgorithmOptions.isSet(flag);
}

int IfInsertionGap;
static cl::opt<int, true> 
    IfInsertionGapOption("if-insertion-gap",  cl::location(IfInsertionGap), cl::init(3),
                         cl::desc("minimum number of nonempty elements between inserted if short-circuit tests"), 
                         cl::cat(RegexOptions));

RE * resolveModesAndExternalSymbols(RE * r, bool globallyCaseInsensitive) {
    if (PrintOptions.isSet(ShowAllREs) || PrintOptions.isSet(ShowREs)) {
        errs() << "Parser:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    r = resolveGraphemeMode(r, false /* not in grapheme mode at top level*/);
    if (PrintOptions.isSet(ShowAllREs)) {
        errs() << "resolveGraphemeMode:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    r = re::resolveUnicodeProperties(r);
    if (PrintOptions.isSet(ShowAllREs)) {
        errs() << "resolveUnicodeProperties:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    r = resolveCaseInsensitiveMode(r, globallyCaseInsensitive);
    if (PrintOptions.isSet(ShowAllREs)) {
        errs() << "resolveCaseInsensitiveMode:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    return r;
}

RE * excludeUnicodeLineBreak(RE * r) {
    r = exclude_CC(r, re::makeCC(re::makeCC(0x0A, 0x0D), re::makeCC(re::makeCC(0x85), re::makeCC(0x2028, 0x2029))));
    if (PrintOptions.isSet(ShowAllREs)) {
        errs() << "excludeUnicodeLineBreak:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    return r;
}

RE * regular_expression_passes(RE * r) {

    //Optimization passes to simplify the AST.
    r = RE_Nullable::removeNullablePrefix(r);
    if (PrintOptions.isSet(ShowAllREs) || PrintOptions.isSet(ShowStrippedREs)) {
        errs() << "RemoveNullablePrefix:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    r = RE_Nullable::removeNullableSuffix(r);
    if (PrintOptions.isSet(ShowAllREs) || PrintOptions.isSet(ShowStrippedREs)) {
        errs() << "RemoveNullableSuffix:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    r = RE_Star_Normal::star_normal(r);
    if (PrintOptions.isSet(ShowAllREs) || PrintOptions.isSet(ShowSimplifiedREs)) {
        //Print to the terminal the AST that was transformed to the star normal form.
        errs() << "Star_Normal_Form:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    r = re::resolveNames(r);
    if (PrintOptions.isSet(ShowAllREs)) {
        errs() << "Resolve Names:\n" << Printer_RE::PrintRE(r) << '\n';
    }
    if (codegen::OptLevel > 1) {
        r = RE_Minimizer::minimize(r);
    } else {
        r = RE_Simplifier::simplify(r);
    }
    if (PrintOptions.isSet(ShowAllREs) || PrintOptions.isSet(ShowSimplifiedREs)) {
        //Print to the terminal the AST that was generated by the simplifier.
        errs() << "Simplifier:\n" << Printer_RE::PrintRE(r) << '\n';
    }

    if (!DefiniteLengthBackReferencesOnly(r)) {
        llvm::report_fatal_error("Future back reference support: references must be within a fixed distance from a fixed-length capture.");
    }
    return r;
}

}
