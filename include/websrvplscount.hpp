#pragma once
#include "webserver.hpp"

#include <string>
class WebserverPulsecounter
{
    Webserver server;

public:
    void start(const char *serverCert = NULL, const char *caCert = NULL, const unsigned char *privateKey = NULL);
    void stop();
};