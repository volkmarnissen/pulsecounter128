#include <esp_http_server.h>
#include <esp_log.h>
#include "hardware.hpp"
#include <esp_https_server.h>
#include <esp_err.h>
#include "webserver.hpp"
static const char *TAG = "webserver";

void Webserver::start(httpd_ssl_config_t *sslConfig)
{
    stop();
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    // Start the httpd server
    if (sslConfig == NULL)
        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        isSsl = false;
        // Set URI handlers
        handle = server;
    }
    else if (httpd_ssl_start(&server, sslConfig) == ESP_OK)
    {
        isSsl = true;
        // Set URI handlers
        handle = server;
    }
    if (server == nullptr)
    {
        ESP_LOGI(TAG, "Error starting server!");
        handle = NULL;
    }
    else
        ESP_LOGI(TAG, "Started Web server!");
}
esp_err_t Webserver::registerUriHandler(httpd_uri_t *uriHandler)
{
    return httpd_register_uri_handler(handle, uriHandler);
};

void Webserver::stop()
{
    // Stop the httpd server
    if (handle != NULL)
    {
        if (isSsl)
            httpd_ssl_stop(handle);
        else
            httpd_stop(handle);
    }

    handle = NULL;
}
