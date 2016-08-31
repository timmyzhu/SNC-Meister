// MGFTest.cpp - MGF test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <cmath>
#include <iostream>
#include "../json/json.h"
#include "../SNC-Library/MGF.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

void MGFDeterministicTest()
{
    MGFDeterministic X;
    ProcessedTraceEntry traceEntry;
    traceEntry.work = 3;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.5), exp(3.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), exp(3.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), exp(3.0 * 2.0)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.5), 1.0));
    assert(approxEqual(X.calcMGF(1.0), 1.0));
    assert(approxEqual(X.calcMGF(2.0), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.5), 0.5 + 0.5 * exp(3.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), 0.5 + 0.5 * exp(3.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), 0.5 + 0.5 * exp(3.0 * 2.0)));
    traceEntry.work = 1;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.5), exp(2.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), exp(2.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), exp(2.0 * 2.0)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.5), 1.0));
    assert(approxEqual(X.calcMGF(1.0), 1.0));
    assert(approxEqual(X.calcMGF(2.0), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.5), 0.5 + 0.5 * exp(2.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), 0.5 + 0.5 * exp(2.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), 0.5 + 0.5 * exp(2.0 * 2.0)));
    traceEntry.work = 8;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.5), exp(4.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), exp(4.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), exp(4.0 * 2.0)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.5), 1.0));
    assert(approxEqual(X.calcMGF(1.0), 1.0));
    assert(approxEqual(X.calcMGF(2.0), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.5), 0.5 + 0.5 * exp(4.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), 0.5 + 0.5 * exp(4.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), 0.5 + 0.5 * exp(4.0 * 2.0)));
    Json::Value data;
    serializeJSON(data, "MGF", &X);
    MGF* Y = NULL;
    deserializeJSON(data, "MGF", Y);
    assert(approxEqual(Y->calcMGF(0.5), 0.5 + 0.5 * exp(4.0 * 0.5)));
    assert(approxEqual(Y->calcMGF(1.0), 0.5 + 0.5 * exp(4.0 * 1.0)));
    assert(approxEqual(Y->calcMGF(2.0), 0.5 + 0.5 * exp(4.0 * 2.0)));
    delete Y;
    cout << "PASS MGFDeterministicTest" << endl;
}

void MGFExponentialTest()
{
    MGFExponential X;
    ProcessedTraceEntry traceEntry;
    traceEntry.work = 3;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 1.0 / (1.0 - 3.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 1.0 / (1.0 - 3.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 1.0 / (1.0 - 3.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.5 / (1.0 - 3.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.5 / (1.0 - 3.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.5 / (1.0 - 3.0 * 0.2)));
    traceEntry.work = 1;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 1.0 / (1.0 - 2.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 1.0 / (1.0 - 2.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 1.0 / (1.0 - 2.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.5 / (1.0 - 2.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.5 / (1.0 - 2.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.5 / (1.0 - 2.0 * 0.2)));
    traceEntry.work = 8;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 1.0 / (1.0 - 4.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 1.0 / (1.0 - 4.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 1.0 / (1.0 - 4.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.5 / (1.0 - 4.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.5 / (1.0 - 4.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.5 / (1.0 - 4.0 * 0.2)));
    Json::Value data;
    serializeJSON(data, "MGF", &X);
    MGF* Y = NULL;
    deserializeJSON(data, "MGF", Y);
    assert(approxEqual(Y->calcMGF(0.05), 0.5 + 0.5 / (1.0 - 4.0 * 0.05)));
    assert(approxEqual(Y->calcMGF(0.1), 0.5 + 0.5 / (1.0 - 4.0 * 0.1)));
    assert(approxEqual(Y->calcMGF(0.2), 0.5 + 0.5 / (1.0 - 4.0 * 0.2)));
    delete Y;
    cout << "PASS MGFExponentialTest" << endl;
}

void MGFHyperexponentialTest()
{
    MGFHyperexponential X;
    ProcessedTraceEntry traceEntry;
    traceEntry.work = 3;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 1.0 / (1.0 - 3.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 1.0 / (1.0 - 3.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 1.0 / (1.0 - 3.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.5 / (1.0 - 3.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.5 / (1.0 - 3.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.5 / (1.0 - 3.0 * 0.2)));
    traceEntry.work = 1;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 1.0 / (1.0 - 2.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 1.0 / (1.0 - 2.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 1.0 / (1.0 - 2.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.5 / (1.0 - 2.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.5 / (1.0 - 2.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.5 / (1.0 - 2.0 * 0.2)));
    traceEntry.work = 11;
    X.addSampleRequest(traceEntry);
    double p = 1.0 / (2.0 * 1.12 - 1.0);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.01), p / (1.0 - 5.6 * 0.01) + (1.0 - p) / (1.0 - 2.5 * 0.01)));
    assert(approxEqual(X.calcMGF(0.02), p / (1.0 - 5.6 * 0.02) + (1.0 - p) / (1.0 - 2.5 * 0.02)));
    assert(approxEqual(X.calcMGF(0.05), p / (1.0 - 5.6 * 0.05) + (1.0 - p) / (1.0 - 2.5 * 0.05)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.01), 1.0));
    assert(approxEqual(X.calcMGF(0.02), 1.0));
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.01), 0.5 + 0.5 * p / (1.0 - 5.6 * 0.01) + 0.5 * (1.0 - p) / (1.0 - 2.5 * 0.01)));
    assert(approxEqual(X.calcMGF(0.02), 0.5 + 0.5 * p / (1.0 - 5.6 * 0.02) + 0.5 * (1.0 - p) / (1.0 - 2.5 * 0.02)));
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.5 * p / (1.0 - 5.6 * 0.05) + 0.5 * (1.0 - p) / (1.0 - 2.5 * 0.05)));
    Json::Value data;
    serializeJSON(data, "MGF", &X);
    MGF* Y = NULL;
    deserializeJSON(data, "MGF", Y);
    assert(approxEqual(Y->calcMGF(0.01), 0.5 + 0.5 * p / (1.0 - 5.6 * 0.01) + 0.5 * (1.0 - p) / (1.0 - 2.5 * 0.01)));
    assert(approxEqual(Y->calcMGF(0.02), 0.5 + 0.5 * p / (1.0 - 5.6 * 0.02) + 0.5 * (1.0 - p) / (1.0 - 2.5 * 0.02)));
    assert(approxEqual(Y->calcMGF(0.05), 0.5 + 0.5 * p / (1.0 - 5.6 * 0.05) + 0.5 * (1.0 - p) / (1.0 - 2.5 * 0.05)));
    delete Y;
    cout << "PASS MGFHyperexponentialTest" << endl;
}

void MGFHyperexponentialGetPutTest()
{
    MGFHyperexponentialGetPut X;
    ProcessedTraceEntry traceEntryGet;
    ProcessedTraceEntry traceEntryPut;
    traceEntryGet.isGet = true;
    traceEntryPut.isGet = false;
    traceEntryGet.work = 3;
    traceEntryPut.work = 1;
    X.addSampleRequest(traceEntryGet);
    X.addSampleRequest(traceEntryPut);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 0.5 / (1.0 - 3.0 * 0.05) + 0.5 / (1.0 - 1.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 / (1.0 - 3.0 * 0.1) + 0.5 / (1.0 - 1.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 / (1.0 - 3.0 * 0.2) + 0.5 / (1.0 - 1.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.25 / (1.0 - 3.0 * 0.05) + 0.25 / (1.0 - 1.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.25 / (1.0 - 3.0 * 0.1) + 0.25 / (1.0 - 1.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.25 / (1.0 - 3.0 * 0.2) + 0.25 / (1.0 - 1.0 * 0.2)));
    traceEntryGet.work = 1;
    traceEntryPut.work = 5;
    X.addSampleRequest(traceEntryGet);
    X.addSampleRequest(traceEntryPut);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 0.5 / (1.0 - 2.0 * 0.05) + 0.5 / (1.0 - 3.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 / (1.0 - 2.0 * 0.1) + 0.5 / (1.0 - 3.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 / (1.0 - 2.0 * 0.2) + 0.5 / (1.0 - 3.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.25 / (1.0 - 2.0 * 0.05) + 0.25 / (1.0 - 3.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.25 / (1.0 - 2.0 * 0.1) + 0.25 / (1.0 - 3.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.25 / (1.0 - 2.0 * 0.2) + 0.25 / (1.0 - 3.0 * 0.2)));
    traceEntryGet.work = 8;
    traceEntryPut.work = 0;
    X.addSampleRequest(traceEntryGet);
    X.addSampleRequest(traceEntryPut);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.05), 0.5 / (1.0 - 4.0 * 0.05) + 0.5 / (1.0 - 2.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 / (1.0 - 4.0 * 0.1) + 0.5 / (1.0 - 2.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 / (1.0 - 4.0 * 0.2) + 0.5 / (1.0 - 2.0 * 0.2)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.05), 1.0));
    assert(approxEqual(X.calcMGF(0.1), 1.0));
    assert(approxEqual(X.calcMGF(0.2), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.05), 0.5 + 0.25 / (1.0 - 4.0 * 0.05) + 0.25 / (1.0 - 2.0 * 0.05)));
    assert(approxEqual(X.calcMGF(0.1), 0.5 + 0.25 / (1.0 - 4.0 * 0.1) + 0.25 / (1.0 - 2.0 * 0.1)));
    assert(approxEqual(X.calcMGF(0.2), 0.5 + 0.25 / (1.0 - 4.0 * 0.2) + 0.25 / (1.0 - 2.0 * 0.2)));
    Json::Value data;
    serializeJSON(data, "MGF", &X);
    MGF* Y = NULL;
    deserializeJSON(data, "MGF", Y);
    assert(approxEqual(Y->calcMGF(0.05), 0.5 + 0.25 / (1.0 - 4.0 * 0.05) + 0.25 / (1.0 - 2.0 * 0.05)));
    assert(approxEqual(Y->calcMGF(0.1), 0.5 + 0.25 / (1.0 - 4.0 * 0.1) + 0.25 / (1.0 - 2.0 * 0.1)));
    assert(approxEqual(Y->calcMGF(0.2), 0.5 + 0.25 / (1.0 - 4.0 * 0.2) + 0.25 / (1.0 - 2.0 * 0.2)));
    delete Y;
    cout << "PASS MGFHyperexponentialGetPutTest" << endl;
}

void MGFTraceTest()
{
    MGFTrace X;
    ProcessedTraceEntry traceEntry;
    traceEntry.work = 3;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.5), exp(3.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), exp(3.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), exp(3.0 * 2.0)));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.5), 1.0));
    assert(approxEqual(X.calcMGF(1.0), 1.0));
    assert(approxEqual(X.calcMGF(2.0), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.5), 0.5 + 0.5 * exp(3.0 * 0.5)));
    assert(approxEqual(X.calcMGF(1.0), 0.5 + 0.5 * exp(3.0 * 1.0)));
    assert(approxEqual(X.calcMGF(2.0), 0.5 + 0.5 * exp(3.0 * 2.0)));
    traceEntry.work = 1;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.5), (exp(3.0 * 0.5) + exp(1.0 * 0.5)) / 2.0));
    assert(approxEqual(X.calcMGF(1.0), (exp(3.0 * 1.0) + exp(1.0 * 1.0)) / 2.0));
    assert(approxEqual(X.calcMGF(2.0), (exp(3.0 * 2.0) + exp(1.0 * 2.0)) / 2.0));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.5), 1.0));
    assert(approxEqual(X.calcMGF(1.0), 1.0));
    assert(approxEqual(X.calcMGF(2.0), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.5), 0.5 + 0.5 * (exp(3.0 * 0.5) + exp(1.0 * 0.5)) / 2.0));
    assert(approxEqual(X.calcMGF(1.0), 0.5 + 0.5 * (exp(3.0 * 1.0) + exp(1.0 * 1.0)) / 2.0));
    assert(approxEqual(X.calcMGF(2.0), 0.5 + 0.5 * (exp(3.0 * 2.0) + exp(1.0 * 2.0)) / 2.0));
    traceEntry.work = 8;
    X.addSampleRequest(traceEntry);
    X.setProbRequest(1);
    assert(approxEqual(X.calcMGF(0.5), (exp(3.0 * 0.5) + exp(1.0 * 0.5) + exp(8.0 * 0.5)) / 3.0));
    assert(approxEqual(X.calcMGF(1.0), (exp(3.0 * 1.0) + exp(1.0 * 1.0) + exp(8.0 * 1.0)) / 3.0));
    assert(approxEqual(X.calcMGF(2.0), (exp(3.0 * 2.0) + exp(1.0 * 2.0) + exp(8.0 * 2.0)) / 3.0));
    X.setProbRequest(0);
    assert(approxEqual(X.calcMGF(0.5), 1.0));
    assert(approxEqual(X.calcMGF(1.0), 1.0));
    assert(approxEqual(X.calcMGF(2.0), 1.0));
    X.setProbRequest(0.5);
    assert(approxEqual(X.calcMGF(0.5), 0.5 + 0.5 * (exp(3.0 * 0.5) + exp(1.0 * 0.5) + exp(8.0 * 0.5)) / 3.0));
    assert(approxEqual(X.calcMGF(1.0), 0.5 + 0.5 * (exp(3.0 * 1.0) + exp(1.0 * 1.0) + exp(8.0 * 1.0)) / 3.0));
    assert(approxEqual(X.calcMGF(2.0), 0.5 + 0.5 * (exp(3.0 * 2.0) + exp(1.0 * 2.0) + exp(8.0 * 2.0)) / 3.0));
    Json::Value data;
    serializeJSON(data, "MGF", &X);
    MGF* Y = NULL;
    deserializeJSON(data, "MGF", Y);
    assert(approxEqual(Y->calcMGF(0.5), 0.5 + 0.5 * (exp(3.0 * 0.5) + exp(1.0 * 0.5) + exp(8.0 * 0.5)) / 3.0));
    assert(approxEqual(Y->calcMGF(1.0), 0.5 + 0.5 * (exp(3.0 * 1.0) + exp(1.0 * 1.0) + exp(8.0 * 1.0)) / 3.0));
    assert(approxEqual(Y->calcMGF(2.0), 0.5 + 0.5 * (exp(3.0 * 2.0) + exp(1.0 * 2.0) + exp(8.0 * 2.0)) / 3.0));
    delete Y;
    cout << "PASS MGFTraceTest" << endl;
}

void MGFTest()
{
    MGFDeterministicTest();
    MGFExponentialTest();
    MGFHyperexponentialTest();
    MGFHyperexponentialGetPutTest();
    MGFTraceTest();
}
