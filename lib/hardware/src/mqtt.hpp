#include "common.hpp"
#include "config.hpp"
#ifndef NATIVE
#include "mqtt_client.h"
#endif
class MqttClient
{
    // static MqttClient *theInstance;

protected:
    MqttClient(const MqttConfig &config, const NetworkConfig &network);
#ifndef NATIVE
    esp_mqtt_client_handle_t client;
    static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
#endif
    virtual void subscribeAndPublish() {};
    virtual void onMessage(const char *topic, const char *payload) {};
    virtual void onPublished(const char *topic, const char *payload) {};
    virtual void onError(void) {};

public:
    int start();
    int stop();
    // static MqttClient &getMqttClient(const MqttConfig &config)
    // {
    //     if (theInstance == nullptr)
    //         theInstance = new MqttClient(config);
    //     return *theInstance;
    // }
    void publish(const char *topic, const char *payload);
};
