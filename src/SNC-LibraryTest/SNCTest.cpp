// SNCTest.cpp - SNC test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include "../SNC-Library/MGF.hpp"
#include "../SNC-Library/NC.hpp"
#include "../SNC-Library/SNC.hpp"
#include "../SNC-Library/SNCOperators.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

class TestMGFDeterministicArrival : public SNCArrival
{
private:
    MGFDeterministic _mgf;

public:
    TestMGFDeterministicArrival(double work, double p)
    {
        ProcessedTraceEntry traceEntry;
        traceEntry.work = work;
        _mgf.addSampleRequest(traceEntry);
        _mgf.setProbRequest(p);
    }
    virtual ~TestMGFDeterministicArrival()
    {
    }

    virtual void calcBound(double theta, double* sigma, double* rho) const
    {
        *sigma = 0;
        *rho = log(_mgf.calcMGF(theta)) / theta;
    }
};

class TestSNC : public SNC
{
private:
    double _work;
    double _p;
    double _epsilon;

protected:
    virtual FlowId initFlow(Flow* f, const Json::Value& flowInfo, ClientId clientId)
    {
        SNCFlow* sf = new SNCFlow;
        sf->arrival = new TestMGFDeterministicArrival(_work, _p);
        sf->epsilon = _epsilon;
        f = sf;
        FlowId flowId = NC::initFlow(sf, flowInfo, clientId);
        // Add dependency
        sf->arrival->addDependency(flowId);
        return flowId;
    }

public:
    TestSNC(SNCAlgorithm algorithm, double work, double p, double epsilon)
        : SNC(algorithm),
          _work(work),
          _p(p),
          _epsilon(epsilon)
    {}
    virtual ~TestSNC()
    {}

    static void SNCTestNetwork(NC* nc);
    static void SNCTestResetOptBounds(DependencyParams& bound);
    static void SNCTestAggregateAnalysisTwoHopDep();
    static void SNCTestHopByHopAnalysis();
    static void SNCTestConvolutionAnalysis();
};

void TestSNC::SNCTestNetwork(NC* nc)
{
    // Setup queues
    Json::Value queueInfo;
    queueInfo["bandwidth"] = Json::Value(1);

    queueInfo["name"] = Json::Value("Q1");
    nc->addQueue(queueInfo);

    queueInfo["name"] = Json::Value("Q2");
    nc->addQueue(queueInfo);

    queueInfo["name"] = Json::Value("Q3");
    nc->addQueue(queueInfo);

    // Setup flow's queues
    Json::Value queueListA(Json::arrayValue);
    queueListA.append(Json::Value("Q1"));
    queueListA.append(Json::Value("Q2"));
    Json::Value queueListB(Json::arrayValue);
    queueListB.append(Json::Value("Q3"));
    queueListB.append(Json::Value("Q2"));

    // Setup client's flows
    Json::Value clientInfo;
    clientInfo["flows"] = Json::arrayValue;
    clientInfo["flows"].resize(1);
    clientInfo["SLO"] = Json::Value(1);
    clientInfo["SLOpercentile"] = Json::Value(99.9);
    Json::Value& flowInfo = clientInfo["flows"][0];

    // Setup clients and flows
    flowInfo["name"] = Json::Value("F1");
    flowInfo["queues"] = queueListA;
    flowInfo["priority"] = Json::Value(1);
    clientInfo["name"] = Json::Value("C1");
    nc->addClient(clientInfo);

    flowInfo["name"] = Json::Value("F2");
    flowInfo["queues"] = queueListA;
    flowInfo["priority"] = Json::Value(2);
    clientInfo["name"] = Json::Value("C2");
    nc->addClient(clientInfo);

    flowInfo["name"] = Json::Value("F3");
    flowInfo["queues"] = queueListB;
    flowInfo["priority"] = Json::Value(3);
    clientInfo["name"] = Json::Value("C3");
    nc->addClient(clientInfo);

    flowInfo["name"] = Json::Value("F4");
    flowInfo["queues"] = queueListB;
    flowInfo["priority"] = Json::Value(4);
    clientInfo["name"] = Json::Value("C4");
    nc->addClient(clientInfo);
}

void TestSNC::SNCTestResetOptBounds(DependencyParams& bound)
{
    const vector<DependencyParams*>& bounds = bound.getDependentBounds();
    for (unsigned int index = 0; index < bounds.size(); index++) {
        bounds[index]->resetOptBounds();
    }
}

void TestSNC::SNCTestAggregateAnalysisTwoHopDep()
{
    TestSNC nc(SNC_ALGORITHM_AGGREGATE, 0.1, 1.0 * stepSize, 0.001);
    SNCTestNetwork(&nc);
    const Queue* q1 = nc.getQueue(nc.getQueueIdByName("Q1"));
    const Queue* q2 = nc.getQueue(nc.getQueueIdByName("Q2"));
    const Queue* q3 = nc.getQueue(nc.getQueueIdByName("Q3"));
    const SNCFlow* f1 = nc.getSNCFlow(nc.getFlowIdByName("F1"));
    const SNCFlow* f2 = nc.getSNCFlow(nc.getFlowIdByName("F2"));
    const SNCFlow* f3 = nc.getSNCFlow(nc.getFlowIdByName("F3"));
    const SNCFlow* f4 = nc.getSNCFlow(nc.getFlowIdByName("F4"));
    ConstantService serviceQ1(q1->bandwidth);
    ConstantService serviceQ2(q2->bandwidth);
    ConstantService serviceQ3(q3->bandwidth);

    ConvolutionService f1Service(&serviceQ1, &serviceQ2);
    LatencyBound f1Latency(f1->arrival, &f1Service, f1->epsilon);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F1")), f1Latency.dependencyOptimization()));

    LeftoverService f2Service(f1->arrival, &f1Service);
    LatencyBound f2Latency(f2->arrival, &f2Service, f2->epsilon);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F2")), f2Latency.dependencyOptimization()));

    AggregateArrival aggregateF1F2(f1->arrival, f2->arrival);
    OutputArrival outputArrival(&aggregateF1F2, &serviceQ1);
    LeftoverService leftoverQ2(&outputArrival, &serviceQ2);
    ConvolutionService f3Service(&serviceQ3, &leftoverQ2);
    LatencyBound f3Latency(f3->arrival, &f3Service, f3->epsilon);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F3")), f3Latency.dependencyOptimization()));

    LeftoverService f4Service(f3->arrival, &f3Service);
    LatencyBound f4Latency(f4->arrival, &f4Service, f4->epsilon);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F4")), f4Latency.dependencyOptimization()));
    cout << "PASS SNCTestAggregateAnalysisTwoHopDep" << endl;
}

void TestSNC::SNCTestHopByHopAnalysis()
{
    TestSNC nc(SNC_ALGORITHM_HOP_BY_HOP, 0.1, 1.0 * stepSize, 0.001);
    SNCTestNetwork(&nc);
    const Queue* q1 = nc.getQueue(nc.getQueueIdByName("Q1"));
    const Queue* q2 = nc.getQueue(nc.getQueueIdByName("Q2"));
    const Queue* q3 = nc.getQueue(nc.getQueueIdByName("Q3"));
    const SNCFlow* f1 = nc.getSNCFlow(nc.getFlowIdByName("F1"));
    const SNCFlow* f2 = nc.getSNCFlow(nc.getFlowIdByName("F2"));
    const SNCFlow* f3 = nc.getSNCFlow(nc.getFlowIdByName("F3"));
    const SNCFlow* f4 = nc.getSNCFlow(nc.getFlowIdByName("F4"));
    ConstantService serviceQ1(q1->bandwidth);
    ConstantService serviceQ2(q2->bandwidth);
    ConstantService serviceQ3(q3->bandwidth);

    LatencyBound f1LatencyQ1(f1->arrival, &serviceQ1, f1->epsilon / 2.0);
    OutputArrival f1ArrivalQ2(f1->arrival, &serviceQ1);
    LatencyBound f1LatencyQ2(&f1ArrivalQ2, &serviceQ2, f1->epsilon / 2.0);
    SNCTestResetOptBounds(f1LatencyQ1);
    SNCTestResetOptBounds(f1LatencyQ2);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F1")), f1LatencyQ1.dependencyOptimization() + f1LatencyQ2.dependencyOptimization()));

    LeftoverService f2ServiceQ1(f1->arrival, &serviceQ1);
    LatencyBound f2LatencyQ1(f2->arrival, &f2ServiceQ1, f2->epsilon / 2.0);
    OutputArrival f2ArrivalQ2(f2->arrival, &f2ServiceQ1);
    LeftoverService f2ServiceQ2(&f1ArrivalQ2, &serviceQ2);
    LatencyBound f2LatencyQ2(&f2ArrivalQ2, &f2ServiceQ2, f2->epsilon / 2.0);
    SNCTestResetOptBounds(f2LatencyQ1);
    SNCTestResetOptBounds(f2LatencyQ2);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F2")), f2LatencyQ1.dependencyOptimization() + f2LatencyQ2.dependencyOptimization()));

    LatencyBound f3LatencyQ3(f3->arrival, &serviceQ3, f3->epsilon / 2.0);
    OutputArrival f3ArrivalQ2(f3->arrival, &serviceQ3);
    LeftoverService f3ServiceQ2(&f2ArrivalQ2, &f2ServiceQ2);
    LatencyBound f3LatencyQ2(&f3ArrivalQ2, &f3ServiceQ2, f3->epsilon / 2.0);
    SNCTestResetOptBounds(f3LatencyQ3);
    SNCTestResetOptBounds(f3LatencyQ2);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F3")), f3LatencyQ3.dependencyOptimization() + f3LatencyQ2.dependencyOptimization()));

    LeftoverService f4ServiceQ3(f3->arrival, &serviceQ3);
    LatencyBound f4LatencyQ3(f4->arrival, &f4ServiceQ3, f4->epsilon / 2.0);
    OutputArrival f4ArrivalQ2(f4->arrival, &f4ServiceQ3);
    LeftoverService f4ServiceQ2(&f3ArrivalQ2, &f3ServiceQ2);
    LatencyBound f4LatencyQ2(&f4ArrivalQ2, &f4ServiceQ2, f4->epsilon / 2.0);
    SNCTestResetOptBounds(f4LatencyQ3);
    SNCTestResetOptBounds(f4LatencyQ2);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F4")), f4LatencyQ3.dependencyOptimization() + f4LatencyQ2.dependencyOptimization()));
    cout << "PASS SNCTestHopByHopAnalysis" << endl;
}

void TestSNC::SNCTestConvolutionAnalysis()
{
    TestSNC nc(SNC_ALGORITHM_CONVOLUTION, 0.1, 1.0 * stepSize, 0.001);
    SNCTestNetwork(&nc);
    const Queue* q1 = nc.getQueue(nc.getQueueIdByName("Q1"));
    const Queue* q2 = nc.getQueue(nc.getQueueIdByName("Q2"));
    const Queue* q3 = nc.getQueue(nc.getQueueIdByName("Q3"));
    const SNCFlow* f1 = nc.getSNCFlow(nc.getFlowIdByName("F1"));
    const SNCFlow* f2 = nc.getSNCFlow(nc.getFlowIdByName("F2"));
    const SNCFlow* f3 = nc.getSNCFlow(nc.getFlowIdByName("F3"));
    const SNCFlow* f4 = nc.getSNCFlow(nc.getFlowIdByName("F4"));
    ConstantService serviceQ1(q1->bandwidth);
    ConstantService serviceQ2(q2->bandwidth);
    ConstantService serviceQ3(q3->bandwidth);

    ConvolutionService f1Service(&serviceQ1, &serviceQ2);
    LatencyBound f1Latency(f1->arrival, &f1Service, f1->epsilon);
    SNCTestResetOptBounds(f1Latency);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F1")), f1Latency.dependencyOptimization()));

    LeftoverService f2ServiceQ1(f1->arrival, &serviceQ1);
    OutputArrival f1ArrivalQ2(f1->arrival, &serviceQ1);
    LeftoverService f2ServiceQ2(&f1ArrivalQ2, &serviceQ2);
    ConvolutionService f2Service(&f2ServiceQ1, &f2ServiceQ2);
    LatencyBound f2Latency(f2->arrival, &f2Service, f2->epsilon);
    SNCTestResetOptBounds(f2Latency);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F2")), f2Latency.dependencyOptimization()));

    OutputArrival f2ArrivalQ2(f2->arrival, &f2ServiceQ1);
    LeftoverService f3ServiceQ2(&f2ArrivalQ2, &f2ServiceQ2);
    ConvolutionService f3Service(&serviceQ3, &f3ServiceQ2);
    LatencyBound f3Latency(f3->arrival, &f3Service, f3->epsilon);
    SNCTestResetOptBounds(f3Latency);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F3")), f3Latency.dependencyOptimization()));

    LeftoverService f4ServiceQ3(f3->arrival, &serviceQ3);
    OutputArrival f3ArrivalQ2(f3->arrival, &serviceQ3);
    LeftoverService f4ServiceQ2(&f3ArrivalQ2, &f3ServiceQ2);
    ConvolutionService f4Service(&f4ServiceQ3, &f4ServiceQ2);
    LatencyBound f4Latency(f4->arrival, &f4Service, f4->epsilon);
    SNCTestResetOptBounds(f4Latency);
    assert(approxEqual(nc.calcFlowLatency(nc.getFlowIdByName("F4")), f4Latency.dependencyOptimization()));
    cout << "PASS SNCTestConvolutionAnalysis" << endl;
}

void SNCTest()
{
    TestSNC::SNCTestAggregateAnalysisTwoHopDep();
    TestSNC::SNCTestHopByHopAnalysis();
    TestSNC::SNCTestConvolutionAnalysis();
}
