// ProcessedTrace.cpp - Code for processing a trace with an estimator.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <string>
#include "Estimator.hpp"
#include "TraceReader.hpp"
#include "ProcessedTrace.hpp"

using namespace std;

ProcessedTrace::ProcessedTrace(string filename, Estimator* pEst)
    : _traceReader(filename),
      _pEst(pEst)
{
}

ProcessedTrace::~ProcessedTrace()
{
    delete _pEst;
}

bool ProcessedTrace::nextEntry(ProcessedTraceEntry& entry)
{
    TraceEntry traceEntry;
    if (_traceReader.nextEntry(traceEntry)) {
        entry.arrivalTime = traceEntry.arrivalTime;
        entry.work = _pEst->estimateWork(traceEntry.requestSize, traceEntry.isGet);
        entry.isGet = traceEntry.isGet;
        return true;
    }
    return false;
}

void ProcessedTrace::reset()
{
    _traceReader.reset();
    _pEst->reset();
}
