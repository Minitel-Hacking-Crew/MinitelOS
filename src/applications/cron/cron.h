#pragma once
#include <Arduino.h>
#include <vector>

struct CronTask
{
    String command;
    unsigned long intervalMs;
    unsigned long lastRun;
};

void load_crontab();
void start_cronTask();
extern std::vector<CronTask> cronTasks;
