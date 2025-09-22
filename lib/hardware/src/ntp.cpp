#include "ntp.hpp"

static const char *TAG = "ntp";
#ifdef NATIVE
NtpService::NtpService(bool eth, const char *ntpserver) {};
bool NtpService::wait(unsigned int timeout) { return true; };
void NtpService::start() {};
void NtpService::restart() {};

void NtpService::stop() {};
#else
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include <arpa/inet.h>
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

NtpService::NtpService(bool isEthernet, const char *ntpserver)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(0, {});
    config.start = false; // start the SNTP service explicitly
    if (ntpserver != nullptr && strlen(ntpserver) > 0)
    {
        config.servers[0] = ntpserver;
        config.num_of_servers = 1;
    }
    else
        config.server_from_dhcp = true;       // accept the NTP offer from the DHCP server
    config.renew_servers_after_new_IP = true; // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1;         // updates from server num 1, leaving server 0 (from DHCP) intact
    // configure the event on which we renew servers
    if (isEthernet)
        config.ip_event_to_renew = IP_EVENT_ETH_GOT_IP;
    else
        config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
    config.sync_cb = time_sync_notification_cb; // Note: This is only needed if we want
    ESP_LOGI(TAG, "NTP init");
    esp_netif_sntp_init(&config);
};
bool NtpService::wait(unsigned int timeout)
{
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(timeout)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to update system time within %d s timeout", timeout / 1000);
        return false;
    }
    return true;
};
void NtpService::start()
{
    ESP_LOGI(TAG, "NTP start");
    esp_netif_sntp_start();
};

void NtpService::restart()
{
    ESP_LOGI(TAG, "NTP restart");
    esp_sntp_restart();
    sntp_sync_status_t status = sntp_get_sync_status();
    const char *server = esp_sntp_getservername(0);
    const ip_addr_t *ip = esp_sntp_getserver(0);
    if (server == nullptr)
    {
        if (ip == nullptr)
        {
            ESP_LOGE(TAG, "No ntp ip address found!");
            return;
        }
        else
        {
            ESP_LOGI(TAG, "No server found!");
        }
    }
    switch (status)
    {
    case SNTP_SYNC_STATUS_RESET:
        ESP_LOGI(TAG, "Status Reset");
        break;
    case SNTP_SYNC_STATUS_COMPLETED:
        ESP_LOGI(TAG, "Status Completed");
        break;
    default:
        ESP_LOGI(TAG, "???");
    };
};

void NtpService::stop()
{
    ESP_LOGI(TAG, "NTP stop");
    esp_sntp_stop();
    esp_netif_sntp_deinit();
};
#endif