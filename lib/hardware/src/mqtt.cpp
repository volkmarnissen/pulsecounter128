/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
#include "mqtt.hpp"
#include <functional>
#ifdef NATIVE

MqttClient::MqttClient(const MqttConfig &config, const NetworkConfig &network) {};
int MqttClient::start() { return 0; };
int MqttClient::stop() { return 0; };
void MqttClient::publish(const char *topic, const char *payload) {};
#else
#include "mqtt_client.h"
#include "esp_event.h"
#include <esp_log.h>

static const char *TAG = "mqtt";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

MqttClient::~MqttClient()
{
    esp_mqtt_client_destroy(client);
}

MqttClient::MqttClient(const MqttConfig &config, const NetworkConfig &network)
{
    esp_mqtt_client_config_t mqtt_cfg{};
    mqtt_cfg.broker.address.uri = config.getMqtturl();
    mqtt_cfg.credentials = {};
    mqtt_cfg.credentials.authentication = {};
    if (clientId != "")
        mqtt_cfg.credentials.client_id = (std::string(network.getHostname()) + clientId).c_str();

    switch (config.getAuthenticationMethod())
    {
    case userPassword:
        mqtt_cfg.credentials.username = config.getUsername();
        mqtt_cfg.credentials.authentication.password = config.getPassword();
        break;
    case SSL:
        if (network.getSslCa() == nullptr || network.getSslCa()[0] == '\0')
            ESP_LOGE(TAG, "Network Root Certificate not configured");
        if (network.getSslHost() == nullptr || network.getSslHost()[0] == '\0')
            ESP_LOGE(TAG, "Network Server Certificate not configured");
        if (network.getSslHostKey() != nullptr || network.getSslHostKey()[0] == '\0')
            ESP_LOGE(TAG, "Network Server Certificate Key not configured");
        mqtt_cfg.broker.verification = {};
        mqtt_cfg.broker.verification.certificate = network.getSslCa();
        mqtt_cfg.credentials.authentication.certificate = network.getSslHost();
        mqtt_cfg.credentials.authentication.key = network.getSslHostKey();
        break;
    default:
        break;
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
}
int MqttClient::start(void)
{
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_err_t rc = ESP_OK;
    esp_mqtt_client_unregister_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, MqttClient::eventHandler);
    rc = esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, MqttClient::eventHandler, this);
    if (ESP_OK != rc)
        return rc;
    rc = esp_mqtt_client_start(client);
    if (ESP_OK != rc)
        esp_mqtt_client_unregister_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, MqttClient::eventHandler);
    return rc;
}
int MqttClient::stop(void)
{
    esp_mqtt_client_unregister_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, MqttClient::eventHandler);
    return esp_mqtt_client_stop(client);
}

// Forwards to mqttClient->subscribeAndPublish() and onMessage();
void MqttClient::logerror(const char *message, unsigned int code)
{
    this->onError(message, code);
    if (code != ESP_OK)
    {
        log_error_if_nonzero(message, code);
    }
}

void MqttClient::eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    MqttClient *mqttClient = (MqttClient *)(handler_args);
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    int msg_id = event->msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED \"%s\"", mqttClient->clientId.c_str());
        mqttClient->subscribeAndPublish();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED \"%s\"", mqttClient->clientId.c_str());
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        mqttClient->onPublished(event->topic, event->data);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA \"%s\"", mqttClient->clientId.c_str());
        mqttClient->onMessage(event->topic, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR \"%s\"", mqttClient->clientId.c_str());
        switch (event->error_handle->error_type)
        {
        case MQTT_ERROR_TYPE_TCP_TRANSPORT:
            mqttClient->logerror("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            mqttClient->logerror("reported from tls stack", event->error_handle->esp_tls_stack_err);
            mqttClient->logerror("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            mqttClient->logerror(strerror(event->error_handle->esp_transport_sock_errno), event->error_handle->esp_transport_sock_errno);
            break;
        case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
            mqttClient->logerror("Connection refused", 1);
            break;
        default:
            mqttClient->logerror("Other error", 1);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id 0x%04X \"%s\"", (unsigned int)event_id, mqttClient->clientId.c_str());
        break;
    }
}
void MqttClient::publish(const char *topic, const char *payload)
{
    // No error handling. published will be called only in case of success
    esp_mqtt_client_publish(client, topic, payload, strlen(payload), 1, 0);
};
// MqttClient *MqttClient::theInstance = nullptr;

#endif