#pragma once
#include <string>
#include <vector>
#include "common.hpp"
static const std::string defaultHostName = "plscount";
class OutputConfiguration
{
public:
    OutputConfigurationType type;
};
class OutputConfig
{
protected:
    OutputConfiguration config;
    int port;

public:
    const OutputConfiguration getConfiguration() const { return config; };
    int getPort() const { return port; };
};

class CounterConfig
{
protected:
    int inputPort;
    int outputPort;
    int divider;
    std::string mqttname;

public:
    int getInputPort() const { return inputPort; };
    int getOutputPort() const { return outputPort; };
    int getDivider() const { return divider; };
    const char *getMqttName() const { return mqttname.c_str(); };
};

class NetworkConfig
{
protected:
    std::string hostname;
    std::string sslhost;
    std::string sslhostkey;
    std::string sslca;
    std::string ntpserver;

public:
    const char *getSslHost() const { return sslhost.c_str(); };
    const char *getSslHostKey() const { return sslhostkey.c_str(); };
    const char *getSslCa() const { return sslca.c_str(); };
    const char *getHostname() const { return (hostname.empty() ? defaultHostName.c_str() : hostname.c_str()); };
    const char *getNtpserver() const { return ntpserver.c_str(); };
};

class MqttConfig
{
protected:
    std::string mqtturl;
    std::string topic;
    std::string username;
    std::string password;
    AuthenticationMethod authenticationMethod;

public:
    const char *getMqtturl() const { return mqtturl.c_str(); };
    const char *getTopic() const { return topic.c_str(); };
    const char *getUsername() const { return username.c_str(); };
    const char *getPassword() const { return password.c_str(); };
    AuthenticationMethod getAuthenticationMethod() const { return authenticationMethod; };
};

class ScheduleConfig
{
protected:
    std::vector<int> hour;
    std::vector<int> minute;
    std::vector<int> second;

public:
    const std::vector<int> &getHour() const { return const_cast<std::vector<int> &>(hour); };
    const std::vector<int> &getMinute() const { return const_cast<std::vector<int> &>(minute); };
    const std::vector<int> &getSecond() const { return const_cast<std::vector<int> &>(second); };
};

class Config
{
    static std::string addBuildDate(const char *json);

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
    static const std::string getJson();
    static void setJson(const char *setJson);
};
