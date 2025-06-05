#pragma once
#include <esp_http_server.h>
#include <esp_https_server.h>

class Webserver
{
    httpd_handle_t handle = NULL;
    bool isSsl = false;

public:
    void start(httpd_ssl_config_t *sslConfig = NULL);
    void stop(void);
    esp_err_t registerUriHandler(httpd_uri_t *uriHandler);
};
