/*
 *  Copyright (c) 2016 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters.
 */

#ifndef RE_UTILITY_H
#define RE_UTILITY_H

namespace re { class RE; }
namespace re { class Name; }

namespace re {

RE * makeComplement(RE * s);
RE * makeWordBoundary();
RE * makeWordNonBoundary();
RE * makeWordBegin();
RE * makeWordEnd();
Name * makeDigitSet();
Name * makeAlphaNumeric();
Name * makeWhitespaceSet();
Name * makeWordSet();

}

#endif // RE_UTILITY_H
