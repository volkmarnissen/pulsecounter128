#include <unity.h>
#include <hardware.hpp>
#include <esp_log.h>

void i2c_read()
{
    I2c::get()->writeOutputs(0x08);
    TEST_ASSERT_EQUAL_CHAR(0x08, I2c::get()->readOutputPorts());
};

void hardware_tests()
{
    RUN_TEST(i2c_read);
}
