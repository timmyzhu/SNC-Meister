// NetworkEstimatorTest.cpp - NetworkEstimator test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include "../json/json.h"
#include "../SNC-Library/Estimator.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

void NetworkInEstimatorTest()
{
    Json::Value estimatorInfo;
    estimatorInfo["name"] = Json::Value("testEstimator");
    estimatorInfo["type"] = Json::Value("networkIn");
    estimatorInfo["nonDataConstant"] = Json::Value(1000);
    estimatorInfo["nonDataFactor"] = Json::Value(0.2);
    estimatorInfo["dataConstant"] = Json::Value(2000);
    estimatorInfo["dataFactor"] = Json::Value(0.1);
    Estimator* pEst = Estimator::Create(estimatorInfo);
    assert(pEst->estimateWork(100, true) == 1020);
    assert(pEst->estimateWork(200, true) == 1040);
    assert(pEst->estimateWork(300, true) == 1060);
    assert(pEst->estimateWork(100, false) == 2010);
    assert(pEst->estimateWork(200, false) == 2020);
    assert(pEst->estimateWork(300, false) == 2030);
    delete pEst;
    cout << "PASS NetworkInEstimatorTest" << endl;
}

void NetworkOutEstimatorTest()
{
    Json::Value estimatorInfo;
    estimatorInfo["name"] = Json::Value("testEstimator");
    estimatorInfo["type"] = Json::Value("networkOut");
    estimatorInfo["nonDataConstant"] = Json::Value(1000);
    estimatorInfo["nonDataFactor"] = Json::Value(0.2);
    estimatorInfo["dataConstant"] = Json::Value(2000);
    estimatorInfo["dataFactor"] = Json::Value(0.1);
    Estimator* pEst = Estimator::Create(estimatorInfo);
    assert(pEst->estimateWork(100, true) == 2010);
    assert(pEst->estimateWork(200, true) == 2020);
    assert(pEst->estimateWork(300, true) == 2030);
    assert(pEst->estimateWork(100, false) == 1020);
    assert(pEst->estimateWork(200, false) == 1040);
    assert(pEst->estimateWork(300, false) == 1060);
    delete pEst;
    cout << "PASS NetworkOutEstimatorTest" << endl;
}

void NetworkEstimatorTest()
{
    NetworkInEstimatorTest();
    NetworkOutEstimatorTest();
}
