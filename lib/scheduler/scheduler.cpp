#include <scheduler.hpp>
#include <thread>
#include <algorithm>
#include <sys/time.h>
#include <ctime>
#include "pclog.hpp"
#define TAG "scheduler"
// template <typename T>
// void CopyableVector<T>::operator=(const T &vec)
// {
// }

Scheduler::Scheduler(CONST_CONFIG ScheduleConfig &config) : hour(const_cast<std::vector<int> &>(config.getHour())),
                                                            minute(const_cast<std::vector<int> &>(config.getMinute())),
                                                            second(const_cast<std::vector<int> &>(config.getSecond()))
{
    maxWaitTime = getMaxWaitTime();
};

void Scheduler::setConfig(CONST_CONFIG ScheduleConfig &config)
{
    hour = const_cast<std::vector<int> &>(config.getHour());
    minute = const_cast<std::vector<int> &>(config.getMinute());
    second = const_cast<std::vector<int> &>(config.getSecond());
    stopThread();
    joinThread();
    maxWaitTime = getMaxWaitTime();
    stopRequest = false;
    run();
};

int Scheduler::getMaxWaitTime() const
{
    int maxWaitTime = getMaxWaitTime(hour, 24);
    if (maxWaitTime > 1)
        return maxWaitTime * 60 * 60 * 1000;
    else
    {
        if (maxWaitTime == -1)
            return -1;
        maxWaitTime = getMaxWaitTime(minute, 60);
        if (maxWaitTime > 1)
            return maxWaitTime * 60 * 1000;
        else
        {
            if (maxWaitTime == -1)
                return -1;
            maxWaitTime = getMaxWaitTime(second, 60);
            if (maxWaitTime == -1)
                return -1;
            return maxWaitTime * 1000;
        }
    }
}

int Scheduler::getMaxWaitTime(std::vector<int> &v, int biggest) const
{
    if (v.begin() == v.end()) // empty Array
        return -1;
    if (v.end() - v.begin() == 1) // one entry
        return biggest;
    else
    { // More than one entry
        int maxDiff = 0;
        for (auto it = v.begin(); it < v.end() - 1; ++it)
        {
            int diff = it[1] - *it;
            if (diff > maxDiff)
                maxDiff = diff;
        }
        // difference from last to first entry
        int diff = (*v.begin() + biggest) - *(v.end() - 1);
        if (diff > maxDiff)
            return diff;
        return maxDiff;
    }
}
void Scheduler::run()
{
    ESP_LOGI(TAG, "Scheduler is running\n");
    stopRequest = false;
    t = new std::thread(&Scheduler::executeLocal, this);
};

static struct timeval lastRun = {0, 0};
void Scheduler::executeLocal()
{
    while (!stopRequest)
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        int millis = 0;
        if (lastRun.tv_sec == now.tv_sec)
        {
            ESP_LOGD(TAG, "Same Second as last run\n", millis);
            now.tv_sec++;
            millis = getMilliSecondsToNextRun(now) + 1000;
        }
        else
            millis = getMilliSecondsToNextRun(now);
        if (millis == 0)
        {
            ESP_LOGD(TAG, "Same Second\n", millis);
            now.tv_sec++;
            millis = getMilliSecondsToNextRun(now) + 1000;
        }
        if (millis > 0)
        {
            std::unique_lock<std::mutex> lk(cv_m);
            ESP_LOGD(TAG, "Wait for %d ms\n", millis);
            cv.wait_for(lk, std::chrono::milliseconds(millis));
            if (!stopRequest)
            {
                ESP_LOGD(TAG, "Executing\n");
                lastRun = now;
                execute();
            }
        }
        else
        {
            ESP_LOGI(TAG, "Waittime is negative: %d Stopping Thread\n", millis);
            stopRequest = true;
        }
    }
    ESP_LOGI(TAG, "Scheduler Thread is stopped: No further executions until reconfiguration\n");
};

void Scheduler::stopThread()
{
    std::lock_guard<std::mutex> lk(cv_m);
    stopRequest = true;
    cv.notify_one();
};

void Scheduler::joinThread()
{
    if (t)
    {
        (*t).join();
        delete t;
        t = NULL;
    }
}

int getIndex(const std::vector<int> &v, int searchObject)
{
    auto it = std::find(v.begin(), v.end(), searchObject);
    if (it == v.end())
        return -1;
    return std::distance(v.begin(), it);
}

int getNextTime(const std::vector<int> &v, int current, int max, const char *name)
{
    ESP_LOGD(TAG, "getNextTime %s %d(%d)", name, current, max);
    int idx = getIndex(v, current);
    while (idx == -1 && current < max)
        idx = getIndex(v, ++current);
    ESP_LOGD(TAG, "getNextTime %s idx=%d current=%d", name, idx, current);
    if (idx == -1)
    {
        if (v.begin() != v.end())
        {
            ESP_LOGD(TAG, "Next cycle=%d", *v.begin());
            return max; // First entry of vector
        }
        else
        {
            ESP_LOGI(TAG, "No configured %s found for %d(%d)", name, current, max);
            return -999;
        }
    }
    return current;
}
static bool vectorIsEmpty(std::vector<int> &v, const char *name)
{
    if (v.begin() == v.end())
    {
        ESP_LOGI(TAG, "Schedule: No %s configured. No execution", name);
        return true;
    }
    return false;
}
int Scheduler::getMilliSecondsToNextRun(struct timeval currentTime)
{
    struct timeval before;
    gettimeofday(&before, NULL);
    // copy static structure to datetime prevents overwrite
    std::tm datetime = *std::localtime(&currentTime.tv_sec);
    if (vectorIsEmpty(hour, "hour") || vectorIsEmpty(minute, "minute") || vectorIsEmpty(second, "second"))
        return -1;
    std::tm lastSecondOfDay = datetime;
    lastSecondOfDay.tm_hour = *(hour.end() - 1);
    lastSecondOfDay.tm_min = *(minute.end() - 1);
    lastSecondOfDay.tm_sec = *(second.end() - 1);
    time_t lastSecondOfDayT = mktime(&lastSecondOfDay);
    int currentMilliSecond = currentTime.tv_usec / 1000L;
    // Need day switch?
    if (lastSecondOfDayT < currentTime.tv_sec || (currentTime.tv_usec >= 1000 && lastSecondOfDayT == currentTime.tv_sec))
    {
        datetime.tm_mday++;
        lastSecondOfDay.tm_mday = datetime.tm_mday;
        datetime.tm_hour = 0;
        datetime.tm_min = 0;
        datetime.tm_sec = 0;
        currentMilliSecond = 0;
    }
    datetime.tm_hour = getNextTime(hour, datetime.tm_hour, 24, "hour"); // hour needs no more changes
    if (datetime.tm_hour == -999)
        return -1;

    if (datetime.tm_sec > lastSecondOfDay.tm_sec)
    { // After last second of array increment minute
        datetime.tm_min++;
        datetime.tm_sec = 0;
        currentMilliSecond = 0;
    }
    lastSecondOfDayT = mktime(&lastSecondOfDay);
    if (lastSecondOfDayT < currentTime.tv_sec || (currentTime.tv_usec >= 1000 && lastSecondOfDayT == currentTime.tv_sec))
    {
        datetime.tm_min = 0;
        lastSecondOfDay.tm_min = 0;
        datetime.tm_sec = 0;
        currentMilliSecond = 0;
    }

    datetime.tm_min = getNextTime(minute, datetime.tm_min, 60, "minute"); // minute needs no more changes
    if (datetime.tm_min == -999)
        return -1;
    lastSecondOfDayT = mktime(&lastSecondOfDay);

    if (lastSecondOfDayT < currentTime.tv_sec || (currentTime.tv_usec >= 1000 && lastSecondOfDayT == currentTime.tv_sec) || datetime.tm_min == 60)
    {
        datetime.tm_hour++;
        datetime.tm_min = minute[0];
        datetime.tm_sec = 0;
        lastSecondOfDay.tm_sec = 0;
        currentMilliSecond = 0;
    }
    datetime.tm_sec = getNextTime(second, datetime.tm_sec, 60, "second"); // second needs no more changes
    if (datetime.tm_sec == -999)
        return -1;
    if (currentTime.tv_usec >= 1000 && lastSecondOfDayT == currentTime.tv_sec)
        currentMilliSecond = 0;

    struct timeval after;
    gettimeofday(&after, NULL);
    int consumedTime = (after.tv_usec - before.tv_usec) / 1000L;
    std::time_t d = std::mktime(&datetime);

    int waitTime = (d - currentTime.tv_sec) * 1000L - consumedTime;
    if (waitTime > currentMilliSecond) // avoid negative waitTime
        waitTime -= currentMilliSecond;
    else
        waitTime = 0;
    ESP_LOGD(TAG, "Schedule: waitTime %d, MaxWaitTime %d\n", waitTime, maxWaitTime);

    if (waitTime >= 0 && waitTime < maxWaitTime)
        return waitTime;
    else
    {
        std::tm currentDateTime = *std::localtime(&currentTime.tv_sec);
        ESP_LOGI(TAG, "Schedule: Wait Time is out of range current Time: %2d:%2d:%2d:%ld next Run Time: %2d:%2d:%2d:%d WaitTime:%d MaxWaitTime %d, consumed: %d\n",
                 currentDateTime.tm_hour, currentDateTime.tm_min, currentDateTime.tm_sec, currentTime.tv_usec / 1000,
                 datetime.tm_hour, datetime.tm_min, datetime.tm_sec, currentMilliSecond, waitTime, maxWaitTime, consumedTime);
        return -1;
    }
}
