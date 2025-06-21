#pragma once
extern void pulsecounter_tests();
extern void config_tests();
extern void scheduler_tests();
extern void pcscheduler_tests();
void alltests()
{
    pulsecounter_tests();
    config_tests();
    scheduler_tests();
    pcscheduler_tests();
}