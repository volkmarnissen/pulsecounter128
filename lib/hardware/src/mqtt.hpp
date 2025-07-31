#pragma once
#include "common.hpp"
#include "config.hpp"
#include <functional>
#ifndef NATIVE
#include "mqtt_client.h"
#endif
const int maxClientIdLength = 100;
enum EventType
{
    MQTT_EV_CONNECTED,
    MQTT_EV_PUBLISHED,
    MQTT_EV_MESSAGE,
    MQTT_EV_ERROR
};

class MqttClient;
typedef void (*listenerFunction)(MqttClient *mqttClient, const char *, const char *);
typedef struct
{
    EventType evType;
    listenerFunction callback;
} EventListener;
class MqttClient
{

    // static MqttClient *theInstance;
    void logerror(const char *message, unsigned int code);
    char clientId[maxClientIdLength] = "clientId";
    std::vector<EventListener> listeners;
    void triggerEvent(EventType event, const char *topic = nullptr, const char *payload = nullptr)
    {
        for (const auto &listener : listeners)
        {
            if (listener.evType == event)
                (*listener.callback)(this, topic, payload);
        }
    }

protected:
    MqttClient();
    void init(const MqttConfig &config, const NetworkConfig &network);
    virtual ~MqttClient();
    void setClientId(const char *hostname, const char *clientId);
#ifndef NATIVE
    esp_mqtt_client_handle_t client;
    static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
#endif

public:
    int start(const MqttConfig &config, const NetworkConfig &network);
    void registerListener(EventType type, listenerFunction callback)
    {
        EventListener l = {type, callback};
        listeners.push_back(l);
    }

    const char *getClientId() const { return clientId; };
    virtual int stop();
    // static MqttClient &getMqttClient(const MqttConfig &config)
    // {
    //     if (theInstance == nullptr)
    //         theInstance = new MqttClient(config);
    //     return *theInstance;
    // }
    void publish(const char *topic, const char *payload);
};
