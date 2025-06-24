#include <esp_http_server.h>
#include <esp_log.h>
#include "hardware.hpp"
#include <esp_https_server.h>
#include <esp_err.h>
#include "webserver.hpp"
static const char *TAG = "webserver";

void Webserver::start(const char *serverCert, const char *caCert, const unsigned char *privateKey)
{
    stop();
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_ssl_config_t sslConfig = HTTPD_SSL_CONFIG_DEFAULT();
    bool sslIsConfigured = false;
    if (serverCert != nullptr && strlen(serverCert) &&
        caCert != nullptr && strlen(caCert) &&
        privateKey != nullptr && strlen((const char *)privateKey))
    {
        sslConfig.servercert = (uint8_t *)serverCert;
        sslConfig.servercert_len = strlen(serverCert);
        sslConfig.prvtkey_pem = privateKey;
        sslConfig.prvtkey_len = strlen((const char *)privateKey);
        sslConfig.cacert_pem = (const unsigned char *)caCert;
        sslConfig.cacert_len = strlen((const char *)caCert);
        sslIsConfigured = true;
    }
    // TODO sslConfig.prvtkey_len = ;
    config.uri_match_fn = httpd_uri_match_wildcard;
    // Start the httpd server
    if (sslIsConfigured)
    {
        ESP_LOGI(TAG, "Starting SSL server on port: '%d'", config.server_port);
        if (httpd_ssl_start(&server, &sslConfig) == ESP_OK)
        {
            isSsl = true;
            // Set URI handlers
            handle = server;
        }
    }
    else
    {
        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
        if (!sslIsConfigured && httpd_start(&server, &config) == ESP_OK)
        {
            isSsl = false;
            // Set URI handlers
            handle = server;
        }
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
    httpd_unregister_uri_handler(handle, uriHandler->uri, uriHandler->method);
    return httpd_register_uri_handler(handle, uriHandler);
};

void Webserver::stop()
{
    ESP_LOGI(TAG, "Webserver stop: %ld %s", (long)handle, isSsl ? "SSL" : "HTTP");
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
