#include "pcscheduler.hpp"
#include "pulsecounter.hpp"
#include "mqtt.hpp"
#include <stdio.h>
#include <string.h>
static CountersStorage countersStorage[128];
static uint8_t counterStorageCount = 0;
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
        stop();
    };
    virtual void onError(const char *message, unsigned int code)
    {
        stop();
    };

public:
    PCMqttClient(const Config &config, PulseCounterScheduler &_scheduler) : MqttClient(config.getMqtt(), config.getNetwork()), scheduler(_scheduler) {
                                                                            };
};

void PulseCounterScheduler::reset()
{
    counterStorageCount = 0;
}

void PulseCounterScheduler::publish(const char *topic, const char *payload)
{
    PCMqttClient mqttClient(config, *this);
    mqttClient.start();
    mqttClient.publish(topic, payload);
}
static const char *checkRequiredParameter(const char *name, const char *value)
{
    if (value != nullptr && strlen(value) > 0)
        return nullptr;
    return (std::string("Required parameter ") + std::string(name) + std::string(" is not configured")).c_str();
}

const char *PulseCounterScheduler::isConfigured() const
{
    const char *parameter = checkRequiredParameter("MQTT URL", config.getMqtt().getMqtturl());
    if (parameter != nullptr)
        return parameter;
    switch (config.getMqtt().getAuthenticationMethod())
    {
    case userPassword:
        parameter = checkRequiredParameter("MQTT Username", config.getMqtt().getUsername());
        if (parameter != nullptr)
            return parameter;
        return checkRequiredParameter("MQTT Password", config.getMqtt().getPassword());
    case SSL:
        parameter = checkRequiredParameter("SSL root ca", config.getNetwork().getSslCa());
        if (parameter != nullptr)
            return parameter;
        parameter = checkRequiredParameter("SSL host", config.getNetwork().getSslHost());
        if (parameter != nullptr)
            return parameter;
        return checkRequiredParameter("SSL host key", config.getNetwork().getSslHostKey());
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