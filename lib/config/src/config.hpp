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
    int divider;

public:
    int getInputPort() const { return inputPort; };
    int getOutputPort() const { return outputPort; };
    int getDivider() const { return divider; };
};

class NetworkConfig
{
protected:
    std::string hostname;
    std::string sslcert;

public:
    const char *getSslcert() const { return sslcert.c_str(); };
    const char *getHostname() const { return hostname.c_str(); };
};

class MqttConfig
{
protected:
    std::string mqtturl;
    std::string username;
    std::string password;
    AuthenticationMethod authenticationMethod;

public:
    const char *getMqtturl() const { return mqtturl.c_str(); };
    const char *getUsername() const { return username.c_str(); };
    const char *getPassword() const { return password.c_str(); };
    AuthenticationMethod getAuthenticationMethod() const { return authenticationMethod; };
};

class ScheduleConfig
{
protected:
    std::string hour;
    std::string minute;
    std::string second;

public:
    const char *getHour() const { return hour.c_str(); };
    const char *getMinute() const { return minute.c_str(); };
    const char *getSecond() const { return second.c_str(); };
};

class Config
{
protected:
    std::vector<CounterConfig> counters;
    std::vector<OutputConfig> outputs;
    NetworkConfig network;
    MqttConfig mqtt;
    ScheduleConfig schedule;

public:
    const std::vector<CounterConfig> &getCounters() const { return const_cast<std::vector<CounterConfig> &>(counters); };
    const std::vector<OutputConfig> &getOutputs() const { return const_cast<std::vector<OutputConfig> &>(outputs); };
    const NetworkConfig &getNetwork() const { return const_cast<NetworkConfig &>(network); };
    const MqttConfig &getMqtt() const { return const_cast<MqttConfig &>(mqtt); };
    const ScheduleConfig &getSchedule() const { return const_cast<ScheduleConfig &>(schedule); };
    static const Config &getConfig(const char *jsonContent = NULL);
};
