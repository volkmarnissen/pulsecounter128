#include "hardware.hpp"
#include <cstddef>

#ifdef MOCK_I2C

namespace MockI2c
{
    omask_t (*mock_readOutputPorts)() = NULL;
    imask_t (*mock_readInputPorts)() = NULL;
    void (*mock_writeOutputs)(omask_t mask) = NULL;
}

omask_t readOutputPorts()
{
    if (MockI2c::mock_readOutputPorts != NULL)
        return MockI2c::mock_readOutputPorts();
    return 0x08;
};

imask_t readInputPorts()
{
    if (MockI2c::mock_readInputPorts != NULL)
        return MockI2c::mock_readInputPorts();
    return 0x0F;
};

void writeOutputs(omask_t mask)
{
    if (MockI2c::mock_writeOutputs != NULL)
        MockI2c::mock_writeOutputs(mask);
};

#
#else

void writeOutputs(omask_t mask)
{
}
#endif

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
