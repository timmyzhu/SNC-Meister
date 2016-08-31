// SNC-Calculator.cpp - Calculate latency based on stochastic network calculus (SNC).
// Takes a config file in JSON format with the information about the system and calculates the latency.
// See SNC-ConfigGen for how to generate config files based on higher level input.
// The format of the JSON config file is as follows (examples in examples/ directory):
// "clients": list client - list of all clients (see NC.hpp for format)
// "queues": list queue - list of all queues (see NC.hpp for format)
// "outputConfig": string - file path to write output
// "debug": int (optional) - if field exists, latency of flows and clients will be output to stdout
//
// Additionally, dependencies between clients can be specified in each clientInfo:
// "dependencies": list string (optional, SNC) - list of other client names that are dependent on this client; defaults to no dependencies
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include "../json/json.h"
#include "../SNC-Library/NC.hpp"
#include "../SNC-Library/SNC.hpp"
#include "../SNC-Library/priorityAlgoBySLO.hpp"

using namespace std;

// Print the current date/time.
void printTime()
{
    time_t t;
    time(&t);
    cout << ctime(&t);
}

// Initialize network calculus calculator based on config file.
void initNC(NC* nc, Json::Value& rootConfig)
{
    // Add queues
    const Json::Value& queueInfos = rootConfig["queues"];
    for (unsigned int queueInfoIndex = 0; queueInfoIndex < queueInfos.size(); queueInfoIndex++) {
        const Json::Value& queueInfo = queueInfos[queueInfoIndex];
        nc->addQueue(queueInfo);
    }
    // Add clients
    const Json::Value& clientInfos = rootConfig["clients"];
    for (unsigned int clientInfoIndex = 0; clientInfoIndex < clientInfos.size(); clientInfoIndex++) {
        const Json::Value& clientInfo = clientInfos[clientInfoIndex];
        nc->addClient(clientInfo);
    }
    // Add dependencies
    for (unsigned int clientInfoIndex = 0; clientInfoIndex < clientInfos.size(); clientInfoIndex++) {
        const Json::Value& clientInfo = clientInfos[clientInfoIndex];
        if (clientInfo.isMember("dependencies")) {
            ClientId clientId = nc->getClientIdByName(clientInfo["name"].asString());
            assert(clientId != InvalidClientId);
            const Json::Value& dependencies = clientInfo["dependencies"];
            for (unsigned int i = 0; i < dependencies.size(); i++) {
                ClientId dependency = nc->getClientIdByName(dependencies[i].asString());
                assert(dependency != InvalidClientId);
                nc->addDependency(clientId, dependency);
            }
        }
    }
}

// Append latency and priority information to config file and output to file.
void genOutput(NC* nc, Json::Value& rootConfig, ofstream& outputFile)
{
    bool feasible = true;
    // Add client latencies
    Json::Value& clientInfos = rootConfig["clients"];
    for (unsigned int clientInfoIndex = 0; clientInfoIndex < clientInfos.size(); clientInfoIndex++) {
        Json::Value& clientInfo = clientInfos[clientInfoIndex];
        ClientId clientId = nc->getClientIdByName(clientInfo["name"].asString());
        const Client* c = nc->getClient(clientId);
        // Assign latency
        clientInfo["latency"] = Json::Value(c->latency);
        if (c->latency > c->SLO) {
            feasible = false;
        }
        // Add flow latencies and priorities
        Json::Value& flowInfos = clientInfo["flows"];
        for (unsigned int flowIndex = 0; flowIndex < flowInfos.size(); flowIndex++) {
            Json::Value& flowInfo = flowInfos[flowIndex];
            FlowId flowId = nc->getFlowIdByName(flowInfo["name"].asString());
            const Flow* f = nc->getFlow(flowId);
            assert(f->name == flowInfo["name"].asString());
            // Assign latency
            flowInfo["latency"] = Json::Value(f->latency);
            // Assign priority
            flowInfo["priority"] = Json::Value(f->priority);
        }
    }
    // Output config file
    Json::StyledStreamWriter writer;
    writer.write(outputFile, rootConfig);
    // Print debug output
    if (rootConfig.isMember("debug")) {
        for (unsigned int clientInfoIndex = 0; clientInfoIndex < clientInfos.size(); clientInfoIndex++) {
            Json::Value& clientInfo = clientInfos[clientInfoIndex];
            ClientId clientId = nc->getClientIdByName(clientInfo["name"].asString());
            const Client* c = nc->getClient(clientId);
            cout << "Client: " << c->name << endl;
            for (unsigned int flowIndex = 0; flowIndex < c->flowIds.size(); flowIndex++) {
                const Flow* f = nc->getFlow(c->flowIds[flowIndex]);
                cout << f->name << " Latency: " << f->latency << endl;
            }
            cout << "Latency: " << c->latency << " SLO: " << c->SLO << endl;
        }
    }
    if (feasible) {
        cout << "feasible" << endl;
    } else {
        cout << "infeasible" << endl;
    }
}

int main(int argc, char** argv)
{
    int opt = 0;
    char* configFilename = NULL;
    do {
        opt = getopt(argc, argv, "c:");
        switch (opt) {
            case 'c':
                configFilename = optarg;
                break;

            case -1:
                break;

            default:
                break;
        }
    } while (opt != -1);

    if (configFilename == NULL) {
        cout << "Usage: " << argv[0] << " -c configFilename" << endl;
        return -1;
    }

    printTime();

    // Open config file
    ifstream inputFile(configFilename);
    if (!inputFile.good()) {
        cerr << "Failed to read config file " << configFilename << endl;
        return -1;
    }
    // Parse file
    Json::Reader reader;
    Json::Value rootConfig;
    if (!reader.parse(inputFile, rootConfig)) {
        cerr << "Failed to parse config file " << configFilename << endl;
        return -1;
    }
    // Initialize NC
    NC* nc = new SNC();
    initNC(nc, rootConfig);

    // Configure priorities
    configurePrioritiesBySLO(nc);

    // Calculate latencies
    nc->calcAllLatency();

    // Open output file
    ofstream outputFile(rootConfig["outputConfig"].asCString());
    if (!outputFile.good()) {
        cerr << "Failed to open output file " << rootConfig["outputConfig"].asString() << endl;
        delete nc;
        return -1;
    }
    // Output result
    genOutput(nc, rootConfig, outputFile);

    printTime();
    delete nc;
    return 0;
}
