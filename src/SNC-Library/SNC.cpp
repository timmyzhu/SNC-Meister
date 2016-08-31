// SNC.cpp - Code for stochastic network calculus (SNC) algorithms.
// See SNC.hpp for details.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include "serializeJSON.hpp"
#include "SNCOperators.hpp"
#include "NC.hpp"
#include "SNC.hpp"
#include "Estimator.hpp"
#include "ProcessedTrace.hpp"

using namespace std;

SNCArrival* SNC::aggregateArrivals(const vector<SNCArrival*>& arrivals, vector<DependencyParams*>& toDelete) const
{
    assert(!arrivals.empty());
    // Group arrivals into sets of independent arrivals
    vector<SNCArrival*> arrivalGroups;
    for (vector<SNCArrival*>::const_iterator it = arrivals.begin(); it != arrivals.end(); it++) {
        SNCArrival* arrival = *it;
        unsigned int group;
        for (group = 0; group < arrivalGroups.size(); group++) {
            if (!arrival->checkDependence(arrivalGroups[group])) {
                arrivalGroups[group] = new AggregateArrival(arrival, arrivalGroups[group]);
                toDelete.push_back(arrivalGroups[group]);
                break;
            }
        }
        if (group == arrivalGroups.size()) {
            arrivalGroups.push_back(arrival);
        }
    }
    // Aggregate groups
    SNCArrival* aggregate = arrivalGroups[0];
    for (unsigned int group = 1; group < arrivalGroups.size(); group++) {
        aggregate = new AggregateArrival(aggregate, arrivalGroups[group]);
        toDelete.push_back(aggregate);
    }
    return aggregate;
}

void SNC::aggregateAnalysisTwoHopDep(SNCFlow* flow)
{
    assert(flow->queueIds.size() <= 2);
    vector<DependencyParams*> toDelete;
    if (flow->queueIds.size() == 1) {
        //
        // One hop
        //
        // Calculate leftover service from higher priority flows and arrival of flow at first hop
        QueueId firstQueueId = flow->queueIds[0];
        const SNCQueue* firstQueue = getSNCQueue(firstQueueId);
        vector<SNCArrival*> arrivals;
        for (unsigned int i = 0; i < firstQueue->flows.size(); i++) {
            assert(firstQueue->flows[i].index == 0);
            const SNCFlow* f = getSNCFlow(firstQueue->flows[i].flowId);
            assert(f->queueIds[0] == firstQueueId);
            // Only consider other flows of higher (or equal) priority (i.e. <= flow->priority)
            if ((f->priority <= flow->priority) && (f->flowId != flow->flowId)) {
                arrivals.push_back(f->arrival);
            }
        }
        SNCService* firstQueueService = new ConstantService(firstQueue->bandwidth);
        toDelete.push_back(firstQueueService);
        if (!arrivals.empty()) {
            firstQueueService = new LeftoverService(aggregateArrivals(arrivals, toDelete), firstQueueService);
            toDelete.push_back(firstQueueService);
        }
        // Calculate latency of first hop
        flow->latency = LatencyBound(flow->arrival, firstQueueService, flow->epsilon / flow->queueIds.size()).calcLatency();
    } else if (flow->queueIds.size() == 2) {
        //
        // Two hops
        //
        // Identify ids of first queues that feed into this particular second queue
        QueueId firstQueueId = flow->queueIds[0];
        QueueId secondQueueId = flow->queueIds[1];
        const SNCQueue* secondQueue = getSNCQueue(secondQueueId);
        map<QueueId, unsigned int> firstQueueIds; // first QueueId -> priority
        for (unsigned int i = 0; i < secondQueue->flows.size(); i++) {
            assert(secondQueue->flows[i].index == 1);
            const SNCFlow* f = getSNCFlow(secondQueue->flows[i].flowId);
            assert(f->queueIds[1] == secondQueueId);
            // Only consider flows of higher (or equal) priority (i.e. <= flow->priority)
            if (f->priority <= flow->priority) {
                map<QueueId, unsigned int>::iterator it = firstQueueIds.find(f->queueIds[0]);
                if (it == firstQueueIds.end()) {
                    firstQueueIds[f->queueIds[0]] = f->priority;
                // Of the competing high priority flows for a given first queue, identify the lowest priority (i.e., max value)
                } else if (f->priority > it->second) {
                    it->second = f->priority;
                }
            }
        }
        // Loop through first queues to calculate second queue leftover service and aggregate arrival
        SNCService* firstQueueService = NULL;
        SNCArrival* aggregateArrivalShared = NULL;
        vector<SNCArrival*> arrivalsSecondQueue;
        for (map<QueueId, unsigned int>::const_iterator it = firstQueueIds.begin(); it != firstQueueIds.end(); it++) {
            const SNCQueue* q = getSNCQueue(it->first);
            vector<SNCArrival*> arrivalsShared;
            vector<SNCArrival*> arrivalsNonShared;
            for (unsigned int i = 0; i < q->flows.size(); i++) {
                assert(q->flows[i].index == 0);
                const SNCFlow* f = getSNCFlow(q->flows[i].flowId);
                assert(f->queueIds[0] == it->first);
                // Only consider other flows of higher (or equal) priority than the lowest priority competing flow as identified in firstQueueIds
                if ((f->priority <= it->second) && (f->flowId != flow->flowId)) {
                    SNCArrival* arrival = f->arrival;
                    // Check if sharing second queue
                    if (f->queueIds[1] == secondQueueId) {
                        arrivalsShared.push_back(arrival);
                    } else {
                        arrivalsNonShared.push_back(arrival);
                    }
                }
            }
            SNCService* service = new ConstantService(q->bandwidth);
            toDelete.push_back(service);
            if (!arrivalsNonShared.empty()) {
                service = new LeftoverService(aggregateArrivals(arrivalsNonShared, toDelete), service);
                toDelete.push_back(service);
            }
            if (it->first == firstQueueId) {
                // Handle first queue differently
                firstQueueService = service;
                if (!arrivalsShared.empty()) {
                    aggregateArrivalShared = aggregateArrivals(arrivalsShared, toDelete);
                }
            } else {
                if (!arrivalsShared.empty()) {
                    // Generate output bound on high priority flows that share second queue
                    SNCArrival* output = new OutputArrival(aggregateArrivals(arrivalsShared, toDelete), service);
                    toDelete.push_back(output);
                    // Aggregate arrivals to second queue
                    arrivalsSecondQueue.push_back(output);
                }
            }
        }
        SNCService* secondQueueService = new ConstantService(secondQueue->bandwidth);
        toDelete.push_back(secondQueueService);
        if (!arrivalsSecondQueue.empty()) {
            secondQueueService = new LeftoverService(aggregateArrivals(arrivalsSecondQueue, toDelete), secondQueueService);
            toDelete.push_back(secondQueueService);
        }
        // Calculate latency
        SNCService* finalService = new ConvolutionService(firstQueueService, secondQueueService);
        toDelete.push_back(finalService);
        if (aggregateArrivalShared) {
            finalService = new LeftoverService(aggregateArrivalShared, finalService);
            toDelete.push_back(finalService);
        }
        flow->latency = LatencyBound(flow->arrival, finalService, flow->epsilon).dependencyOptimization();
    }
    // Delete bounds
    while (!toDelete.empty()) {
        delete toDelete.back();
        toDelete.pop_back();
    }
}

void SNC::hopByHopAnalysis(SNCFlow* flow)
{
    vector<DependencyParams*> toDelete;
    // Initialize queue SNC bounds
    for (map<QueueId, Queue*>::const_iterator it = queuesBegin(); it != queuesEnd(); it++) {
        SNCQueue* q = getSNCQueue(it->first);
        q->leftoverService = new ConstantService(q->bandwidth);
        toDelete.push_back(q->leftoverService);
    }
    // Sort flows into priority order
    vector<SNCFlow*> sortedFlows;
    for (map<FlowId, Flow*>::const_iterator it = flowsBegin(); it != flowsEnd(); it++) {
        SNCFlow* f = getSNCFlow(it->first);
        // Only consider flows of higher (or equal) priority (i.e. <= flow->priority)
        if ((flow == NULL) || ((f->priority <= flow->priority) && (f->flowId != flow->flowId))) {
            sortedFlows.push_back(f);
        }
    }
    sort(sortedFlows.begin(), sortedFlows.end(), priorityCompare);
    if (flow) {
        sortedFlows.push_back(flow);
    }
    // Loop over flows f in priority order
    for (unsigned int i = 0; i < sortedFlows.size(); i++) {
        SNCFlow* f = sortedFlows[i];
        if ((flow == NULL) || (flow->flowId == f->flowId)) {
            f->latency = 0;
        }
        SNCArrival* arrival = f->arrival;
        // Loop through queues q in flow f
        for (unsigned int index = 0; index < f->queueIds.size(); index++) {
            SNCQueue* q = getSNCQueue(f->queueIds[index]);
            SNCService* service = q->leftoverService;
            double epsilon = f->epsilon;
            // Calculate latency bound of flow f at queue q
            if ((flow == NULL) || (flow->flowId == f->flowId)) {
                f->latency += LatencyBound(arrival, service, epsilon / f->queueIds.size()).dependencyOptimization();
            }
            // Calculate leftover service curve
            q->leftoverService = new LeftoverService(arrival, service);
            toDelete.push_back(q->leftoverService);
            // Calculate output arrival bound
            arrival = new OutputArrival(arrival, service);
            toDelete.push_back(arrival);
        }
    }
    // Delete bounds
    while (!toDelete.empty()) {
        delete toDelete.back();
        toDelete.pop_back();
    }
}

void SNC::convolutionAnalysis(SNCFlow* flow)
{
    vector<DependencyParams*> toDelete;
    // Initialize queue SNC bounds
    for (map<QueueId, Queue*>::const_iterator it = queuesBegin(); it != queuesEnd(); it++) {
        SNCQueue* q = getSNCQueue(it->first);
        q->leftoverService = new ConstantService(q->bandwidth);
        toDelete.push_back(q->leftoverService);
    }
    // Sort flows into priority order
    vector<SNCFlow*> sortedFlows;
    for (map<FlowId, Flow*>::const_iterator it = flowsBegin(); it != flowsEnd(); it++) {
        SNCFlow* f = getSNCFlow(it->first);
        // Only consider flows of higher (or equal) priority (i.e. <= flow->priority)
        if ((flow == NULL) || ((f->priority <= flow->priority) && (f->flowId != flow->flowId))) {
            sortedFlows.push_back(f);
        }
    }
    sort(sortedFlows.begin(), sortedFlows.end(), priorityCompare);
    if (flow) {
        sortedFlows.push_back(flow);
    }
    // Loop over flows f in priority order
    for (unsigned int i = 0; i < sortedFlows.size(); i++) {
        SNCFlow* f = sortedFlows[i];
        // Convolute leftover service curves in all queues in flow
        const SNCQueue* firstQueue = getSNCQueue(f->queueIds[0]);
        SNCService* convolutedService = firstQueue->leftoverService;
        for (unsigned int index = 1; index < f->queueIds.size(); index++) {
            const SNCQueue* q = getSNCQueue(f->queueIds[index]);
            SNCService* service = q->leftoverService;
            convolutedService = new ConvolutionService(convolutedService, service);
            toDelete.push_back(convolutedService);
        }
        // Calculate latency bound of flow f
        SNCArrival* arrival = f->arrival;
        double epsilon = f->epsilon;
        if ((flow == NULL) || (flow->flowId == f->flowId)) {
            f->latency = LatencyBound(arrival, convolutedService, epsilon).dependencyOptimization();
        }
        // Loop through queues q in flow f
        for (unsigned int index = 0; index < f->queueIds.size(); index++) {
            SNCQueue* q = getSNCQueue(f->queueIds[index]);
            SNCService* service = q->leftoverService;
            // Calculate leftover service curve
            q->leftoverService = new LeftoverService(arrival, service);
            toDelete.push_back(q->leftoverService);
            // Calculate output arrival bound
            arrival = new OutputArrival(arrival, service);
            toDelete.push_back(arrival);
        }
    }
    // Delete bounds
    while (!toDelete.empty()) {
        delete toDelete.back();
        toDelete.pop_back();
    }
}

FlowId SNC::initFlow(Flow* f, const Json::Value& flowInfo, ClientId clientId)
{
    if (f == NULL) {
        f = new SNCFlow;
    }
    FlowId flowId = NC::initFlow(f, flowInfo, clientId);
    const Client* c = getClient(clientId);
    // Get SNC parameters
    MMBPArrival* arrival = NULL;
    deserializeJSON(flowInfo, "arrivalInfo", arrival);
    arrival->addDependency(flowId); // add itself as dependency
    SNCFlow* sf = getSNCFlow(flowId);
    sf->arrival = arrival;
    double epsilon = 1.0 - (c->SLOpercentile / 100.0);
    sf->epsilon = epsilon / c->flowIds.size();
    return flowId;
}

QueueId SNC::initQueue(Queue* q, const Json::Value& queueInfo)
{
    if (q == NULL) {
        q = new SNCQueue;
    }
    return NC::initQueue(q, queueInfo);
}

// Assumes priorities are set
double SNC::calcFlowLatency(FlowId flowId)
{
    SNCFlow* f = getSNCFlow(flowId);
    switch (_algorithm) {
        case SNC_ALGORITHM_AGGREGATE:
            aggregateAnalysisTwoHopDep(f);
            break;

        case SNC_ALGORITHM_HOP_BY_HOP:
            hopByHopAnalysis(f);
            break;

        case SNC_ALGORITHM_CONVOLUTION:
            convolutionAnalysis(f);
            break;

        default:
            cerr << "Invalid algorithm " << _algorithm << endl;
            f->latency = 0;
            break;
    }
    return f->latency;
}

void SNC::addDependency(ClientId clientId1, ClientId clientId2)
{
    const Client* c1 = getClient(clientId1);
    const Client* c2 = getClient(clientId2);
    for (vector<FlowId>::const_iterator it = c1->flowIds.begin(); it != c1->flowIds.end(); it++) {
        SNCFlow* f = getSNCFlow(*it);
        f->arrival->addDependencies(c2->flowIds);
    }
    for (vector<FlowId>::const_iterator it = c2->flowIds.begin(); it != c2->flowIds.end(); it++) {
        SNCFlow* f = getSNCFlow(*it);
        f->arrival->addDependencies(c1->flowIds);
    }
}

void SNC::setArrivalInfo(Json::Value& flowInfo, string trace, const Json::Value& estimatorInfo)
{
    // Init estimator
    Estimator* pEst = Estimator::Create(estimatorInfo);
    // Read trace
    ProcessedTrace* pTrace = new ProcessedTrace(trace, pEst);
    MMBPArrival* arrival = new MMBPArrival(pTrace);
    serializeJSON(flowInfo, "arrivalInfo", arrival);
    delete arrival;
    delete pTrace;
}
