// TraceReaderTest.cpp - TraceReader test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include "../SNC-Library/TraceReader.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

void TraceReaderTest()
{
    TraceReader traceReader("testTrace.txt");
    TraceEntry entry;
    for (int i = 0; i < 3; i++) {
        assert(traceReader.nextEntry(entry) == true);
        assert(entry.arrivalTime == 0);
        assert(entry.requestSize == 1024);
        assert(entry.isGet == true);
        assert(traceReader.nextEntry(entry) == true);
        assert(entry.arrivalTime == 1000);
        assert(entry.requestSize == 0);
        assert(entry.isGet == false);
        assert(traceReader.nextEntry(entry) == true);
        assert(entry.arrivalTime == 10000);
        assert(entry.requestSize == 4096);
        assert(entry.isGet == true);
        assert(traceReader.nextEntry(entry) == true);
        assert(entry.arrivalTime == 20000);
        assert(entry.requestSize == 512);
        assert(entry.isGet == false);
        assert(traceReader.nextEntry(entry) == false);
        traceReader.reset();
    }
    cout << "PASS TraceReaderTest" << endl;
}
