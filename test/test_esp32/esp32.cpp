#include <unity.h>
extern void network_tests();
extern void config_tests();

void alltests()
{
    network_tests();
    config_tests();
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