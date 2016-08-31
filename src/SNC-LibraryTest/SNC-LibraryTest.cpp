// SNC-LibraryTest.cpp - SNC-Library test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <iostream>
#include "SNC-LibraryTest.hpp"

using namespace std;

int main(int argc, char** argv)
{
    searchTest();
    TraceReaderTest();
    NetworkEstimatorTest();
    ProcessedTraceTest();
    MGFTest();
    SNCOperatorsTest();
    priorityAlgoBySLOTest();
    NCTest();
    SNCTest();
    cout << "PASS" << endl;
    return 0;
}
