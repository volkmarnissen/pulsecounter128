#include "pcscheduler.hpp"
#include "pulsecounter.hpp"
#include "mqtt.hpp"

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
                        countersStorage[a].datetime,
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
#ifdef NATIVE
protected:
#endif
    virtual void onPublished(const char *topic, const char *payload)
    {
        scheduler.reset();
        stop();
    };
    virtual void onError(void)
    {
        stop();
    };

public:
    PCMqttClient(const Config &config, PulseCounterScheduler &_scheduler) : MqttClient(config.getMqtt(), config.getNetwork()), scheduler(_scheduler) {};
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