// search.cpp - Helper functions for searching over a real-valued function.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <limits>
#include "search.hpp"

using namespace std;

// Assuming f is an increasing function, performs a binary search to find x in the range (min, max) such that f(x) == targetVal. Search stops when max - min <= stopThreshold.
double binarySearch(double min, double max, double targetVal, double stopThreshold, EvalFunction f, void* params)
{
    double mid;
    while ((max - min) > stopThreshold) {
        mid = min + ((max - min) / 2);
        if (f(mid, params) < targetVal) {
            min = mid;
        } else {
            max = mid;
        }
    }
    mid = min + ((max - min) / 2);
    return mid;
}

// Assuming f is an decreasing function, performs a binary search to find x in the range (min, max) such that f(x) == targetVal. Search stops when max - min <= stopThreshold.
double binarySearchReverse(double min, double max, double targetVal, double stopThreshold, EvalFunction f, void* params)
{
    double mid;
    while ((max - min) > stopThreshold) {
        mid = min + ((max - min) / 2);
        if (f(mid, params) > targetVal) {
            min = mid;
        } else {
            max = mid;
        }
    }
    mid = min + ((max - min) / 2);
    return mid;
}

// Searches for x in the range (min, max) with the minimum f(x) value. Search performs brute-force evaluations of f in increments of stepSize.
double minSearch(double min, double max, double stepSize, EvalFunction f, void* params)
{
    double result = min;
    double minVal = numeric_limits<double>::infinity();
    for (double x = min; x <= max; x += stepSize) {
        double val = f(x, params);
        if (val < minVal) {
            minVal = val;
            result = x;
        }
    }
    return result;
}
