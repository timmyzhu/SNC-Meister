// SNCOperatorsTest.cpp - SNCOperators test code.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <iostream>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include "../SNC-Library/ProcessedTrace.hpp"
#include "../SNC-Library/Estimator.hpp"
#include "../SNC-Library/time.hpp"
#include "../SNC-Library/SNCOperators.hpp"
#include "SNC-LibraryTest.hpp"

using namespace std;

class TestArrival : public SNCArrival
{
private:
    double _sigma;
    double _rho;

public:
    TestArrival(double sigma, double rho)
        : _sigma(sigma),
          _rho(rho)
    {
    }
    virtual ~TestArrival()
    {
    }

    virtual void calcBound(double theta, double* sigma, double* rho) const
    {
        *sigma = _sigma + theta;
        *rho = _rho + theta;
    }
};

class TestService : public SNCService
{
private:
    double _sigma;
    double _rho;

public:
    TestService(double sigma, double rho)
        : _sigma(sigma),
          _rho(rho)
    {
    }
    virtual ~TestService()
    {
    }

    virtual void calcBound(double theta, double* sigma, double* rho) const
    {
        *sigma = _sigma + theta;
        *rho = _rho + theta;
    }
};

class TestSNCOperators
{
public:
    static void DependencyParamsTest();
    static ProcessedTrace* MMBPArrivalTestTrace();
    static double MMBPArrivalSpectralRadiusV1(const MMBPArrival& arrival, double theta);
    static double MMBPArrivalSpectralRadiusV2(const MMBPArrival& arrival, double theta);
    static void MMBPArrivalTest();
    static void ConstantServiceTest();
    static void AggregateArrivalTest();
    static void ConvolutionServiceTest();
    static void OutputArrivalTest();
    static void LeftoverServiceTest();
    static void LatencyBoundTest(double epsilon);
    static void LatencyBoundTest();
};

void TestSNCOperators::DependencyParamsTest()
{
    DependencyParams dp;
    // Test setP, setQ
    dp.setP(1.5);
    assert(approxEqual(dp.getP(), 1.5));
    assert(approxEqual(dp.getQ(), 3.0));
    dp.setQ(1.5);
    assert(approxEqual(dp.getP(), 3.0));
    assert(approxEqual(dp.getQ(), 1.5));
    // Test setLowerP, setUpperP, setLowerQ, setUpperQ, setDependent
    dp.setLowerP(1.2);
    assert(approxEqual(dp.getLowerP(), 1.2));
    dp.setUpperP(1.3);
    assert(approxEqual(dp.getUpperP(), 1.3));
    dp.setLowerQ(1.4);
    assert(approxEqual(dp.getLowerQ(), 1.4));
    dp.setUpperQ(1.5);
    assert(approxEqual(dp.getUpperQ(), 1.5));
    assert(dp.getDependentBounds().empty());
    dp.setDependent();
    assert(dp.getDependentBounds().size() == 1);
    assert(dp.getDependentBounds()[0] == &dp);
    // Test addDependencies, getDependencies, checkDependence
    assert(dp.getDependencies().empty());
    set<FlowId> newDependencies;
    newDependencies.insert(3);
    newDependencies.insert(7);
    dp.addDependencies(newDependencies);
    const set<FlowId>& dependencies = dp.getDependencies();
    assert(dependencies.size() == 2);
    assert(dependencies.find(3) != dependencies.end());
    assert(dependencies.find(7) != dependencies.end());
    DependencyParams dp2;
    set<FlowId> newDependencies2;
    newDependencies2.insert(4);
    newDependencies2.insert(8);
    dp2.addDependencies(newDependencies2);
    assert(dp.checkDependence(&dp2) == false);
    assert(dp2.checkDependence(&dp) == false);
    newDependencies2.insert(7);
    dp2.addDependencies(newDependencies2);
    assert(dp.checkDependence(&dp2) == true);
    assert(dp2.checkDependence(&dp) == true);
    // Test getDependentBounds
    assert(dp2.getDependentBounds().empty());
    dp2.setDependent();
    assert(dp2.getDependentBounds().size() == 1);
    assert(dp2.getDependentBounds()[0] == &dp2);
    dp2.addDependencies(&dp);
    const set<FlowId>& dependencies2 = dp2.getDependencies();
    assert(dependencies2.size() == 4);
    assert(dependencies2.find(3) != dependencies2.end());
    assert(dependencies2.find(4) != dependencies2.end());
    assert(dependencies2.find(7) != dependencies2.end());
    assert(dependencies2.find(8) != dependencies2.end());
    assert(dp2.getDependentBounds().size() == 2);
    assert(dp2.getDependentBounds()[0] == &dp2);
    assert(dp2.getDependentBounds()[1] == &dp);
    cout << "PASS DependencyParamsTest" << endl;
}

class MMBPArrivalTestProcessedTrace : public ProcessedTrace
{
private:
    vector<ProcessedTraceEntry> _trace;
    unsigned int _curIndex;

public:
    MMBPArrivalTestProcessedTrace(const vector<ProcessedTraceEntry>& trace, Json::Value& estimatorInfo)
        : ProcessedTrace("testTrace.txt", Estimator::Create(estimatorInfo)),
          _trace(trace),
          _curIndex(0)
    {}
    virtual ~MMBPArrivalTestProcessedTrace()
    {}

    virtual bool nextEntry(ProcessedTraceEntry& entry)
    {
        if (_curIndex < _trace.size()) {
            entry = _trace[_curIndex++];
            return true;
        }
        return false;
    }

    virtual void reset()
    {
        _curIndex = 0;
    }
};

void MMBPArrivalTestAddTraceEntry(vector<ProcessedTraceEntry>& trace, uint64_t arrivalTime, double work, bool isGet)
{
    ProcessedTraceEntry entry = {arrivalTime, work, isGet};
    trace.push_back(entry);
}
ProcessedTrace* TestSNCOperators::MMBPArrivalTestTrace()
{
    vector<ProcessedTraceEntry> trace;
    uint64_t arrivalTime = 0;
    MMBPArrivalTestAddTraceEntry(trace, arrivalTime, 0.2, true);
    MMBPArrivalTestAddTraceEntry(trace, arrivalTime, 0.1, true);
    arrivalTime += MMBPArrival::intervalWidth;
    for (int i = 0; i < 24; i++) {
        MMBPArrivalTestAddTraceEntry(trace, arrivalTime, 0.3, true);
    }
    arrivalTime += MMBPArrival::intervalWidth;
    MMBPArrivalTestAddTraceEntry(trace, arrivalTime, 0.3, true);
    Json::Value estimatorInfo;
    setEstimatorInfoProcessedTraceTest(estimatorInfo);
    return new MMBPArrivalTestProcessedTrace(trace, estimatorInfo);
}

double TestSNCOperators::MMBPArrivalSpectralRadiusV1(const MMBPArrival& arrival, double theta)
{
    // Hand-solved solution for 2 states
    double MGF0 = arrival._MGFs[0]->calcMGF(theta);
    double MGF1 = arrival._MGFs[1]->calcMGF(theta);
    double L1 = (arrival._transitionMatrix[0][0] * MGF0 + arrival._transitionMatrix[1][1] * MGF1 + sqrt((arrival._transitionMatrix[0][0] * MGF0 - arrival._transitionMatrix[1][1] * MGF1) * (arrival._transitionMatrix[0][0] * MGF0 - arrival._transitionMatrix[1][1] * MGF1) + 4.0 * arrival._transitionMatrix[0][1] * arrival._transitionMatrix[1][0] * MGF0 * MGF1)) / 2.0;
    double L2 = (arrival._transitionMatrix[0][0] * MGF0 + arrival._transitionMatrix[1][1] * MGF1 - sqrt((arrival._transitionMatrix[0][0] * MGF0 - arrival._transitionMatrix[1][1] * MGF1) * (arrival._transitionMatrix[0][0] * MGF0 - arrival._transitionMatrix[1][1] * MGF1) + 4.0 * arrival._transitionMatrix[0][1] * arrival._transitionMatrix[1][0] * MGF0 * MGF1)) / 2.0;
    return max(fabs(L1), fabs(L2));
}

double TestSNCOperators::MMBPArrivalSpectralRadiusV2(const MMBPArrival& arrival, double theta)
{
    // Generic solution for n states
    Eigen::MatrixXd m(arrival._MGFs.size(), arrival._MGFs.size());
    for (unsigned int fromState = 0; fromState < arrival._transitionMatrix.size(); fromState++) {
        double stateMGF = arrival._MGFs[fromState]->calcMGF(theta);
        if (!isfinite(stateMGF)) {
            return numeric_limits<double>::infinity();
        }
        for (unsigned int toState = 0; toState < arrival._transitionMatrix[fromState].size(); toState++) {
            m(fromState, toState) = stateMGF * arrival._transitionMatrix[fromState][toState];
        }
    }
    return m.eigenvalues().cwiseAbs().maxCoeff();
}

void TestSNCOperators::MMBPArrivalTest()
{
    ProcessedTrace* pTrace = MMBPArrivalTestTrace();
    MMBPArrival arrival(pTrace);
    arrival._transitionMatrix.clear();
    while (!arrival._MGFs.empty()) {
        delete arrival._MGFs.back();
        arrival._MGFs.pop_back();
    }
    // Test countArrivalIntervals
    vector<double> intervals;
    arrival.countArrivalIntervals(pTrace, intervals);
    assert(intervals.size() == 3);
    assert(approxEqual(intervals[0], 2));
    assert(approxEqual(intervals[1], 24));
    assert(approxEqual(intervals[2], 1));
    // Test determineStatesLAMBDA
    vector<unsigned int> states;
    vector<double> lambdas;
    unsigned int numStates = arrival.determineStatesLAMBDA(intervals, states, lambdas, 2.0);
    assert(numStates == 2);
    assert(states.size() == 3);
    assert(states[0] == states[2]);
    assert(states[1] != states[2]);
    assert(lambdas.size() == 2);
    assert(approxEqual(lambdas[0], 4.0));
    assert(approxEqual(lambdas[1], 16.0));
    // Test initTransitionMatrix
    arrival.initTransitionMatrix(numStates, states);
    assert(arrival._transitionMatrix.size() == 2);
    assert(arrival._transitionMatrix[0].size() == 2);
    assert(arrival._transitionMatrix[1].size() == 2);
    assert(approxEqual(arrival._transitionMatrix[0][0], 1.0 - 1.0 / floor(ConvertTimeToSeconds(2.0 * MMBPArrival::intervalWidth) / stepSize)));
    assert(approxEqual(arrival._transitionMatrix[0][1], 1.0 / floor(ConvertTimeToSeconds(2.0 * MMBPArrival::intervalWidth) / stepSize)));
    assert(approxEqual(arrival._transitionMatrix[1][0], 1.0 / floor(ConvertTimeToSeconds(1.0 * MMBPArrival::intervalWidth) / stepSize)));
    assert(approxEqual(arrival._transitionMatrix[1][1], 1.0 - 1.0 / floor(ConvertTimeToSeconds(1.0 * MMBPArrival::intervalWidth) / stepSize)));
    // Test initMGFs
    arrival.initMGFs(pTrace, numStates, states, lambdas);
    assert(arrival._MGFs.size() == 2);
    MGF* state0 = arrival.createMMBPStateMGF();
    MGF* state1 = arrival.createMMBPStateMGF();
    pTrace->reset();
    ProcessedTraceEntry entry;
    pTrace->nextEntry(entry);
    state0->addSampleRequest(entry);
    pTrace->nextEntry(entry);
    state0->addSampleRequest(entry);
    for (int i = 0; i < 24; i++) {
        pTrace->nextEntry(entry);
        state1->addSampleRequest(entry);
    }
    pTrace->nextEntry(entry);
    state0->addSampleRequest(entry);
    state0->setProbRequest(4.0 * stepSize / ConvertTimeToSeconds(MMBPArrival::intervalWidth));
    state1->setProbRequest(16.0 * stepSize / ConvertTimeToSeconds(MMBPArrival::intervalWidth));
    assert(approxEqual(arrival._MGFs[0]->calcMGF(0.1), state0->calcMGF(0.1)));
    assert(approxEqual(arrival._MGFs[0]->calcMGF(0.2), state0->calcMGF(0.2)));
    assert(approxEqual(arrival._MGFs[0]->calcMGF(2.0), state0->calcMGF(2.0)));
    assert(approxEqual(arrival._MGFs[1]->calcMGF(0.1), state1->calcMGF(0.1)));
    assert(approxEqual(arrival._MGFs[1]->calcMGF(0.2), state1->calcMGF(0.2)));
    assert(approxEqual(arrival._MGFs[1]->calcMGF(2.0), state1->calcMGF(2.0)));
    delete state0;
    delete state1;
    // Test calcSpectralRadius
    assert(approxEqual(MMBPArrivalSpectralRadiusV1(arrival, 0.1), MMBPArrivalSpectralRadiusV2(arrival, 0.1)));
    assert(approxEqual(MMBPArrivalSpectralRadiusV1(arrival, 0.2), MMBPArrivalSpectralRadiusV2(arrival, 0.2)));
    assert(approxEqual(MMBPArrivalSpectralRadiusV1(arrival, 2.0), MMBPArrivalSpectralRadiusV2(arrival, 2.0)));
    double spectralRadius0_1 = MMBPArrivalSpectralRadiusV2(arrival, 0.1);
    double spectralRadius0_2 = MMBPArrivalSpectralRadiusV2(arrival, 0.2);
    double spectralRadius2_0 = MMBPArrivalSpectralRadiusV2(arrival, 2.0);
    assert(approxEqual(arrival.calcSpectralRadius(0.1), spectralRadius0_1));
    assert(approxEqual(arrival.calcSpectralRadius(0.2), spectralRadius0_2));
    assert(approxEqual(arrival.calcSpectralRadius(2.0), spectralRadius2_0));
    // Test calcBound
    double sigma = 0;
    double rho = 0;
    arrival.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, log(spectralRadius0_1) / 0.1));
    arrival.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, log(spectralRadius0_2) / 0.2));
    arrival.calcBound(2.0, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, log(spectralRadius2_0) / 2.0));
    // Test serialization/deserialization
    Json::Value data;
    arrival.serialize(data);
    MMBPArrival arrival2(data);
    arrival2.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, log(spectralRadius0_1) / 0.1));
    arrival2.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, log(spectralRadius0_2) / 0.2));
    arrival2.calcBound(2.0, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, log(spectralRadius2_0) / 2.0));
    cout << "PASS MMBPArrivalTest" << endl;
}

void TestSNCOperators::ConstantServiceTest()
{
    ConstantService c0(3);
    double sigma = 0;
    double rho = 0;
    c0.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -3 * stepSize));
    c0.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -3 * stepSize));
    c0.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -3 * stepSize));
    ConstantService c1(4);
    c1.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -4 * stepSize));
    c1.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -4 * stepSize));
    c1.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -4 * stepSize));
    ConstantService c2(5);
    c2.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -5 * stepSize));
    c2.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -5 * stepSize));
    c2.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 0));
    assert(approxEqual(rho, -5 * stepSize));
    cout << "PASS ConstantServiceTest" << endl;
}

void TestSNCOperators::AggregateArrivalTest()
{
    TestArrival a0(1, 2);
    TestArrival a1(3, 4);
    AggregateArrival aggregateArrival(&a0, &a1);
    double sigma = 0;
    double rho = 0;
    // Independent
    aggregateArrival.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 4.2));
    assert(approxEqual(rho, 6.2));
    aggregateArrival.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 4.4));
    assert(approxEqual(rho, 6.4));
    aggregateArrival.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 8));
    assert(approxEqual(rho, 10));
    // Dependent p = 2
    aggregateArrival.setDependent();
    aggregateArrival.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 4.4));
    assert(approxEqual(rho, 6.4));
    aggregateArrival.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 4.8));
    assert(approxEqual(rho, 6.8));
    aggregateArrival.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 12));
    assert(approxEqual(rho, 14));
    // Dependent p = 1.5
    aggregateArrival.setP(1.5);
    aggregateArrival.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 4.45));
    assert(approxEqual(rho, 6.45));
    aggregateArrival.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 4.9));
    assert(approxEqual(rho, 6.9));
    aggregateArrival.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 13));
    assert(approxEqual(rho, 15));
    cout << "PASS AggregateArrivalTest" << endl;
}

void TestSNCOperators::ConvolutionServiceTest()
{
    TestService s0(2, -3);
    TestService s1(4, -5);
    ConvolutionService convolutionService(&s0, &s1);
    double sigma = 0;
    double rho = 0;
    // Independent
    convolutionService.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 6.2 - log(1.0 - exp(-0.2)) / 0.1));
    assert(approxEqual(rho, -2.9));
    convolutionService.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 6.4 - log(1.0 - exp(-0.4)) / 0.2));
    assert(approxEqual(rho, -2.8));
    convolutionService.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 10 - log(1.0 - exp(-4)) / 2.0));
    assert(approxEqual(rho, -1));
    // Dependent p = 2
    convolutionService.setDependent();
    convolutionService.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 6.4 - log(1.0 - exp(-0.2)) / 0.1));
    assert(approxEqual(rho, -2.8));
    convolutionService.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 6.8 - log(1.0 - exp(-0.4)) / 0.2));
    assert(approxEqual(rho, -2.6));
    convolutionService.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 14 - log(1.0 - exp(-4)) / 2.0));
    assert(approxEqual(rho, 1));
    // Dependent p = 1.5
    convolutionService.setP(1.5);
    convolutionService.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 6.45 - log(1.0 - exp(-0.185)) / 0.1));
    assert(approxEqual(rho, -2.85));
    convolutionService.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 6.9 - log(1.0 - exp(-0.34)) / 0.2));
    assert(approxEqual(rho, -2.7));
    convolutionService.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 15 - log(1.0 - exp(-2)) / 2.0));
    assert(approxEqual(rho, 1));
    // Equal rho
    TestService s2(5, -10);
    TestService s3(5, -10);
    ConvolutionService convolutionServiceEqual(&s2, &s3);
    convolutionServiceEqual.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 10.2 - log(1.0 - exp(-0.0099)) / 0.1));
    assert(approxEqual(rho, -9.801));
    convolutionServiceEqual.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 10.4 - log(1.0 - exp(-0.0196)) / 0.2));
    assert(approxEqual(rho, -9.702));
    convolutionServiceEqual.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 14 - log(1.0 - exp(-0.16)) / 2.0));
    assert(approxEqual(rho, -7.92));
    cout << "PASS ConvolutionServiceTest" << endl;
}

void TestSNCOperators::OutputArrivalTest()
{
    TestArrival a(1, 2);
    TestService s(3, -8);
    OutputArrival outputArrival(&a, &s);
    double sigma = 0;
    double rho = 0;
    // Independent
    outputArrival.calcBound(0.25, &sigma, &rho);
    assert(approxEqual(sigma, 4.5 - log(1.0 - exp(-1.375)) / 0.25));
    assert(approxEqual(rho, 2.25));
    outputArrival.calcBound(0.5, &sigma, &rho);
    assert(approxEqual(sigma, 5.0 - log(1.0 - exp(-2.5)) / 0.5));
    assert(approxEqual(rho, 2.5));
    outputArrival.calcBound(1, &sigma, &rho);
    assert(approxEqual(sigma, 6.0 - log(1.0 - exp(-4.0)) / 1.0));
    assert(approxEqual(rho, 3));
    // Dependent p = 2
    outputArrival.setDependent();
    outputArrival.calcBound(0.25, &sigma, &rho);
    assert(approxEqual(sigma, 5.0 - log(1.0 - exp(-1.25)) / 0.25));
    assert(approxEqual(rho, 2.5));
    outputArrival.calcBound(0.5, &sigma, &rho);
    assert(approxEqual(sigma, 6.0 - log(1.0 - exp(-2.0)) / 0.5));
    assert(approxEqual(rho, 3));
    outputArrival.calcBound(1, &sigma, &rho);
    assert(approxEqual(sigma, 8.0 - log(1.0 - exp(-2.0)) / 1.0));
    assert(approxEqual(rho, 4));
    // Dependent p = 1.5
    outputArrival.setP(1.5);
    outputArrival.calcBound(0.25, &sigma, &rho);
    assert(approxEqual(sigma, 5.125 - log(1.0 - exp(-1.21875)) / 0.25));
    assert(approxEqual(rho, 2.375));
    outputArrival.calcBound(0.5, &sigma, &rho);
    assert(approxEqual(sigma, 6.25 - log(1.0 - exp(-1.875)) / 0.5));
    assert(approxEqual(rho, 2.75));
    outputArrival.calcBound(1, &sigma, &rho);
    assert(approxEqual(sigma, 8.5 - log(1.0 - exp(-1.5)) / 1.0));
    assert(approxEqual(rho, 3.5));
    cout << "PASS OutputArrivalTest" << endl;
}

void TestSNCOperators::LeftoverServiceTest()
{
    TestArrival a(1, 2);
    TestService s(3, 4);
    LeftoverService leftoverService(&a, &s);
    double sigma = 0;
    double rho = 0;
    // Independent
    leftoverService.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 4.2));
    assert(approxEqual(rho, 6.2));
    leftoverService.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 4.4));
    assert(approxEqual(rho, 6.4));
    leftoverService.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 8));
    assert(approxEqual(rho, 10));
    // Dependent p = 2
    leftoverService.setDependent();
    leftoverService.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 4.4));
    assert(approxEqual(rho, 6.4));
    leftoverService.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 4.8));
    assert(approxEqual(rho, 6.8));
    leftoverService.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 12));
    assert(approxEqual(rho, 14));
    // Dependent p = 1.5
    leftoverService.setP(1.5);
    leftoverService.calcBound(0.1, &sigma, &rho);
    assert(approxEqual(sigma, 4.45));
    assert(approxEqual(rho, 6.45));
    leftoverService.calcBound(0.2, &sigma, &rho);
    assert(approxEqual(sigma, 4.9));
    assert(approxEqual(rho, 6.9));
    leftoverService.calcBound(2, &sigma, &rho);
    assert(approxEqual(sigma, 13));
    assert(approxEqual(rho, 15));
    cout << "PASS LeftoverServiceTest" << endl;
}

void TestSNCOperators::LatencyBoundTest(double epsilon)
{
    TestArrival a(1, 2);
    TestService s(3, -8);
    LatencyBound latencyBound(&a, &s, epsilon);
    // Independent
    assert(approxEqual(latencyBound.calcLatency(0.25), stepSize * ((log(epsilon * (1.0 - exp(-1.375))) / 0.25) - 4.5) / (-7.75)));
    assert(approxEqual(latencyBound.calcLatency(0.5), stepSize * ((log(epsilon * (1.0 - exp(-2.5))) / 0.5) - 5.0) / (-7.5)));
    assert(approxEqual(latencyBound.calcLatency(1), stepSize * ((log(epsilon * (1.0 - exp(-4.0))) / 1.0) - 6.0) / (-7.0)));
    // Dependent p = 2
    latencyBound.setDependent();
    assert(approxEqual(latencyBound.calcLatency(0.25), stepSize * ((log(epsilon * (1.0 - exp(-1.25))) / 0.25) - 5.0) / (-7.5)));
    assert(approxEqual(latencyBound.calcLatency(0.5), stepSize * ((log(epsilon * (1.0 - exp(-2.0))) / 0.5) - 6.0) / (-7.0)));
    assert(approxEqual(latencyBound.calcLatency(1), stepSize * ((log(epsilon * (1.0 - exp(-2.0))) / 1.0) - 8.0) / (-6.0)));
    // Dependent p = 1.5
    latencyBound.setP(1.5);
    assert(approxEqual(latencyBound.calcLatency(0.25), stepSize * ((log(epsilon * (1.0 - exp(-1.21875))) / 0.25) - 5.125) / (-7.25)));
    assert(approxEqual(latencyBound.calcLatency(0.5), stepSize * ((log(epsilon * (1.0 - exp(-1.875))) / 0.5) - 6.25) / (-6.5)));
    assert(approxEqual(latencyBound.calcLatency(1), stepSize * ((log(epsilon * (1.0 - exp(-1.5))) / 1.0) - 8.5) / (-5.0)));
}

void TestSNCOperators::LatencyBoundTest()
{
    // Epsilon 0.01
    LatencyBoundTest(0.01);
    // Epsilon 0.001
    LatencyBoundTest(0.001);
    // Epsilon 0.0001
    LatencyBoundTest(0.0001);
    cout << "PASS LatencyBoundTest" << endl;
}

void SNCOperatorsTest()
{
    TestSNCOperators::DependencyParamsTest();
    TestSNCOperators::MMBPArrivalTest();
    TestSNCOperators::ConstantServiceTest();
    TestSNCOperators::AggregateArrivalTest();
    TestSNCOperators::ConvolutionServiceTest();
    TestSNCOperators::OutputArrivalTest();
    TestSNCOperators::LeftoverServiceTest();
    TestSNCOperators::LatencyBoundTest();
}
