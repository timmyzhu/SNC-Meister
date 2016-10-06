// MGF.cpp - Code for calculating moment generating functions (MGFs).
// See MGF.hpp for details.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cmath>
#include <limits>
#include <vector>
#include "ProcessedTrace.hpp"
#include "MGF.hpp"

using namespace std;

void MGF::setProbRequest(double probRequest)
{
    _p = probRequest;
}

MGF* MGF::create(const Json::Value& json)
{
    MGF* mgf = NULL;
    if (json.isMember("type")) {
        string type = json["type"].asString();
        if (type == "MGFDeterministic") {
            mgf = MGFDeterministic::create(json);
        } else if (type == "MGFExponential") {
            mgf = MGFExponential::create(json);
        } else if (type == "MGFHyperexponential") {
            mgf = MGFHyperexponential::create(json);
        } else if (type == "MGFHyperexponentialGetPut") {
            mgf = MGFHyperexponentialGetPut::create(json);
        } else if (type == "MGFTrace") {
            mgf = MGFTrace::create(json);
        } else {
            cerr << "Invalid serialization data: Invalid MGF type " << type << endl;
        }
    } else {
        cerr << "Invalid serialization data: Missing MGF type" << endl;
    }
    return mgf;
}

MGFDeterministic::MGFDeterministic()
    : _totalSize(0),
      _count(0),
      _meanSize(0)
{
}

double MGFDeterministic::calcMGF(double theta) const
{
    double MGFIncrement = exp(_meanSize * theta);
    return _p * MGFIncrement + (1.0 - _p);
}

void MGFDeterministic::addSampleRequest(const ProcessedTraceEntry& traceEntry)
{
    _totalSize += traceEntry.work;
    _count++;
    _meanSize = _totalSize / static_cast<double>(_count);
}

MGFExponential::MGFExponential()
    : _totalSize(0),
      _count(0),
      _lambda(1000000) // use large lambda in case of no sample Requests
{
}

double MGFExponential::calcMGF(double theta) const
{
    if (theta < _lambda) {
        double MGFIncrement = _lambda / (_lambda - theta);
        return _p * MGFIncrement + (1.0 - _p);
    } else {
        return numeric_limits<double>::infinity();
    }
}

void MGFExponential::addSampleRequest(const ProcessedTraceEntry& traceEntry)
{
    _totalSize += traceEntry.work;
    _count++;
    _lambda = static_cast<double>(_count) / _totalSize;
}

MGFHyperexponential::MGFHyperexponential()
    : _meanSize(0),
      _varSumSize(0),
      _count(0),
      _lambda1(1000000), // use large lambda in case of no sample Requests
      _lambda2(1000000), // use large lambda in case of no sample Requests
      _prob1(0)
{
}

double MGFHyperexponential::calcMGF(double theta) const
{
    if ((theta < _lambda1) && (theta < _lambda2)) {
        double MGFIncrement = (_prob1 * _lambda1 / (_lambda1 - theta)) + ((1.0 - _prob1) * _lambda2 / (_lambda2 - theta));
        return _p * MGFIncrement + (1.0 - _p);
    } else {
        return numeric_limits<double>::infinity();
    }
}

void MGFHyperexponential::addSampleRequest(const ProcessedTraceEntry& traceEntry)
{
    _count++;
    _varSumSize += static_cast<double>(_count - 1) * (traceEntry.work - _meanSize) * (traceEntry.work - _meanSize) / static_cast<double>(_count);
    _meanSize += (traceEntry.work - _meanSize) / static_cast<double>(_count);
    double lambdaAvg = 1.0 / _meanSize;
    double sampleVar = _varSumSize / static_cast<double>(_count - 1);
    double C2 = sampleVar / (_meanSize * _meanSize);
    if (isfinite(C2) && (C2 >= 1.0)) {
        _lambda1 = lambdaAvg / C2;
        _lambda2 = lambdaAvg * 2.0;
        _prob1 = 1.0 / (2.0 * C2 - 1.0);
    } else {
        _lambda1 = lambdaAvg;
        _lambda2 = lambdaAvg;
        _prob1 = 1.0;
    }
}

MGFHyperexponentialGetPut::MGFHyperexponentialGetPut()
    : _getSize(0),
      _putSize(0),
      _getCount(0),
      _putCount(0)
{
}

void MGFHyperexponentialGetPut::addSampleRequest(const ProcessedTraceEntry& traceEntry)
{
    if (traceEntry.isGet) {
        _getSize += traceEntry.work;
        _getCount++;
        _lambda1 = static_cast<double>(_getCount) / _getSize;
    } else {
        _putSize += traceEntry.work;
        _putCount++;
        _lambda2 = static_cast<double>(_putCount) / _putSize;
    }
    _prob1 = static_cast<double>(_getCount) / static_cast<double>(_getCount + _putCount);
}

MGFTrace::MGFTrace()
{
}

double MGFTrace::calcMGF(double theta) const
{
    if (_sizes.empty()) {
        return 1;
    }
    double MGFIncrement;
    // Memoization
    if (_MGFTable.find(theta) != _MGFTable.end()) {
        MGFIncrement = _MGFTable[theta];
    } else {
        double sum = 0;
        for (vector<double>::const_iterator it = _sizes.begin(); it != _sizes.end(); it++) {
            sum += exp(*it * theta);
        }
        MGFIncrement = sum / static_cast<double>(_sizes.size());
        _MGFTable[theta] = MGFIncrement;
    }
    return _p * MGFIncrement + (1.0 - _p);
}

void MGFTrace::addSampleRequest(const ProcessedTraceEntry& traceEntry)
{
    _sizes.push_back(traceEntry.work);
    _MGFTable.clear();
}
