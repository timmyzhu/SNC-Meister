// SNC-Meister.cpp - SNC-Meister admission control server.
// Performs admission control for a network based on Stochastic Network Calculus (SNC) using SNC-Library.
// When a set of tenants (a.k.a. clients) seeks admission, their latency is calculated using SNC and compared against their SLO.
// Also, other clients that are affected by the new clients are checked to see if they meet their SLOs.
// If the new clients and affected clients meet their SLOs, then the new clients are admitted.
// If admitted and enforcerAddr/dstAddr/srcAddr are set in a client's flow (see "addrPrefix" option in SNC-ConfigGen), then NetEnforcer will be updated with its network priority.
// Priority is determined with the BySLO policy (i.e., with the tightest SLO getting the highest priority).
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <sys/socket.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include "../prot/SNC_Meister_prot.h"
#include "../prot/NetEnforcer_prot.h"
#include "../json/json.h"
#include "../SNC-Library/NC.hpp"
#include "../SNC-Library/SNC.hpp"
#include "../SNC-Library/priorityAlgoBySLO.hpp"

using namespace std;

// Global network calculus calculator
NC* nc;

// Global storage for clientInfos
map<ClientId, Json::Value> clientInfoStore;

// Convert a string internet address to an IP address
unsigned long addrInfo(string addr)
{
    unsigned long s_addr = 0;
    struct addrinfo* result;
    int err = getaddrinfo(addr.c_str(), NULL, NULL, &result);
    if ((err != 0) || (result == NULL)) {
        cerr << "Error in getaddrinfo: " << err << endl;
    } else {
        for (struct addrinfo* res = result; res != NULL; res = res->ai_next) {
            if (result->ai_addr->sa_family == AF_INET) {
                s_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr.s_addr;
                break;
            }
        }
        freeaddrinfo(result);
    }
    return s_addr;
}

// Send RPC to NetEnforcer to update client
void updateClient(string enforcerAddr, string dstAddr, string srcAddr, unsigned int priority)
{
    // Connect to enforcer
    CLIENT* cl;
    if ((cl = clnt_create(enforcerAddr.c_str(), NET_ENFORCER_PROGRAM, NET_ENFORCER_V1, "tcp")) == NULL) {
        clnt_pcreateerror(enforcerAddr.c_str());
        return;
    }
    // Call RPC
    NetClientUpdate arg;
    arg.client.s_dstAddr = addrInfo(dstAddr);
    arg.client.s_srcAddr = addrInfo(srcAddr);
    arg.priority = priority;
    NetUpdateClientsArgs args = {1, &arg};
    if (net_enforcer_update_clients_1(args, cl) == NULL) {
        clnt_perror(cl, "Failed network RPC");
    }
    // Destroy client
    clnt_destroy(cl);
}

// Send RPC to NetEnforcer to remove client
void removeClient(string enforcerAddr, string dstAddr, string srcAddr)
{
    // Connect to enforcer
    CLIENT* cl;
    if ((cl = clnt_create(enforcerAddr.c_str(), NET_ENFORCER_PROGRAM, NET_ENFORCER_V1, "tcp")) == NULL) {
        clnt_pcreateerror(enforcerAddr.c_str());
        return;
    }
    // Call RPC
    NetClient arg;
    arg.s_dstAddr = addrInfo(dstAddr);
    arg.s_srcAddr = addrInfo(srcAddr);
    NetRemoveClientsArgs args = {1, &arg};
    if (net_enforcer_remove_clients_1(args, cl) == NULL) {
        clnt_perror(cl, "Failed network RPC");
    }
    // Destroy client
    clnt_destroy(cl);
}

// Check the JSON flowInfo objects.
// Returns error for invalid arguments.
SNCStatus checkFlowInfo(set<string>& flowNames, const Json::Value& flowInfo)
{
    // Check name
    if (!flowInfo.isMember("name")) {
        return SNC_ERR_MISSING_ARGUMENT;
    }
    string name = flowInfo["name"].asString();
    if (nc->getFlowIdByName(name) != InvalidFlowId) {
        return SNC_ERR_FLOW_NAME_IN_USE;
    }
    if (flowNames.find(name) != flowNames.end()) {
        return SNC_ERR_FLOW_NAME_IN_USE;
    }
    flowNames.insert(name);
    // Check queues
    if (!flowInfo.isMember("queues")) {
        return SNC_ERR_MISSING_ARGUMENT;
    }
    const Json::Value& flowQueues = flowInfo["queues"];
    if (!flowQueues.isArray()) {
        return SNC_ERR_INVALID_ARGUMENT;
    }
    for (unsigned int index = 0; index < flowQueues.size(); index++) {
        string queueName = flowQueues[index].asString();
        if (nc->getQueueIdByName(queueName) == InvalidQueueId) {
            return SNC_ERR_QUEUE_NAME_NONEXISTENT;
        }
    }
    // Check arrivalInfo
    if (!flowInfo.isMember("arrivalInfo")) {
        return SNC_ERR_MISSING_ARGUMENT;
    }
    return SNC_SUCCESS;
}

// Check the JSON clientInfo objects.
// Returns error for invalid arguments.
SNCStatus checkClientInfo(set<string>& clientNames, set<string>& flowNames, const Json::Value& clientInfo)
{
    // Check name
    if (!clientInfo.isMember("name")) {
        return SNC_ERR_MISSING_ARGUMENT;
    }
    string name = clientInfo["name"].asString();
    if (nc->getClientIdByName(name) != InvalidClientId) {
        return SNC_ERR_CLIENT_NAME_IN_USE;
    }
    if (clientNames.find(name) != clientNames.end()) {
        return SNC_ERR_CLIENT_NAME_IN_USE;
    }
    clientNames.insert(name);
    // Check SLO
    if (!clientInfo.isMember("SLO")) {
        return SNC_ERR_MISSING_ARGUMENT;
    }
    double SLO = clientInfo["SLO"].asDouble();
    if (SLO <= 0) {
        return SNC_ERR_INVALID_ARGUMENT;
    }
    // Check SLOpercentile
    if (clientInfo.isMember("SLOpercentile")) {
        double SLOpercentile = clientInfo["SLOpercentile"].asDouble();
        if (!((0 < SLOpercentile) && (SLOpercentile < 100))) {
            return SNC_ERR_INVALID_ARGUMENT;
        }
    }
    // Check client's flows
    if (!clientInfo.isMember("flows")) {
        return SNC_ERR_MISSING_ARGUMENT;
    }
    const Json::Value& clientFlows = clientInfo["flows"];
    if (!clientFlows.isArray()) {
        return SNC_ERR_INVALID_ARGUMENT;
    }
    for (unsigned int flowIndex = 0; flowIndex < clientFlows.size(); flowIndex++) {
        SNCStatus status = checkFlowInfo(flowNames, clientFlows[flowIndex]);
        if (status != SNC_SUCCESS) {
            return status;
        }
    }
    return SNC_SUCCESS;
}

// Check list of JSON clientInfo objects.
// Returns error for invalid arguments.
SNCStatus checkClientInfos(const Json::Value& clientInfos)
{
    // Check clientInfos is an array
    if (!clientInfos.isArray()) {
        return SNC_ERR_INVALID_ARGUMENT;
    }
    set<string> clientNames; // ensure no duplicate names
    set<string> flowNames; // ensure no duplicate names
    for (unsigned int i = 0; i < clientInfos.size(); i++) {
        SNCStatus status = checkClientInfo(clientNames, flowNames, clientInfos[i]);
        if (status != SNC_SUCCESS) {
            return status;
        }
    }
    return SNC_SUCCESS;
}

// Adds dependencies between clients based on the RPC arguments.
// Returns error for invalid arguments.
SNCStatus addDependencies(const Json::Value& clientInfo)
{
    if (clientInfo.isMember("dependencies")) {
        const Json::Value& dependencies = clientInfo["dependencies"];
        if (!dependencies.isArray()) {
            return SNC_ERR_INVALID_ARGUMENT;
        }
        ClientId clientId = nc->getClientIdByName(clientInfo["name"].asString());
        assert(clientId != InvalidClientId);
        for (unsigned int i = 0; i < dependencies.size(); i++) {
            ClientId dependency = nc->getClientIdByName(dependencies[i].asString());
            if (dependency == InvalidClientId) {
                return SNC_ERR_CLIENT_NAME_NONEXISTENT;
            }
            nc->addDependency(clientId, dependency);
        }
    }
    return SNC_SUCCESS;
}

// FlowIndex less than function
bool operator< (const FlowIndex& fi1, const FlowIndex& fi2)
{
    if (fi1.flowId == fi2.flowId) {
        return (fi1.index < fi2.index);
    }
    return (fi1.flowId < fi2.flowId);
}

// Mark flows affected at a priority level starting from a flow at a given index.
void markAffectedFlows(set<FlowIndex>& affectedFlows, const FlowIndex& fi, unsigned int priority)
{
    const Flow* f = nc->getFlow(fi.flowId);
    // If f is higher priority, it is unaffected
    if (f->priority < priority) {
        return;
    }
    // If we've already marked flow at given index, stop
    if (affectedFlows.find(fi) != affectedFlows.end()) {
        return;
    }
    affectedFlows.insert(fi);
    // Loop through queues affected by flow starting at index
    for (unsigned int index = fi.index; index < f->queueIds.size(); index++) {
        const Queue* q = nc->getQueue(f->queueIds[index]);
        // Try marking other flows sharing queue
        for (vector<FlowIndex>::const_iterator itFi = q->flows.begin(); itFi != q->flows.end(); itFi++) {
            markAffectedFlows(affectedFlows, *itFi, f->priority);
        }
    }
}

// AddClients RPC - performs admission control check on a set of clients and adds clients to system if admitted.
SNCAddClientsRes* snc_meister_add_clients_svc(SNCAddClientsArgs* argp, struct svc_req* rqstp)
{
    static SNCAddClientsRes result;
    // Initialize result
    result.admitted = true;
    result.status = SNC_SUCCESS;
    // Parse input
    Json::Reader reader;
    Json::Value clientInfos;
    string clientInfosStr(argp->clientInfos);
    if (!reader.parse(clientInfosStr, clientInfos)) {
        result.status = SNC_ERR_INVALID_ARGUMENT;
        result.admitted = false;
        return &result;
    }
    // Check parameters
    result.status = checkClientInfos(clientInfos);
    if (result.status != SNC_SUCCESS) {
        result.admitted = false;
        return &result;
    }
    // Add clients
    set<ClientId> clientIds;
    for (unsigned int i = 0; i < clientInfos.size(); i++) {
        const Json::Value& clientInfo = clientInfos[i];
        ClientId clientId = nc->addClient(clientInfo);
        clientIds.insert(clientId);
        clientInfoStore[clientId] = clientInfo;
    }
    // Add dependencies
    for (unsigned int i = 0; i < clientInfos.size(); i++) {
        result.status = addDependencies(clientInfos[i]);
        if (result.status != SNC_SUCCESS) {
            result.admitted = false;
            break;
        }
    }
    if (result.status == SNC_SUCCESS) {
        // Configure priorities
        configurePrioritiesBySLO(nc);
        // Check latency of added clients
        set<FlowIndex> affectedFlows;
        for (set<ClientId>::const_iterator it = clientIds.begin(); it != clientIds.end(); it++) {
            ClientId clientId = *it;
            nc->calcClientLatency(clientId);
            const Client* c = nc->getClient(clientId);
            if (c->latency > c->SLO) {
                result.admitted = false;
                break;
            }
            // Add affected flows
            for (vector<FlowId>::const_iterator itF = c->flowIds.begin(); itF != c->flowIds.end(); itF++) {
                FlowIndex fi;
                fi.flowId = *itF;
                fi.index = 0;
                markAffectedFlows(affectedFlows, fi, 0);
            }
        }
        if (result.admitted) {
            // Get clientIds of affected flows
            set<ClientId> affectedClientIds;
            for (set<FlowIndex>::const_iterator it = affectedFlows.begin(); it != affectedFlows.end(); it++) {
                const Flow* f = nc->getFlow(it->flowId);
                affectedClientIds.insert(f->clientId);
            }
            // Check latency of other affected clients
            for (set<ClientId>::const_iterator it = affectedClientIds.begin(); it != affectedClientIds.end(); it++) {
                ClientId clientId = *it;
                if (clientIds.find(clientId) == clientIds.end()) {
                    nc->calcClientLatency(clientId);
                    const Client* c = nc->getClient(clientId);
                    if (c->latency > c->SLO) {
                        result.admitted = false;
                        break;
                    }
                }
            }
        }
    }
    if (result.admitted) {
        // Send RPC to NetEnforcer to update client
        for (unsigned int i = 0; i < clientInfos.size(); i++) {
            const Json::Value& clientInfo = clientInfos[i];
            const Json::Value& clientFlows = clientInfo["flows"];
            for (unsigned int flowIndex = 0; flowIndex < clientFlows.size(); flowIndex++) {
                const Json::Value& flowInfo = clientFlows[flowIndex];
                if (flowInfo.isMember("enforcerAddr") && flowInfo.isMember("dstAddr") && flowInfo.isMember("srcAddr")) {
                    FlowId flowId = nc->getFlowIdByName(flowInfo["name"].asString());
                    const Flow* f = nc->getFlow(flowId);
                    updateClient(flowInfo["enforcerAddr"].asString(), flowInfo["dstAddr"].asString(), flowInfo["srcAddr"].asString(), f->priority);
                }
            }
        }
    } else {
        // Delete clients
        for (set<ClientId>::const_iterator it = clientIds.begin(); it != clientIds.end(); it++) {
            ClientId clientId = *it;
            clientInfoStore.erase(clientId);
            nc->delClient(clientId);
        }
    }
    return &result;
}

// DelClient RPC - delete a client from system.
SNCDelClientRes* snc_meister_del_client_svc(SNCDelClientArgs* argp, struct svc_req* rqstp)
{
    static SNCDelClientRes result;
    string name(argp->name);
    ClientId clientId = nc->getClientIdByName(name);
    // Check that client exists
    if (clientId == InvalidClientId) {
        result.status = SNC_ERR_CLIENT_NAME_NONEXISTENT;
        return &result;
    }
    // Send RPC to NetEnforcer to remove client
    assert(clientInfoStore.find(clientId) != clientInfoStore.end());
    const Json::Value& clientInfo = clientInfoStore[clientId];
    const Json::Value& clientFlows = clientInfo["flows"];
    for (unsigned int flowIndex = 0; flowIndex < clientFlows.size(); flowIndex++) {
        const Json::Value& flowInfo = clientFlows[flowIndex];
        if (flowInfo.isMember("enforcerAddr") && flowInfo.isMember("dstAddr") && flowInfo.isMember("srcAddr")) {
            removeClient(flowInfo["enforcerAddr"].asString(), flowInfo["dstAddr"].asString(), flowInfo["srcAddr"].asString());
        }
    }
    // Delete client
    clientInfoStore.erase(clientId);
    nc->delClient(clientId);
    result.status = SNC_SUCCESS;
    return &result;
}

// AddQueue RPC - add a queue to system.
SNCAddQueueRes* snc_meister_add_queue_svc(SNCAddQueueArgs* argp, struct svc_req* rqstp)
{
    static SNCAddQueueRes result;
    // Parse input
    Json::Reader reader;
    Json::Value queueInfo;
    string queueInfoStr(argp->queueInfo);
    if (!reader.parse(queueInfoStr, queueInfo)) {
        result.status = SNC_ERR_INVALID_ARGUMENT;
        return &result;
    }
    // Check for valid name
    if (!queueInfo.isMember("name")) {
        result.status = SNC_ERR_MISSING_ARGUMENT;
        return &result;
    }
    if (nc->getQueueIdByName(queueInfo["name"].asString()) != InvalidQueueId) {
        result.status = SNC_ERR_QUEUE_NAME_IN_USE;
        return &result;
    }
    // Check for valid bandwidth
    if (!queueInfo.isMember("bandwidth")) {
        result.status = SNC_ERR_MISSING_ARGUMENT;
        return &result;
    }
    if (queueInfo["bandwidth"].asDouble() <= 0) {
        result.status = SNC_ERR_INVALID_ARGUMENT;
        return &result;
    }
    // Add queue
    nc->addQueue(queueInfo);
    result.status = SNC_SUCCESS;
    return &result;
}

// DelQueue RPC - delete a queue from system.
SNCDelQueueRes* snc_meister_del_queue_svc(SNCDelQueueArgs* argp, struct svc_req* rqstp)
{
    static SNCDelQueueRes result;
    string name(argp->name);
    QueueId queueId = nc->getQueueIdByName(name);
    // Check that queue exists
    if (queueId == InvalidQueueId) {
        result.status = SNC_ERR_QUEUE_NAME_NONEXISTENT;
        return &result;
    }
    // Check that queue is empty
    const Queue* q = nc->getQueue(queueId);
    assert(q != NULL);
    if (!q->flows.empty()) {
        result.status = SNC_ERR_QUEUE_HAS_ACTIVE_FLOWS;
        return &result;
    }
    // Delete queue
    nc->delQueue(queueId);
    result.status = SNC_SUCCESS;
    return &result;
}

// Main RPC handler
void snc_meister_program(struct svc_req* rqstp, register SVCXPRT* transp)
{
    union {
        SNCAddClientsArgs snc_meister_add_clients_arg;
        SNCDelClientArgs snc_meister_del_client_arg;
        SNCAddQueueArgs snc_meister_add_queue_arg;
        SNCDelQueueArgs snc_meister_del_queue_arg;
    } argument;
    char* result;
    xdrproc_t _xdr_argument, _xdr_result;
    char* (*local)(char*, struct svc_req*);

    switch (rqstp->rq_proc) {
        case SNC_MEISTER_NULL:
            svc_sendreply(transp, (xdrproc_t)xdr_void, (caddr_t)NULL);
            return;

        case SNC_MEISTER_ADD_CLIENTS:
            _xdr_argument = (xdrproc_t)xdr_SNCAddClientsArgs;
            _xdr_result = (xdrproc_t)xdr_SNCAddClientsRes;
            local = (char* (*)(char*, struct svc_req*))snc_meister_add_clients_svc;
            break;

        case SNC_MEISTER_DEL_CLIENT:
            _xdr_argument = (xdrproc_t)xdr_SNCDelClientArgs;
            _xdr_result = (xdrproc_t)xdr_SNCDelClientRes;
            local = (char* (*)(char*, struct svc_req*))snc_meister_del_client_svc;
            break;

        case SNC_MEISTER_ADD_QUEUE:
            _xdr_argument = (xdrproc_t)xdr_SNCAddQueueArgs;
            _xdr_result = (xdrproc_t)xdr_SNCAddQueueRes;
            local = (char* (*)(char*, struct svc_req*))snc_meister_add_queue_svc;
            break;

        case SNC_MEISTER_DEL_QUEUE:
            _xdr_argument = (xdrproc_t)xdr_SNCDelQueueArgs;
            _xdr_result = (xdrproc_t)xdr_SNCDelQueueRes;
            local = (char* (*)(char*, struct svc_req*))snc_meister_del_queue_svc;
            break;

        default:
            svcerr_noproc(transp);
            return;
    }
    memset((char*)&argument, 0, sizeof(argument));
    if (!svc_getargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
        svcerr_decode(transp);
        return;
    }
    result = (*local)((char*)&argument, rqstp);
    if (result != NULL && !svc_sendreply(transp, (xdrproc_t)_xdr_result, result)) {
        svcerr_systemerr(transp);
    }
    if (!svc_freeargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
        cerr << "Unable to free arguments" << endl;
    }
}

int main(int argc, char** argv)
{
    // Create NC
    nc = new SNC();

    // Unregister SNC-Meister RPC handlers
    pmap_unset(SNC_MEISTER_PROGRAM, SNC_MEISTER_V1);

    // Replace tcp RPC handlers
    register SVCXPRT *transp;
    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        cerr << "Failed to create tcp service" << endl;
        delete nc;
        return 1;
    }
    if (!svc_register(transp, SNC_MEISTER_PROGRAM, SNC_MEISTER_V1, snc_meister_program, IPPROTO_TCP)) {
        cerr << "Failed to register tcp SNC-Meister" << endl;
        delete nc;
        return 1;
    }

    // Run proxy
    svc_run();
    cerr << "svc_run returned" << endl;
    delete nc;
    return 1;
}
