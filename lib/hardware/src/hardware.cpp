#include "hardware.hpp"
#include <cstddef>

#ifdef MOCK_I2C

namespace MockI2c
{
    omask_t (*mock_readOutputPorts)() = NULL;
    imask_t (*mock_readInputPorts)() = NULL;
    void (*mock_writeOutputs)(omask_t mask) = NULL;
}

omask_t I2c::readOutputPorts()
{
    if (MockI2c::mock_readOutputPorts != NULL)
        return MockI2c::mock_readOutputPorts();
    return 0x08;
};

imask_t I2c::readInputPorts()
{
    if (MockI2c::mock_readInputPorts != NULL)
        return MockI2c::mock_readInputPorts();
    return 0x0F;
};

void I2c::writeOutputs(omask_t mask)
{
    if (MockI2c::mock_writeOutputs != NULL)
        MockI2c::mock_writeOutputs(mask);
};
I2c *I2c::get()
{
    if (I2c::theInstance == nullptr)
        I2c::theInstance = new I2c();
    return I2c::theInstance;
};

#else
#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include <driver/gpio.h>
#include <i2cdev.h>
#include "pcf8574.h"
static const int i2cAddressInputs1_8 = 0x22;
static const int i2cAddressInputs9_16 = 0x21;
static const int i2cAddressOutputs1_8 = 0x24;
static const int i2cAddressOutputs9_16 = 0x25;
static const gpio_num_t i2cScl = GPIO_NUM_5;
static const gpio_num_t i2cSda = GPIO_NUM_4;
static const int i2cPort = -1;
static const char *TAG = "hardware";

bool I2c::initBus()
{
    i2c_master_port = I2C_NUM_1;
    // i2c_config_t conf;
    // conf.mode = I2C_MODE_MASTER;
    // conf.sda_io_num = i2cSda;
    // conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    // conf.scl_io_num = i2cScl;
    // conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    // conf.master.clk_speed = 100000;
    // if (ESP_OK != i2c_param_config(i2c_master_port, &conf))
    //     return false;

    // return (ESP_OK != i2c_driver_install(i2c_master_port, conf.mode,
    //                                      0,
    //                                      0, 0));
    return true;
}

bool I2c::initDevices(const int *devAddresses, i2c_dev_t *devHandles)
{
    for (int idx = 0; idx < 2; idx++)
    {
        ESP_LOGI(TAG, "Initializing 0x%02X\n", devAddresses[idx]);
        memset(devHandles + idx, 0, sizeof(devHandles[0]));
        if (ESP_OK != pcf8574_init_desc(devHandles + idx, devAddresses[idx], i2c_master_port, i2cSda, i2cScl))
        {
            ESP_LOGE(TAG, "Initializing device failed 0x%02X\n", devHandles[idx].addr);
            return false;
        }
        else
        {
            ESP_LOGE(TAG, "Initializing device successful 0x%02X\n", devHandles[idx].addr);
        }

        ;
    }
    ESP_LOGI(TAG, "Devices initialized\n");
    return true;
}

I2c::I2c()
{
    i2c_master_port = I2C_NUM_1;
    isInitialized = false;
};

I2c::~I2c()
{
    if (isInitialized)
        for (int idx = 0; idx < 2; idx++)
        {
            pcf8574_free_desc(dev_handleReads + idx);
            pcf8574_free_desc(dev_handleWrites + idx);
        }
};

omask_t I2c::readOutputPorts()
{
    omask_t rc;
    esp_err_t esprc;
    if (ESP_OK != (esprc = pcf8574_port_read(dev_handleWrites, &rc)))
        ESP_LOGE("I2C", "Unable to read from I2C %X\n", esprc);
    return rc;
};

imask_t I2c::readInputPorts()
{
    imask_t rc;
    unsigned char *rdBuf = (unsigned char *)&rc;
    if (ESP_OK != pcf8574_port_read(dev_handleReads, rdBuf))
        ESP_LOGE("I2C", "readInputPorts: Unable to read from I2C");
    if (ESP_OK != pcf8574_port_read(dev_handleReads + 1, rdBuf + 1))
        ESP_LOGE("I2C", "readInputPorts: Unable to read from I2C");
    return rc;
};

void I2c::writeOutputs(omask_t mask)
{
    if (ESP_OK != pcf8574_port_write(dev_handleWrites, mask))
        ESP_LOGE("I2C", "Unable to write to I2C");
};

const int readAddresses[] = {i2cAddressInputs1_8, i2cAddressInputs9_16};
const int writeAddresses[] = {i2cAddressOutputs1_8, i2cAddressOutputs9_16};

I2c *I2c::get()
{
    if (I2c::theInstance == nullptr)
    {
        I2c::theInstance = new I2c();
        if (!I2c::theInstance->initDevices(readAddresses, I2c::theInstance->dev_handleReads))
        {
            delete theInstance;
            I2c::theInstance = nullptr;
        };
        if (!I2c::theInstance->initDevices(writeAddresses, I2c::theInstance->dev_handleWrites))
        {
            delete theInstance;
            I2c::theInstance = nullptr;
        };
    }
    return I2c::theInstance;
};

#endif

I2c *I2c::theInstance = nullptr;

void I2c::deleteInstance()
{
    if (theInstance != nullptr)
        delete theInstance;
    theInstance = nullptr;
};

#ifdef MOCK_LOG
#include <iostream>
void logi(const char *tag, const char *message)
{
    std::cout << tag << message;
};
void loge(const char *tag, const char *message)
{
    std::cout << tag << message;
};

#else
#include "esp_log.h"
void logi(const char *tag, const char *message)
{
    esp_log_write(ESP_LOG_INFO, tag, "%s\n", message);
};
void loge(const char *tag, const char *message)
{
    esp_log_write(ESP_LOG_ERROR, tag, "%s\n", message);
};
#endif
