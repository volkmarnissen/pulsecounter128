#include <unity.h>
#include <pulsecounter.hpp>
#include <hardware.hpp>
#include "native.hpp"

using namespace Pulsecounter;

static int readInputCount = 0;
static omask_t outputMask = 0;

imask_t mock_readInputPorts2()
{
    static imask_t currentState0 = 0;
    static imask_t currentState4 = 0;
    if (outputMask == 0x10)
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

omask_t mock_readOutputPorts(int idx)
{
    return outputMask;
};
void mock_writeOutputs(omask_t o, int idx)
{
    outputMask = o;
};

void pulsecounter_simple()
{
    Pulsecounter::init();
    //  TEST_ASSERT_TRUE( inputHasChanged( 0,0));
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    Pulsecounter::setOutputConfiguration(config.getOutputs()[0], config);
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
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    Pulsecounter::init();
    //  TEST_ASSERT_TRUE( inputHasChanged( 0,0));
    const OutputConfig &output = config.getOutputs()[0];
    Pulsecounter::setOutputConfiguration(output, config);
    Pulsecounter::setPulseCounter(0, 0);
    Pulsecounter::setOutputConfiguration(config.getOutputs()[1], config);
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
};
class TestOutput : public OutputConfig
{
public:
    TestOutput(int _port, OutputConfigurationType _type)
    {
        port = _port;
        config.type = _type;
    };
};
class TestCounter : public CounterConfig
{
public:
    TestCounter(int inport, int outport, int _divider)
    {
        inputPort = inport;
        outputPort = outport;
        divider = _divider;
    }
};

class TestConfig : public Config
{
public:
    TestConfig(TestOutput *_outputs[], int outputCount, TestCounter *_counters[], int counterCount)
    {
        for (int idx = 0; idx < outputCount; idx++)
            outputs.push_back(*(_outputs[idx]));
        for (int idx = 0; idx < counterCount; idx++)
            counters.push_back(*(_counters[idx]));
    }
};

void pulsecounter_setOutputConfiguration()
{
    TestOutput o0(0, EMeterType);
    TestOutput o4(4, EMeterType);
    TestOutput *oa[] = {&o0, &o4};
    TestCounter c0(0, 0, 1000);
    TestCounter c1(1, 0, 500);
    TestCounter c2(2, 0, 1500);
    TestCounter *ca[] = {&c0, &c1, &c2};
    TestConfig cfg(oa, sizeof(oa) / sizeof(oa[0]), ca, sizeof(ca) / sizeof(ca[0]));
    Pulsecounter::setOutputConfiguration(o0, cfg);
    OutputData *outputdata = Pulsecounter::getOutputData();
    TEST_ASSERT_EQUAL_INT32(1, outputdata->maxCount);
    TEST_ASSERT_EQUAL_INT32(1 | (1 << 4), outputdata->pcMask);
    TEST_ASSERT_EQUAL_INT32(1 << 4, outputdata->onMask);
    TEST_ASSERT_EQUAL_INT32(1, outputdata->offMask);
    TEST_ASSERT_TRUE_MESSAGE(Pulsecounter::readInputsRisingEdge(), "any has rising edge");
}

void pulsecounter_tests()
{
    RUN_TEST(pulsecounter_setOutputConfiguration);
    RUN_TEST(pulsecounter_simple);
    RUN_TEST(pulsecounter_readInputsRisingEdge);
}
