#include <unity.h>
#include "config.hpp"
#include "native.hpp"
#include "ArduinoJson.h"
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

void config_addBuildDate()
{
    std::string rc = Config::getJson();
    TEST_ASSERT_EQUAL_STRING("{\n\"builddate\": \"20.2.2022 02:02:02\", \"someKey\": \"SomeValue\"}", rc.c_str());
}
void config_jsonCheck()
{

    ;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, readFile("cypress/fixtures/config.json"));
    if (error)
    {
        fprintf(stderr, "deserializeJson() failed: %s", error.c_str());
    }
    std::string out;
    serializeJson(doc, out);
    fprintf(stderr, "serializeJson: %s", out.c_str());
}

void config_tests()
{
    // RUN_TEST(config_jsonCheck);
    RUN_TEST(config_addBuildDate);
    RUN_TEST(config_lohmann);
    RUN_TEST(config_simple);
}