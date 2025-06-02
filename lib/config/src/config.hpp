#pragma once
#include <string>
#include <vector>
#include "common.hpp"
class OutputConfig
{
protected:
    OutputConfigurationType type;
    int port;

public:
    OutputConfigurationType getType() { return type; };
    int getPort() { return port; };
};

class CounterConfig
{
protected:
    int inputPort;
    int outputPort;
    int multiplier;

public:
    int getInputPort() { return inputPort; };
    int getOutputPort() { return outputPort; };
    int getMultiplier() { return multiplier; };
};

class NetworkConfig
{
protected:
    std::string hostname;
    std::string sslcert;

public:
    const char *getSslcert() { return sslcert.c_str(); };
    const char *getHostname() { return hostname.c_str(); };
};

class Config
{
protected:
    std::vector<CounterConfig> counters;
    std::vector<OutputConfig> outputs;
    NetworkConfig network;

public:
    const std::vector<CounterConfig> &getCounters() { return const_cast<std::vector<CounterConfig> &>(counters); };
    const std::vector<OutputConfig> &getOutputs() { return const_cast<std::vector<OutputConfig> &>(outputs); };
    static const Config &getConfig(const char *jsonContent = NULL);
};
