#include "webserver.hpp"
#include <esp_log.h>
#include <algorithm> // std::min

#include "hardware.hpp"
#include "esp_https_server.h"
#include <string>
#include "websrvplscount.hpp"

static const char *TAG = "wbsrvplscount";
static const int SCRATCH_BUFSIZE = 8192;

extern const char _binary_index_html_start[] asm("_binary_index_html_start");
extern const char _binary_pulse_css_start[] asm("_binary_pulse_css_start");
extern const char _binary_pulse_js_start[] asm("_binary_pulse_js_start");

static esp_err_t getHandler(httpd_req_t *r)
{
    if (std::string(r->uri).ends_with("pulse.js"))
        httpd_resp_send(r, _binary_pulse_js_start, HTTPD_RESP_USE_STRLEN);
    else if (std::string(r->uri).ends_with("pulse.css"))
        httpd_resp_send(r, _binary_pulse_css_start, HTTPD_RESP_USE_STRLEN);
    else
        httpd_resp_send(r, _binary_index_html_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
};
static esp_err_t postConfigHandler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Receiving configuration");
    int received;
    int remaining = req->content_len;
    char *content = (char *)malloc(remaining);
    int receivedTotal = 0;
    while (remaining > 0)
    {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        if ((received = httpd_req_recv(req, content + receivedTotal, std::min(remaining, SCRATCH_BUFSIZE))) <= 0)
        {
            /* Retry if timeout occurred */
            if (received == HTTPD_SOCK_ERR_TIMEOUT)
                continue;
            free(content);
            /* In case of unrecoverable error, close and delete the unfinished file*/
            ESP_LOGE(TAG, "Reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive configuration file");
            return ESP_FAIL;
        }
        receivedTotal += received;
        remaining -= received;
    };
    ESP_LOGE(TAG, "Reception failed!");
    // TODO: validate content and store in NVS when finished
    // TODO: send Response
    free(content);
    return ESP_OK;
};

/* URI handler structure for GET /uri */
static httpd_uri_t indexUri = {
    .uri = "*",
    .method = HTTP_GET,
    .handler = getHandler,
    .user_ctx = NULL};

/* URI handler structure for POST /uri */
static httpd_uri_t postConfigUri = {
    .uri = "/config",
    .method = HTTP_POST,
    .handler = postConfigHandler,
    .user_ctx = NULL};

void WebserverPulsecounter::start()
{
    server.start();
    ESP_ERROR_CHECK(server.registerUriHandler(&indexUri));
    ESP_ERROR_CHECK(server.registerUriHandler(&postConfigUri));
};
void WebserverPulsecounter::stop()
{
    server.stop();
}
