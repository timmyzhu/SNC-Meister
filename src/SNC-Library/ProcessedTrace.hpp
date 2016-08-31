// ProcessedTrace.hpp - Class definitions representing a trace that has been processed with an estimator.
// Uses the given estimator to convert request sizes in a trace into generic "work" units (see Estimator.hpp for details).
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _PROCESSED_TRACE_HPP
#define _PROCESSED_TRACE_HPP

#include <string>
#include <stdint.h>
#include "Estimator.hpp"
#include "TraceReader.hpp"

using namespace std;

struct ProcessedTraceEntry {
    uint64_t arrivalTime; // in nanoseconds
    double work; // in "work" units, as defined by the estimator
    bool isGet; // true if get request, false if put request
};

// Reads requests from trace file with TraceReader and converts each request's request size into work using the given estimator.
// ProcessedTrace is not thread-safe.
class ProcessedTrace
{
private:
    TraceReader _traceReader;
    Estimator* _pEst;

public:
    ProcessedTrace(string filename, Estimator* pEst);
    virtual ~ProcessedTrace();

    // Fills entry with the next request from the trace. Returns false if end of trace.
    virtual bool nextEntry(ProcessedTraceEntry& entry);
    // Resets trace reader back to beginning of trace.
    virtual void reset();
};

#endif // _PROCESSED_TRACE_HPP
