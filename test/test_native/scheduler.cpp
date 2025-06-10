#include <unity.h>
#include "config.hpp"
#include "scheduler.hpp"
#include "native.hpp"
#include <sys/time.h>
#include <ctime>
#include <cmath>

void scheduler_simple()
{
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    Scheduler sch(const_cast<ScheduleConfig &>(config.getSchedule()));
    sch.run();
    sch.joinThread();
}
class TestScheduler : Scheduler
{
public:
    TestScheduler(ScheduleConfig &config) : Scheduler(config) {};
    void setHour(const std::vector<int> &_hour)
    {
        hour = _hour;
    }
};

static void testGetMilliSecondsToNextRun(std::tm expectedTimeTm, int expectedTimeMs, std::tm testTimeTm, int testTimeMs)
{
    // adjust mday if required
    std::time_t expectedTime = std::mktime(&expectedTimeTm);
    expectedTimeTm = *std::localtime(&expectedTime);
    struct timeval testTime;
    testTime.tv_sec = std::mktime(&testTimeTm);
    testTime.tv_usec = testTimeMs * 1000; // 100ms
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    Scheduler sch(const_cast<ScheduleConfig &>(config.getSchedule()));
    int rc = sch.getMilliSecondsToNextRun(testTime);
    int millis = rc % 1000;
    time_t testTimeT = testTime.tv_sec;
    testTimeT += rc / 1000;
    std::tm rctime = *std::localtime(&testTimeT);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(expectedTimeTm.tm_mday, rctime.tm_mday, "Days are different");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(expectedTimeTm.tm_hour, rctime.tm_hour, "Hours are different");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(expectedTimeTm.tm_min, rctime.tm_min, "Minutes are different");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(expectedTimeTm.tm_sec, rctime.tm_sec, "Seconds are different");
    TEST_ASSERT_TRUE_MESSAGE(abs(expectedTimeMs - millis) < 10, "Milliseconds are different(tolerance for processing time)");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(expectedTime, testTimeT, "Something else is different");
}

void scheduler_getMilliSecondsToNextRun()
{
    std::tm testTimeTm{};
    std::tm expectedTimeTm = testTimeTm;
    testTimeTm.tm_hour = 21;
    testTimeTm.tm_min = 7;
    testTimeTm.tm_sec = 14;
    testTimeTm.tm_mday = 7;

    expectedTimeTm.tm_hour = 23;
    expectedTimeTm.tm_min = 50;
    expectedTimeTm.tm_sec = 49;
    expectedTimeTm.tm_mday = 7;
    testGetMilliSecondsToNextRun(expectedTimeTm, 900, testTimeTm, 100);
}

void scheduler_getMilliSecondsToNextRun_NextDay()
{
    std::tm testTimeTm{};
    std::tm expectedTimeTm = testTimeTm;
    testTimeTm.tm_hour = 23;
    testTimeTm.tm_min = 59;
    testTimeTm.tm_sec = 14;
    testTimeTm.tm_mday = 7;

    expectedTimeTm.tm_hour = 23;
    expectedTimeTm.tm_min = 50;
    expectedTimeTm.tm_sec = 11;
    expectedTimeTm.tm_mday = 8;
    testGetMilliSecondsToNextRun(expectedTimeTm, 900, testTimeTm, 100);
}
void scheduler_getMilliSecondsToNextRun_NextMonth()
{
    std::tm testTimeTm{};
    std::tm expectedTimeTm = testTimeTm;
    testTimeTm.tm_hour = 23;
    testTimeTm.tm_min = 59;
    testTimeTm.tm_sec = 14;
    testTimeTm.tm_mday = 31;
    testTimeTm.tm_mon = 0;

    expectedTimeTm.tm_hour = 23;
    expectedTimeTm.tm_min = 50;
    expectedTimeTm.tm_sec = 11;
    expectedTimeTm.tm_mday = 1;
    expectedTimeTm.tm_mon = 1;
    testGetMilliSecondsToNextRun(expectedTimeTm, 900, testTimeTm, 100);
}
void scheduler_tests()
{
    RUN_TEST(scheduler_simple);
    RUN_TEST(scheduler_getMilliSecondsToNextRun);
    RUN_TEST(scheduler_getMilliSecondsToNextRun_NextDay);
    RUN_TEST(scheduler_getMilliSecondsToNextRun_NextMonth);
}