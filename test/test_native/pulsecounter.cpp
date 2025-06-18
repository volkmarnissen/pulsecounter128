#include <unity.h>
#include <pulsecounter.hpp>
#include <hardware.hpp>
using namespace Pulsecounter;

static int readInputCount = 0;
static omask_t outputMask = 0;

imask_t mock_readInputPorts2()
{
    static imask_t currentState0 = 0;
    static imask_t currentState4 = 0;
    if (outputMask == 0x01)
    {
        switch (currentState0)
        {
        case 0:
            currentState0 = 0x0001;
            break;
        case 0x0001:
            currentState0 = 0x0401;
            break;
        case 0x0401:
            currentState0 = 0x0400;
            break;
        case 0x0400:
            // No changes
            break;
        }
        return currentState0;
    }
    else
    {
        if (currentState0 == 0x0400)
            switch (currentState4)
            {
            case 0:
                currentState4 = 0x0020; // 1 2 4 8 16 32
                break;
            case 0x0020:
                currentState4 = 0x0420;
                break;
            case 0x0420:
                currentState4 = 0x0400;
                break;
            case 0x0400:
                // No changes
                break;
            }

        return currentState4; // always 5th bit set
    }
};
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
    Pulsecounter::setPulseCounter(4, 5);
    // MockI2C in hardware.hpp
    MockI2c::mock_readInputPorts = mock_readInputPorts;
    MockI2c::mock_readOutputPorts = mock_readOutputPorts;
    MockI2c::mock_writeOutputs = mock_writeOutputs;
    Pulsecounter::startThread();
    Pulsecounter::joinThread();
}

void pulsecounter_readInputsRisingEdge()
{
    Pulsecounter::init();
    //  TEST_ASSERT_TRUE( inputHasChanged( 0,0));
    Pulsecounter::setOutputConfiguration(0, {EMeterType});
    Pulsecounter::setPulseCounter(0, 0);
    Pulsecounter::setOutputConfiguration(4, {EMeterType});
    Pulsecounter::setPulseCounter(4, 5);

    // MockI2C in hardware.hpp
    MockI2c::mock_readInputPorts = mock_readInputPorts2;
    MockI2c::mock_readOutputPorts = mock_readOutputPorts;
    MockI2c::mock_writeOutputs = mock_writeOutputs;
    PulseCounterType pc0 = {0, 0, 0};
    PulseCounterType pc4 = {5, 4, 0};

    TEST_ASSERT_TRUE_MESSAGE(Pulsecounter::readInputsRisingEdge(), "any has rising edge");
    TEST_ASSERT_TRUE_MESSAGE(Pulsecounter::inputHasRisingEdge(pc0), "0,0 has rising edge");
    TEST_ASSERT_TRUE_MESSAGE(Pulsecounter::readInputsRisingEdge(), "some other bit has changed");
    TEST_ASSERT_FALSE_MESSAGE(Pulsecounter::inputHasRisingEdge(pc0), "0,0 no rising edge");

    TEST_ASSERT_TRUE_MESSAGE(Pulsecounter::readInputsRisingEdge(), "0,0 has falling edge, but 4,5 has rising edge");
    TEST_ASSERT_FALSE_MESSAGE(Pulsecounter::inputHasRisingEdge(pc0), "0,0 has falling edge");
    TEST_ASSERT_TRUE_MESSAGE(Pulsecounter::inputHasRisingEdge(pc4), "4,5 has rising edge");

    TEST_ASSERT_TRUE_MESSAGE(Pulsecounter::readInputsRisingEdge(), "4,5 some other bit has changed");
    TEST_ASSERT_FALSE_MESSAGE(Pulsecounter::inputHasRisingEdge(pc0), "4,5 has no rising edge");
    TEST_ASSERT_FALSE_MESSAGE(Pulsecounter::readInputsRisingEdge(), "4,5 has falling edge");
    TEST_ASSERT_FALSE_MESSAGE(Pulsecounter::inputHasRisingEdge(pc0), "4,5 has falling edge");
}

void pulsecounter_tests()
{
    RUN_TEST(pulsecounter_simple);
    RUN_TEST(pulsecounter_readInputsRisingEdge);
}
