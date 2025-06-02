#include "network.hpp"
#include "esp_eth_driver.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "esp_check.h"

static const char *TAG = "pulsecounter_eth_init";

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

esp_err_t example_eth_deinit(esp_eth_handle_t eth_handle)
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

void Ethernet::init()
{
    eth_handle = eth_init_internal(NULL, NULL);
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    eth_netif = esp_netif_new(&cfg);
    eth_netif_glue = esp_eth_new_netif_glue(eth_handle);
    // Attach Ethernet driver to TCP/IP stack
    esp_netif_attach(eth_netif, eth_netif_glue);
    // ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
};

void Ethernet::deinit()
{
    ESP_LOGI(TAG, "stop and deinitialize Ethernet network...");
    // Stop Ethernet driver state machine and destroy netif
    ESP_ERROR_CHECK(esp_eth_stop(eth_handle));
    ESP_ERROR_CHECK(esp_eth_del_netif_glue(eth_netif_glue));
    esp_netif_destroy(eth_netif);
    esp_netif_deinit();
    ESP_ERROR_CHECK(example_eth_deinit(eth_handle));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler));
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
};
