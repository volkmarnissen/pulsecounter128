#include "esp_netif.h"
#include "esp_eth.h"

class Ethernet
{
    esp_eth_handle_t eth_handle;
    esp_netif_t *eth_netif;
    esp_eth_netif_glue_handle_t eth_netif_glue;

public:
    Ethernet() { eth_handle = nullptr; }
    void init();
    void deinit();
};