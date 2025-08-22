#include "cron.h"
#include <FS.h>
#include <LittleFS.h>
#include "globals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern bool in_ssh_session;
extern bool shell_redirect_mode;
bool cron_paused = false;
extern String shell_output_buffer;

std::vector<CronTask> cronTasks;
TaskHandle_t cronTaskHandle = NULL;

void load_crontab()
{
    cronTasks.clear();
    File f = LittleFS.open("/root/.crontab", "r");
    if (!f)
    {
        minitel.println("Erreur d'ouverture du fichier de crontab.");
        return;
    }
    int taskCount = 0;
    while (f.available())
    {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#"))
            continue;
        int sep = line.indexOf(' ');
        if (sep == -1)
            continue;
        String intervalStr = line.substring(0, sep);
        String cmd = line.substring(sep + 1);
        unsigned long interval = intervalStr.toInt();
        if (interval > 0 && cmd.length() > 0)
        {
            cronTasks.push_back({cmd, interval * 1000, millis()});
            taskCount++;
        }
    }
    f.close();
    minitel.println("[CRON] " + String(taskCount) + " taches chargees.");
}

void cronTask(void *pvParameters)
{
    while (true)
    {
        if (!in_ssh_session && !cron_paused)
        {
            unsigned long now = millis();
            for (auto &task : cronTasks)
            {
                if (now - task.lastRun >= task.intervalMs)
                {
                    bool oldRedirect = shell_redirect_mode;
                    shell_redirect_mode = true;
                    shell_output_buffer = "";
                    if (task.command.startsWith("run "))
                    {
                        shell_run(task.command.substring(4));
                    }
                    else
                    {
                        int spaceIdx = task.command.indexOf(' ');
                        String cmd = (spaceIdx == -1) ? task.command : task.command.substring(0, spaceIdx);
                        String args = (spaceIdx == -1) ? "" : task.command.substring(spaceIdx + 1);
                        for (int i = 0; i < numCommands; ++i)
                        {
                            if (cmd == commands[i].name)
                            {
                                commands[i].func(args);
                                break;
                            }
                        }
                    }
                    shell_redirect_mode = oldRedirect;
                    shell_output_buffer = "";
                    task.lastRun = now;
                }
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void start_cronTask()
{
    load_crontab();
    if (cronTaskHandle == NULL)
    {
        xTaskCreatePinnedToCore(cronTask, "cronTask", 8192, NULL, 1, &cronTaskHandle, 1);
    }
}
