#include <unity.h>
#include <config.hpp>
#include <esp_log.h>

void config_readWrite()
{
    std::string json = Config::getJson();
    std::string jsonOrig = json;
    TEST_ASSERT_TRUE_MESSAGE(json != "", "Expected json content");
    Config::setJson("teststring");
    json = Config::getJson();
    TEST_ASSERT_TRUE_MESSAGE(json == "teststring", "Expected modified content");
    Config::setJson(jsonOrig.c_str());
};

void config_tests()
{
    RUN_TEST(config_readWrite);
}
