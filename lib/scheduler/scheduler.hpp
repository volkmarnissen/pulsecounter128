#include <config.hpp>

class Scheduler {
    const ScheduleConfig *config;
public:    
    Scheduler(const ScheduleConfig *_config);
    void run();
    void execute();
};