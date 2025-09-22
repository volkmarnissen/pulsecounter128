#include <unity.h>
#include <pulsecounter.hpp>
#include <hardware.hpp>
#include "native.hpp"
#include <cstring>
#include <string>

using namespace Pulsecounter;

static int readInputCount = 0;
static omask_t outputMask = 0;

extern OutputData outputData[8];
extern NoOutputData noOutputData;
static bool resetRequest = false;

// local static data =====================
extern PulseCounterType pulseCounters[maxPulseCounters];
extern int pulseCounterCount;

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
    if (readInputCount == 10)
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
void mock_writeOutputsShouldNotBeCalled(omask_t o, int idx)
{
    TEST_FAIL_MESSAGE("writeOutput should not be called");
};

void pulsecounter_simple()
{
    Pulsecounter::init();
    //  TEST_ASSERT_TRUE( inputHasChanged( 0,0));
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    Pulsecounter::setOutputConfiguration(config.getOutputs()[0], config);
    Pulsecounter::setPulseCounter(0, 0);
    Pulsecounter::setPulseCounter(4, 5);
    Pulsecounter::setMqttStatus("{\"lastPublished\": 1234567890 }");
    // MockI2C in hardware.hpp
    readInputCount = 0;

    MockI2c::mock_readInputPorts = mock_readInputPorts;
    MockI2c::mock_readOutputPorts = mock_readOutputPorts;
    MockI2c::mock_writeOutputs = mock_writeOutputs;
    Pulsecounter::startThread();
    Pulsecounter::joinThread();
    OutputData *outputdata = Pulsecounter::getOutputData();
    TEST_ASSERT_EQUAL_INT32(4, Pulsecounter::getCounts(0, 0));
    TEST_ASSERT_EQUAL_INT32(0, Pulsecounter::getCounts(4, 5));
}

void pulsecounter_inputOnly()
{
    Pulsecounter::init();
    //  TEST_ASSERT_TRUE( inputHasChanged( 0,0));
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    readInputCount = 0;
    Pulsecounter::setPulseCounter(-1, 0);
    Pulsecounter::setPulseCounter(-1, 5);
    // MockI2C in hardware.hpp
    MockI2c::mock_readInputPorts = mock_readInputPorts;
    MockI2c::mock_readOutputPorts = mock_readOutputPorts;
    MockI2c::mock_writeOutputs = mock_writeOutputsShouldNotBeCalled;
    Pulsecounter::startThread();
    Pulsecounter::joinThread();
    OutputData *outputdata = Pulsecounter::getOutputData();
    TEST_ASSERT_EQUAL_INT32(5, Pulsecounter::getCounts(-1, 0));
    TEST_ASSERT_EQUAL_INT32(0, Pulsecounter::getCounts(-1, 5));
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
extern void readInputsRisingEdgeNoOutputs();
void testCountPulses(imask_t imaskp, imask_t imaskc, int expPcNum, const char *expJson, const char *msg)
{
    noOutputData.currentInputMask = imaskc;
    noOutputData.previousInputMask = imaskp;
    pulseCounterCount = 8;
    for (int idx = 0; idx < pulseCounterCount; idx++)
    {
        pulseCounters[idx].numOutPort = 0xFF;
        pulseCounters[idx].numInputPort = idx;
        pulseCounters[idx].counter = 0;
        pulseCounters[idx].lastSecond = 1234;
    }
    Pulsecounter::countPulses(2222);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(1, pulseCounters[expPcNum].counter, msg);
    std::string ss = Pulsecounter::getStatusJson();
    const char *isStr = ss.c_str();
    int cmp = strlen(isStr);
    int exp = strlen(expJson);
    for (int idx = 0; idx < exp; idx++)
    {
        if (isStr[idx] != expJson[idx])
            printf("%d\n", idx);
    }
    cmp = strcmp(isStr, expJson);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expJson, isStr, msg);
}
void pulsecounter_countPulses()
{
    char expJson[] = "[{\"lastPublished\": 1234567890 },\n{ \"input\": 0, \"output\":255, \"last\": 0, \"lastSecond\": 1234 },\
{ \"input\": 1, \"output\":255, \"last\": 0, \"lastSecond\": 1234 },\
{ \"input\": 2, \"output\":255, \"last\": 0, \"lastSecond\": 1234 },\
{ \"input\": 3, \"output\":255, \"last\": 0, \"lastSecond\": 1234 },\
{ \"input\": 4, \"output\":255, \"last\": 0, \"lastSecond\": 1234 },\
{ \"input\": 5, \"output\":255, \"last\": 0, \"lastSecond\": 1234 },\
{ \"input\": 6, \"output\":255, \"last\": 0, \"lastSecond\": 1234 },\
{ \"input\": 7, \"output\":255, \"last\": 1, \"lastSecond\": 2222 }]";
    Pulsecounter::setMqttStatus("{\"lastPublished\": 1234567890 }");

    testCountPulses(0xff3f, 0xffbf, 7, expJson, "0xff3f, 0xffbf");
    testCountPulses(0xff7f, 0xffff, 7, expJson, "0xff7f, 0xffff");
}

void pulsecounter_getStatusJson()
{
    Pulsecounter::setErrors("");
    Pulsecounter::setMqttStatus("{\"lastPublised\": 123456789}");
    std::string rc = Pulsecounter::getStatusJson();
}

void pulsecounter_setOutputConfiguration()
{
    TestOutput o0(0, EMeterType);
    TestOutput o4(4, EMeterType);
    TestOutput *oa[] = {&o0, &o4};
    TestCounter c0(0, 0, 1000);
    TestCounter c1(1, 0, 500);
    TestCounter c2(2, 0, 1500);
    TestCounter c3(4, 255, 300);
    TestCounter *ca[] = {&c0, &c1, &c2, &c3};
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
    RUN_TEST(pulsecounter_countPulses);
    RUN_TEST(pulsecounter_simple);
    RUN_TEST(pulsecounter_inputOnly);
    RUN_TEST(pulsecounter_readInputsRisingEdge);
    RUN_TEST(pulsecounter_getStatusJson);
}
