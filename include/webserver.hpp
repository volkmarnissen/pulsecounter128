#pragma once
#include <esp_http_server.h>
#include <esp_https_server.h>

class Webserver
{
    httpd_handle_t handle = NULL;
    bool isSsl = false;

public:
    void start(const char *serverCert = NULL, const char *caCert = NULL, const char *privateKey = NULL);
    void stop(void);
    esp_err_t registerUriHandler(httpd_uri_t *uriHandler);
};
