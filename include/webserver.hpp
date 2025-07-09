#pragma once
#include <esp_http_server.h>
#include <esp_https_server.h>

class Webserver
{
    httpd_handle_t handle = NULL;
    bool isSsl = false;
    httpd_uri_t *uriHandlers;
    int uriHandlerCount;

public:
    Webserver(httpd_uri_t *_uriHandlers, int _uriHandlerCount) : uriHandlers(_uriHandlers), uriHandlerCount(_uriHandlerCount) {};
    void start(const char *serverCert = NULL, const char *caCert = NULL, const unsigned char *privateKey = NULL);
    void stop(void);
};
