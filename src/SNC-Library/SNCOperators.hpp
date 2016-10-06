// SNCOperators.hpp - Class definitions for SNC operators.
// SNC analysis is performed by combining SNC operators based on the queueing network structure and calculating the latency using LatencyBound.
// The algorithm for determining how to combine the SNC operators is described in SNC.hpp.
// This file only defines the mathematics behind how the SNC operators manipulate probabilistic distributions on the request arrivals and queue service rates.
// We refer to these distributions as arrival processes and service processes, represented as abstract base classes SNCArrival and SNCService.
// The list of available SNC operators are as follows:
// MMBPArrival - initial arrival process derived from the trace of a flow
// ConstantService - initial service process of a queue
// AggregateArrival - arrival process representing the aggregate behavior of two arrival processes A and B
// ConvolutionService - service process representing the combined behavior of two tandem queues S and T (i.e., one after another in series)
// OutputArrival - arrival process into the next queue of an arrival process A flowing through a queue with service process S
// LeftoverService - service process leftover at a queue once we account for an arrival process A flowing through that queue with service process S
// Lastly, we have LatencyBound, which allows us to compute the latency of an arrival process A at a queue with service process S.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _SNC_OPERATORS_HPP
#define _SNC_OPERATORS_HPP

#include <stdint.h>
#include <vector>
#include <set>
#include <map>
#include "serializeJSON.hpp"
#include "NC.hpp"
#include "MGF.hpp"
#include "ProcessedTrace.hpp"

using namespace std;

// Forward declare test class.
class TestSNCOperators;

// We use the more common discrete-time version of SNC, where time is discretized into small time steps.
// We choose a small step size to approximate the behavior of a continuous time system.
const double stepSize = 0.00001; // in seconds

// All SNC operators and LatencyBound have two versions of equations, depending on if their sub-components (e.g., A and B for AggregateArrival; S and T for ConvolutionService) have dependencies between them.
// To handle the case of dependencies in SNC, we use the Hoelder inequality, which has two parameters p and q where 1/p + 1/q = 1.
// All values of p and q greater than 1 that satisfy this constraint are valid for computing latency, so we treat the choice of p and q as an optimization problem.
// To handle the independent case, we set p = q = 1, which happens to degenerate into the independent version of the equations for all SNC operators.
class DependencyParams
{
private:
    // Set of flow ids that this is dependent on
    set<FlowId> _dependencies;
    // Related components that are dependent. Includes this DependencyParams and any sub-components that are dependent.
    vector<DependencyParams*> _dependentBounds;
    // Dependency parameters and their optimization bounds
    double _p;
    double _q;
    double _lowerP; // lower bound for p when performing optimization
    double _upperP; // upper bound for p when performing optimization
    double _lowerQ; // lower bound for q when performing optimization
    double _upperQ; // upper bound for q when performing optimization

public:
    DependencyParams()
        : _p(1),
          _q(1),
          _lowerP(1),
          _upperP(1),
          _lowerQ(1),
          _upperQ(1)
    {}
    virtual ~DependencyParams()
    {}

    double getP() const { return _p; }
    double getQ() const { return _q; }
    double getLowerP() const { return _lowerP; }
    double getUpperP() const { return _upperP; }
    double getLowerQ() const { return _lowerQ; }
    double getUpperQ() const { return _upperQ; }
    void setLowerP(double p) { _lowerP = p; }
    void setUpperP(double p) { _upperP = p; }
    void setLowerQ(double q) { _lowerQ = q; }
    void setUpperQ(double q) { _upperQ = q; }
    void setP(double p)
    {
        if (p <= 1.0) {
            // Set to be independent
            _p = 1;
            _q = 1;
            setLowerP(1);
            setUpperP(1);
            setLowerQ(1);
            setUpperQ(1);
        } else {
            _p = p;
            _q = 1.0 / (1.0 - (1.0 / p)); // ensure 1/p + 1/q = 1
        }
    }
    void setQ(double q)
    {
        if (q <= 1.0) {
            // Set to be independent
            _p = 1;
            _q = 1;
            setLowerP(1);
            setUpperP(1);
            setLowerQ(1);
            setUpperQ(1);
        } else {
            _p = 1.0 / (1.0 - (1.0 / q)); // ensure 1/p + 1/q = 1
            _q = q;
        }
    }
    void resetOptBounds()
    {
        setLowerP(1.001);
        setUpperP(2.0);
        setLowerQ(1.001);
        setUpperQ(2.0);
        setP(2.0);
    }
    void setDependent()
    {
        resetOptBounds();
        _dependentBounds.push_back(this);
    }
    // Check if there is a dependency between bound and this. A dependency exists if both bound and this share the same flow id in their dependency sets.
    bool checkDependence(const DependencyParams* bound) const;
    // Get the components at are dependent
    const vector<DependencyParams*>& getDependentBounds() const { return _dependentBounds; }
    // Get the flow ids that affect this
    const set<FlowId>& getDependencies() const { return _dependencies; }
    // Add dependent flow ids
    void addDependency(FlowId newDependency)
    {
        _dependencies.insert(newDependency);
    }
    void addDependencies(const vector<FlowId>& newDependencies)
    {
        _dependencies.insert(newDependencies.begin(), newDependencies.end());
    }
    void addDependencies(const set<FlowId>& newDependencies)
    {
        _dependencies.insert(newDependencies.begin(), newDependencies.end());
    }
    // Add dependencies from the sub-component bound
    void addDependencies(const DependencyParams* bound)
    {
        addDependencies(bound->getDependencies());
        const vector<DependencyParams*>& dependentBounds = bound->getDependentBounds();
        _dependentBounds.insert(_dependentBounds.end(), dependentBounds.begin(), dependentBounds.end());
    }
};

// Base class for SNC operators representing arrival processes. Each SNCArrival represents an upper bound on an arrival process.
// Conceptually, each SNCArrival represents the full distributional information on an arrival process.
// Mathematically, this is done using the (rho_A, sigma_A) form where an arrival process A has a moment generating function (MGF):
//  MGF_A(m,n)(theta) <= exp(theta*(rho_A(theta)(n-m) + sigma_A(theta)))
// Importantly, to use SNC, we simply need to be able to calculate rho_A(theta) and sigma_A(theta) for any (positive) value of theta, which is performed via calcBound.
class SNCArrival : public DependencyParams
{
public:
    SNCArrival()
    {}
    virtual ~SNCArrival()
    {}

    // Calculate the SNC bound at a given theta value. The resulting value of rho_A(theta) and sigma_A(theta) are returned in rho and sigma.
    virtual void calcBound(double theta, double* sigma, double* rho) const = 0;
};

// Base class for SNC operators representing service processes. Each SNCService represents a lower bound on a service process.
// Conceptually, each SNCService represents the full distributional information on a service process.
// Mathematically, this is done using the (rho_A, sigma_A) form where an service process S has a moment generating function (MGF):
//  MGF_S(m,n)(-theta) <= exp(theta*(rho_S(theta)(n-m) + sigma_S(theta)))
// Importantly, to use SNC, we simply need to be able to calculate rho_S(theta) and sigma_S(theta) for any (positive) value of theta, which is performed via calcBound.
class SNCService : public DependencyParams
{
public:
    SNCService()
    {}
    virtual ~SNCService()
    {}

    // Calculate the SNC bound at a given theta value. The resulting value of rho_S(theta) and sigma_S(theta) are returned in rho and sigma.
    virtual void calcBound(double theta, double* sigma, double* rho) const = 0;
};

// struct for the helper function LAMBDAAlgorithm
struct LAMBDAParams_t {
    double low; // min arrival rate in trace
    double high; // max arrival rate in trace
    vector<double>* pLambdas; // output vector of lambda values
};

// SNC operator for an arrival process of a flow as analyzed by its trace.
// Each flow is modeled based on a Markov-Modulated Bernoulli Process (MMBP), and the model parameters are estimated using the analysis in init().
// The MMBP is the discrete-time version of the Markov-Modulated Poisson Process (MMPP), and is used to represent periods of burstiness in a flow.
class MMBPArrival : public SNCArrival, public Serializable
{
    friend class TestSNCOperators;

private:
    // The interval length in nanoseconds for splitting a trace during analysis.
    static const uint64_t intervalWidth; // defined in SNCOperators.cpp
    // The maximum number of allowed MMBP states.
    static const unsigned int maxNumStates = 10;

    // _transitionMatrix[fromState][toState] represents the probability of transitioning from state fromState to state toState.
    vector<vector<double> > _transitionMatrix;
    // _MGFs[s] represents the arrival rate and request size distribution of state s.
    vector<MGF*> _MGFs;

    // Top-level function for analyzing a trace and initializing the MMBP parameters based on the analysis.
    // Analysis is performed using the following steps:
    // 1) Split trace into intervals of length intervalWidth; used to calculate arrival rates in each interval
    // 2) Assign a state to each interval using the LAMBDA algorithm; based on arrival rates in each interval
    // 3) Initialize the transition matrix between states
    // 4) Initialize MMBP parameters for each state, which are represented as moment generating functions (MGFs)
    void init(ProcessedTrace* pTrace);
    // Count the number of arrivals in each interval in the trace pTrace and store the results in intervals.
    void countArrivalIntervals(ProcessedTrace* pTrace, vector<double>& intervals) const;
    // Helper function for performing the LAMBDA algorithm.
    static double LAMBDAAlgorithm(double a, void* params);
    // Assign a state to each interval in the trace using the LAMBDA algorithm. Returns number of states used, up to maxNumStates.
    // The LAMBDA algorithm identifies a set of arrival rates (lambdas) such that a MMBP with these arrival rates can express the behavior in the trace.
    // --- max arrival rate in trace ---
    //     ^
    // lambdas[2]
    //     v
    // ----------
    //     ^
    // lambdas[1]
    //     v
    // ----------
    //     ^
    // lambdas[0]
    //     v
    // --- min arrival rate in trace ---
    // The range around each lambda is computed such that there is a confidence interval around lambda as specified by the parameter a
    // (e.g., a = 2 means a range of two standard deviations above/below each lambda).
    // Each interval in the trace is assigned (in the states vector) a 0-indexed state number indicating which MMBP state it belongs to
    // (e.g., state = 0 indicates the MMBP state with arrival rate lambdas[0]).
    unsigned int determineStatesLAMBDA(const vector<double>& intervals, vector<unsigned int>& states, vector<double>& lambdas, double a) const;
    // Initialize the transition matrix between states.
    void initTransitionMatrix(unsigned int numStates, const vector<unsigned int>& states);
    // Helper function for creating the MGF for a MMBP state.
    MGF* createMMBPStateMGF();
    // Initialize each MMBP state with its associated arrival rate and request size distribution, which is represented by a moment generating function (MGF).
    // The amount of work generated by each MMBP state is affected by both the arrival rate and the request size of each state.
    // initMGFs extracts the request size distribution information from the trace for each state.
    void initMGFs(ProcessedTrace* pTrace, unsigned int numStates, const vector<unsigned int>& states, const vector<double>& lambdas);

    // Calculate the spectral radius of the matrix: Diag(_MGFs(theta)) * _transitionMatrix,
    // where Diag(_MGFs(theta)) is the diagonal matrix from evaluating each entry of _MGFs for a given theta value.
    // The spectral radius is a linear algebra operation that computes the maximum absolute value of the eigenvalues of a matrix.
    double calcSpectralRadius(double theta) const;

public:
    MMBPArrival(ProcessedTrace* pTrace)
    {
        init(pTrace);
    }
    // Create a new MMBPArrival based on serialized JSON object.
    // Dependencies are not serialized since FlowIds are local to an instance.
    MMBPArrival(const Json::Value& json)
    {
        deserialize(json);
    }
    virtual ~MMBPArrival()
    {
        // Delete MGFs
        while (!_MGFs.empty()) {
            delete _MGFs.back();
            _MGFs.pop_back();
        }
    }

    virtual void calcBound(double theta, double* sigma, double* rho) const;

    // Create a new MMBPArrival based on serialized JSON object.
    // Dependencies are not serialized since FlowIds are local to an instance.
    static MMBPArrival* create(const Json::Value& json)
    {
        return new MMBPArrival(json);
    }
    virtual void serialize(Json::Value& json) const
    {
        serializeJSON(json, "transitionMatrix", _transitionMatrix);
        serializeJSON(json, "MGFs", _MGFs);
    }
    virtual void deserialize(const Json::Value& json)
    {
        deserializeJSON(json, "transitionMatrix", _transitionMatrix);
        deserializeJSON(json, "MGFs", _MGFs);
    }
};

// SNC operator for a constant service process with rate c.
// c is specified in terms of work units per second (see Estimator.hpp for definition of work).
class ConstantService : public SNCService
{
private:
    const double _c;

public:
    ConstantService(double c)
        : _c(c * stepSize)
    {}
    virtual ~ConstantService()
    {}

    virtual void calcBound(double theta, double* sigma, double* rho) const;
};

// SNC operator for the aggregation of two arrival processes A and B.
class AggregateArrival : public SNCArrival
{
private:
    const SNCArrival* _A;
    const SNCArrival* _B;

public:
    AggregateArrival(const SNCArrival* A, const SNCArrival* B)
        : _A(A),
          _B(B)
    {
        if (_A->checkDependence(_B)) {
            setDependent();
        }
        addDependencies(_A);
        addDependencies(_B);
    }
    virtual ~AggregateArrival()
    {}

    virtual void calcBound(double theta, double* sigma, double* rho) const;
};

// SNC operator for the convolution of two service processes S and T.
class ConvolutionService : public SNCService
{
private:
    const SNCService* _S;
    const SNCService* _T;

public:
    ConvolutionService(const SNCService* S, const SNCService* T)
        : _S(S),
          _T(T)
    {
        if (_S->checkDependence(_T)) {
            setDependent();
        }
        addDependencies(_S);
        addDependencies(_T);
    }
    virtual ~ConvolutionService()
    {}

    virtual void calcBound(double theta, double* sigma, double* rho) const;
};

// SNC operator for the departure process D of an arrival process A after leaving a queue with service process S (i.e., D = OutputArrival(A, S)).
// D is thus an arrival process into the next queue.
class OutputArrival : public SNCArrival
{
private:
    const SNCArrival* _A;
    const SNCService* _S;

public:
    OutputArrival(const SNCArrival* A, const SNCService* S)
        : _A(A),
          _S(S)
    {
        if (_A->checkDependence(_S)) {
            setDependent();
        }
        addDependencies(_A);
        addDependencies(_S);
    }
    virtual ~OutputArrival()
    {}

    virtual void calcBound(double theta, double* sigma, double* rho) const;
};

// SNC operator for the remaining service process that is leftover once a queue with service process S has accounted for the behavior of an arrival process A.
class LeftoverService : public SNCService
{
private:
    const SNCArrival* _A;
    const SNCService* _S;

public:
    LeftoverService(const SNCArrival* A, const SNCService* S)
        : _A(A),
          _S(S)
    {
        if (_A->checkDependence(_S)) {
            setDependent();
        }
        addDependencies(_A);
        addDependencies(_S);
    }
    virtual ~LeftoverService()
    {}

    virtual void calcBound(double theta, double* sigma, double* rho) const;
};

// Use LatencyBound's calcLatency() function to compute an upper bound on the latency of an arrival process A experiencing a service process S.
// As SNC works with probabilistic quantities, use epsilon to specify the latency percentile of interest (e.g., use epsilon = 0.001 for 99.9th percentile latency).
// Mathematically, Pr[latency > calcLatency()] < epsilon.
class LatencyBound : public DependencyParams
{
    friend class TestSNCOperators;

private:
    const SNCArrival* _A;
    const SNCService* _S;
    const double _epsilon;

    // Calculate the latency bound for a given theta value. Each invocation with a positive theta produces a valid (but possibly sub-optimal) upper bound on latency.
    double calcLatency(double theta) const;
    // Optimize over the space of positive theta values to search for the theta value that produces the best (i.e., tightest) latency bound.
    double calcTheta() const ;
    // Static helper function used by calcTheta.
    static double calcLatency(double theta, void* params);

public:
    LatencyBound(const SNCArrival* A, const SNCService* S, double epsilon)
        : _A(A),
          _S(S),
          _epsilon(epsilon)
    {
        if (_A->checkDependence(_S)) {
            setDependent();
        }
        addDependencies(_A);
        addDependencies(_S);
    }
    ~LatencyBound()
    {}

    // Calculate the latency bound using an optimized theta value.
    double calcLatency() const;

    // Optimize the Hoelder dependency parameters.
    double dependencyOptimization() const;
};

#endif // _SNC_OPERATORS_HPP
