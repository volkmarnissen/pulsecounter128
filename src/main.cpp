#include "config.hpp"
#include "pulsecounter.hpp"
#include "freertos/FreeRTOS.h"
#include "network.hpp"
#include "websrvplscount.hpp"
static const char *TAG = "main";
std::string jsonfile = "{ \"counters\" : \n\
    [\n\
        {\n\
            \"name\" : \"test1\",\n\
            \"multiplier\" : 1000,\n\
            \"inputPort\": 0,\n\
            \"outputPort\": 0\n\
        }\n\
    ],\n\
    \"outputs\" : [\n\
        {\n\
            \"port\" : 0,\n\
            \"type\" : 0\n\
        }\n\
    ],\n\
    \"network\":{\n\
            \"sslcert\" : \"abcdssl\",\n\
            \"hostname\" : \"hostname\"\n\
    }\n\
}\n\
\n";

extern "C" void app_main()
{
    Config cfg = Config::getConfig(jsonfile.c_str());
    loge(TAG, "test");

    for (auto counter : cfg.getCounters())
        Pulsecounter::setPulseCounter(counter.getOutputPort(), counter.getInputPort());
    Ethernet eth;
    eth.setHostname(cfg.getNetwork().getHostname());
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
