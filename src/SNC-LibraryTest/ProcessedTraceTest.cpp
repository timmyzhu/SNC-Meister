// ProcessedTraceTest.cpp - ProcessedTrace test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include "../json/json.h"
#include "../SNC-Library/Estimator.hpp"
#include "../SNC-Library/ProcessedTrace.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

void setEstimatorInfoProcessedTraceTest(Json::Value& estimatorInfo)
{
    estimatorInfo["name"] = Json::Value("testEstimator");
    estimatorInfo["type"] = Json::Value("networkIn");
    estimatorInfo["nonDataConstant"] = Json::Value(1024);
    estimatorInfo["nonDataFactor"] = Json::Value(0.5);
    estimatorInfo["dataConstant"] = Json::Value(512);
    estimatorInfo["dataFactor"] = Json::Value(2.0);
}

void ProcessedTraceTest(ProcessedTrace* pTrace)
{
    ProcessedTraceEntry entry;
    for (int i = 0; i < 3; i++) {
        assert(pTrace->nextEntry(entry) == true);
        assert(entry.arrivalTime == 0);
        assert(entry.work == 1536);
        assert(entry.isGet == true);
        assert(pTrace->nextEntry(entry) == true);
        assert(entry.arrivalTime == 1000);
        assert(entry.work == 512);
        assert(entry.isGet == false);
        assert(pTrace->nextEntry(entry) == true);
        assert(entry.arrivalTime == 10000);
        assert(entry.work == 3072);
        assert(entry.isGet == true);
        assert(pTrace->nextEntry(entry) == true);
        assert(entry.arrivalTime == 20000);
        assert(entry.work == 1536);
        assert(entry.isGet == false);
        assert(pTrace->nextEntry(entry) == false);
        pTrace->reset();
    }
}

void ProcessedTraceTest()
{
    Json::Value estimatorInfo;
    setEstimatorInfoProcessedTraceTest(estimatorInfo);
    Estimator* pEst = Estimator::Create(estimatorInfo);
    ProcessedTrace processedTrace("testTrace.txt", pEst);
    ProcessedTraceTest(&processedTrace);
    cout << "PASS ProcessedTraceTest" << endl;
}
