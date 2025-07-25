#pragma once
#include "common.hpp"
#include "config.hpp"
#ifndef NATIVE
#include "mqtt_client.h"
#endif
const int maxClientIdLength = 100;
class MqttClient
{
    // static MqttClient *theInstance;
    void logerror(const char *message, unsigned int code);
    char clientId[maxClientIdLength] = "clientId";

protected:
    MqttClient();
    void init(const MqttConfig &config, const NetworkConfig &network);
    virtual ~MqttClient();
    void setClientId(const char *hostname, const char *clientId);
#ifndef NATIVE
    esp_mqtt_client_handle_t client;
    static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
#endif
    virtual void subscribeAndPublish() {};
    virtual void onMessage(const char *topic, const char *payload) {};
    virtual void onPublished(const char *topic, const char *payload) {};
    virtual void onError(const char *message, unsigned int code) {};

public:
    int start(const MqttConfig &config, const NetworkConfig &network);
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
