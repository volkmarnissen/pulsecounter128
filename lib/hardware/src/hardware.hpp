#pragma once
#include <cstdint>
#ifndef MOCK_I2C
#include <pcf8574.h>
#endif
typedef unsigned char omask_t;
typedef unsigned short imask_t;

const int numOutPorts = 8;
const int numInputPorts = 16;
class I2c
{
    static I2c *theInstance;
#ifndef MOCK_I2C
    i2c_port_t i2c_master_port;
    i2c_dev_t dev_handleReads[2];
    i2c_dev_t dev_handleWrites[2];
    bool isInitialized;
    bool initDevices(const int *devAddresses, i2c_dev_t *devHandles);
    bool initBus();
    I2c();
    ~I2c();
#endif

public:
    omask_t readOutputPorts();
    imask_t readInputPorts();
    void writeOutputs(omask_t mask);
    static I2c *get();
    static void deleteInstance();
};
extern void logi(const char *tag, const char *message);
extern void loge(const char *tag, const char *message);

#ifdef MOCK_I2C
namespace MockI2c
{
    extern omask_t (*mock_readOutputPorts)();
    extern imask_t (*mock_readInputPorts)();
    extern void (*mock_writeOutputs)(omask_t mask);
}
#endif