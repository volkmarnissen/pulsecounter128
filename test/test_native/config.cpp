#include <unity.h>
#include "config.hpp"

std::string jsonfile("{ \"counters\" : \n\
    [\n\
        {\n\
            \"mqttname\" : \"test1\",\n\
            \"divider\" : 888,\n\
            \"inputPort\": 0,\n\
            \"outputPort\": 0\n\
        }\n\
    ],\n\
  \"outputs\" : [\n\
        {\n\
            \"port\" : 0,\n\
            \"type\" : 0\n\
        }\n\
    ],\n\
    \"network\":{\n\
            \"sslcert\" : \"abcdssl\",\n\
            \"hostname\" : \"hostname\"\n\
    },\n\
    \"mqtt\":{\n\
        \"mqtturl\": \"mqtts://blabla\",\n\
        \"username\": \"Hugo\", \n\
        \"password\": \"abcd1234\",  \n\
        \"authenticationMethod\": 0 \n\
    },\n\
    \"schedule\":{\n\
        \"hour\": \"23\",\n\
        \"minute\": \"52\",\n\
        \"second\": \"12\"\n\
    }\n\
\n\
}\n");

void config_simple()
{
    Config config = Config::getConfig(jsonfile.c_str());
    TEST_ASSERT_TRUE(std::string("hostname") == std::string(config.getNetwork().getHostname()));
    TEST_ASSERT_TRUE(std::string("Hugo") == std::string(config.getMqtt().getUsername()));
    TEST_ASSERT_TRUE(std::string("23") == std::string(config.getSchedule().getHour()));
    int mp = config.getCounters()[0].getDivider();
    TEST_ASSERT_TRUE(888 == mp);
}
void config_tests()
{
    RUN_TEST(config_simple);
}