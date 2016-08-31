// search.hpp - Helper functions for searching over a real-valued function.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _SEARCH_HPP
#define _SEARCH_HPP

// Defines a real-valued function f(x) that we want to search over. params allows for user-defined private data.
typedef double (*EvalFunction)(double x, void* params);

// Assuming f is an increasing function, performs a binary search to find x in the range (min, max) such that f(x) == targetVal. Search stops when max - min <= stopThreshold.
double binarySearch(double min, double max, double targetVal, double stopThreshold, EvalFunction f, void* params);
// Assuming f is an decreasing function, performs a binary search to find x in the range (min, max) such that f(x) == targetVal. Search stops when max - min <= stopThreshold.
double binarySearchReverse(double min, double max, double targetVal, double stopThreshold, EvalFunction f, void* params);
// Searches for x in the range (min, max) with the minimum f(x) value. Search performs brute-force evaluations of f in increments of stepSize.
double minSearch(double min, double max, double stepSize, EvalFunction f, void* params);

#endif // _SEARCH_HPP
