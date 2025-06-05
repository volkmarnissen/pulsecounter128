#include "esp_netif.h"
#include "esp_eth.h"
#include <string>

class NetworkEventHandler
{
    static NetworkEventHandler *theInstance;

public:
    const char *attachEventHandlers();
    static NetworkEventHandler &getNetworkEventHandler()
    {
        if (theInstance == nullptr)
            theInstance = new NetworkEventHandler();
        return *theInstance;
    }
};
class Ethernet
{
    esp_eth_handle_t eth_handle;
    esp_netif_t *eth_netif;
    esp_eth_netif_glue_handle_t eth_netif_glue;
    const char *hostname;

public:
    Ethernet() { eth_handle = nullptr; }
    void setHostname(const char *hostname)
    {
        this->hostname = hostname;
    }
    const char *init();
    const char *deinit();
    bool waitForConnection(int waitTimeMs = 20000, const char *hostname = "www.google.com");
};