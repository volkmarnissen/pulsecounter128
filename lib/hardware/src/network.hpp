#include "esp_netif.h"
#include "esp_eth.h"
#include <string>
#include "ntp.hpp"
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
    NtpService *ntp;
    const char *hostname;
    const char *ntpServer;

public:
    Ethernet()
    {
        eth_handle = nullptr;
        ntpServer = nullptr;
    }
    void setHostname(const char *hostname)
    {
        this->hostname = hostname;
    }
    void setNtpServer(const char *ntpServer)
    {
        this->ntpServer = ntpServer;
    }
    const char *init();
    const char *deinit();
    bool waitForConnection(int waitTimeMs = 20000, const char *hostname = "www.google.com");
    bool waitForSntp(int waitTimeMs);
};