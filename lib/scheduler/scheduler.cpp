#include <scheduler.hpp>
#include <thread>
#include <algorithm>
#include <sys/time.h>
#include <ctime>

// template <typename T>
// void CopyableVector<T>::operator=(const T &vec)
// {
// }

Scheduler::Scheduler(CONST_CONFIG ScheduleConfig &config) : hour(const_cast<CONST_CONFIG std::vector<int> &>(config.getHour())),
                                                            minute(const_cast<CONST_CONFIG std::vector<int> &>(config.getMinute())),
                                                            second(const_cast<CONST_CONFIG std::vector<int> &>(config.getSecond())) {
                                                            };

void Scheduler::run()
{
    printf("Running\n");
    t = new std::thread(&Scheduler::executeLocal, this);
};
void Scheduler::executeLocal()
{
    while (!stopRequest)
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        int millis(getMilliSecondsToNextRun(now));
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait_for(lk, std::chrono::milliseconds(millis));
        if (!stopRequest)
            execute();
    }
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

int getNextTime(const std::vector<int> &v, int current, int max)
{
    int idx = getIndex(v, current);
    while (idx == -1 && current < max)
        idx = getIndex(v, ++current);
    if (idx == -1)
        return -1;
    return current;
}

int Scheduler::getMilliSecondsToNextRun(struct timeval currentTime)
{
    struct timeval before;
    gettimeofday(&before, NULL);
    // copy static structure to datetime prevents overwrite
    std::tm datetime = *std::localtime(&currentTime.tv_sec);
    std::tm datetimeNextDay = datetime;
    datetimeNextDay.tm_hour = 0;
    datetimeNextDay.tm_min = 0;
    datetimeNextDay.tm_sec = 0;
    datetimeNextDay.tm_mday++;
    int nextHour = getNextTime(hour, datetime.tm_hour, 24);
    if (nextHour == -1)
    {
        nextHour = getNextTime(hour, datetimeNextDay.tm_hour, 24);
        if (nextHour != -1)
            datetime = datetimeNextDay;
    }
    if (nextHour != -1)
    {
        int nextMinute = getNextTime(minute, datetime.tm_min, 60);
        if (nextMinute == -1 && datetime.tm_min != 0)
        {
            nextMinute = getNextTime(minute, datetimeNextDay.tm_min, 60);
            if (nextMinute != -1)
                datetime = datetimeNextDay;
        }
        if (nextMinute != -1)
        {
            int nextSecond = getNextTime(second, datetime.tm_sec, 60);
            if (nextSecond == -1 && datetime.tm_min != 0)
            {
                nextSecond = getNextTime(second, datetimeNextDay.tm_sec, 60);
                if (nextSecond != -1)
                    datetime = datetimeNextDay;
            }
            if (nextSecond != -1)
            {
                datetime.tm_min = nextMinute;
                datetime.tm_sec = nextSecond;
                datetime.tm_hour = nextHour;
                struct timeval after;
                gettimeofday(&after, NULL);
                int consumedTime = (after.tv_usec - before.tv_usec) / 1000L;
                std::time_t d = std::mktime(&datetime);
                return (d - currentTime.tv_sec) * 1000L - currentTime.tv_usec / 1000L - consumedTime;
            }
        }
    }
    return -1;
}
