// NetworkEstimator.cpp - Code for estimating network traffic based on request sizes.
// See Estimator.hpp for details.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include "Estimator.hpp"

double NetworkInEstimator::estimateWork(int requestSize, bool isGetRequest)
{
    if (isGetRequest) {
        return _nonDataConstant + _nonDataFactor * (double)requestSize;
    } else {
        return _dataConstant + _dataFactor * (double)requestSize;
    }
}

double NetworkOutEstimator::estimateWork(int requestSize, bool isGetRequest)
{
    if (!isGetRequest) {
        return _nonDataConstant + _nonDataFactor * (double)requestSize;
    } else {
        return _dataConstant + _dataFactor * (double)requestSize;
    }
}
