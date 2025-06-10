#include <config.hpp>
#include <thread>
#include <ctime>

// template <typename T>
// class CopyableVector : std::vector<T>
// {
// public:
//     void operator=(const T &vec);
// };

class Scheduler
{
    std::thread *t;
    int getSecondsToNextRun(struct timeval currentTime);
#ifdef NATIVE
    // protected for native unit tests
protected:
    std::vector<int> &hour;
    std::vector<int> &minute;
    std::vector<int> &second;
#define CONST_CONFIG
#else
    const std::vector<int> &hour;
    const std::vector<int> &minute;
    const std::vector<int> &second;
#define CONST_CONFIG const
#endif

public:
    Scheduler(CONST_CONFIG ScheduleConfig &_config);
    void run();
    void execute();
    void joinThread();
    int getMilliSecondsToNextRun(struct timeval currentTime);
};