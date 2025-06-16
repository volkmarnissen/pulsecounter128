#include <unity.h>
#include <string>
#include <fstream>
#include <streambuf>
extern void network_tests();
extern void config_tests();
extern void hardware_tests();
extern void mqtt_tests();

void alltests()
{
    network_tests();
    config_tests();
    hardware_tests();
    // mqtt_tests();
}
void setUp()
{
}
void tearDown()
{
}

extern "C" int app_main(void)
{
    UNITY_BEGIN();
    alltests();
    UNITY_END();
    return 0;
}
