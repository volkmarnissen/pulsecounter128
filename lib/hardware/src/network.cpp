#ifndef NATIVE
#include "network.hpp"
#include "esp_eth_driver.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include <lwip/dns.h>
#include "lwip/ip4.h"

static const char *TAG = "pulsecounter_eth_init";

// Define an event group for DNS resolution
#define NETWORK_CONNECTED_BIT BIT0
#define TCPIP_CONNECTED_BIT BIT1
#define DNS_BIT BIT2

static EventGroupHandle_t s_network_event_group;
NetworkEventHandler *NetworkEventHandler::theInstance = nullptr;
;

static esp_eth_handle_t eth_init_internal(esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
    esp_eth_handle_t ret = NULL;

    // Init common MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 0;
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    // Update vendor specific MAC config based on board configuration
    esp32_emac_config.smi_gpio.mdc_num = 23;
    esp32_emac_config.smi_gpio.mdio_num = 18;
    esp32_emac_config.clock_config.rmii.clock_gpio = EMAC_CLK_OUT_180_GPIO;
    esp32_emac_config.clock_config.rmii.clock_mode = EMAC_CLK_OUT;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_GOTO_ON_FALSE(esp_eth_driver_install(&config, &eth_handle) == ESP_OK, NULL,
                      err, TAG, "Ethernet driver install failed");

    if (mac_out != NULL)
        *mac_out = mac;
    if (phy_out != NULL)
        *phy_out = phy;
    return eth_handle;
err:
    if (eth_handle != NULL)
        esp_eth_driver_uninstall(eth_handle);
    if (mac != NULL)
        mac->del(mac);
    if (phy != NULL)
        phy->del(phy);
    return ret;
}

static esp_err_t example_eth_deinit(esp_eth_handle_t eth_handle)
{
    ESP_RETURN_ON_FALSE(eth_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Ethernet handle cannot be NULL");
    esp_eth_mac_t *mac = NULL;
    esp_eth_phy_t *phy = NULL;
    if (eth_handle != NULL)
    {
        esp_eth_get_mac_instance(eth_handle, &mac);
        esp_eth_get_phy_instance(eth_handle, &phy);
        ESP_RETURN_ON_ERROR(esp_eth_driver_uninstall(eth_handle), TAG, "Ethernet %p uninstall failed", eth_handle);
        if (mac != NULL)
            mac->del(mac);
        if (phy != NULL)
            phy->del(phy);
    }
    return ESP_OK;
}

const char *Ethernet::init()
{
    eth_handle = eth_init_internal(NULL, NULL);
    if (eth_handle == nullptr)
        return "Unable to initialize Ethernet Controller";
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    if (ESP_OK != esp_netif_init())
        return "Unable to initialize network interface";

    // Create default event loop that running in background
    if (ESP_OK != esp_event_loop_create_default())
        return "Unable to initialize";

    eth_netif = esp_netif_new(&cfg);
    if (hostname != NULL)
        esp_netif_set_hostname(eth_netif, hostname);
    eth_netif_glue = esp_eth_new_netif_glue(eth_handle);
    // Attach Ethernet driver to TCP/IP stack
    esp_netif_attach(eth_netif, eth_netif_glue);
    const char *msg = NetworkEventHandler::getNetworkEventHandler().attachEventHandlers();
    if (msg != nullptr)
        return msg;
    if (ESP_OK != esp_eth_start(eth_handle))
        return "Unable to initialize TCP/IP stack";
    return nullptr;
};

const char *Ethernet::deinit()
{
    ESP_LOGI(TAG, "stop and deinitialize Ethernet network...");
    // Stop Ethernet driver state machine and destroy netif
    if (ESP_OK != esp_eth_stop(eth_handle))
        return "Unable to stop Ethernet Controller";

    if (ESP_OK != esp_eth_del_netif_glue(eth_netif_glue))
        return "Unable to stop network interface";
    esp_netif_destroy(eth_netif);
    esp_netif_deinit();
    if (ESP_OK != example_eth_deinit(eth_handle))
        return "Unable to stop ethernet";
    // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler));
    if (ESP_OK != esp_event_loop_delete_default())
        return "Unable to deinitialize ethernet";
    return nullptr;
};

// Callback function to handle DNS resolution results
static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr != NULL)
    {
        xEventGroupSetBits(s_network_event_group, DNS_BIT);
    }
    else
    {
        printf("DNS resolution failed for %s\n", name);
    }
}
bool Ethernet::waitForConnection(int waitTimeMs, const char *hostname)
{
    ip_addr_t addr;
    s_network_event_group = xEventGroupCreate();
    // Optionally, wait for DNS resolution to complete
    const TickType_t xTicksToWait = pdMS_TO_TICKS(waitTimeMs);
    EventBits_t bits = xEventGroupWaitBits(s_network_event_group, NETWORK_CONNECTED_BIT | TCPIP_CONNECTED_BIT, false, true, xTicksToWait);
    printf("Got IP Address");
    dns_gethostbyname(hostname, &addr, dns_found_cb, nullptr);
    bits = xEventGroupWaitBits(s_network_event_group, DNS_BIT, false, true, xTicksToWait);
    return (bits & DNS_BIT);
}

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        xEventGroupSetBits(s_network_event_group, NETWORK_CONNECTED_BIT);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        xEventGroupClearBits(s_network_event_group, NETWORK_CONNECTED_BIT);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    xEventGroupSetBits(s_network_event_group, TCPIP_CONNECTED_BIT);

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

const char *NetworkEventHandler::attachEventHandlers()
{
    if (ESP_OK != esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL))
        return "Unable to register ethernet event handler";
    if (ESP_OK != esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL))
        return "Unable to register ethernet event handler";
    return nullptr;
};

#endif