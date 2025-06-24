#include "config.hpp"
#include "pulsecounter.hpp"
#include "freertos/FreeRTOS.h"
#include "network.hpp"
#include "websrvplscount.hpp"
#include "pcscheduler.hpp"
#include "esp_log.h"
#include <chrono>
#include <thread>
static const char *TAG = "main";
WebserverPulsecounter webserver;
Ethernet eth;
Config cfg = Config::getConfig(Config::getJson().c_str());
PulseCounterScheduler pcscheduler(cfg);
void reconfigure()
{
    ESP_LOGI(TAG, "Reconfiguring");
    cfg = Config::getConfig(Config::getJson().c_str());
    if (nullptr == pcscheduler.checkConfiguration(cfg))
        pcscheduler.setConfig(cfg);
    webserver.setConfig(cfg.getNetwork());
}

extern "C" void app_main()
{

    Pulsecounter::setConfig(cfg);
    Pulsecounter::startThread();
    eth.setHostname(cfg.getNetwork().getHostname());
    eth.setNtpServer(cfg.getNetwork().getNtpserver());
    eth.init();

    if (eth.waitForConnection())
    {
        const char *msg = pcscheduler.checkConfiguration(cfg);
        if (msg == nullptr)
            pcscheduler.run();
        else
            ESP_LOGE(TAG, "%s", msg);

        webserver.setConfig(cfg.getNetwork());
    }
    else
    {
        loge(TAG, "Unable to connect to the internet");
    }
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (webserver.reconfigureRequest)
        {
            reconfigure();
        }
    }
}
