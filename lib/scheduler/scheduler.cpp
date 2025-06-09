#include <scheduler.hpp>
#include <string.hpp>

Scheduler::Scheduler(const ScheduleConfig *_config){
    config = _config;
};

int setInit(int a[], int max, const std::string &inp ){
    if( inp == "")
        for(int h=0; h < 24;h++)
            a[h] = h;
    else{
           std::istringstream iss(inp);
           int idx=0;
           while (std::getline(iss, item, ';')) {
              a[idx]=std::stoi( item );
           }
    }

}
void Scheduler::run(){
    int hours[24];
    int minutes[60];
    int seconds[60];
    if (config->getHour() == "")
        for(int h=0; h < 24;h++)
            hours[h] = h;
    
        hours="0,1,2,3,4,5,6,7,8,9,10,"

        for(int h=0; h < 24;h++)

};
void Scheduler::execute(){

};