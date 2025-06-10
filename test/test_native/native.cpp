#include <unity.h>
#include "alltestclasses.hpp"
#include <string>
#include <fstream>
#include <streambuf>

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

std::string readFile(const char *filename)
{
    std::ifstream t(filename);
    return std::string((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());
}
