// TraceReader.hpp - Class definitions for reading trace files.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _TRACE_READER_HPP
#define _TRACE_READER_HPP

#include <stdint.h>
#include <string>
#include <vector>

using namespace std;

struct TraceEntry {
    uint64_t arrivalTime; // in nanoseconds
    uint32_t requestSize; // in bytes
    bool isGet; // true if get request, false if put request
};

// Reads and stores requests from trace file on construction.
// Trace file must be in CSV format with one request per line. Each line contains 3 columns:
// 1) (decimal) arrival time of request in nanoseconds
// 2) (hex) number of bytes in request
// 3) (string) "Get" or "Put"
// TraceReader is not thread-safe.
class TraceReader
{
private:
    vector<TraceEntry> _trace;
    unsigned int _curIndex;

public:
    TraceReader(string filename);
    virtual ~TraceReader();

    // Fills entry with the next request from the trace. Returns false if end of trace.
    virtual bool nextEntry(TraceEntry& entry);
    // Resets trace reader back to beginning of trace.
    virtual void reset();
};

#endif // _TRACE_READER_HPP
