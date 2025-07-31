#pragma once
#include "scheduler.hpp"
#include "config.hpp"
typedef struct
{
    std::time_t datetime;
    u_int8_t outputPort;
    float counts[16];
} CountersStorage;
class PCMqttClient;
class PulseCounterScheduler : public Scheduler
{
    static PCMqttClient *mqttClient;
#ifdef NATIVE
public:
#endif
    Config &config;
    void storePulseCounts(time_t date) const;
    std::string generatePayload() const;
    int publish(const std::string &payload);

public:
    PulseCounterScheduler(Config &_config);
    void setConfig(Config &config);

    const char *checkConfiguration(const Config &_config) const;
    void execute();
    void reset();
};
