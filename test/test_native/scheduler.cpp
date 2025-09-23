#include <unity.h>
#include "config.hpp"
#include "scheduler.hpp"
#include "native.hpp"
#include <sys/time.h>
#include <ctime>
#include <cmath>
#include <thread>
#include <iostream>
#include <iomanip>
#include <ctime>
#include "pclog.hpp"
#define TAG "schedulertest"
class TestScheduler : public Scheduler
{
public:
    TestScheduler(ScheduleConfig &config) : Scheduler(config) {};
    void setHour(const std::vector<int> &_hour)
    {
#ifdef NATIVE
        hour = _hour;
#endif
    }
    void execute()
    {
        printf("executing\n");
    }
};

void scheduler_stopThread()
{
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    TestScheduler sch(const_cast<ScheduleConfig &>(config.getSchedule()));
    sch.run();
    // No assertions required: If this test doesn't work, it will run forever
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    sch.stopThread();
    sch.joinThread();
    ESP_LOGI(TAG, "scheduler is stopped\n");
    sch.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    sch.stopThread();
    sch.joinThread();
    ESP_LOGI(TAG, "scheduler is stopped\n");
}

static void testGetMilliSecondsToNextRun(std::tm expectedTimeTm, int expectedTimeMs, std::tm testTimeTm, int testTimeMs)
{
    // adjust mday if required
    std::time_t expectedTime = std::mktime(&expectedTimeTm);
    expectedTimeTm = *std::localtime(&expectedTime);
    struct timeval testTime;
    testTime.tv_sec = std::mktime(&testTimeTm);
    testTime.tv_usec = testTimeMs * 1000; // 100ms
    Config config = Config::getConfig(readFile("cypress/fixtures/config.json").c_str());
    TestScheduler sch(const_cast<ScheduleConfig &>(config.getSchedule()));
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
};
class ScheduleEvery15minConfig : public ScheduleConfig
{
public:
    ScheduleEvery15minConfig() : ScheduleConfig()
    {
        for (int h = 0; h < 23; h++)
            this->hour.push_back(h);
        for (int m = 0; m < 59; m += 15)
            this->minute.push_back(m);
        for (int s = 0; s < 59; s++)
            this->second.push_back(s);
    }
};
static void scheduler_getMilliSecondsToNextRun_nonNegative()
{
    ScheduleEvery15minConfig schedule;
    const int secPerDay = 60 * 60 * 24;
    const int minPerDay = 60 * 24;
    for (int s = 0; s < secPerDay; s++)
    {
        std::tm testTimeTm;
        testTimeTm.tm_hour = s / 60 / 60;
        testTimeTm.tm_min = s / 60;
        testTimeTm.tm_sec = s % minPerDay;
        testTimeTm.tm_mday = 7;
        testTimeTm.tm_mon = 7;
        testTimeTm.tm_year = 2021 - 1900;
        int testTimeMs = 0;
        struct timeval testTime;
        testTime.tv_sec = std::mktime(&testTimeTm);
        testTime.tv_usec = testTimeMs * 1000; // 100ms
        TestScheduler sch(schedule);
        int rc = sch.getMilliSecondsToNextRun(testTime);
        char buf[512];
        sprintf(buf, "Negative Milliseconds %d", testTime.tv_sec);
        TEST_ASSERT_TRUE_MESSAGE(rc >= 0, buf);
    }
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
    expectedTimeTm.tm_sec = 12;
    expectedTimeTm.tm_mday = 8;
    testGetMilliSecondsToNextRun(expectedTimeTm, 0, testTimeTm, 100);
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
    expectedTimeTm.tm_sec = 12;
    expectedTimeTm.tm_mday = 1;
    expectedTimeTm.tm_mon = 1;
    testGetMilliSecondsToNextRun(expectedTimeTm, 0, testTimeTm, 100);
}
void scheduler_getMilliSecondsToNextRun_afterLastSecond()
{
    std::tm testTimeTm{};
    std::tm expectedTimeTm = testTimeTm;
    testTimeTm.tm_hour = 23;
    testTimeTm.tm_min = 51;
    testTimeTm.tm_sec = 54;
    testTimeTm.tm_mday = 31;
    testTimeTm.tm_mon = 0;

    expectedTimeTm.tm_hour = 23;
    expectedTimeTm.tm_min = 52;
    expectedTimeTm.tm_sec = 12;
    expectedTimeTm.tm_mday = 31;
    expectedTimeTm.tm_mon = 0;
    testGetMilliSecondsToNextRun(expectedTimeTm, 0, testTimeTm, 100);
}
void scheduler_getMilliSecondsToNextRun_positive()
{
    timeval t;
    gettimeofday(&t, nullptr);
    Config config = Config::getConfig(readFile("cypress/fixtures/configSchedule.json").c_str());
    TestScheduler sch(const_cast<ScheduleConfig &>(config.getSchedule()));
    int rc = sch.getMilliSecondsToNextRun(t);
    TEST_ASSERT_TRUE_MESSAGE(rc > 0, "Expected greater than 0");
}

class TestScheduleConfig : public ScheduleConfig
{
public:
    TestScheduleConfig(std::vector<int> &_hour, std::vector<int> _minute, std::vector<int> _second)
    {
        hour = _hour;
        minute = _minute;
        second = _second;
    }
};

#define MAKEVECTOR(vname, aname) std::vector<int> vname(aname, aname + sizeof(aname) / sizeof(aname[0]));

void scheduler_getMaxWaitTimeOneDay()
{
    int hour[] = {0};
    int minute[] = {0};
    int second[] = {0, 30};
    MAKEVECTOR(hours, hour);
    MAKEVECTOR(minutes, minute);
    MAKEVECTOR(seconds, second);
    TestScheduleConfig schedConfig(hours, minutes, seconds);
    TestScheduler scheduler(schedConfig);

    TEST_ASSERT_EQUAL_INT32_MESSAGE(24 * 60 * 60 * 1000, scheduler.getMaxWaitTime(), "expected 24 hours");
}
void scheduler_getMaxWaitTimeOneHour()
{
    int hour[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
    int minute[] = {0};
    int second[] = {0, 30};
    MAKEVECTOR(hours, hour);
    MAKEVECTOR(minutes, minute);
    MAKEVECTOR(seconds, second);
    TestScheduleConfig schedConfig(hours, minutes, seconds);
    TestScheduler scheduler(schedConfig);

    TEST_ASSERT_EQUAL_INT32_MESSAGE(60 * 60 * 1000, scheduler.getMaxWaitTime(), "expected 1 hour");
}

void scheduler_getMaxWaitTimeOneMinue()
{
    int hour[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
    int minute[60];
    int second[] = {0, 30};
    for (int i = 0; i < 60; i++)
        minute[i] = i;
    MAKEVECTOR(hours, hour);
    MAKEVECTOR(minutes, minute);
    MAKEVECTOR(seconds, second);
    TestScheduleConfig schedConfig(hours, minutes, seconds);
    TestScheduler scheduler(schedConfig);

    TEST_ASSERT_EQUAL_INT32_MESSAGE(30 * 1000, scheduler.getMaxWaitTime(), "expected 30 seconds");
}

void scheduler_tests()
{
    RUN_TEST(scheduler_stopThread);
    RUN_TEST(scheduler_getMilliSecondsToNextRun);
    RUN_TEST(scheduler_getMilliSecondsToNextRun_NextDay);
    RUN_TEST(scheduler_getMilliSecondsToNextRun_NextMonth);
    RUN_TEST(scheduler_getMilliSecondsToNextRun_afterLastSecond);
    RUN_TEST(scheduler_getMilliSecondsToNextRun_positive);
    RUN_TEST(scheduler_getMilliSecondsToNextRun_nonNegative);
    RUN_TEST(scheduler_getMaxWaitTimeOneDay);
    RUN_TEST(scheduler_getMaxWaitTimeOneHour);
    RUN_TEST(scheduler_getMaxWaitTimeOneMinue);
}