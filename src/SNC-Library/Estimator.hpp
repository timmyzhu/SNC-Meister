// Estimator.hpp - Class definitions for estimators.
// To avoid handling different request types when analyzing traces, we consolidate all request sizes into a generic form, which we call "work".
// The units of work only need to relate to the bandwidth of the queues. For example, we represent network work in terms of bytes and network queues in terms of bytes per second.
// For other devices such as storage, we may choose to represent work in terms of storage time, in which case the storage queues would have a bandwidth of 1 storage second per second.
// Thus, we can abstract away the details of estimating for different request types into these estimators.
// Creating accurate estimators is an interesting area for future research, particularly for other types of devices and requests.
// Nevertheless, this abstraction allows the rest of the SNC analysis can operate on "work" as estimated by the estimateWork function.
//
// As a concrete example, consider get and put requests in a key-value store. Put requests send a lot of data to the server, but only get a small response.
// On the other hand, get requests have most of the network traffic from the server back to the VM.
// Thus, we have different estimators based on whether we are looking at the flow from the VM to the server or from the server back to the VM.
//
// Estimators are configured based on the estimatorInfo JSON dictionary, which has the following fields:
// "type": string - indicates the type of estimator
// "nonDataConstant": float (network-only) - constant overhead for non-data heavy requests
// "nonDataFactor": float (network-only) - effect of requestSize for non-data heavy requests; expected slightly above 0.0
// "dataConstant": float (network-only) - constant overhead for data heavy requests
// "dataFactor": float (network-only) - effect of requestSize for data heavy requests; expected slightly above 1.0
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _ESTIMATOR_HPP
#define _ESTIMATOR_HPP

#include "../json/json.h"

using namespace std;

enum EstimatorType {
    ESTIMATOR_NETWORK_IN,
    ESTIMATOR_NETWORK_OUT,
};

// Base class for estimators.
class Estimator
{
public:
    Estimator() {}
    virtual ~Estimator() {}

    // Create appropriate estimator as specified in estimatorInfo.
    static Estimator* Create(const Json::Value& estimatorInfo);

    // Estimate work based on request size and type.
    // This is the main function that converts request size into "work" units.
    virtual double estimateWork(int requestSize, bool isGetRequest) = 0;
    // Returns type of estimator.
    virtual EstimatorType estimatorType() = 0;
    // Reset any estimator state, if any.
    virtual void reset() {}
};

// Estimator for network traffic from VM to server.
// Put requests generate a lot of traffic based on request size, whereas get requests are always small.
// For network estimators, "work" units remain in terms of bytes, but we translate the request size based on request type (i.e., get vs put).
// We perform the mapping using an affine function factor * requestSize + constant to represent both the effect of the request size as well as constant overheads.
// We have two sets of parameters for the data heavy requests (i.e., put) vs the non-data heavy requests (i.e., get).
class NetworkInEstimator : public Estimator
{
private:
    double _nonDataConstant;
    double _nonDataFactor;
    double _dataConstant;
    double _dataFactor;

public:
    NetworkInEstimator(const Json::Value& estimatorInfo)
        : _nonDataConstant(estimatorInfo["nonDataConstant"].asDouble()),
          _nonDataFactor(estimatorInfo["nonDataFactor"].asDouble()),
          _dataConstant(estimatorInfo["dataConstant"].asDouble()),
          _dataFactor(estimatorInfo["dataFactor"].asDouble())
    {}
    virtual ~NetworkInEstimator() {}

    double estimateWork(int requestSize, bool isGetRequest);
    EstimatorType estimatorType() { return ESTIMATOR_NETWORK_IN; }
};

// Estimator for network traffic from server back to VM.
// Get requests generate a lot of traffic based on request size, whereas put requests are always small.
// For network estimators, "work" units remain in terms of bytes, but we translate the request size based on request type (i.e., get vs put).
// We perform the mapping using an affine function factor * requestSize + constant to represent both the effect of the request size as well as constant overheads.
// We have two sets of parameters for the data heavy requests (i.e., get) vs the non-data heavy requests (i.e., put).
class NetworkOutEstimator : public Estimator
{
private:
    double _nonDataConstant;
    double _nonDataFactor;
    double _dataConstant;
    double _dataFactor;

public:
    NetworkOutEstimator(const Json::Value& estimatorInfo)
        : _nonDataConstant(estimatorInfo["nonDataConstant"].asDouble()),
          _nonDataFactor(estimatorInfo["nonDataFactor"].asDouble()),
          _dataConstant(estimatorInfo["dataConstant"].asDouble()),
          _dataFactor(estimatorInfo["dataFactor"].asDouble())
    {}
    virtual ~NetworkOutEstimator() {}

    double estimateWork(int requestSize, bool isGetRequest);
    EstimatorType estimatorType() { return ESTIMATOR_NETWORK_OUT; }
};

#endif // _ESTIMATOR_HPP
