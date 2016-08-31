// NetEnforcer.cpp - Network traffic enforcement.
// NetEnforcer configures Linux Traffic Control (TC) at each host machine to enforce priorities on network traffic.
// NetEnforcer is run on the machines hosting the VMs, and is configured through the NetEnforcer_prot RPC interface (see prot/NetEnforcer_prot.x).
//
// TC allows for a hierarchy of queueing disciplines (qdisc) and classes to manage network QoS.
// TC identifies qdiscs by a handle (e.g., [handle:]). TC identifies classes by a handle and minor number (e.g., [handle:minor]).
//
// NetEnforcer configures TC as follows:
// The root qdisc is a Hierarchical Token Bucket (HTB) with handle [1:].
// Within the root HTB qdisc, there is a tree structure of priority levels, starting with [1:rootHTBMinorHelper(0)].
// [1:rootHTBMinorHelper(0)] branches off into the class representing priority 0, [1:rootHTBMinor(0)], and the class representing the priorities higher than 0, [1:rootHTBMinorHelper(1)].
// [1:rootHTBMinorHelper(1)] branches off into the class representing priority 1, [1:rootHTBMinor(1)], and the class representing the priorities higher than 1, [1:rootHTBMinorHelper(2)].
// This sequence repeats until the last priority level, [1:rootHTBMinor(g_numPriorities - 1)], and the remaining best effort class, [1:rootHTBMinorDefault()].
//
// After this root HTB qdisc, there are DSMARK qdiscs attached to each priority level to tag the DSCP flags.
// For each priority level, there is a DSMARK qdisc with handle [DSMARKHandle(priority):] as a child of the priority level in the root HTB (i.e., [1:rootHTBMinor(priority)]).
// Each DSMARK qdisc, [DSMARKHandle(priority):], has one class [DSMARKHandle(priority):1], which performs the DSCP flag marking.
//
// The entire qdisc/class hierarchy is as follows:
//                           [1:]                                                                                                                              |
//                            |                                                                                                                                |
//                [1:rootHTBMinorHelper(0)]                                                                                                                    |
//                /                       \                                                                                                                    |
// [1:rootHTBMinor(0)]                 [1:rootHTBMinorHelper(1)]                                                                                               |
//          |                          /                       \                                                                                               |
// [DSMARKHandle(0):]   [1:rootHTBMinor(1)]                 [1:rootHTBMinorHelper(2)]                                                                          |
//          |                    |                          /                       \                                                                          |
// [DSMARKHandle(0):1]  [DSMARKHandle(1):]   [1:rootHTBMinor(2)]                 [1:rootHTBMinorHelper(3)]                                                     |
//                               |                    |                          /                       \                                                     |
//                      [DSMARKHandle(1):1]  [DSMARKHandle(2):]   [1:rootHTBMinor(3)]                 [1:rootHTBMinorHelper(4)]                                |
//                                                    |                    |                          /                       \                                |
//                                                   ...                  ...                        ...                     ...                               |
//                                                                                                              /                       \                      |
//                                                                       [1:rootHTBMinorHelper(g_numPriorities - 1)]                 [1:rootHTBMinorDefault()] |
//
// Lastly, as clients are added, src/dst filters are setup to send packets to the corresponding queue for its priority level.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include "../prot/NetEnforcer_prot.h"

#define MAX_CMD_SIZE 256

using namespace std;

struct Client {
    unsigned int id;
    unsigned int priority;
};

// Globals
map<pair<unsigned long, unsigned long>, Client> g_clients;
unsigned int g_nextId = 1;
string g_dev = "eth0";
unsigned int g_maxRate = 125000000; // bytes per second
unsigned int g_numPriorities = 7;

// Handle for root HTB qdisc
unsigned int rootHTBHandle()
{
    return 1;
}

// Minor number within root HTB for class representing queue of a given priority level; starts at 1
unsigned int rootHTBMinor(unsigned int priority)
{
    return priority + 1;
}

// Minor number within root HTB for class helping to represent queue of a given priority level; starts after rootHTBMinor
unsigned int rootHTBMinorHelper(unsigned int priority)
{
    return priority + rootHTBMinor(g_numPriorities);
}

// Minor number within root HTB for default class; must start after rootHTBMinorHelper
unsigned int rootHTBMinorDefault()
{
    return rootHTBMinorHelper(g_numPriorities);
}

// Handle for DSMARK qdisc; starts after rootHTBMinorDefault to avoid confusion from reusing numbers
unsigned int DSMARKHandle(unsigned int priority)
{
    return priority + rootHTBMinorDefault() + 1;
}

// Execute a command and return the output as a string
string runCmd(char* cmd)
{
    FILE* pipe = popen(cmd, "r");
    if (pipe == NULL) {
        cerr << "Error running command: " << cmd << endl;
        return "";
    }
    char buf[128];
    string result = "";
    while (!feof(pipe)) {
        if (fgets(buf, 128, pipe) != NULL) {
            result += buf;
        }
    }
    pclose(pipe);
    return result;
}

// Remove the root qdisc in TC
void removeRoot()
{
    char cmd[MAX_CMD_SIZE];
    snprintf(cmd, MAX_CMD_SIZE,
             "tc qdisc del dev %s root",
             g_dev.c_str());
    runCmd(cmd);
}

// Remove a filter in TC from qdisc [parentHandle:] for a client with given id
void removeFilter(unsigned int parentHandle, unsigned int id)
{
    // We overload prio to be the client id to make the filter easy to identify when removing it.
    // Since only one filter should target a client, setting prio should not have any effect.
    char cmd[MAX_CMD_SIZE];
    snprintf(cmd, MAX_CMD_SIZE,
             "tc filter del dev %s parent %d: prio %d u32",
             g_dev.c_str(),
             parentHandle,
             id);
    runCmd(cmd);
}

// Add a filter in TC to qdisc [parentHandle:] for a client with given id
// Causes packets with given src/dst to use class [parentHandle:minor]
void addFilter(unsigned int parentHandle, unsigned int id, unsigned long s_dstAddr, unsigned long s_srcAddr, unsigned int minor)
{
    // Convert address to string
    char dstAddrStr[INET_ADDRSTRLEN];
    char srcAddrStr[INET_ADDRSTRLEN];
    struct in_addr dstAddr;
    struct in_addr srcAddr;
    dstAddr.s_addr = s_dstAddr;
    inet_ntop(AF_INET, &dstAddr, dstAddrStr, INET_ADDRSTRLEN);
    srcAddr.s_addr = s_srcAddr;
    inet_ntop(AF_INET, &srcAddr, srcAddrStr, INET_ADDRSTRLEN);

    // We overload prio to be the client id to make the filter easy to identify when removing it.
    // Since only one filter should target a client, setting prio should not have any effect.
    char cmd[MAX_CMD_SIZE];
    snprintf(cmd, MAX_CMD_SIZE,
             "tc filter add dev %s parent %d: protocol ip prio %d u32 match ip dst %s match ip src %s flowid %d:%d",
             g_dev.c_str(),
             parentHandle,
             id,
             dstAddrStr,
             srcAddrStr,
             parentHandle,
             minor);
    runCmd(cmd);
}

// Initialize TC with our basic qdisc/class structure (see file header)
void initTC()
{
    // Remove root to start at a clean slate
    removeRoot();
    // Reserve 1% of bandwidth for each priority level, and assign remaining bandwidth to highest priority
    const unsigned int minRate = g_maxRate / 100; // bps
    unsigned int rate = minRate * (g_numPriorities + 1);
    unsigned int ceil = g_maxRate;
    char cmd[MAX_CMD_SIZE];
    // Create root HTB qdisc [1:]
    snprintf(cmd, MAX_CMD_SIZE,
             "tc qdisc add dev %s root handle 1: htb default %d",
             g_dev.c_str(),
             rootHTBMinorDefault());
    runCmd(cmd);
    // Create root HTB class [1:rootHTBMinorHelper(0)]
    snprintf(cmd, MAX_CMD_SIZE,
             "tc class add dev %s parent 1: classid 1:%d htb rate %dbps prio %d",
             g_dev.c_str(),
             rootHTBMinorHelper(0),
             g_maxRate,
             0);
    runCmd(cmd);
    for (unsigned int priority = 0; priority < g_numPriorities; priority++) {
        // Create root HTB class [1:rootHTBMinor(priority)]
        snprintf(cmd, MAX_CMD_SIZE,
                 "tc class add dev %s parent 1:%d classid 1:%d htb rate %dbps ceil %dbps prio %d",
                 g_dev.c_str(),
                 rootHTBMinorHelper(priority),
                 rootHTBMinor(priority),
                 minRate,
                 ceil,
                 priority);
        runCmd(cmd);
        // Add DSMARK qdisc [DSMARKHandle(priority):]
        snprintf(cmd, MAX_CMD_SIZE,
                 "tc qdisc add dev %s parent 1:%d handle %d: dsmark indices 2 default_index 1",
                 g_dev.c_str(),
                 rootHTBMinor(priority),
                 DSMARKHandle(priority));
        runCmd(cmd);
        // Set DSCP flag for DSMARK class [DSMARKHandle(priority):1]
        // Highest priority (0) is cs7 (0b11100000)
        unsigned char value = (7 - priority) << 5;
        snprintf(cmd, MAX_CMD_SIZE,
                 "tc class change dev %s classid %d:1 dsmark mask 0x3 value 0x%x", // must be change, not add
                 g_dev.c_str(),
                 DSMARKHandle(priority),
                 value);
        runCmd(cmd);
        // Create root HTB class [1:rootHTBMinorHelper(priority + 1)]
        rate -= minRate;
        ceil -= minRate;
        snprintf(cmd, MAX_CMD_SIZE,
                 "tc class add dev %s parent 1:%d classid 1:%d htb rate %dbps ceil %dbps prio %d",
                 g_dev.c_str(),
                 rootHTBMinorHelper(priority),
                 rootHTBMinorHelper(priority + 1),
                 rate,
                 ceil,
                 priority + 1);
        runCmd(cmd);
    }
}

// Update client with to use given priority level
void updateClient(unsigned long s_dstAddr, unsigned long s_srcAddr, unsigned int priority)
{
    if (priority >= g_numPriorities) {
        cerr << "Invalid priority: " << priority << ", must be less than " << g_numPriorities << endl;
        return;
    }
    pair<unsigned long, unsigned long> addr(s_dstAddr, s_srcAddr);
    bool newClient = (g_clients.find(addr) == g_clients.end());
    Client& c = g_clients[addr];
    if (newClient) {
        c.id = g_nextId++;
    } else if (c.priority != priority) {
        // Remove old filter
        removeFilter(rootHTBHandle(), c.id);
    } else {
        // Return without doing anything
        return;
    }
    // Add filter
    c.priority = priority;
    addFilter(rootHTBHandle(), c.id, s_dstAddr, s_srcAddr, rootHTBMinor(priority));
}

// Remove client from TC
void removeClient(unsigned long s_dstAddr, unsigned long s_srcAddr)
{
    pair<unsigned long, unsigned long> addr(s_dstAddr, s_srcAddr);
    map<pair<unsigned long, unsigned long>, Client>::iterator it = g_clients.find(addr);
    if (it != g_clients.end()) {
        const Client& c = it->second;
        // Remove filter
        removeFilter(rootHTBHandle(), c.id);
        // Remove from map
        g_clients.erase(it);
    }
}

// UpdateClients RPC - update/add client configurations
void* net_enforcer_update_clients_svc(NetUpdateClientsArgs* argp, struct svc_req* rqstp)
{
    static char* result;
    for (unsigned int i = 0; i < argp->NetUpdateClientsArgs_len; i++) {
        const NetClientUpdate& clientUpdate = argp->NetUpdateClientsArgs_val[i];
        updateClient(clientUpdate.client.s_dstAddr,
                     clientUpdate.client.s_srcAddr,
                     clientUpdate.priority);
    }
    return (void*)&result;
}

// RemoveClients RPC - remove clients
void* net_enforcer_remove_clients_svc(NetRemoveClientsArgs* argp, struct svc_req* rqstp)
{
    static char* result;
    for (unsigned int i = 0; i < argp->NetRemoveClientsArgs_len; i++) {
        const NetClient& client = argp->NetRemoveClientsArgs_val[i];
        removeClient(client.s_dstAddr,
                     client.s_srcAddr);
    }
    return (void*)&result;
}

// Main RPC handler
void net_enforcer_program(struct svc_req* rqstp, register SVCXPRT* transp)
{
    union {
        NetUpdateClientsArgs net_enforcer_update_clients_arg;
        NetRemoveClientsArgs net_enforcer_remove_clients_arg;
    } argument;
    char* result;
    xdrproc_t _xdr_argument, _xdr_result;
    char* (*local)(char*, struct svc_req*);

    switch (rqstp->rq_proc) {
        case NET_ENFORCER_NULL:
            svc_sendreply(transp, (xdrproc_t)xdr_void, (caddr_t)NULL);
            return;

        case NET_ENFORCER_UPDATE_CLIENTS:
            _xdr_argument = (xdrproc_t)xdr_NetUpdateClientsArgs;
            _xdr_result = (xdrproc_t)xdr_void;
            local = (char* (*)(char*, struct svc_req*))net_enforcer_update_clients_svc;
            break;

        case NET_ENFORCER_REMOVE_CLIENTS:
            _xdr_argument = (xdrproc_t)xdr_NetRemoveClientsArgs;
            _xdr_result = (xdrproc_t)xdr_void;
            local = (char* (*)(char*, struct svc_req*))net_enforcer_remove_clients_svc;
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

// SIGTERM/SIGINT signal for cleanup
void term_signal(int signum)
{
    // Unregister NetEnforcer RPC handlers
    pmap_unset(NET_ENFORCER_PROGRAM, NET_ENFORCER_V1);
    // Remove TC root
    removeRoot();
    exit(0);
}

// Usage: ./NetEnforcer [-d dev] [-b maxBandwidth (in bytes per sec)] [-n numPriorities]
int main(int argc, char** argv)
{
    // Initialize globals
    int opt = 0;
    do {
        opt = getopt(argc, argv, "d:b:n:");
        switch (opt) {
            case 'd':
                g_dev.assign(optarg);
                break;

            case 'b':
                g_maxRate = atoi(optarg);
                break;

            case 'n':
                g_numPriorities = atoi(optarg);
                break;

            case -1:
                break;

            default:
                break;
        }
    } while (opt != -1);

    // Setup signal handler
    struct sigaction action;
    action.sa_handler = term_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    // Initialize TC
    initTC();

    // Unregister NetEnforcer RPC handlers
    pmap_unset(NET_ENFORCER_PROGRAM, NET_ENFORCER_V1);

    // Replace tcp RPC handlers
    register SVCXPRT *transp;
    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        cerr << "Failed to create tcp service" << endl;
        return 1;
    }
    if (!svc_register(transp, NET_ENFORCER_PROGRAM, NET_ENFORCER_V1, net_enforcer_program, IPPROTO_TCP)) {
        cerr << "Failed to register tcp NetEnforcer" << endl;
        return 1;
    }

    // Run proxy
    svc_run();
    cerr << "svc_run returned" << endl;
    return 1;
}
