#pragma once
#include "webserver.hpp"
#include "config.hpp"
#include <string>
class ValidationMqttClient;
class WebserverPulsecounter
{
    Webserver server;
    ValidationMqttClient *validationMqttClient = nullptr;
    static httpd_uri_t uriHandlers[];
    static int uriHandlersCount;

public:
    WebserverPulsecounter() : server(uriHandlers, uriHandlersCount) {};
    virtual ~WebserverPulsecounter();
    void startValidationMqttClient(httpd_req_t *_req, const char *_content, const MqttConfig &config, const NetworkConfig &network);
    bool reconfigureRequest;
    void start(const char *serverCert = NULL, const char *caCert = NULL, const unsigned char *privateKey = NULL);
    void stop();
    void setConfig(const NetworkConfig &config, bool reset = false);
};