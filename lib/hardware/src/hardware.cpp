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

    i2c_master_bus_config_t i2c_mst_config;
    memset(&i2c_mst_config, 0, sizeof(i2c_mst_config));
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = i2cPort;
    i2c_mst_config.scl_io_num = i2cScl;
    i2c_mst_config.sda_io_num = i2cSda;

    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;
    i2c_mst_config.intr_priority = 0;
    i2c_mst_config.trans_queue_depth = 0;            // max device_num
    i2c_mst_config.flags.enable_internal_pullup = 1; // max device_num
    i2c_mst_config.flags.allow_pd = 1;               // Other values will core dump!!!
    ESP_LOGI(TAG, "Initializing bus \n");
    return (ESP_OK != i2c_new_master_bus(&i2c_mst_config, &bus_handle));
}

bool I2c::initDevices(const int *devAddresses, i2c_master_dev_handle_t *devHandles)
{
    assert(bus_handle != nullptr);
    ESP_LOGI(TAG, "Initializing devices");
    for (int idx = 0; idx < 2; idx++)
    {
        i2c_device_config_t dev_cfg;
        memset(&dev_cfg, 0, sizeof(dev_cfg));

        dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_cfg.device_address = 0x58;
        dev_cfg.scl_speed_hz = 100000;
        i2c_master_dev_handle_t devHandle;
        ESP_LOGI(TAG, "Initializing device %d\n", (int)bus_handle);
        if (ESP_OK != i2c_master_bus_add_device(bus_handle, &dev_cfg, &devHandle))
        {
            ESP_LOGI(TAG, "Initializing device failed %d\n", idx);
            return false;
        };
    }
    ESP_LOGI(TAG, "Devices initialized\n");
    return true;
}

I2c::I2c()
{
    dev_handleReads[0] = dev_handleReads[1] = nullptr;
    dev_handleWrites[0] = dev_handleWrites[1] = nullptr;
    bus_handle = nullptr;
};

I2c::~I2c()
{
    for (int idx = 0; idx < 2; idx++)
    {
        if (dev_handleReads[idx] != nullptr)
            i2c_master_bus_rm_device(dev_handleReads[idx]);
        if (dev_handleWrites[idx] != nullptr)
            i2c_master_bus_rm_device(dev_handleWrites[idx]);
    }
    i2c_del_master_bus(bus_handle);
};

omask_t I2c::readOutputPorts()
{
    omask_t rc;
    unsigned char *rdBuf = &rc;
    if (ESP_OK != i2c_master_receive(dev_handleReads[0], &rc, 1, -1))
        ESP_LOGE("I2C", "Unable to read to I2C");
    return rc;
};

imask_t I2c::readInputPorts()
{
    imask_t rc;
    unsigned char *rdBuf = (unsigned char *)&rc;
    if (ESP_OK != i2c_master_receive(dev_handleReads[0], rdBuf, 1, -1))
        ESP_LOGE("I2C", "Unable to read to I2C");
    if (ESP_OK != i2c_master_receive(dev_handleReads[1], rdBuf + 1, 1, -1))
        ESP_LOGE("I2C", "Unable to read to I2C");
    return rc;
};

void I2c::writeOutputs(omask_t mask)
{
    if (ESP_OK != i2c_master_transmit(dev_handleWrites[0], &mask, 1, -1))
        ESP_LOGE("I2C", "Unable to write to I2C");
};

const int readAddresses[] = {i2cAddressInputs1_8, i2cAddressInputs9_16};
const int writeAddresses[] = {i2cAddressOutputs1_8, i2cAddressOutputs9_16};

I2c *I2c::get()
{
    if (I2c::theInstance == nullptr)
    {
        I2c::theInstance = new I2c();
        ESP_LOGI("I2C", "BusHandle %d\n", (int)I2c::theInstance->bus_handle);
        bool rc = !I2c::theInstance->initBus();
        ESP_LOGI("I2C", "BusHandle after Init %d\n", (int)I2c::theInstance->bus_handle);
        if (rc || !I2c::theInstance->initDevices(readAddresses, I2c::theInstance->dev_handleReads))
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
