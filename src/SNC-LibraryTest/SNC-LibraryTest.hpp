// SNC-LibraryTest.hpp - function definitions for SNC-Library test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _TEST_HPP
#define _TEST_HPP

#include <cmath>
#include "../json/json.h"
#include "../SNC-Library/ProcessedTrace.hpp"

using namespace std;

inline bool approxEqual(double x, double y)
{
    const double epsilon = 1e-10;
    return (x == y) ? true : ((abs(x - y) / max(abs(x), abs(y))) <= epsilon);
}

void searchTest();
void TraceReaderTest();
void NetworkEstimatorTest();
void setEstimatorInfoProcessedTraceTest(Json::Value& estimatorInfo);
void ProcessedTraceTest();
void MGFTest();
void SNCOperatorsTest();
void priorityAlgoBySLOTest();
void NCTest();
void SNCTest();

#endif // _TEST_HPP
