// NC.cpp - Code for the network calculus toolkit infrastructure.
// See NC.hpp for details.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <string>
#include <vector>
#include <map>
#include "../json/json.h"
#include "NC.hpp"

using namespace std;

bool priorityCompare(const Flow* f1, const Flow* f2)
{
    if (f1->priority == f2->priority) {
        // Heuristic: higher latency flows care more about priority
        return (f1->latency > f2->latency);
    }
    return (f1->priority < f2->priority);
}

NC::NC()
    : _nextFlowId(InvalidFlowId + 1),
      _nextClientId(InvalidClientId + 1),
      _nextQueueId(InvalidQueueId + 1)
{
}

NC::~NC()
{
    // Delete flows
    for (map<FlowId, Flow*>::const_iterator it = flowsBegin(); it != flowsEnd(); it++) {
        delete it->second;
    }
    // Delete clients
    for (map<ClientId, Client*>::const_iterator it = clientsBegin(); it != clientsEnd(); it++) {
        delete it->second;
    }
    // Delete queues
    for (map<QueueId, Queue*>::const_iterator it = queuesBegin(); it != queuesEnd(); it++) {
        delete it->second;
    }
}

FlowId NC::initFlow(Flow* f, const Json::Value& flowInfo, ClientId clientId)
{
    if (f == NULL) {
        f = new Flow;
    }
    FlowId flowId = _nextFlowId++;
    f->flowId = flowId;
    _flows[flowId] = f;
    f->name = flowInfo["name"].asString();
    _flowIds[f->name] = flowId;
    f->clientId = clientId;
    // Add flow to client flows list
    Client* c = _clients[clientId];
    c->flowIds.push_back(flowId);
    const Json::Value& flowQueues = flowInfo["queues"];
    f->queueIds.resize(flowQueues.size());
    for (unsigned int index = 0; index < flowQueues.size(); index++) {
        QueueId queueId = getQueueIdByName(flowQueues[index].asString());
        f->queueIds[index] = queueId;
        // Init queue's list of flows
        FlowIndex fi;
        fi.flowId = flowId;
        fi.index = index;
        _queues[queueId]->flows.push_back(fi);
    }
    f->priority = flowInfo.isMember("priority") ? flowInfo["priority"].asUInt() : 1;
    f->latency = 0;
    return flowId;
}

ClientId NC::initClient(Client* c, const Json::Value& clientInfo)
{
    if (c == NULL) {
        c = new Client;
    }
    ClientId clientId = _nextClientId++;
    c->clientId = clientId;
    _clients[clientId] = c;
    c->name = clientInfo["name"].asString();
    _clientIds[c->name] = clientId;
    c->SLO = clientInfo["SLO"].asDouble();
    c->SLOpercentile = clientInfo.isMember("SLOpercentile") ? clientInfo["SLOpercentile"].asDouble() : 99.9;
    c->latency = 0;
    return clientId;
}

QueueId NC::initQueue(Queue* q, const Json::Value& queueInfo)
{
    if (q == NULL) {
        q = new Queue;
    }
    QueueId queueId = _nextQueueId++;
    q->queueId = queueId;
    _queues[queueId] = q;
    q->name = queueInfo["name"].asString();
    _queueIds[q->name] = queueId;
    q->bandwidth = queueInfo["bandwidth"].asDouble();
    return queueId;
}

ClientId NC::addClient(const Json::Value& clientInfo)
{
    // Initialize client
    ClientId clientId = initClient(NULL, clientInfo);
    // Initialize client's flows
    const Json::Value& clientFlowInfos = clientInfo["flows"];
    for (unsigned int flowIndex = 0; flowIndex < clientFlowInfos.size(); flowIndex++) {
        initFlow(NULL, clientFlowInfos[flowIndex], clientId);
    }
    return clientId;
}

QueueId NC::addQueue(const Json::Value& queueInfo)
{
    return initQueue(NULL, queueInfo);
}

void NC::delClient(ClientId clientId)
{
    Client* c = _clients[clientId];
    // Delete client's flows
    for (unsigned int flowIndex = 0; flowIndex < c->flowIds.size(); flowIndex++) {
        FlowId flowId = c->flowIds[flowIndex];
        Flow* f = _flows[flowId];
        // Delete flow from queues
        for (unsigned int index = 0; index < f->queueIds.size(); index++) {
            Queue* q = _queues[f->queueIds[index]];
            for (vector<FlowIndex>::iterator it = q->flows.begin(); it != q->flows.end(); it++) {
                if (it->flowId == flowId) {
                    q->flows.erase(it);
                    break;
                }
            }
        }
        _flowIds.erase(f->name);
        _flows.erase(flowId);
        delete f;
    }
    // Delete client
    _clientIds.erase(c->name);
    _clients.erase(clientId);
    delete c;
}

void NC::delQueue(QueueId queueId)
{
    Queue* q = _queues[queueId];
    assert(q->flows.empty());
    _queueIds.erase(q->name);
    _queues.erase(queueId);
    delete q;
}

void NC::setFlowPriority(FlowId flowId, unsigned int priority)
{
    Flow* f = _flows[flowId];
    f->priority = priority;
}

void NC::calcAllLatency()
{
    // Loop through clients and calculate latency
    for (map<ClientId, Client*>::const_iterator it = clientsBegin(); it != clientsEnd(); it++) {
        calcClientLatency(it->first);
    }
}

double NC::calcClientLatency(ClientId clientId)
{
    // Loop through client's flows and calculate latency
    Client* c = _clients[clientId];
    c->latency = 0;
    for (unsigned int flowIndex = 0; flowIndex < c->flowIds.size(); flowIndex++) {
        c->latency += calcFlowLatency(c->flowIds[flowIndex]);
    }
    return c->latency;
}
