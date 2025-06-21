#include "config.hpp"
#include "pulsecounter.hpp"
#include "freertos/FreeRTOS.h"
#include "network.hpp"
#include "websrvplscount.hpp"
#include "pcscheduler.hpp"
#include "esp_log.h"

static const char *TAG = "main";

extern "C" void app_main()
{
    Config cfg = Config::getConfig(Config::getJson().c_str());

    for (auto counter : cfg.getCounters())
        Pulsecounter::setPulseCounter(counter.getOutputPort(), counter.getInputPort());
    for (auto output : cfg.getOutputs())
        Pulsecounter::setOutputConfiguration(output.getPort(), output.getConfiguration());
    Pulsecounter::startThread();
    Ethernet eth;
    eth.setHostname(cfg.getNetwork().getHostname());
    eth.setNtpServer(cfg.getNetwork().getNtpserver());
    eth.init();

    if (eth.waitForConnection())
    {
        PulseCounterScheduler pcscheduler(cfg);
        const char *msg = pcscheduler.isConfigured();
        if (msg == nullptr)
            pcscheduler.run();
        else
            ESP_LOGE(TAG, "%s", msg);
        WebserverPulsecounter webserver;
        ESP_LOGI(TAG, "starting web server");
        webserver.start();
    }
    else
    {
        loge(TAG, "Unable to connect to the internet");
    }
}
