#include <unity.h>
#include "config.hpp"
#include "native.hpp"

void config_simple()
{

    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    TEST_ASSERT_TRUE(std::string("hostname") == std::string(config.getNetwork().getHostname()));
    TEST_ASSERT_TRUE(std::string("Hugo") == std::string(config.getMqtt().getUsername()));
    TEST_ASSERT_TRUE(23 == config.getSchedule().getHour()[0]);
    TEST_ASSERT_EQUAL_INT32(5, config.getSchedule().getMinute().size());
    TEST_ASSERT_EQUAL_INT32(50, config.getSchedule().getMinute()[0]);
    TEST_ASSERT_EQUAL_INT32(5, config.getSchedule().getSecond().size());
    TEST_ASSERT_EQUAL_INT32(12, config.getSchedule().getSecond()[0]);
    int mp = config.getCounters()[0].getDivider();
    TEST_ASSERT_TRUE(888 == mp);
}
void config_lohmann()
{

    Config config = Config::getConfig(readFile("cypress/fixtures/configlohmann.json").c_str());
    TEST_ASSERT_EQUAL_INT32(0, config.getSchedule().getHour().size());
}

void config_tests()
{
    RUN_TEST(config_lohmann);
    RUN_TEST(config_simple);
}