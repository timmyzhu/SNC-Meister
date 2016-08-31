// MGF.hpp - Class definitions for moment generating functions (MGFs).
// MGFs are functions that encode full information about a distribution and provide a way to analytically work with distributions.
// MGFs are parameterized by one parameter theta and are mathematically defined as MGF_X(theta) = E[exp(theta * X)] for a random variable X.
// Here, we use MGFs to represent distributions on the amount of work that arrives over time, which is affected by both the arrival rate and request size.
// setProbRequest is used to configure the probability of generating a request in a timestep, which is based on the arrival rate of requests.
// The request size distribution parameters are estimated by repeatedly calling addSampleRequest with empirical data.
// After configuring the MGFs, calcMGF is used to calculate the MGF at a given theta value.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _MGF_HPP
#define _MGF_HPP

#include <map>
#include <vector>
#include <iostream>
#include "ProcessedTrace.hpp"
#include "serializeJSON.hpp"
#include "../json/json.h"

using namespace std;

// Base class for a MGF.
class MGF : public Serializable
{
protected:
    // Probability of generating a request in a timestep.
    double _p;

public:
    virtual ~MGF()
    {}

    // Calculate the MGF at the given theta value.
    virtual double calcMGF(double theta) const = 0;
    // Update MGF-specific parameters for the request size distribution based on the given empirical data.
    // Each call to this function will increase the amount of empirical data used to estimate the MGF parameters.
    virtual void addSampleRequest(const ProcessedTraceEntry& traceEntry) = 0;
    // Set the probability, p, of generating a request in a timestep.
    // That is, the MGF represents generating a request with probability p, and generating a 0-sized request with probability 1-p.
    // Mathematically, if MGFIncrement is the MGF of the request size distribution, then the overall MGF is p * MGFIncrement + (1-p).
    void setProbRequest(double probRequest);

    // Create a new MGF based on serialized JSON object.
    static MGF* create(Json::Value json);
    virtual void serialize(Json::Value& json) const
    {
        serializeJSON(json, "p", _p);
    }
    virtual void deserialize(const Json::Value& json)
    {
        deserializeJSON(json, "p", _p);
    }
};

// MGF for request sizes following a deterministic distribution.
class MGFDeterministic : public MGF
{
private:
    // Data for estimating distribution
    double _totalSize;
    int _count;

protected:
    // Parameters for generating distribution
    double _meanSize;

public:
    MGFDeterministic();
    virtual ~MGFDeterministic()
    {}

    virtual double calcMGF(double theta) const;
    virtual void addSampleRequest(const ProcessedTraceEntry& traceEntry);

    // Create a new MGF based on serialized JSON object.
    static MGFDeterministic* create(Json::Value json)
    {
        MGFDeterministic* mgf = new MGFDeterministic();
        mgf->deserialize(json);
        return mgf;
    }
    virtual void serialize(Json::Value& json) const
    {
        MGF::serialize(json);
        serializeJSON(json, "type", "MGFDeterministic");
        serializeJSON(json, "totalSize", _totalSize);
        serializeJSON(json, "count", _count);
        serializeJSON(json, "meanSize", _meanSize);
    }
    virtual void deserialize(const Json::Value& json)
    {
        MGF::deserialize(json);
        deserializeJSON(json, "totalSize", _totalSize);
        deserializeJSON(json, "count", _count);
        deserializeJSON(json, "meanSize", _meanSize);
    }
};

// MGF for request sizes following a exponential distribution.
class MGFExponential : public MGF
{
private:
    // Data for estimating distribution
    double _totalSize;
    int _count;

protected:
    // Parameters for generating distribution
    double _lambda;

public:
    MGFExponential();
    virtual ~MGFExponential()
    {}

    virtual double calcMGF(double theta) const;
    virtual void addSampleRequest(const ProcessedTraceEntry& traceEntry);

    // Create a new MGF based on serialized JSON object.
    static MGFExponential* create(Json::Value json)
    {
        MGFExponential* mgf = new MGFExponential();
        mgf->deserialize(json);
        return mgf;
    }
    virtual void serialize(Json::Value& json) const
    {
        MGF::serialize(json);
        serializeJSON(json, "type", "MGFExponential");
        serializeJSON(json, "totalSize", _totalSize);
        serializeJSON(json, "count", _count);
        serializeJSON(json, "lambda", _lambda);
    }
    virtual void deserialize(const Json::Value& json)
    {
        MGF::deserialize(json);
        deserializeJSON(json, "totalSize", _totalSize);
        deserializeJSON(json, "count", _count);
        deserializeJSON(json, "lambda", _lambda);
    }
};

// MGF for request sizes following a 2-phase hyperexponential distribution, where we fit the first and second moments.
class MGFHyperexponential : public MGF
{
private:
    // Data for estimating distribution
    double _meanSize;
    double _varSumSize; // used for computing variance
    int _count;

protected:
    // Parameters for generating distribution
    double _lambda1;
    double _lambda2;
    double _prob1; // probability of using lambda1

public:
    MGFHyperexponential();
    virtual ~MGFHyperexponential()
    {}

    virtual double calcMGF(double theta) const;
    virtual void addSampleRequest(const ProcessedTraceEntry& traceEntry);

    // Create a new MGF based on serialized JSON object.
    static MGFHyperexponential* create(Json::Value json)
    {
        MGFHyperexponential* mgf = new MGFHyperexponential();
        mgf->deserialize(json);
        return mgf;
    }
    virtual void serialize(Json::Value& json) const
    {
        MGF::serialize(json);
        serializeJSON(json, "type", "MGFHyperexponential");
        serializeJSON(json, "meanSize", _meanSize);
        serializeJSON(json, "varSumSize", _varSumSize);
        serializeJSON(json, "count", _count);
        serializeJSON(json, "lambda1", _lambda1);
        serializeJSON(json, "lambda2", _lambda2);
        serializeJSON(json, "prob1", _prob1);
    }
    virtual void deserialize(const Json::Value& json)
    {
        MGF::deserialize(json);
        deserializeJSON(json, "meanSize", _meanSize);
        deserializeJSON(json, "varSumSize", _varSumSize);
        deserializeJSON(json, "count", _count);
        deserializeJSON(json, "lambda1", _lambda1);
        deserializeJSON(json, "lambda2", _lambda2);
        deserializeJSON(json, "prob1", _prob1);
    }
};

// MGF for request sizes following a 2-phase hyperexponential distribution, where the two phases represent get and put requests.
class MGFHyperexponentialGetPut : public MGFHyperexponential
{
private:
    // Data for estimating distribution
    double _getSize;
    double _putSize;
    int _getCount;
    int _putCount;

public:
    MGFHyperexponentialGetPut();
    virtual ~MGFHyperexponentialGetPut()
    {}

    virtual void addSampleRequest(const ProcessedTraceEntry& traceEntry);

    // Create a new MGF based on serialized JSON object.
    static MGFHyperexponentialGetPut* create(Json::Value json)
    {
        MGFHyperexponentialGetPut* mgf = new MGFHyperexponentialGetPut();
        mgf->deserialize(json);
        return mgf;
    }
    virtual void serialize(Json::Value& json) const
    {
        MGFHyperexponential::serialize(json);
        serializeJSON(json, "type", "MGFHyperexponentialGetPut");
        serializeJSON(json, "getSize", _getSize);
        serializeJSON(json, "putSize", _putSize);
        serializeJSON(json, "getCount", _getCount);
        serializeJSON(json, "putCount", _putCount);
    }
    virtual void deserialize(const Json::Value& json)
    {
        MGFHyperexponential::deserialize(json);
        deserializeJSON(json, "getSize", _getSize);
        deserializeJSON(json, "putSize", _putSize);
        deserializeJSON(json, "getCount", _getCount);
        deserializeJSON(json, "putCount", _putCount);
    }
};

// Empirical MGF directly based on request sizes in the trace.
class MGFTrace : public MGF
{
protected:
    // Data for estimating distribution
    vector<double> _sizes;
    mutable map<double, double> _MGFTable; // memoization cache

public:
    MGFTrace();
    virtual ~MGFTrace()
    {}

    virtual double calcMGF(double theta) const;
    virtual void addSampleRequest(const ProcessedTraceEntry& traceEntry);

    // Create a new MGF based on serialized JSON object.
    static MGFTrace* create(Json::Value json)
    {
        MGFTrace* mgf = new MGFTrace();
        mgf->deserialize(json);
        return mgf;
    }
    virtual void serialize(Json::Value& json) const
    {
        MGF::serialize(json);
        serializeJSON(json, "type", "MGFTrace");
        serializeJSON(json, "sizes", _sizes);
    }
    virtual void deserialize(const Json::Value& json)
    {
        MGF::deserialize(json);
        deserializeJSON(json, "sizes", _sizes);
    }
};

#endif // _MGF_HPP
