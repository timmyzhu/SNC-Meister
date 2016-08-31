// NC.hpp - Class definitions for network calculus toolkit infrastructure.
// This file defines all of the basic information needed to perform any sort of analysis on networks of queues.
// Queue is used to represent the bandwidth of queues within the network.
// Flow is used to describe a stream of requests that traverse one or more queues.
// Client is used to describe a sequence of flows that represents the end-to-end behavior of a sequence of requests.
//
// Each of the above structures are initialized via JSON dictionaries.
// flowInfo is a JSON dictionary with the following fields:
// "name": string - name of flow
// "queues": list string - ordered list of queue names visited by flow
// "arrivalInfo": JSON object (SNC) - serialized info about flow arrivals from SNC::setArrivalInfo
// "priority": unsigned int (optional) - positive priority number (lower = higher priority); defaults to 1
//
// clientInfo is a JSON dictionary with the following fields:
// "name": string - name of client
// "flows": list flow - ordered list of flows belonging to client
// "SLO": float - client SLO in seconds
// "SLOpercentile": float (optional) - client SLO percentile (e.g., 99.9%); defaults to 99.9
//
// queueInfo is a JSON dictionary with the following fields:
// "name": string - name of queue
// "bandwidth": float - bandwidth of queue, in "work" units (see Estimator.hpp)
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _NC_HPP
#define _NC_HPP

#include <string>
#include <vector>
#include <map>
#include "../json/json.h"

using namespace std;

typedef unsigned int FlowId;
typedef unsigned int ClientId;
typedef unsigned int QueueId;

const FlowId InvalidFlowId = 0;
const ClientId InvalidClientId = 0;
const QueueId InvalidQueueId = 0;

// Base structure for representing a flow.
// A flow identifies a stream of requests that traverses one or more queues, where the request sizes do not change between queues.
// For example, we use a flow to represent the traffic from a VM to a server, and another flow to represent the traffic from the server back to the VM.
struct Flow {
    virtual ~Flow() {}

    FlowId flowId; // Id of flow
    string name; // Name of flow
    ClientId clientId; // Id of client that flow belongs to
    vector<QueueId> queueIds; // Ordered list of queues visited by flow
    unsigned int priority; // Priority of flow (lower = higher priority)
    double latency; // Latency of flow, once calculated
};

// Base structure for representing a client.
// A client identifies an ordered sequence of flows that represent the end-to-end behavior of a sequence of requests.
// For example, we use a client with two flows 1) VM -> server and 2) server -> VM to represent the end-to-end behavior of a stream of get/put requests to a key-value store.
// The end-to-end SLO and SLO percentile (e.g., 10ms for 99.9% of requests) are specified for clients.
struct Client {
    virtual ~Client() {}

    ClientId clientId; // Id of client
    string name; // Name of client
    vector<FlowId> flowIds; // Ordered list of flows that compose client
    double SLO; // Client SLO in seconds
    double SLOpercentile; // Client SLO percentile (e.g., 99.9%); range 0 - 100
    double latency; // Latency of client (i.e., sum of flow latencies), once calculated
};

struct FlowIndex {
    FlowId flowId; // Flow id
    unsigned int index; // Index within flow's queueIds vector
};

// Base structure for representing a queue.
// A queue is used to represent congestion points within the system.
// For a network, this often occurs at the end-host network links, especially in full-bisection bandwidth networks.
struct Queue {
    virtual ~Queue() {}

    QueueId queueId; // Id of queue
    string name; // Name of queue
    vector<FlowIndex> flows; // Unordered list of flows that use queue
    double bandwidth; // Bandwidth of queue, in "work" units (see Estimator.hpp)
};

// Comparison function for sorting flows by priority.
// Returns true if f1 is higher priority than f2.
bool priorityCompare(const Flow* f1, const Flow* f2);

// Base class for representing a network calculus analysis toolkit.
class NC
{
private:
    map<string, FlowId> _flowIds; // map flow name -> flow id
    map<string, ClientId> _clientIds; // map client name -> client id
    map<string, QueueId> _queueIds; // map queue name -> queue id
    map<FlowId, Flow*> _flows; // map flow id -> flow data
    map<ClientId, Client*> _clients; // map client id -> client data
    map<QueueId, Queue*> _queues; // map queue id -> queue data
    FlowId _nextFlowId; // next flow id to use for new flow
    ClientId _nextClientId; // next client id to use for new client
    QueueId _nextQueueId; // next queue id to use for new queue

protected:
    // Initialize a flow. Overridden by derived classes with extra flow information/initialization.
    // If f is NULL, f will be created. See file header for flowInfo description.
    virtual FlowId initFlow(Flow* f, const Json::Value& flowInfo, ClientId clientId);
    // Initialize a client. Overridden by derived classes with extra client information/initialization.
    // If c is NULL, c will be created. See file header for clientInfo description.
    virtual ClientId initClient(Client* c, const Json::Value& clientInfo);
    // Initialize a queue. Overridden by derived classes with extra client information/initialization.
    // If q is NULL, q will be created. See file header for queueInfo description.
    virtual QueueId initQueue(Queue* q, const Json::Value& queueInfo);

public:
    NC();
    virtual ~NC();

    // Add a client and its flows to system.
    // clientFlowInfos is an ordered list with the information on the client's flows.
    // See file header for clientInfo description.
    ClientId addClient(const Json::Value& clientInfo);
    // Add a queue to the system.
    // See file header for queueInfo description.
    QueueId addQueue(const Json::Value& queueInfo);
    // Delete a client and its flows from system.
    void delClient(ClientId clientId);
    // Delete a queue from system.
    void delQueue(QueueId queueId);

    // Set the priority for a flow.
    void setFlowPriority(FlowId flowId, unsigned int priority);

    // Add dependency between clients' flows. Dependencies are symmetric.
    virtual void addDependency(ClientId clientId1, ClientId clientId2)
    {}

    // Calculate the latency for all clients/flows in the system.
    // Assumes priorities are set.
    virtual void calcAllLatency();
    // Calculate the latency for a client and its flows.
    // Assumes priorities are set.
    virtual double calcClientLatency(ClientId clientId);
    // Calculate the latency for a flow.
    // Assumes priorities are set.
    virtual double calcFlowLatency(FlowId flowId) = 0;

    // Read-only accessors
    map<FlowId, Flow*>::const_iterator flowsBegin() const { return _flows.begin(); }
    map<FlowId, Flow*>::const_iterator flowsEnd() const { return _flows.end(); }
    const Flow* getFlow(FlowId flowId) const {
        map<FlowId, Flow*>::const_iterator it = _flows.find(flowId);
        return (it != _flows.end()) ? it->second : NULL;
    }

    map<ClientId, Client*>::const_iterator clientsBegin() const { return _clients.begin(); }
    map<ClientId, Client*>::const_iterator clientsEnd() const { return _clients.end(); }
    const Client* getClient(ClientId clientId) const {
        map<ClientId, Client*>::const_iterator it = _clients.find(clientId);
        return (it != _clients.end()) ? it->second : NULL;
    }

    map<QueueId, Queue*>::const_iterator queuesBegin() const { return _queues.begin(); }
    map<QueueId, Queue*>::const_iterator queuesEnd() const { return _queues.end(); }
    const Queue* getQueue(QueueId queueId) const {
        map<QueueId, Queue*>::const_iterator it = _queues.find(queueId);
        return (it != _queues.end()) ? it->second : NULL;
    }

    FlowId getFlowIdByName(string name) const {
        map<string, FlowId>::const_iterator it = _flowIds.find(name);
        return (it != _flowIds.end()) ? it->second : InvalidFlowId;
    }
    ClientId getClientIdByName(string name) const {
        map<string, ClientId>::const_iterator it = _clientIds.find(name);
        return (it != _clientIds.end()) ? it->second : InvalidClientId;
    }
    QueueId getQueueIdByName(string name) const {
        map<string, QueueId>::const_iterator it = _queueIds.find(name);
        return (it != _queueIds.end()) ? it->second : InvalidQueueId;
    }
};

#endif // _NC_HPP
