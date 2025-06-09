#include <unity.h>
#include "config.hpp"
#include <string>
#include <fstream>
#include <streambuf>

std::string readFile( const char* filename){
    std::ifstream t(filename);
    return std::string((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());
}

void config_simple()
{

    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    TEST_ASSERT_TRUE(std::string("hostname") == std::string(config.getNetwork().getHostname()));
    TEST_ASSERT_TRUE(std::string("Hugo") == std::string(config.getMqtt().getUsername()));
    TEST_ASSERT_TRUE(23 == config.getSchedule().getHour()[0]);
    int mp = config.getCounters()[0].getDivider();
    TEST_ASSERT_TRUE(888 == mp);
}
void config_tests()
{
    RUN_TEST(config_simple);
}