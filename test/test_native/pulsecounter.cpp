#include <unity.h>
#include <pulsecounter.hpp>
#include <hardware.hpp>
using namespace Pulsecounter;

static int readInputCount = 0;
static omask_t outputMask = 0;

imask_t mock_readInputPorts()
{
    static imask_t currentState = 0;
    // toggle first bit
    imask_t save = currentState & ~1;
    currentState = save | (currentState & 1) ^ 0x0001;
    if (readInputCount == 3)
        Pulsecounter::stopThread();
    readInputCount++;
    return currentState;
};

omask_t mock_readOutputPorts()
{
    return outputMask;
};
void mock_writeOutputs(omask_t o)
{
    outputMask = o;
};

void pulsecounter_simple()
{
    Pulsecounter::init();
    //  TEST_ASSERT_TRUE( inputHasChanged( 0,0));
    Pulsecounter::setOutputConfiguration(0, {EMeterType});
    Pulsecounter::setPulseCounter(0, 0);
    // MockI2C in hardware.hpp
    MockI2c::mock_readInputPorts = mock_readInputPorts;
    MockI2c::mock_readOutputPorts = mock_readOutputPorts;
    MockI2c::mock_writeOutputs = mock_writeOutputs;
    Pulsecounter::startThread();
    Pulsecounter::joinThread();
}
void pulsecounter_tests()
{
    RUN_TEST(pulsecounter_simple);
}
