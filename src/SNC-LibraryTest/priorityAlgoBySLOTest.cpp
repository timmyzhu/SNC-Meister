// priorityAlgoBySLOTest.cpp - priorityAlgoBySLO test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include "../json/json.h"
#include "../SNC-Library/NC.hpp"
#include "../SNC-Library/priorityAlgoBySLO.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

class TestNC : public NC
{
public:
    TestNC() {}
    ~TestNC() {}

    virtual double calcFlowLatency(FlowId flowId)
    {
        return 1.0;
    }
};

void priorityAlgoBySLOTest()
{
    TestNC testNC;
    Json::Value queueInfo;
    queueInfo["name"] = Json::Value("Q0");
    queueInfo["bandwidth"] = Json::Value(1);
    testNC.addQueue(queueInfo);
    Json::Value queueList(Json::arrayValue);
    queueList.append(Json::Value("Q0"));

    Json::Value clientInfo;
    clientInfo["flows"] = Json::arrayValue;
    clientInfo["flows"].resize(1);
    Json::Value& flowInfo = clientInfo["flows"][0];
    flowInfo["queues"] = queueList;

    configurePrioritiesBySLO(&testNC);

    flowInfo["name"] = Json::Value("F0");
    clientInfo["name"] = Json::Value("C0");
    clientInfo["SLO"] = Json::Value(1);
    testNC.addClient(clientInfo);

    configurePrioritiesBySLO(&testNC);
    assert(testNC.getFlow(testNC.getFlowIdByName("F0"))->priority == 0);

    flowInfo["name"] = Json::Value("F1");
    clientInfo["name"] = Json::Value("C1");
    clientInfo["SLO"] = Json::Value(0.5);
    testNC.addClient(clientInfo);

    configurePrioritiesBySLO(&testNC);
    assert(testNC.getFlow(testNC.getFlowIdByName("F0"))->priority == 1);
    assert(testNC.getFlow(testNC.getFlowIdByName("F1"))->priority == 0);

    flowInfo["name"] = Json::Value("F2");
    clientInfo["name"] = Json::Value("C2");
    clientInfo["SLO"] = Json::Value(2);
    testNC.addClient(clientInfo);

    configurePrioritiesBySLO(&testNC);
    assert(testNC.getFlow(testNC.getFlowIdByName("F0"))->priority == 1);
    assert(testNC.getFlow(testNC.getFlowIdByName("F1"))->priority == 0);
    assert(testNC.getFlow(testNC.getFlowIdByName("F2"))->priority == 2);

    flowInfo["name"] = Json::Value("F3");
    clientInfo["name"] = Json::Value("C3");
    clientInfo["SLO"] = Json::Value(1);
    testNC.addClient(clientInfo);

    configurePrioritiesBySLO(&testNC);
    assert(testNC.getFlow(testNC.getFlowIdByName("F0"))->priority == 1);
    assert(testNC.getFlow(testNC.getFlowIdByName("F1"))->priority == 0);
    assert(testNC.getFlow(testNC.getFlowIdByName("F2"))->priority == 2);
    assert(testNC.getFlow(testNC.getFlowIdByName("F3"))->priority == 1);

    cout << "PASS priorityAlgoBySLOTest" << endl;
}
