#include "common.hpp"
#include "config.hpp"
#ifndef NATIVE
#include "mqtt_client.h"
class MqttClient
{
    // static MqttClient *theInstance;

protected:
    MqttClient(const MqttConfig &config, const NetworkConfig &network);
#ifndef NATIVE
    esp_mqtt_client_handle_t client;
#endif
    virtual void subscribeAndPublish() {};
    virtual void onMessage(const char *topic, const char *payload) {};

public:
    esp_err_t start();
    esp_err_t stop();
    static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    // static MqttClient &getMqttClient(const MqttConfig &config)
    // {
    //     if (theInstance == nullptr)
    //         theInstance = new MqttClient(config);
    //     return *theInstance;
    // }
};
#endif
