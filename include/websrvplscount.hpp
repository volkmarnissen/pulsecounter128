#pragma once
#include "webserver.hpp"
#include "config.hpp"
#include <string>
class WebserverPulsecounter
{
    Webserver server;

public:
    bool reconfigureRequest;
    void start(const char *serverCert = NULL, const char *caCert = NULL, const unsigned char *privateKey = NULL);
    void stop();
    void setConfig(const NetworkConfig &config, bool reset = false);
};