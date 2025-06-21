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
const char *json = "{\"counters\":[],\"outputs\":[{\"type\":\"-1\",\"port\":0},{\"type\":\"-1\",\"port\":1},{\"type\":\"-1\",\"port\":2},{\"type\":\"-1\",\"port\":3},{\"type\":\"-1\",\"port\":4},{\"type\":\"-1\",\"port\":5},{\"type\":\"-1\",\"port\":6},{\"type\":\"-1\",\"port\":7},{\"type\":\"-1\",\"port\":8}],\"network\":{\"hostname\":\"plscount\",\"sslhost\":\"\",\"sslhostkey\":\"\",\"sslca\":\"\"},\"mqtt\":{\"mqtturl\":\"asdfasd\",\"username\":\"\",\"password\":\"\",\"authenticationMethod\":0},\"schedule\":{\"hour\":[],\"minute\":[],\"second\":[]}}";
void config_readlohmann()
{
    Config config = Config::getConfig(json);
    TEST_ASSERT_EQUAL_INT32(0, config.getSchedule().getHour().size());
}

void config_tests()
{
    RUN_TEST(config_readWrite);
    RUN_TEST(config_readlohmann);
}
