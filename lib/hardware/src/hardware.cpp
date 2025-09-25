#include "hardware.hpp"
#include <cstddef>

#ifdef MOCK_I2C

namespace MockI2c
{
    omask_t (*mock_readOutputPorts)(int idx) = NULL;
    imask_t (*mock_readInputPorts)() = NULL;
    void (*mock_writeOutputs)(omask_t mask, int idx) = NULL;
}

omask_t I2c::readOutputPorts(int idx)
{
    if (MockI2c::mock_readOutputPorts != NULL)
        return MockI2c::mock_readOutputPorts(idx);
    return 0x08;
};

imask_t I2c::readInputPorts()
{
    if (MockI2c::mock_readInputPorts != NULL)
        return MockI2c::mock_readInputPorts();
    return 0x0F;
};

bool I2c::writeOutputs(omask_t mask, int idx)
{
    if (MockI2c::mock_writeOutputs != NULL)
        MockI2c::mock_writeOutputs(mask, idx);
    return true;
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

#include <driver/i2c.h>
#include "esp_log.h"

static const int i2cAddressInputs1_8 = 0x22;
static const int i2cAddressInputs9_16 = 0x21;
static const int i2cAddressOutputs1_8 = 0x24;
static const int i2cAddressOutputs9_16 = 0x25;
static const gpio_num_t i2cScl = GPIO_NUM_5;
static const gpio_num_t i2cSda = GPIO_NUM_4;
static const int i2cPort = -1;
static const char *TAG = "hardware";

#define WRITE_BIT (I2C_MASTER_WRITE) /*!< I2C master write */
#define READ_BIT (I2C_MASTER_READ)   /*!< I2C master read */
#define ACK_CHECK_EN (0x1)           /*!< I2C master will check ack from slave*/

// I2C communication settings
#define I2C_MASTER_FREQ_HZ 100000    // I2C frequency set to 40kHz

// I2C addresses for XL9535 chips and PCF8574
#define PCF8574_ADDR 0x23            // Address for PCF8574 (channels 33-40)

const int readAddresses[] = {i2cAddressInputs1_8, i2cAddressInputs9_16};
const int writeAddresses[] = {i2cAddressOutputs1_8, i2cAddressOutputs9_16};
#ifndef MOCK_I2C
// Read inputs from PCF8574 I/O expander
esp_err_t I2c::pcf8574_read(int addr, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_PERIOD_MS);
    if (ret != 0)
        ESP_LOGE(TAG, "read failed %04x", (unsigned)ret);
    i2c_cmd_link_delete(cmd);
    return ret;
}
esp_err_t I2c::pcf8574_write(int addr, uint8_t data)
{
    // Create a new I2C command link
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // Start the I2C communication
    i2c_master_start(cmd);
    // Send the PCA9555's I2C address with the write bit
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    // Send the register address where the data should be written
    i2c_master_write_byte(cmd, addr, ACK_CHECK_EN);
    // Send the actual data to be written to the register
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    // Stop the I2C communication
    i2c_master_stop(cmd);
    // Execute the I2C command and return the result
    esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_PERIOD_MS);
    if (ret != 0)
        ESP_LOGE(TAG, "write failed %04x", (unsigned)ret);
    // Delete the I2C command link after use
    i2c_cmd_link_delete(cmd);
    return ret;
}
#endif
bool I2c::initBus()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,             // Set I2C mode to master
        .sda_io_num = i2cSda,                // SDA GPIO pin
        .scl_io_num = i2cScl,                // SCL GPIO pin
        .sda_pullup_en = GPIO_PULLUP_ENABLE, // Enable internal pull-up for SDA line
        .scl_pullup_en = GPIO_PULLUP_ENABLE, // Enable internal pull-up for SCL line
        .master = {},                        // Set I2C clock frequency to 40kHz
        .clk_flags = 0};
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    if (ESP_OK != i2c_param_config(i2c_master_port, &conf))
    {
        ESP_LOGE(TAG, "Unable to config I2c");
        return false;
    }

    if (ESP_OK != i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0))
    {
        ESP_LOGE(TAG, "Unable to install driver I2c");
        return false;
    }
    isInitialized = true;
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
        i2c_driver_delete(i2c_master_port);
};

omask_t I2c::readOutputPorts(int idx)
{
    omask_t rc;
    esp_err_t esprc;
    if (ESP_OK != (esprc = pcf8574_read(writeAddresses[idx], &rc)))
        ESP_LOGE("I2C", "Unable to read output from I2C %X\n", esprc);
    return rc;
};

imask_t I2c::readInputPorts()
{
    imask_t rc;
    unsigned char *rdBuf = (unsigned char *)&rc;
    if (ESP_OK != pcf8574_read(readAddresses[0], rdBuf))
        ESP_LOGE("I2C", "Unable to read input from I2C");
    if (ESP_OK != pcf8574_read(readAddresses[1], rdBuf + 1))
        ESP_LOGE("I2C", "Unable to read  input from I2C");
    return rc;
};

bool I2c::writeInputPorts(imask_t inputMask)
{
    unsigned char *rdBuf = (unsigned char *)&inputMask;
    if (ESP_OK != pcf8574_write(readAddresses[0], rdBuf[0]))
    {
        ESP_LOGE("I2C", "Unable to write input to I2C");
        return false;
    }
    if (ESP_OK != pcf8574_write(readAddresses[1], rdBuf[1]))
    {
        ESP_LOGE("I2C", "Unable to write input to I2C");
        return false;
    }
    return true;
};

bool I2c::writeOutputs(omask_t mask, int idx)
{

    if (ESP_OK != pcf8574_write(writeAddresses[idx], mask))
    {
        ESP_LOGE("I2C", "Unable to write to I2C");
        return false;
    }

    return true;
};

I2c *I2c::get()
{
    if (I2c::theInstance == nullptr)
    {
        I2c::theInstance = new I2c();
        if (!I2c::theInstance->initBus())
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
