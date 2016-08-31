SNC-MEISTER README
==================

This document contains the documentation for the SNC-Meister network QoS system.

TO BUILD:
---------

In the src directory, run:

`make`

TOPOLOGY CONFIGURATION FILE:
----------------------------

Topology configuration files are high level user input files that describe the tenants in the system. They are used to generate SNC-Meister configuration files (see SNC-MEISTER CONFIGURATION FILE) using SNC-ConfigGen. Topology files are in JSON format and are comprised of a list of tenants (a.k.a. clients) and the following global entries:

* "debug": int (optional) - set to 1 to enable extra debug information
* "addrPrefix": string (optional) - prefix for the hostname of the client/server VMs; assumes VM hostnames are named according to the getAddr function in SNC-ConfigGen.cpp. If addrPrefix is not specified, SNC-Meister will not update enforcers
* "outputConfig": string - file path to write output
* "clients": list client - list of clients (a.k.a. tenants)

Each tenant configuration in the clients list contains the following entries:

* "name": string (optional) - name of client; defaults to getClientName function in SNC-ConfigGen.cpp
* "SLO": float - client's desired tail latency goal in seconds
* "SLOpercentile": float (optional) - client's desired tail latency percentile for the target SLO between 0 and 100; defaults to 99.9
* "trace": string - file path of trace file describing client behavior; see SNC-Library/TraceReader.hpp for trace file format
* "clientHost": string - hostname of machine that hosts the client VM
* "clientVM": string - identifier for the client VM
* "serverHost": string - hostname of machine that hosts the server VM
* "serverVM": string - identifier for the server VM
* "dependencies": list string (optional) - list of other client names that are dependent on this client; defaults to no dependencies

A sample topology file can be found at examples/sample_topology.txt.

SNC-MEISTER CONFIGURATION FILE:
-------------------------------

While the topology configuration is a high level input that specifies the tenant SLOs, traces, and locations, the SNC-Meister configuration is a lower level input that additionally includes all the details of the cluster configuration. To generate a SNC-Meister configuration, run:

`./src/SNC-ConfigGen/SNC-ConfigGen -t topoFilename -c configFilename`

where `topoFilename` is a path to the topology configuration (see TOPOLOGY CONFIGURATION FILE) and `configFilename` is a path to output the generated SNC-Meister configuration.

A sample config file can be found at examples/sample_config.txt, and it was generated using SNC-ConfigGen based on examples/sample_topology.txt. More details on config file format can be found in SNC-Calculator/SNC-Calculator.cpp and SNC-Library/NC.hpp.

SNC-ConfigGen assumptions:

* uses hard-coded network bandwidth of 125000000 bytes per sec = 1Gbps and various hard-coded traffic estimation parameters (see SNC-ConfigGen.cpp).
* full-bisection bandwidth network topology where the congestion is primarily at the end-host network links; SNC-ConfigGen builds this hard-coded network topology into the SNC-Meister configuration.
* VMs hostnames are named according to the getAddr function in SNC-ConfigGen.cpp (only relevant if deploying on a cluster).

TO RUN SNC-MEISTER:
-------------------

If deploying on a cluster, then on each machine's host OS, run:

`./src/NetEnforcer/NetEnforcer [-d dev] [-b maxBandwidth (in bytes per sec)] [-n numPriorities]`

where `dev` is the network device (default eth0), `maxBandwidth` is the machine's network bandwidth in bytes per sec (default 125000000 = 1Gbps), and `numPriorities` is the maximum number of priorities (default 7; max 8).

To start the SNC-Meister admission control server, run:

`./src/SNC-Meister/SNC-Meister`

For a tenant to seek admission, run:

`./src/SNC-MeisterClient/SNC-MeisterClient -c configFilename -s serverAddr [-g]`

where `configFilename` is a path to the SNC-Meister configuration (see SNC-MEISTER CONFIGURATION FILE), `serverAddr` is the address of the SNC-Meister admission control server, and `g` specifies that all the clients in the config file must be accepted/rejected as a group rather than admitting/rejecting one-by-one.

TO CALCULATE LATENCY WITH SNC:
------------------------------

Run:

`./src/SNC-Calculator/SNC-Calculator -c configFilename`

where `configFilename` is a path to the SNC-Meister configuration (see SNC-MEISTER CONFIGURATION FILE).

SRC DIRECTORIES:
----------------

### Core components

* SNC-Library - core code for calculating tail latency with Stochastic Network Calculus (SNC)
* SNC-Meister - SNC-Meister admission control server
* SNC-MeisterClient - SNC-Meister admission control client
* NetEnforcer - network QoS enforcement module; configures Linux Traffic Control (TC) to perform prioritization of traffic

### Utilities

* SNC-ConfigGen - generate config files based on a high level topology file
* SNC-Calculator - calculate latency using SNC

### Cross-component code

* prot - RPC protocol definitions

### Test code

SNC-LibraryTest - test code for SNC-Library

### Library headers

* json - JSON manipulation library headers
* Eigen - Eigen linear algebra library headers
