#include <config.hpp>
#include <thread>
#include <ctime>
#include <condition_variable>

// template <typename T>
// class CopyableVector : std::vector<T>
// {
// public:
//     void operator=(const T &vec);
// };

class Scheduler
{
    std::thread *t;

    std::condition_variable cv;
    std::mutex cv_m; // This mutex is used for three purposes:
                     // 1) to synchronize accesses to i
                     // 2) to synchronize accesses to std::cerr
                     // 3) for the condition variable cv

    bool stopRequest = false;
    void executeLocal();
    void wait(int millis);
#ifdef NATIVE
    // protected for native unit tests
protected:
    std::vector<int> &hour;
    std::vector<int> &minute;
    std::vector<int> &second;
#define CONST_CONFIG
#else
    std::vector<int> &hour;
    std::vector<int> &minute;
    std::vector<int> &second;
#define CONST_CONFIG const
#endif

public:
    Scheduler(CONST_CONFIG ScheduleConfig &_config);
    void setConfig(CONST_CONFIG ScheduleConfig &config);
    void run();
    virtual void execute() = 0;
    void stopThread();
    void joinThread();
    int getMilliSecondsToNextRun(struct timeval currentTime);
};