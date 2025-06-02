#include <unity.h>
#include "alltestclasses.hpp"

void setUp()
{
}
void tearDown()
{
}

extern "C" int main(void)
{
    UNITY_BEGIN();
    alltests();
    UNITY_END();
}