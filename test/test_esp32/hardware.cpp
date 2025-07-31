#include <unity.h>
#include <hardware.hpp>
#include <pclog.hpp>
#define TAG "hardware"
void i2c_read()
{
    I2c::get()->writeOutputs(0x08);
    TEST_ASSERT_EQUAL_CHAR(0x08, I2c::get()->readOutputPorts());
};

void i2c_writeOutputReadInput()
{
    ESP_LOGI(TAG, "Prepare: Connect Output Port 0 to Input 2/");
    I2c::get()->writeOutputs(0xFF, 1);
    TEST_ASSERT_EQUAL_CHAR(0xFF, I2c::get()->readOutputPorts(1));
    I2c::get()->writeOutputs(0xFF);
    TEST_ASSERT_EQUAL_CHAR(0xFF, I2c::get()->readOutputPorts());
    TEST_ASSERT_EQUAL_CHAR(0xFE, I2c::get()->readInputPorts());
    // I2c::get()->writeOutputs(0xFE, 1);
    // TEST_ASSERT_EQUAL_CHAR(0x00, I2c::get()->readInputPorts());
};

void hardware_tests()
{
    RUN_TEST(i2c_read);
    //   RUN_TEST(i2c_writeOutputReadInput);
}
