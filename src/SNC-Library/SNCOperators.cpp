// SNCOperators.cpp - Code for the SNC operator equations.
// See SNCOperators.hpp for details.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <stdint.h>
#include <set>
#include <vector>
#include <limits>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include "ProcessedTrace.hpp"
#include "time.hpp"
#include "search.hpp"
#include "NC.hpp"
#include "MGF.hpp"
#include "SNCOperators.hpp"

using namespace std;

const uint64_t MMBPArrival::intervalWidth = ConvertSecondsToTime(1);

// Check if there is a dependency between bound and this. A dependency exists if both bound and this share the same flow id in their dependency sets.
bool DependencyParams::checkDependence(const DependencyParams* bound) const
{
    const set<FlowId>& A = getDependencies();
    const set<FlowId>& B = bound->getDependencies();
    set<FlowId>::const_iterator itA = A.begin();
    set<FlowId>::const_iterator itB = B.begin();
    while ((itA != A.end()) && (itB != B.end())) {
        if (*itA == *itB) {
            return true;
        } else if (*itA < *itB) {
            itA++;
        } else {
            itB++;
        }
    }
    return false;
}

// Top-level function for analyzing a trace and initializing the MMBP parameters based on the analysis.
void MMBPArrival::init(ProcessedTrace* pTrace)
{
    // Split trace into intervals of length intervalWidth
    vector<double> intervals;
    countArrivalIntervals(pTrace, intervals);
    // Assign a state to each interval using the LAMBDA algorithm
    vector<unsigned int> states;
    vector<double> lambdas;
    unsigned int numStates = determineStatesLAMBDA(intervals, states, lambdas, 2.0);
    // Initialize the transition matrix between states
    initTransitionMatrix(numStates, states);
    // Initialize MGFs for each state
    initMGFs(pTrace, numStates, states, lambdas);
}

// Count the number of arrivals in each interval in the trace pTrace and store the results in intervals.
void MMBPArrival::countArrivalIntervals(ProcessedTrace* pTrace, vector<double>& intervals) const
{
    intervals.clear();
    int count = 0;
    uint64_t nextIntervalTime = intervalWidth;
    ProcessedTraceEntry traceEntry;
    pTrace->reset();
    while (pTrace->nextEntry(traceEntry)) {
        while (traceEntry.arrivalTime >= nextIntervalTime) {
            intervals.push_back(count);
            count = 0;
            nextIntervalTime += intervalWidth;
        }
        count++;
    }
    intervals.push_back(count);
}

// Helper function for performing the LAMBDA algorithm.
double MMBPArrival::LAMBDAAlgorithm(double a, void* params)
{
    LAMBDAParams_t* p = static_cast<LAMBDAParams_t*>(params);
    double low = p->low;
    double high = p->high;
    vector<double>* pLambdas = p->pLambdas;
    double lambda = sqrt(high + a * a / 4.0) - (a / 2.0);
    lambda = lambda * lambda;
    for (unsigned int state = pLambdas->size() - 1; state > 0; state--) {
        if (lambda < low) {
            return -numeric_limits<double>::infinity();
        }
        (*pLambdas)[state] = lambda;
        lambda = sqrt(lambda) - a;
        lambda = lambda * lambda;
    }
    (*pLambdas)[0] = lambda;
    return lambda - a * sqrt(lambda);
}

// Assign a state to each interval in the trace using the LAMBDA algorithm. Returns number of states used, up to maxNumStates.
unsigned int MMBPArrival::determineStatesLAMBDA(const vector<double>& intervals, vector<unsigned int>& states, vector<double>& lambdas, double a) const
{
    // Find largest and smallest interval size
    LAMBDAParams_t params;
    double& low = params.low;
    double& high = params.high;
    params.pLambdas = &lambdas;
    low = intervals[0];
    high = intervals[0];
    for (unsigned int intervalIndex = 1; intervalIndex < intervals.size(); intervalIndex++) {
        if (intervals[intervalIndex] < low) {
            low = intervals[intervalIndex];
        }
        if (intervals[intervalIndex] > high) {
            high = intervals[intervalIndex];
        }
    }
    // Determine lambdas with LAMBDA algorithm
    unsigned int numStates = maxNumStates;
    lambdas.resize(numStates, 0);
    if (isfinite(LAMBDAAlgorithm(a, &params))) {
        // Maximum number of states used, search for best confidence interval for the max number of states to cover the range
        a = binarySearchReverse(0, high, low, 0.01, LAMBDAAlgorithm, &params);
        LAMBDAAlgorithm(a, &params);
    } else {
        // Less than the max number of states needed for a given confidence interval, remove unnecessary states
        unsigned int unusedStates = 0;
        for (unsigned int state = 0; state < lambdas.size(); state++) {
            if (lambdas[state] > 0) {
                lambdas[state - unusedStates] = lambdas[state];
            } else {
                unusedStates++;
            }
        }
        numStates -= unusedStates;
        lambdas.resize(numStates);
    }
    // Assign states to intervals
    states.resize(intervals.size(), 0);
    for (unsigned int intervalIndex = 0; intervalIndex < intervals.size(); intervalIndex++) {
        states[intervalIndex] = 0;
        for (unsigned int state = lambdas.size() - 1; state > 0; state--) {
            double lambda = lambdas[state];
            if (intervals[intervalIndex] > (lambda - a * sqrt(lambda))) {
                states[intervalIndex] = state;
                break;
            }
        }
    }
    return numStates;
}

// Initialize the transition matrix between states.
void MMBPArrival::initTransitionMatrix(unsigned int numStates, const vector<unsigned int>& states)
{
    // Initialize transition matrix size
    _transitionMatrix.resize(numStates);
    for (unsigned int state = 0; state < numStates; state++) {
        _transitionMatrix[state].resize(numStates);
    }
    // Calculate transition matrix
    vector<uint64_t> stateDurations(numStates, 0);
    unsigned int fromState = states[0];
    stateDurations[fromState] += intervalWidth;
    for (unsigned int stateIndex = 1; stateIndex < states.size(); stateIndex++) {
        unsigned int toState = states[stateIndex];
        stateDurations[toState] += intervalWidth;
        _transitionMatrix[fromState][toState] += 1;
        fromState = toState;
    }
    for (unsigned int fromState = 0; fromState < numStates; fromState++) {
        double stateSteps = floor(ConvertTimeToSeconds(stateDurations[fromState]) / stepSize);
        if (stateSteps == 0) {
            stateSteps = 1;
        }
        double probTransition = 0;
        _transitionMatrix[fromState][fromState] = 0;
        for (unsigned int toState = 0; toState < numStates; toState++) {
            _transitionMatrix[fromState][toState] /= stateSteps;
            probTransition += _transitionMatrix[fromState][toState];
        }
        _transitionMatrix[fromState][fromState] = 1.0 - probTransition;
    }
}

// Helper function for creating the MGF for a MMBP state.
MGF* MMBPArrival::createMMBPStateMGF()
{
    return new MGFExponential();
}

// Initialize each MMBP state with its associated arrival rate and request size distribution, which is represented by a moment generating function (MGF).
void MMBPArrival::initMGFs(ProcessedTrace* pTrace, unsigned int numStates, const vector<unsigned int>& states, const vector<double>& lambdas)
{
    // Initialize MGFs
    _MGFs.resize(numStates);
    for (unsigned int state = 0; state < numStates; state++) {
        _MGFs[state] = createMMBPStateMGF();
    }
    // Estimate MGFs based on trace
    uint64_t nextIntervalTime = intervalWidth;
    unsigned int stateIndex = 0;
    ProcessedTraceEntry traceEntry;
    pTrace->reset();
    while (pTrace->nextEntry(traceEntry)) {
        while (traceEntry.arrivalTime >= nextIntervalTime) {
            stateIndex++;
            nextIntervalTime += intervalWidth;
        }
        unsigned int state = states[stateIndex];
        _MGFs[state]->addSampleRequest(traceEntry);
    }
    // Set prob of generating a request for each MGF
    for (unsigned int state = 0; state < numStates; state++) {
        _MGFs[state]->setProbRequest(lambdas[state] * stepSize / ConvertTimeToSeconds(intervalWidth));
    }
}

// Calculate the spectral radius of the matrix: Diag(_MGFs(theta)) * _transitionMatrix,
// where Diag(_MGFs(theta)) is the diagonal matrix from evaluating each entry of _MGFs for a given theta value.
double MMBPArrival::calcSpectralRadius(double theta) const
{
    if (_MGFs.size() == 2) {
        // Hand-solved solution for 2 states
        double MGF0 = _MGFs[0]->calcMGF(theta);
        double MGF1 = _MGFs[1]->calcMGF(theta);
        double L1 = (_transitionMatrix[0][0] * MGF0 + _transitionMatrix[1][1] * MGF1 + sqrt((_transitionMatrix[0][0] * MGF0 - _transitionMatrix[1][1] * MGF1) * (_transitionMatrix[0][0] * MGF0 - _transitionMatrix[1][1] * MGF1) + 4.0 * _transitionMatrix[0][1] * _transitionMatrix[1][0] * MGF0 * MGF1)) / 2.0;
        double L2 = (_transitionMatrix[0][0] * MGF0 + _transitionMatrix[1][1] * MGF1 - sqrt((_transitionMatrix[0][0] * MGF0 - _transitionMatrix[1][1] * MGF1) * (_transitionMatrix[0][0] * MGF0 - _transitionMatrix[1][1] * MGF1) + 4.0 * _transitionMatrix[0][1] * _transitionMatrix[1][0] * MGF0 * MGF1)) / 2.0;
        return max(fabs(L1), fabs(L2));
    } else {
        // Generic solution for n states
        Eigen::MatrixXd m(_MGFs.size(), _MGFs.size());
        for (unsigned int fromState = 0; fromState < _transitionMatrix.size(); fromState++) {
            double stateMGF = _MGFs[fromState]->calcMGF(theta);
            if (!isfinite(stateMGF)) {
                return numeric_limits<double>::infinity();
            }
            for (unsigned int toState = 0; toState < _transitionMatrix[fromState].size(); toState++) {
                m(fromState, toState) = stateMGF * _transitionMatrix[fromState][toState];
            }
        }
        return m.eigenvalues().cwiseAbs().maxCoeff();
    }
}

// Equations for the MMBPArrival operator, representing an arrival process of a flow as analyzed by its trace.
void MMBPArrival::calcBound(double theta, double* sigma, double* rho) const
{
    *sigma = 0;
    *rho = log(calcSpectralRadius(theta)) / theta;
}

// Equations for the ConstantService operator, representing a constant service process with rate c.
void ConstantService::calcBound(double theta, double* sigma, double* rho) const
{
    *sigma = 0;
    *rho = -_c;
}

// Equations for the AggregateArrival operator, representing the aggregation of two arrival processes A and B.
void AggregateArrival::calcBound(double theta, double* sigma, double* rho) const
{
    double sigmaA, rhoA;
    double sigmaB, rhoB;
    _A->calcBound(getP() * theta, &sigmaA, &rhoA);
    _B->calcBound(getQ() * theta, &sigmaB, &rhoB);
    *sigma = sigmaA + sigmaB;
    *rho = rhoA + rhoB;
}

// Equations for the ConvolutionService operator, representing the convolution of two service processes S and T.
void ConvolutionService::calcBound(double theta, double* sigma, double* rho) const
{
    double sigmaS, rhoS;
    double sigmaT, rhoT;
    _S->calcBound(getP() * theta, &sigmaS, &rhoS);
    _T->calcBound(getQ() * theta, &sigmaT, &rhoT);
    // Handle the rhoS == rhoT case
    if (rhoS == rhoT) {
        rhoS *= 0.99;
    }
    *sigma = sigmaS + sigmaT - log(1.0 - exp(-theta * abs(rhoS - rhoT))) / theta;
    *rho = max(rhoS, rhoT);
}

// Equations for the OutputArrival operator, representing the departure process of an arrival process A after leaving a queue with service process S.
void OutputArrival::calcBound(double theta, double* sigma, double* rho) const
{
    double sigmaA, rhoA;
    double sigmaS, rhoS;
    _A->calcBound(getP() * theta, &sigmaA, &rhoA);
    _S->calcBound(getQ() * theta, &sigmaS, &rhoS);
    *sigma = sigmaA + sigmaS - log(1.0 - exp(theta * (rhoA + rhoS))) / theta;
    *rho = rhoA;
}

// Equations for the LeftoverService operator, representing the remaining service process that is leftover once a queue with service process S has accounted for the behavior of an arrival process A.
void LeftoverService::calcBound(double theta, double* sigma, double* rho) const
{
    double sigmaA, rhoA;
    double sigmaS, rhoS;
    _A->calcBound(getP() * theta, &sigmaA, &rhoA);
    _S->calcBound(getQ() * theta, &sigmaS, &rhoS);
    *sigma = sigmaA + sigmaS;
    *rho = rhoA + rhoS;
}

// Calculate the latency bound for a given theta value. Each invocation with a positive theta produces a valid (but possibly sub-optimal) upper bound on latency.
double LatencyBound::calcLatency(double theta) const
{
    double sigmaA, rhoA;
    double sigmaS, rhoS;
    _A->calcBound(getP() * theta, &sigmaA, &rhoA);
    _S->calcBound(getQ() * theta, &sigmaS, &rhoS);
    double latency = (log(_epsilon * (1.0 - exp(theta * (rhoA + rhoS)))) / theta - (sigmaA + sigmaS)) / rhoS;
    return latency * stepSize;
}

// Optimize over the space of positive theta values to search for the theta value that produces the best (i.e., tightest) latency bound.
double LatencyBound::calcTheta() const
{
    const double MIN_THETA = 1e-9;
    const double INITIAL_THETA = 1000.0;
    const double STEP_SIZE_DECREASE_FACTOR = 10.0;
    const double INITIAL_STEP_SIZE = INITIAL_THETA / STEP_SIZE_DECREASE_FACTOR;
    void* params = const_cast<LatencyBound*>(this);
    double theta = INITIAL_THETA;
    for (double stepSize = INITIAL_STEP_SIZE; stepSize >= MIN_THETA; stepSize /= STEP_SIZE_DECREASE_FACTOR) {
        theta = minSearch(max(theta - (STEP_SIZE_DECREASE_FACTOR * stepSize), MIN_THETA), theta + (STEP_SIZE_DECREASE_FACTOR * stepSize), stepSize, calcLatency, params);
    }
    return theta;
}

// Static helper function used by calcTheta.
double LatencyBound::calcLatency(double theta, void* params)
{
    const LatencyBound* p = static_cast<const LatencyBound*>(params);
    return p->calcLatency(theta);
}

// Calculate the latency bound using an optimized theta value.
double LatencyBound::calcLatency() const
{
    return calcLatency(calcTheta());
}

// Optimize the Hoelder dependency parameters. More research is needed to improve speed and accuracy.
double LatencyBound::dependencyOptimization() const
{
    const vector<DependencyParams*>& bounds = getDependentBounds();
    const unsigned int SEARCH_RANGE_DECREASE_COUNT = 25;
    const double SEARCH_RANGE_DECREASE_FACTOR = 1.2;
    const unsigned int ITERATION_COUNT = bounds.size() * 10;
    double minLatency = calcLatency();
    // Early exit if no dependencyParams to optimize
    if (bounds.size() == 0) {
        return minLatency;
    }
    // Optimize for the best Hoelder p value
    vector<double> bestP(bounds.size());
    for (unsigned int index = 0; index < bounds.size(); index++) {
        bestP[index] = bounds[index]->getP();
    }
    srand(1); // use fixed seed to avoid different latency calculations across multiple calls
    for (unsigned int i = 0; i < SEARCH_RANGE_DECREASE_COUNT; i++) {
        // Randomly search within search space
        for (unsigned int iter = 0; iter < ITERATION_COUNT; iter++) {
            // Set p/q
            for (unsigned int index = 0; index < bounds.size(); index++) {
                DependencyParams* bound = bounds[index];
                double searchRangeP = bound->getUpperP() - bound->getLowerP();
                double searchRangeQ = bound->getUpperQ() - bound->getLowerQ();
                double r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
                r *= searchRangeP + searchRangeQ;
                if (r <= searchRangeP) {
                    bound->setP(bound->getLowerP() + r);
                } else {
                    r -= searchRangeP;
                    bound->setQ(bound->getLowerQ() + r);
                }
            }
            // Check for better latency
            double latency = calcLatency();
            if (latency < minLatency) {
                minLatency = latency;
                for (unsigned int index = 0; index < bounds.size(); index++) {
                    bestP[index] = bounds[index]->getP();
                }
            }
        }
        // Update search space
        for (unsigned int index = 0; index < bounds.size(); index++) {
            DependencyParams* bound = bounds[index];
            bound->setP(bestP[index]);
            double searchRangeP = bound->getUpperP() - bound->getLowerP();
            searchRangeP /= SEARCH_RANGE_DECREASE_FACTOR;
            double searchRangeQ = bound->getUpperQ() - bound->getLowerQ();
            searchRangeQ /= SEARCH_RANGE_DECREASE_FACTOR;
            bound->setLowerP(max(bound->getP() - (searchRangeP / 2.0), 1.001));
            bound->setUpperP(bound->getLowerP() + searchRangeP);
            bound->setLowerQ(max(bound->getQ() - (searchRangeQ / 2.0), 1.001));
            bound->setUpperQ(bound->getLowerQ() + searchRangeQ);
        }
    }
    return minLatency;
}
