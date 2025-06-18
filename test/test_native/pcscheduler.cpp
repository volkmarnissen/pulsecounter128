#include <unity.h>
#include "pcscheduler.hpp"
#include "pulsecounter.hpp"
#include <hardware.hpp>
#include "native.hpp"
#include <iostream>

extern void setPulseCount(uint8_t outputPort, uint8_t inputPort, uint32_t count);
extern int getCounterStorageCount();
extern CountersStorage *getCountersStorage();
// class TestScheduler : public PulseCounterScheduler
// {
// public:
//     TestScheduler(Config &config) : PulseCounterScheduler(config) {};
//     void setHour(const std::vector<int> &_hour)
//     {
// #ifdef NATIVE
//         hour = _hour;
// #endif
//     }
//     void execute()
//     {
//         printf("executing\n");
//     }
// };
void pcscheduler_storeCounts()
{
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());

    PulseCounterScheduler pc(config);
    Pulsecounter::init();
    Pulsecounter::setPulseCounter(0, 0);
    Pulsecounter::setPulseCounter(4, 5);
    setPulseCount(0, 0, 3200);
    setPulseCount(4, 5, 6400);
    time_t t;
    struct tm tm;

    /* fill in values for 2019-08-22 23:22:26 */
    tm.tm_year = 2019 - 1900;
    tm.tm_mon = 8 - 1;
    tm.tm_mday = 22;
    tm.tm_hour = 23;
    tm.tm_min = 22;
    tm.tm_sec = 26;
    tm.tm_isdst = -1;
    t = mktime(&tm);
    pc.storePulseCounts(t);
    int pcc = getCounterStorageCount();
    auto pcs = getCountersStorage();
    TEST_ASSERT_EQUAL_INT32(3200, pcs[0].counts[0]);
    TEST_ASSERT_EQUAL_INT32(0, pcs[0].outputPort);
    TEST_ASSERT_EQUAL_INT32(6400, pcs[1].counts[5]);
    TEST_ASSERT_EQUAL_INT32(4, pcs[1].outputPort);
    setPulseCount(4, 5, 7200);
    pc.storePulseCounts(t); // same time
    TEST_ASSERT_EQUAL_INT32(7200, pcs[1].counts[5]);
    tm.tm_sec = 27;
    t = mktime(&tm);
    setPulseCount(4, 5, 8000);
    pc.storePulseCounts(t); // different time, new entries for 0,0 and 4,5
    TEST_ASSERT_EQUAL_INT32(7200, pcs[1].counts[5]);
    TEST_ASSERT_EQUAL_INT32(8000, pcs[3].counts[5]);
    std::string pyl = pc.generatePayload();
    std::string expected = "[{\"name\":\"test1\"\n\
\"date\":1566508946\n\
\"value\":3.6036\n\
}\n\
,{\"name\":\"test1\"\n\
\"date\":1566508947\n\
\"value\":3.6036\n\
}\n\
,{\"name\":\"test45\"\n\
\"date\":1566508946\n\
\"value\":8.10811\n\
}\n\
,{\"name\":\"test45\"\n\
\"date\":1566508947\n\
\"value\":9.00901\n\
}\n\
]";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), pyl.c_str());
};
void pcscheduler_tests()
{
    RUN_TEST(pcscheduler_storeCounts);
}