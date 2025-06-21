#pragma once
#include "scheduler.hpp"
#include "config.hpp"
typedef struct
{
    std::time_t datetime;
    u_int8_t outputPort;
    float counts[16];
} CountersStorage;
class PulseCounterScheduler : public Scheduler
{
#ifdef NATIVE
public:
#endif
    const Config &config;
    void storePulseCounts(time_t date) const;
    std::string generatePayload() const;
    void publish(const char *topic, const char *payload);

public:
    PulseCounterScheduler(const Config &_config) : Scheduler(const_cast<CONST_CONFIG ScheduleConfig &>(_config.getSchedule())), config(_config) {};
    const char *isConfigured() const;
    void execute();
    void reset();
};
