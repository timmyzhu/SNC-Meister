// SNC.hpp - Class definitions for the stochastic network calculus (SNC) algorithms.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _SNC_HPP
#define _SNC_HPP

#include <vector>
#include "../json/json.h"
#include "NC.hpp"
#include "SNCOperators.hpp"

using namespace std;

// Extends the Flow structure with SNC-specific information.
struct SNCFlow : Flow {
    virtual ~SNCFlow()
    {
        delete arrival;
    }

    SNCArrival* arrival; // SNC arrival process for flow
    double epsilon; // flow's epsilon for LatencyBound
};

// Extends the Queue structure with SNC-specific information.
struct SNCQueue : Queue {
    SNCService* leftoverService; // Leftover service process for hopByHopAnalysis and convolutionAnalysis
};

enum SNCAlgorithm {
    SNC_ALGORITHM_AGGREGATE,
    SNC_ALGORITHM_HOP_BY_HOP,
    SNC_ALGORITHM_CONVOLUTION,
};

// SNC algorithms for calculating latency.
class SNC : public NC
{
private:
    SNCAlgorithm _algorithm;

    // Helper function to aggregates arrival processes in arrivals while minimizing the number of dependent SNC operators.
    SNCArrival* aggregateArrivals(const vector<SNCArrival*>& arrivals, vector<DependencyParams*>& toDelete) const;

    // New SNC algorithm that attempts to avoid introducing unnecessary dependencies.
    // See SNC-Meister paper for details.
    // Currently supported for flows with two queues, as is the case when modeling end-host network links.
    void aggregateAnalysisTwoHopDep(SNCFlow* flow);
    // SNC algorithm that analyzes a flow's latency by considering each queue (a.k.a., "hop") one at a time.
    void hopByHopAnalysis(SNCFlow* flow);
    // SNC algorithm that analyzes a flow's latency by:
    // 1) computing each queue's leftover service process
    // 2) convoluting the leftover service process across the flow's queues
    // 3) computing the flow's latency using the convoluted service process
    void convolutionAnalysis(SNCFlow* flow);

protected:
    virtual FlowId initFlow(Flow* f, const Json::Value& flowInfo, ClientId clientId);
    virtual QueueId initQueue(Queue* q, const Json::Value& queueInfo);

    SNCFlow* getSNCFlow(FlowId flowId) { return static_cast<SNCFlow*>(const_cast<Flow*>(getFlow(flowId))); }
    SNCQueue* getSNCQueue(QueueId queueId) { return static_cast<SNCQueue*>(const_cast<Queue*>(getQueue(queueId))); }

public:
    SNC(SNCAlgorithm algorithm = SNC_ALGORITHM_AGGREGATE)
        : _algorithm(algorithm)
    {}
    virtual ~SNC()
    {}

    virtual double calcFlowLatency(FlowId flowId);

    virtual void addDependency(ClientId clientId1, ClientId clientId2);

    static void setArrivalInfo(Json::Value& flowInfo, string trace, const Json::Value& estimatorInfo);
};

#endif // _SNC_HPP
