#include <esp_ota_ops.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include <algorithm>
#include <esp_log.h>

const char *FNAME = "otaHandler";
char *otaBuffer = nullptr;
const esp_partition_t *otaUpdatePartition;
esp_ota_handle_t otaHandle = {0};
size_t otaSize = 0;
size_t otaReceived = 0;
static bool otaStarted = false;
esp_err_t postUpdateHandler(httpd_req_t *req)
{
    size_t ota_buff_size = 16 * 1024;

    size_t content_length = req->content_len;
    int content_received = 0;
    int recv_len;

    if (content_length < 1)
    {
        return ESP_OK;
    }

    // Received first chunk - start OTA procedure
    if (!otaStarted)
    {
        otaStarted = true;
        otaBuffer = (char *)malloc(ota_buff_size);

        // Get total OTA file size
        size_t buf_len = httpd_req_get_hdr_value_len(req, "X-OTA-SIZE") + 1;
        if (buf_len > 1)
        {
            char otaSizeBuffer[32];
            if (httpd_req_get_hdr_value_str(req, "X-OTA-SIZE", otaSizeBuffer, sizeof(otaSizeBuffer)) == ESP_OK)
            {
                otaSize = atoi(otaSizeBuffer);
                ESP_LOGI(FNAME, "Found header => X-OTA-SIZE: %s", otaSizeBuffer);
            }
        }

        otaUpdatePartition = esp_ota_get_next_update_partition(NULL);
        if (otaUpdatePartition == NULL)
        {
            ESP_LOGE(FNAME, "Error With <esp_ota_get_next_update_partition, Cancelling OTA");
            return ESP_FAIL;
        }
        esp_err_t err = esp_ota_begin(otaUpdatePartition, otaSize, &otaHandle);
        if (err != ESP_OK)
        {
            const char *msg = "";
            switch (err)
            {
            case ESP_ERR_INVALID_ARG:
                msg = "ESP_ERR_INVALID_ARG";
                break;
            case ESP_ERR_NO_MEM:
                msg = "ESP_ERR_NO_MEM";
                break;
            case ESP_ERR_OTA_PARTITION_CONFLICT:
                msg = "ESP_ERR_OTA_PARTITION_CONFLICT";
                break;
            case ESP_ERR_NOT_FOUND:
                msg = "ESP_ERR_NOT_FOUND";
                break;
            case ESP_ERR_OTA_SELECT_INFO_INVALID:
                msg = "ESP_ERR_OTA_SELECT_INFO_INVALID";
                break;
            case ESP_ERR_INVALID_SIZE:
                msg = "ESP_ERR_INVALID_SIZE";
                break;

            case ESP_ERR_OTA_ROLLBACK_INVALID_STATE:
                msg = "ESP_ERR_OTA_ROLLBACK_INVALID_STATE";
                break;
            default: msg = "Other Issue";
            }
            ESP_LOGE(FNAME, "Error With OTA Begin, Cancelling OTA %s", msg);
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGI(FNAME, "Writing to partition subtype %d at offset 0x%x", otaUpdatePartition->subtype, (unsigned)otaUpdatePartition->address);
        }
    }

    do
    {
        // Read the ota data
        // ESP_LOGI(FNAME, "%d %d", content_length, ota_buff_size);
        if ((recv_len = httpd_req_recv(req, otaBuffer, std::min(content_length, ota_buff_size))) < 0)
        {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGW(FNAME, "Socket Timeout");
                /* Retry receiving if timeout occurred */
                continue;
            }
            ESP_LOGE(FNAME, "OTA Other Error %d", recv_len);
            return ESP_FAIL;
        }

        // Write OTA data
        esp_ota_write(otaHandle, otaBuffer, recv_len);
        content_received += recv_len;
        otaReceived += recv_len;

    } while (recv_len > 0 && content_received < content_length);

    // Webserver.setOtaProgress((otaReceived * 100.0f) / otaSize);
    ESP_LOGI(FNAME, "Received %d / %d (%.02f)", otaReceived, otaSize, (otaReceived * 100.0) / otaSize);

    if (otaReceived >= otaSize)
    {
        free(otaBuffer);
        otaStarted = false;
        // End response
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "OK", strlen("OK"));

        if (esp_ota_end(otaHandle) == ESP_OK)
        {
            // Lets update the partition
            if (esp_ota_set_boot_partition(otaUpdatePartition) == ESP_OK)
            {
                const esp_partition_t *boot_partition = esp_ota_get_boot_partition();

                ESP_LOGI(FNAME, "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, (unsigned)boot_partition->address);
                ESP_LOGI(FNAME, "Rebooting");
                // Webserver.setOtaStatus(otaStatus::DONE);
                esp_restart();
            }
            else
            {
                ESP_LOGE(FNAME, "\r\n\r\n !!! Flashed Error !!!\r\n");
            }
        }
        else
        {
            ESP_LOGE(FNAME, "\r\n\r\n !!! OTA End Error !!!\r\n");
        }
    }
    else
    {
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}
