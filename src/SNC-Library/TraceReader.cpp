// TraceReader.cpp - Code for reading trace files.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include "TraceReader.hpp"

using namespace std;

TraceReader::TraceReader(string filename)
    : _curIndex(0)
{
    unsigned long long timestamp; // in nanoseconds
    unsigned long requestSize; // in bytes
    char isGet[32];
    ifstream file(filename.c_str());
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            // Parse line
            if (sscanf(line.c_str(), "%llu,%lx,%s", &timestamp, &requestSize, isGet) == 3) {
                // Store results
                TraceEntry entry;
                entry.arrivalTime = timestamp;
                entry.requestSize = requestSize;
                entry.isGet = (strcmp(isGet, "Get") == 0);
                _trace.push_back(entry);
            }
        }
        file.close();
    } else {
        cerr << "Unable to open " << filename << endl;
    }
}

TraceReader::~TraceReader()
{
}

bool TraceReader::nextEntry(TraceEntry& entry)
{
    if (_curIndex < _trace.size()) {
        entry = _trace[_curIndex++];
        return true;
    }
    return false;
}

void TraceReader::reset()
{
    _curIndex = 0;
}
