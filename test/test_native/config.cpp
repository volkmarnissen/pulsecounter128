#include <unity.h>
#include "config.hpp"

std::string jsonfile = "{ \"counters\" : \n\
    [\n\
        {\n\
            \"name\" : \"test1\",\n\
            \"multiplier\" : 1000,\n\
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
    }\n\
}\n\
\n";

void config_simple()
{
    Config config = Config::getConfig(jsonfile.c_str());
}
void config_tests()
{
    RUN_TEST(config_simple);
}