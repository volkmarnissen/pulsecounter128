#include "config.hpp"
#include "pulsecounter.hpp"
#include "freertos/FreeRTOS.h"
#include "network.hpp"
#include "websrvplscount.hpp"
#include "pcscheduler.hpp"
#include "hardware.hpp"
#include "esp_log.h"
#include <chrono>
#include <thread>
#include <driver/gpio.h>

static const char *TAG = "main";
const gpio_num_t resetButtonPin = GPIO_NUM_0;
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
void reconfigureHttp()
{
    cfg = Config::getConfig(Config::getJson().c_str());
    webserver.setConfig(cfg.getNetwork(), true);
}

void configureResetButton()
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << GPIO_NUM_0;
    // disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
}

extern "C" void app_main()
{
    I2c::get()->writeOutputs(0xFF);

    Pulsecounter::setConfig(cfg);
    Pulsecounter::startThread();
    configureResetButton();
    esp_log_level_set("mqtt", ESP_LOG_DEBUG);
    esp_log_level_set("wbsrvplscount", ESP_LOG_DEBUG);
    eth.setHostname(cfg.getNetwork().getHostname());
    eth.setNtpServer(cfg.getNetwork().getNtpserver());
    eth.init();

    if (eth.waitForConnection())
    {
        const char *msg = pcscheduler.checkConfiguration(cfg);
        if (msg == nullptr)
            pcscheduler.run();
        else
            ESP_LOGE(TAG, "PCScheduler not started %s", msg);

        webserver.setConfig(cfg.getNetwork());
    }
    else
    {
        loge(TAG, "Unable to connect to the internet");
    }
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (gpio_get_level(resetButtonPin) == 0) // Button was pressed
        {
            reconfigureHttp();
        }
        // If program button is pressed for one second, unconfigure https
    }
}
