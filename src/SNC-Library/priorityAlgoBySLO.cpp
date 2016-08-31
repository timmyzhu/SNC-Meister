// priorityAlgoBySLO.hpp - Code for configuring priorities in order of SLO.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <algorithm>
#include <vector>
#include "NC.hpp"
#include "priorityAlgoBySLO.hpp"

using namespace std;

// Comparison function for sorting clients by SLO.
bool SLOCompare(const Client* c1, const Client* c2)
{
    return (c1->SLO < c2->SLO);
}

// Configure priorities in order of SLO where the tightest SLO has the highest priority.
void configurePrioritiesBySLO(NC* nc)
{
    // Sort clients by SLO
    vector<const Client*> clientList;
    for (map<ClientId, Client*>::const_iterator it = nc->clientsBegin(); it != nc->clientsEnd(); it++) {
        clientList.push_back(it->second);
    }
    sort(clientList.begin(), clientList.end(), SLOCompare);

    // Assign priorities by SLO
    unsigned int priority = 0;
    double currentSLO = 0;
    for (unsigned int index = 0; index < clientList.size(); index++) {
        const Client* c = clientList[index];
        if (c->SLO > currentSLO) {
            currentSLO = c->SLO;
            priority++;
        }
        // Set priorities for each flow
        for (unsigned int flowIndex = 0; flowIndex < c->flowIds.size(); flowIndex++) {
            FlowId flowId = c->flowIds[flowIndex];
            nc->setFlowPriority(flowId, priority);
        }
    }
}
