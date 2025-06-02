#include "config.hpp"
#include "pulsecounter.hpp"
#include "freertos/FreeRTOS.h"
#include "network.hpp"

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
    loge("test", "test");

    for (auto counter : cfg.getCounters())
        Pulsecounter::setPulseCounter(counter.getOutputPort(), counter.getInputPort());
    Ethernet eth;
    eth.init();
    while (true)
    {
        loge("test", "delay");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // DOES THE SOFTAP CONTINUE TO EXIST IN A FTM FRIENDLY STATE? IS THE WIFI PAUSED?
        eth.deinit();
    }
}
