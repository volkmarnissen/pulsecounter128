#include "pcscheduler.hpp"
#include "pulsecounter.hpp"
#include "mqtt.hpp"
#include <functional>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <array>
#include "pclog.hpp"
#define TAG "pcscheduler"
static CountersStorage countersStorage[128];
static uint8_t counterStorageCount = 0;

PCMqttClient *PulseCounterScheduler::mqttClient = nullptr;

PulseCounterScheduler::PulseCounterScheduler(Config &_config) : Scheduler(const_cast<CONST_CONFIG ScheduleConfig &>(_config.getSchedule())), config(_config) {
                                                                };

// Save current pulse counts send to MQTT
void PulseCounterScheduler::execute()
{
    ESP_LOGI(TAG, "PulseCounterScheduler::execute\n");
    std::time_t dt = std::time(0);
    storePulseCounts(dt);
    publish(generatePayload().c_str());
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
        ESP_LOGI(TAG, "storePulseCounts %d %lu\n", it->getInputPort(), (unsigned long)counts);
        bool found = false;
        for (int a = 0; !found && a < counterStorageCount; a++)
            if (countersStorage[a].datetime == date && countersStorage[a].outputPort == it->getOutputPort())
            {
                countersStorage[a].counts[it->getInputPort()] = counts;
                found = true;
            };
        if (!found && counterStorageCount < sizeof(countersStorage) / sizeof(countersStorage[0]) - 1)
        {
            countersStorage[counterStorageCount].datetime = date;
            countersStorage[counterStorageCount].outputPort = it->getOutputPort();
            countersStorage[counterStorageCount++].counts[it->getInputPort()] = counts;
        };
    }
    // reset Pulsecounter
    Pulsecounter::reset();
};
int cmpByDate(CountersStorage *a, CountersStorage *b)
{
    if (a->datetime == b->datetime)
        if (a->outputPort == b->outputPort)
            return a->counts[0] > b->counts[0];
        else
            return a->outputPort > b->outputPort;
    else
        return a->datetime > b->datetime;
}

std::string PulseCounterScheduler::generatePayload() const
{
    std::string payload = "";
    qsort(countersStorage, counterStorageCount, sizeof(countersStorage[0]), (__compar_fn_t)&cmpByDate);
    std::time_t date = 0;

    for (int a = 0; a < counterStorageCount; a++)
    {
        if (date != countersStorage[a].datetime)
        {
            date = countersStorage[a].datetime;
            if (a > 0)
                payload += "},\n";
            payload += "{\"date\": " + std::to_string(date);
        };

        for (auto it = begin(config.getCounters()); it != end(config.getCounters()); ++it)
        {
            if (countersStorage[a].outputPort == it->getOutputPort())
            {
                char buf[256];
                sprintf(buf, ",\n\"%s\":%g",
                        it->getMqttName(),
                        ((double)countersStorage[a].counts[it->getInputPort()]) / it->getDivider());
                payload += buf;
            }
        }
    }
    counterStorageCount = 0;
    return "[" + payload + "}]";
};

class PCMqttClient : public MqttClient
{

    PulseCounterScheduler &scheduler;
    bool isConfigured;
    const Config &config;
    const std::string topic;
    const std::string payload;
#ifdef NATIVE
protected:
#endif
    static void onPublished(MqttClient *client, const char *topic, const char *payload)
    {
        ((PCMqttClient *)client)->scheduler.reset();
    };
    static void onConnected(MqttClient *client, const char *, const char *)
    {
        client->publish(((PCMqttClient *)client)->topic.c_str(), ((PCMqttClient *)client)->payload.c_str());
    };

public:
    PCMqttClient(const Config &_config, PulseCounterScheduler &_scheduler, const std::string &_payload) : MqttClient(), scheduler(_scheduler), config(_config), topic(config.getMqtt().getTopic()), payload(_payload)
    {
        setClientId(config.getNetwork().getHostname(), "scheduler");
    };
    int start(void)
    {
        registerListener(MQTT_EV_PUBLISHED, PCMqttClient::onPublished);
        registerListener(MQTT_EV_CONNECTED, PCMqttClient::onConnected);
        return MqttClient::start(config.getMqtt(), config.getNetwork());
    }
};

void PulseCounterScheduler::reset()
{
    counterStorageCount = 0;
}

int PulseCounterScheduler::publish(const std::string &payload)
{
    int rc = 3;
    if (PulseCounterScheduler::mqttClient != nullptr)
    {
        ESP_LOGI(TAG, "PCScheduler: mqttClient is in use abort publishing\n");
        return rc;
    }

    PulseCounterScheduler::mqttClient = new PCMqttClient(config, *this, payload);
    if (payload.empty())
    {
        ESP_LOGI(TAG, "PCScheduler: Payload is empty");
        rc = 1;
    }
    else if (0 == PulseCounterScheduler::mqttClient->start())
    {
        ESP_LOGI(TAG, "PCScheduler: publishing %s %s\n", config.getMqtt().getTopic(), payload.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        rc = 0;
    }
    else
    {
        ESP_LOGI(TAG, "Unable to start MQTT Client for sending pulsecounter result");
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