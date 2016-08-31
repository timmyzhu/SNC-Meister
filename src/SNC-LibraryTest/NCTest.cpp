// NCTest.cpp - NC test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <cstdlib>
#include <iostream>
#include "../SNC-Library/NC.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

class TestNC : public NC
{
private:
    void testAddQueue()
    {
        Json::Value queueInfo;
        queueInfo["name"] = Json::Value("Q0");
        queueInfo["bandwidth"] = Json::Value(1);
        addQueue(queueInfo);

        QueueId queueId = getQueueIdByName("Q0");
        const Queue* q = getQueue(queueId);
        assert(q->queueId == queueId);
        assert(q->name == "Q0");
        assert(q->flows.empty());
        assert(q->bandwidth == 1);
        map<QueueId, Queue*>::const_iterator it = queuesBegin();
        assert(it->first == queueId);
        assert(it->second == q);
        it++;
        assert(it == queuesEnd());

        queueInfo["name"] = Json::Value("Q1");
        addQueue(queueInfo);

        queueId = getQueueIdByName("Q0");
        q = getQueue(queueId);
        assert(q->queueId == queueId);
        assert(q->name == "Q0");
        assert(q->flows.empty());
        assert(q->bandwidth == 1);
        queueId = getQueueIdByName("Q1");
        q = getQueue(queueId);
        assert(q->queueId == queueId);
        assert(q->name == "Q1");
        assert(q->flows.empty());
        assert(q->bandwidth == 1);
        it = queuesBegin();
        it++;
        it++;
        assert(it == queuesEnd());
    }
    void testDelQueue()
    {
        QueueId queueId = getQueueIdByName("Q0");
        delQueue(queueId);

        queueId = getQueueIdByName("Q1");
        const Queue* q = getQueue(queueId);
        assert(q->queueId == queueId);
        assert(q->name == "Q1");
        assert(q->flows.empty());
        assert(q->bandwidth == 1);
        map<QueueId, Queue*>::const_iterator it = queuesBegin();
        assert(it->first == queueId);
        assert(it->second == q);
        it++;
        assert(it == queuesEnd());

        delQueue(queueId);
        assert(queuesBegin() == queuesEnd());
    }
    void testAddClient()
    {
        Json::Value queueList(Json::arrayValue);
        queueList.append(Json::Value("Q0"));
        queueList.append(Json::Value("Q1"));

        Json::Value clientInfo;
        clientInfo["flows"] = Json::arrayValue;
        clientInfo["flows"].resize(1);
        Json::Value& flowInfo = clientInfo["flows"][0];
        flowInfo["queues"] = queueList;

        flowInfo["name"] = Json::Value("F0");
        flowInfo["priority"] = Json::Value(5);
        clientInfo["name"] = Json::Value("C0");
        clientInfo["SLO"] = Json::Value(1);
        clientInfo["SLOpercentile"] = Json::Value(99.9);
        addClient(clientInfo);

        ClientId clientId = getClientIdByName("C0");
        const Client* c = getClient(clientId);
        FlowId flowId = getFlowIdByName("F0");
        const Flow* f = getFlow(flowId);
        QueueId queueId0 = getQueueIdByName("Q0");
        QueueId queueId1 = getQueueIdByName("Q1");
        assert(c->clientId == clientId);
        assert(c->name == "C0");
        assert(c->flowIds.size() == 1);
        assert(c->flowIds[0] == flowId);
        assert(c->SLO == 1);
        assert(c->SLOpercentile == 99.9);
        assert(f->flowId == flowId);
        assert(f->name == "F0");
        assert(f->clientId == clientId);
        assert(f->queueIds.size() == 2);
        assert(f->queueIds[0] == queueId0);
        assert(f->queueIds[1] == queueId1);
        assert(f->priority == 5);
        map<ClientId, Client*>::const_iterator itC = clientsBegin();
        assert(itC->first == clientId);
        assert(itC->second == c);
        itC++;
        assert(itC == clientsEnd());
        map<FlowId, Flow*>::const_iterator itF = flowsBegin();
        assert(itF->first == flowId);
        assert(itF->second == f);
        itF++;
        assert(itF == flowsEnd());

        flowInfo["name"] = Json::Value("F1");
        flowInfo["priority"] = Json::Value(6);
        clientInfo["name"] = Json::Value("C1");
        clientInfo["SLO"] = Json::Value(2);
        clientInfo["SLOpercentile"] = Json::Value(99);
        addClient(clientInfo);

        clientId = getClientIdByName("C0");
        c = getClient(clientId);
        flowId = getFlowIdByName("F0");
        f = getFlow(flowId);
        assert(c->clientId == clientId);
        assert(c->name == "C0");
        assert(c->flowIds.size() == 1);
        assert(c->flowIds[0] == flowId);
        assert(c->SLO == 1);
        assert(c->SLOpercentile == 99.9);
        assert(f->flowId == flowId);
        assert(f->name == "F0");
        assert(f->clientId == clientId);
        assert(f->queueIds.size() == 2);
        assert(f->queueIds[0] == queueId0);
        assert(f->queueIds[1] == queueId1);
        assert(f->priority == 5);
        clientId = getClientIdByName("C1");
        c = getClient(clientId);
        flowId = getFlowIdByName("F1");
        f = getFlow(flowId);
        assert(c->clientId == clientId);
        assert(c->name == "C1");
        assert(c->flowIds.size() == 1);
        assert(c->flowIds[0] == flowId);
        assert(c->SLO == 2);
        assert(c->SLOpercentile == 99);
        assert(f->flowId == flowId);
        assert(f->name == "F1");
        assert(f->clientId == clientId);
        assert(f->queueIds.size() == 2);
        assert(f->queueIds[0] == queueId0);
        assert(f->queueIds[1] == queueId1);
        assert(f->priority == 6);
        itC = clientsBegin();
        itC++;
        itC++;
        assert(itC == clientsEnd());
        itF = flowsBegin();
        itF++;
        itF++;
        assert(itF == flowsEnd());
    }
    void testDelClient()
    {
        ClientId clientId = getClientIdByName("C0");
        delClient(clientId);

        clientId = getClientIdByName("C1");
        const Client* c = getClient(clientId);
        FlowId flowId = getFlowIdByName("F1");
        const Flow* f = getFlow(flowId);
        QueueId queueId0 = getQueueIdByName("Q0");
        QueueId queueId1 = getQueueIdByName("Q1");
        assert(c->clientId == clientId);
        assert(c->name == "C1");
        assert(c->flowIds.size() == 1);
        assert(c->flowIds[0] == flowId);
        assert(c->SLO == 2);
        assert(c->SLOpercentile == 99);
        assert(f->flowId == flowId);
        assert(f->name == "F1");
        assert(f->clientId == clientId);
        assert(f->queueIds.size() == 2);
        assert(f->queueIds[0] == queueId0);
        assert(f->queueIds[1] == queueId1);
        assert(f->priority == 6);
        map<ClientId, Client*>::const_iterator itC = clientsBegin();
        assert(itC->first == clientId);
        assert(itC->second == c);
        itC++;
        assert(itC == clientsEnd());
        map<FlowId, Flow*>::const_iterator itF = flowsBegin();
        assert(itF->first == flowId);
        assert(itF->second == f);
        itF++;
        assert(itF == flowsEnd());

        delClient(clientId);
        assert(clientsBegin() == clientsEnd());
        assert(flowsBegin() == flowsEnd());
    }

public:
    TestNC() {}
    virtual ~TestNC() {}

    virtual double calcFlowLatency(FlowId flowId)
    {
        Flow* f = const_cast<Flow*>(getFlow(flowId));
        f->latency = 1.0;
        return f->latency;
    }

    void test()
    {
        // Test invalid names/ids
        assert(getFlowIdByName("INVALID") == InvalidFlowId);
        assert(getClientIdByName("INVALID") == InvalidClientId);
        assert(getQueueIdByName("INVALID") == InvalidQueueId);
        assert(getFlow(InvalidFlowId) == NULL);
        assert(getClient(InvalidClientId) == NULL);
        assert(getQueue(InvalidQueueId) == NULL);
        // Test addQueue
        testAddQueue();
        // Test addClient
        testAddClient();
        // Test delClient
        testDelClient();
        // Test delQueue
        testDelQueue();
        // Test addQueue after delete
        testAddQueue();
        // Test addClient after delete
        testAddClient();
        // Test setFlowPriority
        FlowId flowId0 = getFlowIdByName("F0");
        FlowId flowId1 = getFlowIdByName("F1");
        setFlowPriority(flowId0, 4);
        setFlowPriority(flowId1, 7);
        const Flow* f0 = getFlow(flowId0);
        const Flow* f1 = getFlow(flowId1);
        assert(f0->priority == 4);
        assert(f1->priority == 7);
        // Test priorityCompare
        assert(priorityCompare(f0, f1) == true);
        assert(priorityCompare(f1, f0) == false);
        // Test calcClientLatency
        ClientId clientId0 = getClientIdByName("C0");
        ClientId clientId1 = getClientIdByName("C1");
        const Client* c0 = getClient(clientId0);
        const Client* c1 = getClient(clientId1);
        const_cast<Client*>(c0)->latency = 0;
        const_cast<Client*>(c1)->latency = 0;
        const_cast<Flow*>(f0)->latency = 0;
        const_cast<Flow*>(f1)->latency = 0;
        calcClientLatency(clientId0);
        assert(c0->latency == 1);
        assert(c1->latency == 0);
        assert(f0->latency == 1);
        assert(f1->latency == 0);
        // Test calcAllLatency
        calcAllLatency();
        assert(c0->latency == 1);
        assert(c1->latency == 1);
        assert(f0->latency == 1);
        assert(f1->latency == 1);
    }
};

struct TestFlow : Flow {
    int data;
};

struct TestClient : Client {
    int data;
};

struct TestQueue : Queue {
    int data;
};

class TestNCOverride : public TestNC
{
protected:
    virtual FlowId initFlow(Flow* f, const Json::Value& flowInfo, ClientId clientId)
    {
        assert(f == NULL);
        TestFlow* testF = new TestFlow;
        testF->data = 7;
        FlowId flowId = NC::initFlow(testF, flowInfo, clientId);
        assert(getTestFlow(flowId)->data == 7);
        return flowId;
    }
    virtual ClientId initClient(Client* c, const Json::Value& clientInfo)
    {
        assert(c == NULL);
        TestClient* testC = new TestClient;
        testC->data = 5;
        ClientId clientId = NC::initClient(testC, clientInfo);
        assert(getTestClient(clientId)->data == 5);
        return clientId;
    }
    virtual QueueId initQueue(Queue* q, const Json::Value& queueInfo)
    {
        assert(q == NULL);
        TestQueue* testQ = new TestQueue;
        testQ->data = 3;
        QueueId queueId = NC::initQueue(testQ, queueInfo);
        assert(getTestQueue(queueId)->data == 3);
        return queueId;
    }

    TestFlow* getTestFlow(FlowId flowId) { return static_cast<TestFlow*>(const_cast<Flow*>(getFlow(flowId))); }
    TestClient* getTestClient(ClientId clientId) { return static_cast<TestClient*>(const_cast<Client*>(getClient(clientId))); }
    TestQueue* getTestQueue(QueueId queueId) { return static_cast<TestQueue*>(const_cast<Queue*>(getQueue(queueId))); }

public:
    TestNCOverride() {}
    virtual ~TestNCOverride() {}
};

void NCTest()
{
    TestNC testNC;
    testNC.test();
    TestNCOverride testNCOverride;
    testNCOverride.test();
    cout << "PASS NCTest" << endl;
}
