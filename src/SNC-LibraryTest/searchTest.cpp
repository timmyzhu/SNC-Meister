// searchTest.cpp - search test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "../SNC-Library/search.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

double f1(double x, void* params)
{
    return exp(x / 1000.0);
}

double f2(double x, void* params)
{
    return 1.0 / (x / 1000.0);
}

double f3(double x, void* params)
{
    return sin(x);
}

void searchTest()
{
    assert(binarySearch(0.0, 2048.0, 1.0, 2.0, f1, NULL) == 1.0);
    assert(binarySearch(0.0, 2048.0, 2.0, 2.0, f1, NULL) == 693.0);
    assert(binarySearch(0.0, 2048.0, 7.0, 2.0, f1, NULL) == 1945.0);
    assert(binarySearchReverse(0.0, 2048.0, 0.50001, 2.0, f2, NULL) == 1999.0);
    assert(binarySearchReverse(0.0, 2048.0, 1.00001, 2.0, f2, NULL) == 999.0);
    assert(binarySearchReverse(0.0, 2048.0, 2.00001, 2.0, f2, NULL) == 499.0);
    assert(binarySearchReverse(0.0, 2048.0, 3.00001, 2.0, f2, NULL) == 333.0);
    assert(binarySearchReverse(0.0, 2048.0, 10.00001, 2.0, f2, NULL) == 99.0);
    assert(minSearch(0.0, 1.0, 1.0, f3, NULL) == 0.0);
    assert(minSearch(0.0, 10.0, 1.0, f3, NULL) == 5.0);
    assert(minSearch(0.0, 100.0, 1.0, f3, NULL) == 11.0);
    cout << "PASS searchTest" << endl;
}
