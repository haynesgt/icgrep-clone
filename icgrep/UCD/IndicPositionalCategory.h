#ifndef INDICPOSITIONALCATEGORY_H
#define INDICPOSITIONALCATEGORY_H
/*
 *  Copyright (c) 2017 International Characters, Inc.
 *  This software is licensed to the public under the Open Software License 3.0.
 *  icgrep is a trademark of International Characters, Inc.
 *
 *  This file is generated by UCD_properties.py - manual edits may be lost.
 */

#include "PropertyAliases.h"
#include "PropertyObjects.h"
#include "PropertyValueAliases.h"
#include "unicode_set.h"

namespace UCD {
  namespace INPC_ns {
    const unsigned independent_prop_values = 15;
    /** Code Point Ranges for NA
    [0000, 08ff], [0904, 0939], [093d, 093d], [0950, 0950], [0958, 0961],
    [0964, 0980], [0984, 09bb], [09bd, 09bd], [09c5, 09c6], [09c9, 09ca],
    [09ce, 09d6], [09d8, 09e1], [09e4, 0a00], [0a04, 0a3b], [0a3d, 0a3d],
    [0a43, 0a46], [0a49, 0a4a], [0a4e, 0a6f], [0a72, 0a74], [0a76, 0a80],
    [0a84, 0abb], [0abd, 0abd], [0ac6, 0ac6], [0aca, 0aca], [0ace, 0ae1],
    [0ae4, 0af9], [0b00, 0b00], [0b04, 0b3b], [0b3d, 0b3d], [0b45, 0b46],
    [0b49, 0b4a], [0b4e, 0b55], [0b58, 0b61], [0b64, 0b81], [0b83, 0bbd],
    [0bc3, 0bc5], [0bc9, 0bc9], [0bce, 0bd6], [0bd8, 0bff], [0c04, 0c3d],
    [0c45, 0c45], [0c49, 0c49], [0c4e, 0c54], [0c57, 0c61], [0c64, 0c80],
    [0c84, 0cbb], [0cbd, 0cbd], [0cc5, 0cc5], [0cc9, 0cc9], [0cce, 0cd4],
    [0cd7, 0ce1], [0ce4, 0cff], [0d04, 0d3a], [0d3d, 0d3d], [0d45, 0d45],
    [0d49, 0d49], [0d4e, 0d56], [0d58, 0d61], [0d64, 0d81], [0d84, 0dc9],
    [0dcb, 0dce], [0dd5, 0dd5], [0dd7, 0dd7], [0de0, 0df1], [0df4, 0e2f],
    [0e3b, 0e3f], [0e46, 0e46], [0e4f, 0eaf], [0eba, 0eba], [0ebd, 0ebf],
    [0ec5, 0ec7], [0ece, 0f17], [0f1a, 0f34], [0f36, 0f36], [0f38, 0f38],
    [0f3a, 0f3d], [0f40, 0f70], [0f85, 0f85], [0f88, 0f8c], [0f98, 0f98],
    [0fbd, 0fc5], [0fc7, 102a], [1039, 1039], [103c, 103c], [103f, 1055],
    [105a, 105d], [1061, 1061], [1065, 1066], [106e, 1070], [1075, 1081],
    [108e, 108e], [1090, 1099], [109e, 1711], [1715, 1731], [1735, 1751],
    [1754, 1771], [1774, 17b5], [17d2, 17d2], [17d4, 17dc], [17de, 191f],
    [192c, 192f], [193c, 19af], [19c1, 19c7], [19ca, 1a16], [1a1c, 1a54],
    [1a5f, 1a60], [1a7d, 1a7e], [1a80, 1aff], [1b05, 1b33], [1b45, 1b6a],
    [1b74, 1b7f], [1b83, 1ba0], [1bab, 1bab], [1bae, 1be5], [1bf4, 1c23],
    [1c38, 1ccf], [1cd3, 1cd3], [1ce9, 1cec], [1cee, 1cf3], [1cf5, 1cf6],
    [1cf8, 1dfa], [1dfc, a805], [a807, a80a], [a80c, a822], [a828, a87f],
    [a882, a8b3], [a8c6, a8df], [a8f2, a92a], [a92e, a946], [a954, a97f],
    [a984, a9b2], [a9c1, a9e4], [a9e6, aa28], [aa37, aa42], [aa44, aa4b],
    [aa4e, aa7a], [aa7e, aaaf], [aac0, aac0], [aac2, aaea], [aaf0, aaf4],
    [aaf6, abe2], [abeb, abeb], [abee, 10a00], [10a04, 10a04],
    [10a07, 10a0b], [10a10, 10a37], [10a3b, 10fff], [11003, 11037],
    [11047, 1107f], [11083, 110af], [110bb, 110ff], [11103, 11126],
    [11133, 11133], [11135, 11172], [11174, 1117f], [11183, 111b2],
    [111c1, 111ca], [111cd, 1122b], [11238, 1123d], [1123f, 112de],
    [112eb, 11300], [11304, 1133d], [11345, 11346], [11349, 1134a],
    [1134e, 11356], [11358, 11361], [11364, 11365], [1136d, 1136f],
    [11375, 11434], [11447, 114af], [114c4, 115ae], [115b6, 115b7],
    [115c1, 115db], [115de, 1162f], [11641, 116aa], [116b8, 1171c],
    [1171e, 1171e], [1172c, 11a00], [11a0b, 11a32], [11a3a, 11a3a],
    [11a3f, 11a50], [11a5c, 11a89], [11a99, 11c2e], [11c37, 11c37],
    [11c40, 11c91], [11ca8, 11ca8], [11cb7, 11d30], [11d37, 11d39],
    [11d3b, 11d3b], [11d3e, 11d3e], [11d45, 11d46], [11d48, 10ffff]**/


    namespace {
    const static UnicodeSet::run_t __na_Set_runs[] = {
    {Full, 72}, {Mixed, 23}, {Full, 1}, {Mixed, 13}, {Full, 1}, {Mixed, 2},
    {Full, 1}, {Mixed, 2}, {Full, 2}, {Mixed, 2}, {Full, 1}, {Mixed, 2},
    {Full, 1}, {Mixed, 4}, {Full, 2}, {Mixed, 4}, {Full, 51}, {Mixed, 4},
    {Full, 1}, {Mixed, 2}, {Full, 10}, {Mixed, 1}, {Full, 3}, {Mixed, 2},
    {Full, 1}, {Mixed, 1}, {Full, 1}, {Mixed, 2}, {Full, 4}, {Mixed, 6},
    {Full, 1}, {Mixed, 1}, {Full, 1}, {Mixed, 1}, {Full, 4}, {Mixed, 2},
    {Full, 7}, {Mixed, 1}, {Full, 1104}, {Mixed, 2}, {Full, 2}, {Mixed, 4},
    {Full, 1}, {Mixed, 2}, {Full, 1}, {Mixed, 4}, {Full, 1}, {Mixed, 3},
    {Full, 1}, {Mixed, 3}, {Full, 7}, {Mixed, 1}, {Full, 752}, {Mixed, 2},
    {Full, 46}, {Mixed, 3}, {Full, 1}, {Mixed, 2}, {Full, 2}, {Mixed, 2},
    {Full, 1}, {Mixed, 4}, {Full, 2}, {Mixed, 1}, {Full, 4}, {Mixed, 6},
    {Full, 5}, {Mixed, 2}, {Full, 2}, {Mixed, 2}, {Full, 6}, {Mixed, 2},
    {Full, 2}, {Mixed, 2}, {Full, 2}, {Mixed, 1}, {Full, 2}, {Mixed, 2},
    {Full, 22}, {Mixed, 3}, {Full, 1}, {Mixed, 1}, {Full, 12}, {Mixed, 1},
    {Full, 2}, {Mixed, 2}, {Full, 3}, {Mixed, 2}, {Full, 32533}};
    const static UnicodeSet::bitquad_t  __na_Set_quads[] = {
    0xfffffff0, 0x23ffffff, 0xff010000, 0xfffffff3, 0xfffffff1, 0x2fffffff,
    0xff7fc660, 0xfffffff3, 0xfffffff1, 0x2fffffff, 0xffffc678, 0xffdcffff,
    0xfffffff1, 0x2fffffff, 0xffffc440, 0x03fffff3, 0xfffffff1, 0x2fffffff,
    0xff3fc660, 0xfffffff3, 0xfffffffb, 0x3fffffff, 0xff7fc238, 0xfffffff0,
    0x3fffffff, 0xff9fc220, 0xfffffff3, 0xfffffff1, 0x2fffffff, 0xff9fc220,
    0xfffffff3, 0xfffffff0, 0x27ffffff, 0xff7fc220, 0xfffffff3, 0xfffffff3,
    0x00a07bff, 0xfff3ffff, 0xf800ffff, 0xffff8040, 0xe400ffff, 0xffffc0e0,
    0xfcffffff, 0x3d5fffff, 0x0001ffff, 0x01001f20, 0xe0000000, 0xffffffbf,
    0x920007ff, 0x3c3fffff, 0xffe1c062, 0xc3ff4003, 0xffe3ffff, 0xffe3ffff,
    0xfff3ffff, 0xfff3ffff, 0x003fffff, 0xdff40000, 0xf000f000, 0x0000ffff,
    0xfffffcfe, 0xf07fffff, 0x801fffff, 0x60000001, 0xffffffe0, 0x000fffff,
    0xffffffe0, 0xfff007ff, 0xfffffff8, 0xffffc801, 0xfff0003f, 0xff00000f,
    0x0008ffff, 0xff6fde00, 0xf7ffffff, 0xfffff7bf, 0xffffff07, 0xfffffffc,
    0x000fffff, 0xffffffc0, 0xfffc0000, 0xffffc7ff, 0xfff0007f, 0xfffffff0,
    0x0007ffff, 0xfffffffe, 0xffffffdf, 0xff8001ff, 0xffffcff7, 0xc7ffffff,
    0x0000ffff, 0xfffffffd, 0xffdf07ff, 0xffffc807, 0xffff0f91, 0xf8ffffff,
    0xfffffff8, 0x00ffffff, 0xffffff80, 0xfffffff8, 0xf800ffff, 0xfffffff8,
    0xffe8007f, 0xfff7ffff, 0xfffffff8, 0x0007ffff, 0xffffe7fe, 0xbf000fff,
    0x7fffffff, 0xfffff800, 0xfffffff1, 0x3fffffff, 0xff7fc660, 0xffe0e033,
    0x001fffff, 0xffffff80, 0x0000ffff, 0xfffffff0, 0x00c07fff, 0xcffffffe,
    0x0000ffff, 0xfffffffe, 0xff0007ff, 0x5fffffff, 0xfffff000, 0xfffff801,
    0x8407ffff, 0xf001ffff, 0xfe0003ff, 0x00807fff, 0x0003ffff, 0xff800100,
    0x4b81ffff, 0xffffff60};
    }

    const static UnicodeSet na_Set{const_cast<UnicodeSet::run_t *>(__na_Set_runs), 89, 0, const_cast<UnicodeSet::bitquad_t *>(__na_Set_quads), 134, 0};

    /** Code Point Ranges for Right
    [0903, 0903], [093b, 093b], [093e, 093e], [0940, 0940], [0949, 094c],
    [094f, 094f], [0982, 0983], [09be, 09be], [09c0, 09c0], [09d7, 09d7],
    [0a03, 0a03], [0a3e, 0a3e], [0a40, 0a40], [0a83, 0a83], [0abe, 0abe],
    [0ac0, 0ac0], [0acb, 0acc], [0b02, 0b03], [0b3e, 0b3e], [0b40, 0b40],
    [0bbe, 0bbf], [0bc1, 0bc2], [0bd7, 0bd7], [0c01, 0c03], [0c41, 0c44],
    [0c82, 0c83], [0cbe, 0cbe], [0cc1, 0cc4], [0cd5, 0cd6], [0d02, 0d03],
    [0d3e, 0d42], [0d57, 0d57], [0d82, 0d83], [0dcf, 0dd1], [0dd8, 0dd8],
    [0ddf, 0ddf], [0df2, 0df3], [0e30, 0e30], [0e32, 0e33], [0e45, 0e45],
    [0eb0, 0eb0], [0eb2, 0eb3], [0f3e, 0f3e], [0f7f, 0f7f], [102b, 102c],
    [1038, 1038], [103b, 103b], [1056, 1057], [1062, 1064], [1067, 106d],
    [1083, 1083], [1087, 108c], [108f, 108f], [109a, 109c], [17b6, 17b6],
    [17c7, 17c8], [1923, 1924], [1929, 192b], [1930, 1931], [1933, 1938],
    [19b0, 19b4], [19b8, 19b9], [19bb, 19c0], [19c8, 19c9], [1a1a, 1a1a],
    [1a57, 1a57], [1a61, 1a61], [1a63, 1a64], [1a6d, 1a6d], [1b04, 1b04],
    [1b35, 1b35], [1b44, 1b44], [1b82, 1b82], [1ba1, 1ba1], [1ba7, 1ba7],
    [1baa, 1baa], [1be7, 1be7], [1bea, 1bec], [1bee, 1bee], [1bf2, 1bf3],
    [1c24, 1c26], [1c2a, 1c2b], [1ce1, 1ce1], [1cf7, 1cf7], [a823, a824],
    [a827, a827], [a880, a881], [a8b4, a8c3], [a952, a953], [a983, a983],
    [a9b4, a9b5], [a9bd, a9be], [aa33, aa33], [aa4d, aa4d], [aa7b, aa7b],
    [aa7d, aa7d], [aab1, aab1], [aaba, aaba], [aabd, aabd], [aaef, aaef],
    [aaf5, aaf5], [abe3, abe4], [abe6, abe7], [abe9, abea], [abec, abec],
    [11000, 11000], [11002, 11002], [11082, 11082], [110b0, 110b0],
    [110b2, 110b2], [110b7, 110b8], [11182, 11182], [111b3, 111b3],
    [111b5, 111b5], [111c0, 111c0], [1122c, 1122e], [11235, 11235],
    [112e0, 112e0], [112e2, 112e2], [11302, 11303], [1133e, 1133f],
    [11341, 11344], [1134d, 1134d], [11357, 11357], [11362, 11363],
    [11435, 11435], [11437, 11437], [11440, 11441], [11445, 11445],
    [114b0, 114b0], [114b2, 114b2], [114bd, 114bd], [114c1, 114c1],
    [115af, 115af], [115b1, 115b1], [115be, 115be], [11630, 11632],
    [1163b, 1163c], [1163e, 1163e], [116ac, 116ac], [116af, 116af],
    [116b6, 116b6], [11720, 11721], [11a39, 11a39], [11a57, 11a58],
    [11a97, 11a97], [11c2f, 11c2f], [11c3e, 11c3e], [11ca9, 11ca9],
    [11cb4, 11cb4]**/


    namespace {
    const static UnicodeSet::run_t __right_Set_runs[] = {
    {Empty, 72}, {Mixed, 3}, {Empty, 1}, {Mixed, 3}, {Empty, 1}, {Mixed, 3},
    {Empty, 1}, {Mixed, 3}, {Empty, 1}, {Mixed, 3}, {Empty, 2}, {Mixed, 2},
    {Empty, 1}, {Mixed, 1}, {Empty, 1}, {Mixed, 1}, {Empty, 1}, {Mixed, 3},
    {Empty, 1}, {Mixed, 3}, {Empty, 1}, {Mixed, 1}, {Empty, 1}, {Mixed, 2},
    {Empty, 1}, {Mixed, 2}, {Empty, 2}, {Mixed, 1}, {Empty, 3}, {Mixed, 1},
    {Empty, 1}, {Mixed, 1}, {Empty, 5}, {Mixed, 4}, {Empty, 56}, {Mixed, 2},
    {Empty, 10}, {Mixed, 1}, {Empty, 3}, {Mixed, 2}, {Empty, 1}, {Mixed, 1},
    {Empty, 1}, {Mixed, 2}, {Empty, 4}, {Mixed, 3}, {Empty, 1}, {Mixed, 2},
    {Empty, 1}, {Mixed, 1}, {Empty, 1}, {Mixed, 1}, {Empty, 5}, {Mixed, 1},
    {Empty, 1113}, {Mixed, 1}, {Empty, 2}, {Mixed, 3}, {Empty, 3},
    {Mixed, 1}, {Empty, 1}, {Mixed, 2}, {Empty, 3}, {Mixed, 3}, {Empty, 1},
    {Mixed, 1}, {Empty, 1}, {Mixed, 1}, {Empty, 7}, {Mixed, 1},
    {Empty, 800}, {Mixed, 1}, {Empty, 3}, {Mixed, 2}, {Empty, 6},
    {Mixed, 3}, {Empty, 2}, {Mixed, 1}, {Empty, 5}, {Mixed, 5}, {Empty, 5},
    {Mixed, 2}, {Empty, 2}, {Mixed, 2}, {Empty, 6}, {Mixed, 1}, {Empty, 3},
    {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 23},
    {Mixed, 2}, {Empty, 1}, {Mixed, 1}, {Empty, 12}, {Mixed, 1}, {Empty, 3},
    {Mixed, 1}, {Empty, 32538}};
    const static UnicodeSet::bitquad_t  __right_Set_quads[] = {
    0x00000008, 0x48000000, 0x00009e01, 0x0000000c, 0x40000000, 0x00800001,
    0x00000008, 0x40000000, 0x00000001, 0x00000008, 0x40000000, 0x00001801,
    0x0000000c, 0x40000000, 0x00000001, 0xc0000000, 0x00800006, 0x0000000e,
    0x0000001e, 0x0000000c, 0x40000000, 0x0060001e, 0x0000000c, 0xc0000000,
    0x00800007, 0x0000000c, 0x81038000, 0x000c0000, 0x000d0000, 0x00000020,
    0x000d0000, 0x40000000, 0x80000000, 0x09001800, 0x00c00000, 0x00003f9c,
    0x1c009f88, 0x00400000, 0x00000180, 0x01fb0e18, 0xfb1f0000, 0x00000301,
    0x04000000, 0x00800000, 0x0000201a, 0x00000010, 0x00200000, 0x00000010,
    0x00000004, 0x00000482, 0x000c5c80, 0x00000c70, 0x00800002, 0x00000098,
    0x00000003, 0xfff00000, 0x0000000f, 0x000c0000, 0x00000008, 0x60300000,
    0x00080000, 0x00002000, 0x28000000, 0x24020000, 0x00208000, 0x000016d8,
    0x00000005, 0x00000004, 0x01850000, 0x00000004, 0x00280000, 0x00000001,
    0x00207000, 0x00000005, 0x0000000c, 0xc0000000, 0x0080201e, 0x0000000c,
    0x00a00000, 0x00000023, 0x20050000, 0x00000002, 0x40028000, 0x58070000,
    0x00409000, 0x00000003, 0x02000000, 0x01800000, 0x00800000, 0x40008000,
    0x00100200};
    }

    const static UnicodeSet right_Set{const_cast<UnicodeSet::run_t *>(__right_Set_runs), 101, 0, const_cast<UnicodeSet::bitquad_t *>(__right_Set_quads), 91, 0};

    /** Code Point Ranges for Left
    [093f, 093f], [094e, 094e], [09bf, 09bf], [09c7, 09c8], [0a3f, 0a3f],
    [0abf, 0abf], [0b47, 0b47], [0bc6, 0bc8], [0d46, 0d48], [0dd9, 0dd9],
    [0ddb, 0ddb], [0f3f, 0f3f], [1031, 1031], [1084, 1084], [17c1, 17c3],
    [1a19, 1a19], [1a55, 1a55], [1a6e, 1a72], [1b3e, 1b3f], [1ba6, 1ba6],
    [1c27, 1c28], [1c34, 1c35], [a9ba, a9bb], [aa2f, aa30], [aa34, aa34],
    [aaeb, aaeb], [aaee, aaee], [110b1, 110b1], [1112c, 1112c],
    [111b4, 111b4], [112e1, 112e1], [11347, 11348], [11436, 11436],
    [114b1, 114b1], [114b9, 114b9], [115b0, 115b0], [115b8, 115b8],
    [116ae, 116ae], [11726, 11726], [11cb1, 11cb1]**/


    namespace {
    const static UnicodeSet::run_t __left_Set_runs[] = {
    {Empty, 73}, {Mixed, 2}, {Empty, 2}, {Mixed, 2}, {Empty, 2}, {Mixed, 1},
    {Empty, 3}, {Mixed, 1}, {Empty, 4}, {Mixed, 1}, {Empty, 3}, {Mixed, 1},
    {Empty, 11}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 10},
    {Mixed, 1}, {Empty, 7}, {Mixed, 1}, {Empty, 2}, {Mixed, 1}, {Empty, 57},
    {Mixed, 1}, {Empty, 17}, {Mixed, 1}, {Empty, 1}, {Mixed, 2}, {Empty, 5},
    {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 3}, {Mixed, 1},
    {Empty, 1131}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 5},
    {Mixed, 1}, {Empty, 813}, {Mixed, 1}, {Empty, 3}, {Mixed, 1},
    {Empty, 3}, {Mixed, 1}, {Empty, 9}, {Mixed, 1}, {Empty, 2}, {Mixed, 1},
    {Empty, 6}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 7}, {Mixed, 1},
    {Empty, 7}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 43}, {Mixed, 1},
    {Empty, 32538}};
    const static UnicodeSet::bitquad_t  __left_Set_quads[] = {
    0x80000000, 0x00004000, 0x80000000, 0x00000180, 0x80000000, 0x80000000,
    0x00000080, 0x000001c0, 0x000001c0, 0x0a000000, 0x80000000, 0x00020000,
    0x00000010, 0x0000000e, 0x02000000, 0x00200000, 0x0007c000, 0xc0000000,
    0x00000040, 0x00300180, 0x0c000000, 0x00118000, 0x00004800, 0x00020000,
    0x00001000, 0x00100000, 0x00000002, 0x00000180, 0x00400000, 0x02020000,
    0x01010000, 0x00004000, 0x00000040, 0x00020000};
    }

    const static UnicodeSet left_Set{const_cast<UnicodeSet::run_t *>(__left_Set_runs), 63, 0, const_cast<UnicodeSet::bitquad_t *>(__left_Set_quads), 34, 0};

    /** Code Point Ranges for Visual_Order_Left
    [0e40, 0e44], [0ec0, 0ec4], [19b5, 19b7], [19ba, 19ba], [aab5, aab6],
    [aab9, aab9], [aabb, aabc]**/


    namespace {
    const static UnicodeSet::run_t __visual_order_left_Set_runs[] = {
    {Empty, 114}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 86},
    {Mixed, 1}, {Empty, 1159}, {Mixed, 1}, {Empty, 33450}};
    const static UnicodeSet::bitquad_t  __visual_order_left_Set_quads[] = {
    0x0000001f, 0x0000001f, 0x04e00000, 0x1a600000};
    }

    const static UnicodeSet visual_order_left_Set{const_cast<UnicodeSet::run_t *>(__visual_order_left_Set_runs), 9, 0, const_cast<UnicodeSet::bitquad_t *>(__visual_order_left_Set_quads), 4, 0};

    /** Code Point Ranges for Left_And_Right
    [09cb, 09cc], [0b4b, 0b4b], [0bca, 0bcc], [0d4a, 0d4c], [0ddc, 0ddc],
    [0dde, 0dde], [17c0, 17c0], [17c4, 17c5], [1b40, 1b41], [1134b, 1134c],
    [114bc, 114bc], [114be, 114be], [115ba, 115ba]**/


    namespace {
    const static UnicodeSet::run_t __left_and_right_Set_runs[] = {
    {Empty, 78}, {Mixed, 1}, {Empty, 11}, {Mixed, 1}, {Empty, 3},
    {Mixed, 1}, {Empty, 11}, {Mixed, 1}, {Empty, 3}, {Mixed, 1},
    {Empty, 79}, {Mixed, 1}, {Empty, 27}, {Mixed, 1}, {Empty, 1983},
    {Mixed, 1}, {Empty, 10}, {Mixed, 1}, {Empty, 7}, {Mixed, 1},
    {Empty, 32594}};
    const static UnicodeSet::bitquad_t  __left_and_right_Set_quads[] = {
    0x00001800, 0x00000800, 0x00001c00, 0x00001c00, 0x50000000, 0x00000031,
    0x00000003, 0x00001800, 0x50000000, 0x04000000};
    }

    const static UnicodeSet left_and_right_Set{const_cast<UnicodeSet::run_t *>(__left_and_right_Set_runs), 21, 0, const_cast<UnicodeSet::bitquad_t *>(__left_and_right_Set_quads), 10, 0};

    /** Code Point Ranges for Top
    [0900, 0902], [093a, 093a], [0945, 0948], [0951, 0951], [0953, 0955],
    [0981, 0981], [0a01, 0a02], [0a47, 0a48], [0a4b, 0a4c], [0a70, 0a71],
    [0a81, 0a82], [0ac5, 0ac5], [0ac7, 0ac8], [0afa, 0aff], [0b01, 0b01],
    [0b3f, 0b3f], [0b56, 0b56], [0b82, 0b82], [0bc0, 0bc0], [0bcd, 0bcd],
    [0c00, 0c00], [0c3e, 0c40], [0c46, 0c47], [0c4a, 0c4d], [0c55, 0c55],
    [0c81, 0c81], [0cbf, 0cbf], [0cc6, 0cc6], [0ccc, 0ccd], [0d00, 0d01],
    [0d3b, 0d3c], [0d4d, 0d4d], [0dca, 0dca], [0dd2, 0dd3], [0e31, 0e31],
    [0e34, 0e37], [0e47, 0e4e], [0eb1, 0eb1], [0eb4, 0eb7], [0ebb, 0ebb],
    [0ec8, 0ecd], [0f39, 0f39], [0f72, 0f72], [0f7a, 0f7e], [0f80, 0f80],
    [0f82, 0f83], [0f86, 0f87], [102d, 102e], [1032, 1036], [103a, 103a],
    [1071, 1074], [1085, 1086], [109d, 109d], [1712, 1712], [1732, 1732],
    [1752, 1752], [1772, 1772], [17b7, 17ba], [17c6, 17c6], [17c9, 17d1],
    [17d3, 17d3], [17dd, 17dd], [1920, 1921], [1927, 1928], [193a, 193a],
    [1a17, 1a17], [1a1b, 1a1b], [1a58, 1a5a], [1a62, 1a62], [1a65, 1a68],
    [1a6b, 1a6b], [1a73, 1a7c], [1b00, 1b03], [1b34, 1b34], [1b36, 1b37],
    [1b42, 1b42], [1b6b, 1b6b], [1b6d, 1b73], [1b80, 1b81], [1ba4, 1ba4],
    [1ba8, 1ba9], [1be6, 1be6], [1be8, 1be9], [1bed, 1bed], [1bef, 1bf1],
    [1c2d, 1c33], [1c36, 1c36], [1cd0, 1cd2], [1cda, 1cdb], [1ce0, 1ce0],
    [1cf4, 1cf4], [1dfb, 1dfb], [a806, a806], [a80b, a80b], [a826, a826],
    [a8c5, a8c5], [a8e0, a8f1], [a94a, a94a], [a94f, a951], [a980, a982],
    [a9b3, a9b3], [a9b6, a9b7], [a9bc, a9bc], [a9e5, a9e5], [aa29, aa2c],
    [aa2e, aa2e], [aa31, aa31], [aa43, aa43], [aa4c, aa4c], [aa7c, aa7c],
    [aab0, aab0], [aab2, aab3], [aab7, aab8], [aabe, aabf], [aac1, aac1],
    [aaed, aaed], [abe5, abe5], [10a05, 10a05], [10a0f, 10a0f],
    [10a38, 10a38], [11001, 11001], [11038, 1103b], [11042, 11046],
    [11080, 11081], [110b5, 110b6], [11100, 11102], [11127, 11129],
    [1112d, 1112d], [11130, 11130], [11134, 11134], [11180, 11181],
    [111bc, 111be], [111cb, 111cb], [11230, 11231], [11234, 11234],
    [11236, 11237], [1123e, 1123e], [112df, 112df], [112e5, 112e8],
    [11301, 11301], [11340, 11340], [11366, 1136c], [11370, 11374],
    [1143e, 1143f], [11443, 11444], [114ba, 114ba], [114bf, 114c0],
    [115bc, 115bd], [11639, 1163a], [1163d, 1163d], [11640, 11640],
    [116ab, 116ab], [116ad, 116ad], [116b2, 116b5], [1171f, 1171f],
    [11722, 11723], [11727, 11727], [11729, 1172b], [11a01, 11a01],
    [11a04, 11a09], [11a35, 11a38], [11a51, 11a51], [11a54, 11a56],
    [11a96, 11a96], [11a98, 11a98], [11c30, 11c31], [11c38, 11c3d],
    [11cb3, 11cb3], [11cb5, 11cb6], [11d31, 11d35], [11d3a, 11d3a],
    [11d3c, 11d3d], [11d3f, 11d41], [11d43, 11d43]**/


    namespace {
    const static UnicodeSet::run_t __top_Set_runs[] = {
    {Empty, 72}, {Mixed, 3}, {Empty, 1}, {Mixed, 1}, {Empty, 3}, {Mixed, 1},
    {Empty, 1}, {Mixed, 3}, {Empty, 1}, {Mixed, 5}, {Empty, 1}, {Mixed, 1},
    {Empty, 1}, {Mixed, 1}, {Empty, 1}, {Mixed, 3}, {Empty, 1}, {Mixed, 3},
    {Empty, 1}, {Mixed, 3}, {Empty, 3}, {Mixed, 1}, {Empty, 2}, {Mixed, 2},
    {Empty, 2}, {Mixed, 2}, {Empty, 2}, {Mixed, 1}, {Empty, 1}, {Mixed, 2},
    {Empty, 4}, {Mixed, 1}, {Empty, 1}, {Mixed, 2}, {Empty, 51}, {Mixed, 4},
    {Empty, 1}, {Mixed, 2}, {Empty, 10}, {Mixed, 1}, {Empty, 6}, {Mixed, 1},
    {Empty, 1}, {Mixed, 2}, {Empty, 4}, {Mixed, 6}, {Empty, 1}, {Mixed, 1},
    {Empty, 1}, {Mixed, 1}, {Empty, 4}, {Mixed, 2}, {Empty, 7}, {Mixed, 1},
    {Empty, 1104}, {Mixed, 2}, {Empty, 4}, {Mixed, 2}, {Empty, 2},
    {Mixed, 1}, {Empty, 1}, {Mixed, 2}, {Empty, 1}, {Mixed, 1}, {Empty, 1},
    {Mixed, 3}, {Empty, 1}, {Mixed, 3}, {Empty, 7}, {Mixed, 1},
    {Empty, 752}, {Mixed, 2}, {Empty, 46}, {Mixed, 3}, {Empty, 1},
    {Mixed, 2}, {Empty, 2}, {Mixed, 2}, {Empty, 2}, {Mixed, 3}, {Empty, 2},
    {Mixed, 1}, {Empty, 4}, {Mixed, 3}, {Empty, 1}, {Mixed, 2}, {Empty, 5},
    {Mixed, 2}, {Empty, 2}, {Mixed, 2}, {Empty, 6}, {Mixed, 1}, {Empty, 3},
    {Mixed, 2}, {Empty, 2}, {Mixed, 1}, {Empty, 2}, {Mixed, 2}, {Empty, 22},
    {Mixed, 3}, {Empty, 1}, {Mixed, 1}, {Empty, 12}, {Mixed, 1}, {Empty, 3},
    {Mixed, 1}, {Empty, 3}, {Mixed, 2}, {Empty, 32533}};
    const static UnicodeSet::bitquad_t  __top_Set_quads[] = {
    0x00000007, 0x04000000, 0x003a01e0, 0x00000002, 0x00000006, 0x00001980,
    0x00030000, 0x00000006, 0x000001a0, 0xfc000000, 0x00000002, 0x80000000,
    0x00400000, 0x00000004, 0x00002001, 0x00000001, 0xc0000000, 0x00203cc1,
    0x00000002, 0x80000000, 0x00003040, 0x00000003, 0x18000000, 0x00002000,
    0x000c0400, 0x00f20000, 0x00007f80, 0x08f20000, 0x00003f00, 0x02000000,
    0x7c040000, 0x000000cd, 0x047c6000, 0x001e0000, 0x20000060, 0x00040000,
    0x00040000, 0x00040000, 0x00040000, 0x07800000, 0x200bfe40, 0x04000183,
    0x08800000, 0x07000000, 0x1ff809e4, 0x0000000f, 0x00d00000, 0x00000004,
    0x000fe800, 0x00000003, 0x00000310, 0x0003a340, 0x004fe000, 0x0c070000,
    0x00100001, 0x08000000, 0x00000840, 0x00000040, 0x00000020, 0x0003ffff,
    0x00038400, 0x00000007, 0x10c80000, 0x00000020, 0x00025e00, 0x00001008,
    0x10000000, 0xc18d0000, 0x00000002, 0x00002000, 0x00000020, 0x00008020,
    0x01000000, 0x00000002, 0x0f000000, 0x0000007c, 0x00000003, 0x00600000,
    0x00000007, 0x00112380, 0x00000003, 0x70000000, 0x00000800, 0x40d30000,
    0x80000000, 0x000001e0, 0x00000002, 0x00000001, 0x001f1fc0, 0xc0000000,
    0x00000018, 0x84000000, 0x00000001, 0x30000000, 0x26000000, 0x00000001,
    0x003c2800, 0x80000000, 0x00000e8c, 0x000003f2, 0x01e00000, 0x00720000,
    0x01400000, 0x3f030000, 0x00680000, 0xb43e0000, 0x0000000b};
    }

    const static UnicodeSet top_Set{const_cast<UnicodeSet::run_t *>(__top_Set_runs), 109, 0, const_cast<UnicodeSet::bitquad_t *>(__top_Set_quads), 107, 0};

    /** Code Point Ranges for Bottom
    [093c, 093c], [0941, 0944], [094d, 094d], [0952, 0952], [0956, 0957],
    [0962, 0963], [09bc, 09bc], [09c1, 09c4], [09cd, 09cd], [09e2, 09e3],
    [0a3c, 0a3c], [0a41, 0a42], [0a4d, 0a4d], [0a75, 0a75], [0abc, 0abc],
    [0ac1, 0ac4], [0acd, 0acd], [0ae2, 0ae3], [0b3c, 0b3c], [0b41, 0b44],
    [0b4d, 0b4d], [0b62, 0b63], [0c56, 0c56], [0c62, 0c63], [0cbc, 0cbc],
    [0ce2, 0ce3], [0d43, 0d44], [0d62, 0d63], [0dd4, 0dd4], [0dd6, 0dd6],
    [0e38, 0e3a], [0eb8, 0eb9], [0ebc, 0ebc], [0f18, 0f19], [0f35, 0f35],
    [0f37, 0f37], [0f71, 0f71], [0f74, 0f75], [0f84, 0f84], [0f8d, 0f97],
    [0f99, 0fbc], [0fc6, 0fc6], [102f, 1030], [1037, 1037], [103d, 103e],
    [1058, 1059], [105e, 1060], [1082, 1082], [108d, 108d], [1713, 1714],
    [1733, 1734], [1753, 1753], [1773, 1773], [17bb, 17bd], [1922, 1922],
    [1932, 1932], [1939, 1939], [193b, 193b], [1a18, 1a18], [1a56, 1a56],
    [1a5b, 1a5e], [1a69, 1a6a], [1a6c, 1a6c], [1a7f, 1a7f], [1b38, 1b3a],
    [1b6c, 1b6c], [1ba2, 1ba3], [1ba5, 1ba5], [1bac, 1bad], [1c2c, 1c2c],
    [1c37, 1c37], [1cd5, 1cd9], [1cdc, 1cdf], [1ced, 1ced], [a825, a825],
    [a8c4, a8c4], [a92b, a92d], [a947, a949], [a94b, a94e], [a9b8, a9b9],
    [aa2d, aa2d], [aa32, aa32], [aa35, aa36], [aab4, aab4], [aaec, aaec],
    [abe8, abe8], [abed, abed], [10a02, 10a03], [10a0c, 10a0e],
    [10a39, 10a3a], [1103c, 11041], [110b3, 110b4], [110b9, 110ba],
    [1112a, 1112b], [11131, 11132], [11173, 11173], [111b6, 111bb],
    [111cc, 111cc], [1122f, 1122f], [112e3, 112e4], [112e9, 112ea],
    [11438, 1143d], [11442, 11442], [11446, 11446], [114b3, 114b8],
    [114c2, 114c3], [115b2, 115b5], [115bf, 115c0], [115dc, 115dd],
    [11633, 11638], [1163f, 1163f], [116b0, 116b1], [116b7, 116b7],
    [1171d, 1171d], [11724, 11725], [11728, 11728], [11a02, 11a03],
    [11a0a, 11a0a], [11a33, 11a34], [11a3b, 11a3e], [11a52, 11a53],
    [11a59, 11a5b], [11a8a, 11a95], [11c32, 11c36], [11c3f, 11c3f],
    [11c92, 11ca7], [11caa, 11cb0], [11cb2, 11cb2], [11d36, 11d36],
    [11d42, 11d42], [11d44, 11d44], [11d47, 11d47]**/


    namespace {
    const static UnicodeSet::run_t __bottom_Set_runs[] = {
    {Empty, 73}, {Mixed, 3}, {Empty, 1}, {Mixed, 3}, {Empty, 1}, {Mixed, 3},
    {Empty, 1}, {Mixed, 3}, {Empty, 1}, {Mixed, 3}, {Empty, 6}, {Mixed, 2},
    {Empty, 1}, {Mixed, 1}, {Empty, 1}, {Mixed, 1}, {Empty, 2}, {Mixed, 2},
    {Empty, 2}, {Mixed, 1}, {Empty, 2}, {Mixed, 1}, {Empty, 3}, {Mixed, 1},
    {Empty, 2}, {Mixed, 2}, {Empty, 1}, {Mixed, 4}, {Empty, 2}, {Mixed, 4},
    {Empty, 51}, {Mixed, 4}, {Empty, 1}, {Mixed, 1}, {Empty, 11},
    {Mixed, 1}, {Empty, 6}, {Mixed, 1}, {Empty, 1}, {Mixed, 2}, {Empty, 5},
    {Mixed, 1}, {Empty, 1}, {Mixed, 1}, {Empty, 1}, {Mixed, 1}, {Empty, 3},
    {Mixed, 1}, {Empty, 4}, {Mixed, 2}, {Empty, 1113}, {Mixed, 1},
    {Empty, 4}, {Mixed, 1}, {Empty, 2}, {Mixed, 2}, {Empty, 2}, {Mixed, 1},
    {Empty, 3}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 1}, {Mixed, 1},
    {Empty, 7}, {Mixed, 1}, {Empty, 752}, {Mixed, 2}, {Empty, 47},
    {Mixed, 2}, {Empty, 2}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 1},
    {Mixed, 1}, {Empty, 1}, {Mixed, 2}, {Empty, 2}, {Mixed, 1}, {Empty, 5},
    {Mixed, 1}, {Empty, 9}, {Mixed, 2}, {Empty, 2}, {Mixed, 2}, {Empty, 6},
    {Mixed, 2}, {Empty, 2}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 2},
    {Mixed, 2}, {Empty, 22}, {Mixed, 3}, {Empty, 1}, {Mixed, 1},
    {Empty, 12}, {Mixed, 1}, {Empty, 2}, {Mixed, 2}, {Empty, 3}, {Mixed, 2},
    {Empty, 32533}};
    const static UnicodeSet::bitquad_t  __bottom_Set_quads[] = {
    0x10000000, 0x00c4201e, 0x0000000c, 0x10000000, 0x0000201e, 0x0000000c,
    0x10000000, 0x00002006, 0x00200000, 0x10000000, 0x0000201e, 0x0000000c,
    0x10000000, 0x0000201e, 0x0000000c, 0x00400000, 0x0000000c, 0x10000000,
    0x0000000c, 0x00000018, 0x0000000c, 0x00500000, 0x07000000, 0x13000000,
    0x03000000, 0x00a00000, 0x00320000, 0xfeffe010, 0x1fffffff, 0x00000040,
    0x60818000, 0xc3000000, 0x00000001, 0x00002004, 0x00180000, 0x00180000,
    0x00080000, 0x00080000, 0x38000000, 0x0a040004, 0x01000000, 0x78400000,
    0x80001600, 0x07000000, 0x00001000, 0x0000302c, 0x00801000, 0xf3e00000,
    0x00002000, 0x00000020, 0x00000010, 0x00003800, 0x00007b80, 0x03000000,
    0x00642000, 0x00100000, 0x00001000, 0x00002100, 0x0000700c, 0x06000000,
    0xf0000000, 0x00000003, 0x06180000, 0x00060c00, 0x00080000, 0x0fc00000,
    0x00001000, 0x00008000, 0x00000618, 0x3f000000, 0x00000044, 0x01f80000,
    0x0000000c, 0x803c0000, 0x30000001, 0x81f80000, 0x00830000, 0x20000000,
    0x00000130, 0x0000040c, 0x78180000, 0x0e0c0000, 0x003ffc00, 0x807c0000,
    0xfffc0000, 0x0005fcff, 0x00400000, 0x00000094};
    }

    const static UnicodeSet bottom_Set{const_cast<UnicodeSet::run_t *>(__bottom_Set_runs), 105, 0, const_cast<UnicodeSet::bitquad_t *>(__bottom_Set_quads), 88, 0};

    /** Code Point Ranges for Top_And_Bottom
    [0c48, 0c48], [0f73, 0f73], [0f76, 0f79], [0f81, 0f81], [1b3c, 1b3c],
    [1112e, 1112f]**/


    namespace {
    const static UnicodeSet::run_t __top_and_bottom_Set_runs[] = {
    {Empty, 98}, {Mixed, 1}, {Empty, 24}, {Mixed, 2}, {Empty, 92},
    {Mixed, 1}, {Empty, 1967}, {Mixed, 1}, {Empty, 32630}};
    const static UnicodeSet::bitquad_t  __top_and_bottom_Set_quads[] = {
    0x00000100, 0x03c80000, 0x00000002, 0x10000000, 0x0000c000};
    }

    const static UnicodeSet top_and_bottom_Set{const_cast<UnicodeSet::run_t *>(__top_and_bottom_Set_runs), 9, 0, const_cast<UnicodeSet::bitquad_t *>(__top_and_bottom_Set_quads), 5, 0};

    /** Code Point Ranges for Top_And_Right
    [0ac9, 0ac9], [0b57, 0b57], [0cc0, 0cc0], [0cc7, 0cc8], [0cca, 0ccb],
    [1925, 1926], [1b43, 1b43], [111bf, 111bf], [11232, 11233]**/


    namespace {
    const static UnicodeSet::run_t __top_and_right_Set_runs[] = {
    {Empty, 86}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 11},
    {Mixed, 1}, {Empty, 98}, {Mixed, 1}, {Empty, 16}, {Mixed, 1},
    {Empty, 1970}, {Mixed, 1}, {Empty, 3}, {Mixed, 1}, {Empty, 32622}};
    const static UnicodeSet::bitquad_t  __top_and_right_Set_quads[] = {
    0x00000200, 0x00800000, 0x00000d81, 0x00000060, 0x00000008, 0x80000000,
    0x000c0000};
    }

    const static UnicodeSet top_and_right_Set{const_cast<UnicodeSet::run_t *>(__top_and_right_Set_runs), 15, 0, const_cast<UnicodeSet::bitquad_t *>(__top_and_right_Set_quads), 7, 0};

    /** Code Point Ranges for Top_And_Left
    [0b48, 0b48], [0dda, 0dda], [17be, 17be], [1c29, 1c29], [114bb, 114bb],
    [115b9, 115b9]**/


    namespace {
    const static UnicodeSet::run_t __top_and_left_Set_runs[] = {
    {Empty, 90}, {Mixed, 1}, {Empty, 19}, {Mixed, 1}, {Empty, 78},
    {Mixed, 1}, {Empty, 35}, {Mixed, 1}, {Empty, 1987}, {Mixed, 1},
    {Empty, 7}, {Mixed, 1}, {Empty, 32594}};
    const static UnicodeSet::bitquad_t  __top_and_left_Set_quads[] = {
    0x00000100, 0x04000000, 0x40000000, 0x00000200, 0x08000000, 0x02000000};
    }

    const static UnicodeSet top_and_left_Set{const_cast<UnicodeSet::run_t *>(__top_and_left_Set_runs), 13, 0, const_cast<UnicodeSet::bitquad_t *>(__top_and_left_Set_quads), 6, 0};

    /** Code Point Ranges for Top_And_Left_And_Right
    [0b4c, 0b4c], [0ddd, 0ddd], [17bf, 17bf], [115bb, 115bb]**/


    namespace {
    const static UnicodeSet::run_t __top_and_left_and_right_Set_runs[] = {
    {Empty, 90}, {Mixed, 1}, {Empty, 19}, {Mixed, 1}, {Empty, 78},
    {Mixed, 1}, {Empty, 2031}, {Mixed, 1}, {Empty, 32594}};
    const static UnicodeSet::bitquad_t  __top_and_left_and_right_Set_quads[] = {
    0x00001000, 0x20000000, 0x80000000, 0x08000000};
    }

    const static UnicodeSet top_and_left_and_right_Set{const_cast<UnicodeSet::run_t *>(__top_and_left_and_right_Set_runs), 9, 0, const_cast<UnicodeSet::bitquad_t *>(__top_and_left_and_right_Set_quads), 4, 0};

    /** Code Point Ranges for Bottom_And_Right
    [1b3b, 1b3b], [a9c0, a9c0]**/


    namespace {
    const static UnicodeSet::run_t __bottom_and_right_Set_runs[] = {
    {Empty, 217}, {Mixed, 1}, {Empty, 1140}, {Mixed, 1}, {Empty, 33457}};
    const static UnicodeSet::bitquad_t  __bottom_and_right_Set_quads[] = {
    0x08000000, 0x00000001};
    }

    const static UnicodeSet bottom_and_right_Set{const_cast<UnicodeSet::run_t *>(__bottom_and_right_Set_runs), 5, 0, const_cast<UnicodeSet::bitquad_t *>(__bottom_and_right_Set_quads), 2, 0};

    /** Code Point Ranges for Bottom_And_Left
    [a9bf, a9bf]**/


    namespace {
    const static UnicodeSet::run_t __bottom_and_left_Set_runs[] = {
    {Empty, 1357}, {Mixed, 1}, {Empty, 33458}};
    const static UnicodeSet::bitquad_t  __bottom_and_left_Set_quads[] = {
    0x80000000};
    }

    const static UnicodeSet bottom_and_left_Set{const_cast<UnicodeSet::run_t *>(__bottom_and_left_Set_runs), 3, 0, const_cast<UnicodeSet::bitquad_t *>(__bottom_and_left_Set_quads), 1, 0};

    /** Code Point Ranges for Top_And_Bottom_And_Right
    [1b3d, 1b3d]**/


    namespace {
    const static UnicodeSet::run_t __top_and_bottom_and_right_Set_runs[] = {
    {Empty, 217}, {Mixed, 1}, {Empty, 34598}};
    const static UnicodeSet::bitquad_t  __top_and_bottom_and_right_Set_quads[] = {
    0x20000000};
    }

    const static UnicodeSet top_and_bottom_and_right_Set{const_cast<UnicodeSet::run_t *>(__top_and_bottom_and_right_Set_runs), 3, 0, const_cast<UnicodeSet::bitquad_t *>(__top_and_bottom_and_right_Set_quads), 1, 0};

    /** Code Point Ranges for Overstruck
    [1cd4, 1cd4], [1ce2, 1ce8], [10a01, 10a01], [10a06, 10a06]**/


    namespace {
    const static UnicodeSet::run_t __overstruck_Set_runs[] = {
    {Empty, 230}, {Mixed, 2}, {Empty, 1896}, {Mixed, 1}, {Empty, 32687}};
    const static UnicodeSet::bitquad_t  __overstruck_Set_quads[] = {
    0x00100000, 0x000001fc, 0x00000042};
    }

    const static UnicodeSet overstruck_Set{const_cast<UnicodeSet::run_t *>(__overstruck_Set_runs), 5, 0, const_cast<UnicodeSet::bitquad_t *>(__overstruck_Set_quads), 3, 0};

    static EnumeratedPropertyObject property_object
        {InPC,
        INPC_ns::independent_prop_values,
        std::move(INPC_ns::enum_names),
        std::move(INPC_ns::value_names),
        std::move(INPC_ns::aliases_only_map),{
        &na_Set, &right_Set, &left_Set, &visual_order_left_Set,
        &left_and_right_Set, &top_Set, &bottom_Set, &top_and_bottom_Set,
        &top_and_right_Set, &top_and_left_Set, &top_and_left_and_right_Set,
        &bottom_and_right_Set, &bottom_and_left_Set,
        &top_and_bottom_and_right_Set, &overstruck_Set
        }};
    }
}

#endif
