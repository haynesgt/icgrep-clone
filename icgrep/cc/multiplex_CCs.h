/*
 *  Copyright (c) 2017 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
 */
#ifndef MULTIPLEX_CCS_H
#define MULTIPLEX_CCS_H

#include <vector>
#include <UCD/unicode_set.h>


void doMultiplexCCs(std::vector<UCD::UnicodeSet> CCs,
                    std::vector<std::vector<unsigned>> & exclusiveSetIDs,
                    std::vector<UCD::UnicodeSet> & multiplexedCCs);

#endif
