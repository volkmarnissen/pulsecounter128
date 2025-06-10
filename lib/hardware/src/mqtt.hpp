#include "common.hpp"
#include "config.hpp"
#ifndef NATIVE
#include "mqtt_client.h"
#endif
class MqttClient
{

protected:
#ifndef NATIVE
    esp_mqtt_client_handle_t client;
#endif
    virtual void subscribeAndPublish();
    virtual void onMessage();

public:
    MqttClient(const MqttConfig &config);
    esp_err_t start();
    esp_err_t stop();
};