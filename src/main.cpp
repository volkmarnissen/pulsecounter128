#include "config.hpp"
#include "pulsecounter.hpp"
#include "freertos/FreeRTOS.h"
#include "network.hpp"
#include "websrvplscount.hpp"
static const char *TAG = "main";

extern "C" void app_main()
{
    Config cfg = Config::getConfig(Config::getJson().c_str());

    for (auto counter : cfg.getCounters())
        Pulsecounter::setPulseCounter(counter.getOutputPort(), counter.getInputPort());
    for (auto output : cfg.getOutputs())
        Pulsecounter::setOutputConfiguration(output.getPort(), output.getConfiguration());
    Ethernet eth;
    eth.setHostname(cfg.getNetwork().getHostname());
    eth.setNtpServer(cfg.getNetwork().getNtpserver());
    eth.init();
    if (eth.waitForConnection())
    {
        WebserverPulsecounter webserver;
        webserver.start();
    }
    else
    {
        loge(TAG, "Unable to connect to the internet");
    }
}
