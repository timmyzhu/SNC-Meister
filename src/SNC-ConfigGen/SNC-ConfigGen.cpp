// SNC-ConfigGen.cpp - Generate config files based on a high level topology file.
// Takes a high level topology file in JSON format with the information about the traces, SLOs, and client/server locations
// and outputs a config file for use with SNC-Calculator and SNC-MeisterClient.
// SNC-ConfigGen encodes information about the system (e.g., network bandwidth).
//
// The format of the JSON topology file is as follows (examples in examples/ directory):
// "debug": int (optional) - set to 1 to enable extra debug information
// "addrPrefix": string (optional) - prefix for the hostname of the client/server VMs; assumes VM hostnames are named according to the getAddr function below. If addrPrefix is not specified, SNC-Meister will not update enforcers
// "outputConfig": string - file path to write output
// "clients": list client - list of clients (a.k.a. tenants)
//
// Each client is structured as follows:
// "name": string (optional) - name of client; defaults to getClientName function below
// "SLO": float - client's desired tail latency goal in seconds
// "SLOpercentile": float (optional) - client's desired tail latency percentile for the target SLO between 0 and 100; defaults to 99.9
// "trace": string - file path of trace file describing client behavior; see SNC-Library/TraceReader.hpp for trace file format
// "clientHost": string - hostname of machine that hosts the client VM
// "clientVM": string - identifier for the client VM
// "serverHost": string - hostname of machine that hosts the server VM
// "serverVM": string - identifier for the server VM
// "dependencies": list string (optional) - list of other client names that are dependent on this client; defaults to no dependencies
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <set>
#include <unistd.h>
#include "../json/json.h"
#include "../SNC-Library/SNC.hpp"

using namespace std;

const double NETWORK_BANDWIDTH = 125000000; // bytes/sec

// Return a name for client based on index in client list
string getClientName(unsigned int clientIndex)
{
    ostringstream oss;
    oss << "C" << clientIndex;
    return oss.str();
}

// Return a name for flow into server based on index in client list
string getFlowNetworkInName(unsigned int clientIndex)
{
    ostringstream oss;
    oss << "F" << clientIndex << "In";
    return oss.str();
}

// Return a name for flow out of server based on index in client list
string getFlowNetworkOutName(unsigned int clientIndex)
{
    ostringstream oss;
    oss << "F" << clientIndex << "Out";
    return oss.str();
}

// Return a name for queue into host machine
string getQueueInName(string host)
{
    return host + "-in";
}

// Return a name for queue out of host machine
string getQueueOutName(string host)
{
    return host + "-out";
}

// Return the hostname of a particular VM
string getAddr(string prefix, string host, string VM)
{
    return prefix + "-" + host + "vm" + VM;
}

// Generate config file based on topology file.
void configGen(const char* topoFilename, const char* configFilename)
{
    // Open topology file
    ifstream inputFile(topoFilename);
    if (!inputFile.good()) {
        cerr << "Failed to read topology file " << topoFilename << endl;
        return;
    }
    // Parse file
    Json::Reader reader;
    Json::Value rootConfig;
    if (!reader.parse(inputFile, rootConfig)) {
        cerr << "Failed to parse topology file " << topoFilename << endl;
        return;
    }
    // Setup estimator info
    Json::Value networkInEstimatorInfo;
    networkInEstimatorInfo["type"] = Json::Value("networkIn");
    networkInEstimatorInfo["nonDataConstant"] = Json::Value(200.0);
    networkInEstimatorInfo["nonDataFactor"] = Json::Value(0.025);
    networkInEstimatorInfo["dataConstant"] = Json::Value(200.0);
    networkInEstimatorInfo["dataFactor"] = Json::Value(1.1);
    Json::Value networkOutEstimatorInfo;
    networkOutEstimatorInfo["type"] = Json::Value("networkOut");
    networkOutEstimatorInfo["nonDataConstant"] = Json::Value(200.0);
    networkOutEstimatorInfo["nonDataFactor"] = Json::Value(0.025);
    networkOutEstimatorInfo["dataConstant"] = Json::Value(200.0);
    networkOutEstimatorInfo["dataFactor"] = Json::Value(1.1);
    // Generate clients
    set<string> hosts;
    Json::Value& clientInfos = rootConfig["clients"];
    for (unsigned int clientIndex = 0; clientIndex < clientInfos.size(); clientIndex++) {
        Json::Value& clientInfo = clientInfos[clientIndex];
        if (!clientInfo.isMember("name")) {
            clientInfo["name"] = Json::Value(getClientName(clientIndex));
        }
        string clientHost = clientInfo["clientHost"].asString();
        string clientVM = clientInfo["clientVM"].asString();
        string serverHost = clientInfo["serverHost"].asString();
        string serverVM = clientInfo["serverVM"].asString();
        clientInfo.removeMember("clientHost");
        clientInfo.removeMember("clientVM");
        clientInfo.removeMember("serverHost");
        clientInfo.removeMember("serverVM");
        hosts.insert(clientHost);
        hosts.insert(serverHost);
        string clientAddr;
        string serverAddr;
        if (rootConfig.isMember("addrPrefix")) {
            clientAddr = getAddr(rootConfig["addrPrefix"].asString(), clientHost, clientVM);
            serverAddr = getAddr(rootConfig["addrPrefix"].asString(), serverHost, serverVM);
        }
        Json::Value& clientFlows = clientInfo["flows"];
        clientFlows = Json::arrayValue;
        clientFlows.resize(2);
        // Setup flow from client to server
        Json::Value& flowInInfo = clientFlows[0];
        flowInInfo["name"] = Json::Value(getFlowNetworkInName(clientIndex));
        if (rootConfig.isMember("addrPrefix")) {
            flowInInfo["enforcerAddr"] = Json::Value(clientHost);
            flowInInfo["dstAddr"] = Json::Value(serverAddr);
            flowInInfo["srcAddr"] = Json::Value(clientAddr);
        }
        Json::Value& flowInQueues = flowInInfo["queues"];
        flowInQueues = Json::arrayValue;
        flowInQueues.resize(2);
        flowInQueues[0] = Json::Value(getQueueOutName(clientHost));
        flowInQueues[1] = Json::Value(getQueueInName(serverHost));
        SNC::setArrivalInfo(flowInInfo, clientInfo["trace"].asString(), networkInEstimatorInfo);
        // Setup flow from server to client
        Json::Value& flowOutInfo = clientFlows[1];
        flowOutInfo["name"] = Json::Value(getFlowNetworkOutName(clientIndex));
        if (rootConfig.isMember("addrPrefix")) {
            flowOutInfo["enforcerAddr"] = Json::Value(serverHost);
            flowOutInfo["dstAddr"] = Json::Value(clientAddr);
            flowOutInfo["srcAddr"] = Json::Value(serverAddr);
        }
        Json::Value& flowOutQueues = flowOutInfo["queues"];
        flowOutQueues = Json::arrayValue;
        flowOutQueues.resize(2);
        flowOutQueues[0] = Json::Value(getQueueOutName(serverHost));
        flowOutQueues[1] = Json::Value(getQueueInName(clientHost));
        SNC::setArrivalInfo(flowOutInfo, clientInfo["trace"].asString(), networkOutEstimatorInfo);
    }
    // Generate queues
    Json::Value& queueInfos = rootConfig["queues"];
    queueInfos = Json::arrayValue;
    queueInfos.resize(hosts.size() * 2);
    unsigned int queueInfoIndex = 0;
    for (set<string>::const_iterator it = hosts.begin(); it != hosts.end(); it++) {
        string host = *it;
        Json::Value& queueInInfo = queueInfos[queueInfoIndex];
        queueInInfo["name"] = Json::Value(getQueueInName(host));
        queueInInfo["bandwidth"] = Json::Value(NETWORK_BANDWIDTH);
        queueInfoIndex++;
        Json::Value& queueOutInfo = queueInfos[queueInfoIndex];
        queueOutInfo["name"] = Json::Value(getQueueOutName(host));
        queueOutInfo["bandwidth"] = Json::Value(NETWORK_BANDWIDTH);
        queueInfoIndex++;
    }
    // Open config file
    ofstream outputFile(configFilename);
    if (!outputFile.good()) {
        cerr << "Failed to open output file " << configFilename << endl;
        return;
    }
    // Output config file
    Json::StyledStreamWriter writer;
    writer.write(outputFile, rootConfig);
}

// Print the current date/time.
void printTime()
{
    time_t t;
    time(&t);
    cout << ctime(&t);
}

int main(int argc, char** argv)
{
    int opt = 0;
    char* topoFilename = NULL;
    char* configFilename = NULL;
    do {
        opt = getopt(argc, argv, "t:c:");
        switch (opt) {
            case 't':
                topoFilename = optarg;
                break;

            case 'c':
                configFilename = optarg;
                break;

            case -1:
                break;

            default:
                break;
        }
    } while (opt != -1);

    if ((topoFilename == NULL) || (configFilename == NULL)) {
        cout << "Usage: " << argv[0] << " -t topoFilename -c configFilename" << endl;
        return -1;
    }

    printTime();

    configGen(topoFilename, configFilename);

    printTime();
    return 0;
}
