#include "webserver.hpp"
#include <esp_log.h>
#include <algorithm> // std::min

#include "hardware.hpp"
#include "esp_https_server.h"
#include <string>
#include <chrono>
#include <thread>
#include "websrvplscount.hpp"
#include "pulsecounter.hpp"
#include "config.hpp"
#include "mqtt.hpp"

static const char *TAG = "wbsrvplscount";
static const int SCRATCH_BUFSIZE = 8192;

extern const char _binary_index_html_start[] asm("_binary_index_html_start");
extern const char _binary_pulse_css_start[] asm("_binary_pulse_css_start");
extern const char _binary_pulse_js_start[] asm("_binary_pulse_js_start");
extern const char _binary_ota_js_start[] asm("_binary_ota_js_start");
extern esp_err_t postUpdateHandler(httpd_req_t *req);
extern void reconfigurePcScheduler();

static esp_err_t getHandler(httpd_req_t *r)
{
    if (std::string(r->uri).ends_with("pulse.js"))
    {
        httpd_resp_set_type(r, "text/css");
        httpd_resp_send(r, _binary_pulse_js_start, HTTPD_RESP_USE_STRLEN);
    }

    if (std::string(r->uri).ends_with("ota.js"))
    {
        httpd_resp_set_type(r, "text/javascript");
        httpd_resp_send(r, _binary_ota_js_start, HTTPD_RESP_USE_STRLEN);
    }
    else if (std::string(r->uri).ends_with("pulse.css"))
    {
        httpd_resp_set_type(r, "text/css");
        httpd_resp_send(r, _binary_pulse_css_start, HTTPD_RESP_USE_STRLEN);
    }
    else if (std::string(r->uri).ends_with("api/config"))
    {
        httpd_resp_set_type(r, "application/json");
        httpd_resp_send(r, Config::getJson().c_str(), HTTPD_RESP_USE_STRLEN);
    }
    else if (std::string(r->uri).ends_with("api/status"))
    {
        httpd_resp_set_type(r, "application/json");
        httpd_resp_send(r, Pulsecounter::getStatusJson().c_str(), HTTPD_RESP_USE_STRLEN);
    }
    else if (std::string(r->uri).ends_with("api/restartSchedule"))
    {
        httpd_resp_set_type(r, "application/json");
        reconfigurePcScheduler();
        httpd_resp_send(r, "{\"message\":\"restarted\"}", HTTPD_RESP_USE_STRLEN);
    }
    else if (std::string(r->uri).ends_with("api/resetTicks"))
    {
        httpd_resp_set_type(r, "application/json");
        httpd_resp_send(r, Pulsecounter::resetLastSeconds().c_str(), HTTPD_RESP_USE_STRLEN);
    }
    else
    {
        httpd_resp_set_type(r, "text/html");
        httpd_resp_send(r, _binary_index_html_start, HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
};

class ValidationMqttClient : public MqttClient
{
    httpd_req_t *req;
    std::string content;
    const NetworkConfig &network;

public:
    bool reconfigureRequest;
    virtual ~ValidationMqttClient() {
    };
    ValidationMqttClient(httpd_req_t *_req, const char *_content, const NetworkConfig &_network) : MqttClient(), req(_req), content(_content), network(_network), reconfigureRequest(false)
    {
        setClientId(network.getHostname(), "validation");
    }
    static void onConnected(MqttClient *client, const char *, const char *)
    {
        if (client != NULL)
        {
            Config::setJson(((ValidationMqttClient *)client)->content.c_str());
            httpd_resp_set_hdr(((ValidationMqttClient *)client)->req, "Content-Type", "application/json");
            httpd_resp_send(((ValidationMqttClient *)client)->req, "", HTTPD_RESP_USE_STRLEN);
            ((ValidationMqttClient *)client)->reconfigureRequest = true;
        }
        else
            ESP_LOGI(TAG, "onConnected: client is NULL");
    }
    static void onError(MqttClient *client, const char *message, const char *code)
    {
        char buf[512];
        if (client != NULL)
        {
            sprintf(buf, "[{ \"errorcode\": %d, \"errormessage\": \"%s\"}]", (int)code, message);
            httpd_resp_set_hdr(((ValidationMqttClient *)client)->req, "Content-Type", "application/json");
            httpd_resp_set_status(((ValidationMqttClient *)client)->req, "422 Unprocessable Entity");
            httpd_resp_send(((ValidationMqttClient *)client)->req, buf, HTTPD_RESP_USE_STRLEN);
        }
        else
            ESP_LOGI(TAG, "onConnected: client is NULL");
    }
    int stop()
    {
        int rc = MqttClient::stop();
        delete this;
        return rc;
    }
    int start(const MqttConfig &config)
    {
        registerListener(MQTT_EV_CONNECTED, ValidationMqttClient::onConnected);
        return MqttClient::start(config, network);
    }
};
static esp_err_t postConfigHandler(httpd_req_t *req)
{
    int received;
    int remaining = req->content_len;
    char *content = (char *)malloc(remaining + 1);
    int receivedTotal = 0;
    while (remaining > 0)
    {

        if ((received = httpd_req_recv(req, content + receivedTotal, std::min(remaining, SCRATCH_BUFSIZE))) < 0)
        {
            /* Retry if timeout occurred */
            if (received == HTTPD_SOCK_ERR_TIMEOUT)
                continue;
            free(content);
            /* In case of unrecoverable error, close and delete the unfinished file*/
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive configuration file");
            return ESP_FAIL;
        }
        receivedTotal += received;
        remaining -= received;
    };
    content[receivedTotal] = 0;
    ESP_LOGI(TAG, "Reception successful  %s", content);
    const Config &newCfg = Config::getConfig(content);
    // create an mqtt client and initialize it the callbacks will send a response
    // and store the new content
    if (req->user_ctx)
        ((WebserverPulsecounter *)req->user_ctx)->startValidationMqttClient(req, content, newCfg.getMqtt(), newCfg.getNetwork());
    else
        ESP_LOGE(TAG, "No User Context for config handler");
    return ESP_OK;
};

WebserverPulsecounter::~WebserverPulsecounter()
{
    if (validationMqttClient)
        delete validationMqttClient;
};
void WebserverPulsecounter::startValidationMqttClient(httpd_req_t *req, const char *content, const MqttConfig &config, const NetworkConfig &network)
{
    if (validationMqttClient != nullptr)
        delete validationMqttClient;
    validationMqttClient = new ValidationMqttClient(req, content, network);
    validationMqttClient->start(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    if (validationMqttClient->reconfigureRequest)
        reconfigureRequest = true;
}
/* URI handler structure for GET /uri */
httpd_uri_t WebserverPulsecounter::uriHandlers[] = {
    {.uri = "*",
     .method = HTTP_GET,
     .handler = getHandler,
     .user_ctx = NULL},
    {.uri = "/api/config",
     .method = HTTP_POST,
     .handler = postConfigHandler,
     .user_ctx = NULL},
    {.uri = "/api/update",
     .method = HTTP_POST,
     .handler = postUpdateHandler,
     .user_ctx = NULL},
};
int WebserverPulsecounter::uriHandlersCount = sizeof(uriHandlers) / sizeof(uriHandlers[0]);

void WebserverPulsecounter::setConfig(const NetworkConfig &config, bool useHttp)
{
    server.stop();
    if (useHttp)
    {
        // TODO start with http (not SSL)
    }
    // TODO: Add ssl parameter later
    ESP_LOGI(TAG, "Stopped Web Server");
    WebserverPulsecounter::start();
}

void WebserverPulsecounter::start(const char *serverCert, const char *caCert, const unsigned char *privateKey)
{
    ESP_LOGI(TAG, "starting web server");
    reconfigureRequest = false;
    server.start(serverCert, caCert, privateKey);
};
void WebserverPulsecounter::stop()
{
    server.stop();
}
