#include "pcscheduler.hpp"
#include "pulsecounter.hpp"
#include "mqtt.hpp"
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>

static CountersStorage countersStorage[128];
static uint8_t counterStorageCount = 0;

PCMqttClient *PulseCounterScheduler::mqttClient = nullptr;

PulseCounterScheduler::PulseCounterScheduler(Config &_config) : Scheduler(const_cast<CONST_CONFIG ScheduleConfig &>(_config.getSchedule())), config(_config) {
                                                                };

// Save current pulse counts send to MQTT
void PulseCounterScheduler::execute()
{
    std::time_t dt = std::time(0);
    storePulseCounts(dt);
    publish(config.getMqtt().getTopic(), generatePayload().c_str());
    // build MQTT payload
    // MQTT publish
    // Success:
    // counterStorageCount = 0
    // Failure:
    //  Just continue.
}

void PulseCounterScheduler::setConfig(Config &_config)
{
    config = _config;
    Scheduler::setConfig((ScheduleConfig &)config.getSchedule());
}

void PulseCounterScheduler::storePulseCounts(time_t date) const
{
    // Store in memory
    for (auto it = begin(config.getCounters()); it != end(config.getCounters()); ++it)
    {
        uint32_t counts = Pulsecounter::getCounts(it->getOutputPort(), it->getInputPort());
        bool found = false;
        for (int a = 0; !found && a < counterStorageCount; a++)
            if (countersStorage[a].datetime == date && countersStorage[a].outputPort == it->getOutputPort())
            {
                countersStorage[a].counts[it->getInputPort()] = counts;
                found = true;
            };
        if (!found)
        {
            countersStorage[counterStorageCount].datetime = date;
            countersStorage[counterStorageCount].outputPort = it->getOutputPort();
            countersStorage[counterStorageCount++].counts[it->getInputPort()] = counts;
        };
    }
    // reset Pulsecounter
    Pulsecounter::reset();
};

std::string PulseCounterScheduler::generatePayload() const
{
    std::string payload = "";
    for (auto it = begin(config.getCounters()); it != end(config.getCounters()); ++it)
    {
        for (int a = 0; a < counterStorageCount; a++)
            if (countersStorage[a].outputPort == it->getOutputPort())
            {
                char buf[256];
                sprintf(buf, "{\"name\":\"%s\"\n\"date\":%ld\n\"value\":%g\n}\n",
                        it->getMqttName(),
                        (long)countersStorage[a].datetime,
                        ((double)countersStorage[a].counts[it->getInputPort()]) / it->getDivider());
                if (payload.length() > 0)
                    payload += ",";
                payload += buf;
            }
    }
    return "[" + payload + "]";
};

class PCMqttClient : public MqttClient
{
    PulseCounterScheduler &scheduler;
    bool isConfigured;
#ifdef NATIVE
protected:
#endif
    virtual void onPublished(const char *topic, const char *payload)
    {
        scheduler.reset();
    };
    virtual void onError(const char *message, unsigned int code) {
    };

public:
    PCMqttClient(const Config &config, PulseCounterScheduler &_scheduler) : MqttClient(config.getMqtt(), config.getNetwork()), scheduler(_scheduler) {
                                                                            };
};

void PulseCounterScheduler::reset()
{
    counterStorageCount = 0;
}

int PulseCounterScheduler::publish(const char *topic, const char *payload)
{
    int rc = 3;
    if (PulseCounterScheduler::mqttClient != nullptr)
        return rc;
    PulseCounterScheduler::mqttClient = new PCMqttClient(config, *this);
    if (topic == nullptr || topic[0] == '\0')
    {
        fprintf(stderr, "PCScheduler: No topic passed to publish %s", payload);
        rc = 1;
    }
    else if (payload == nullptr || payload[0] == '\0')
    {
        fprintf(stderr, "PCScheduler: No payload passed for topic %s", topic);
        rc = 1;
    }
    else if (0 == PulseCounterScheduler::mqttClient->start())
    {
        PulseCounterScheduler::mqttClient->publish(topic, payload);
        std::this_thread::sleep_for(std::chrono::milliseconds(400));

        rc = 0;
    }
    else
    {
        fprintf(stderr, "Unable to start MQTT Client for sending pulsecounter result");
        delete PulseCounterScheduler::mqttClient;
        PulseCounterScheduler::mqttClient = nullptr;
        rc = 2;
    }
    delete PulseCounterScheduler::mqttClient;
    PulseCounterScheduler::mqttClient = nullptr;
    return rc;
}

static const char *checkRequiredParameter(const char *name, const char *value)
{
    if (value != nullptr && strlen(value) > 0)
        return nullptr;
    return (std::string("Required parameter ") + std::string(name) + std::string(" is not configured")).c_str();
}

const char *PulseCounterScheduler::checkConfiguration(const Config &cfg) const
{
    const char *parameter = checkRequiredParameter("MQTT URL", cfg.getMqtt().getMqtturl());
    if (parameter != nullptr)
        return parameter;
    parameter = checkRequiredParameter("MQTT topic", cfg.getMqtt().getTopic());
    if (parameter != nullptr)
        return parameter;

    switch (cfg.getMqtt().getAuthenticationMethod())
    {
    case userPassword:
        parameter = checkRequiredParameter("MQTT Username", cfg.getMqtt().getUsername());
        if (parameter != nullptr)
            return parameter;
        return checkRequiredParameter("MQTT Password", cfg.getMqtt().getPassword());
    case SSL:
        parameter = checkRequiredParameter("SSL root ca", cfg.getNetwork().getSslCa());
        if (parameter != nullptr)
            return parameter;
        parameter = checkRequiredParameter("SSL host", cfg.getNetwork().getSslHost());
        if (parameter != nullptr)
            return parameter;
        return checkRequiredParameter("SSL host key", cfg.getNetwork().getSslHostKey());
    default:
        return nullptr;
    }
}
#ifdef NATIVE

CountersStorage *getCountersStorage()
{
    return countersStorage;
}
int getCounterStorageCount()
{
    return counterStorageCount;
}
#endif