#ifndef GLOBALS_H
#define GLOBALS_H


//MANDATORY
#include <Arduino.h>
#include <map>
#include <vector>
#include <deque>
#include <regex>
#include <WString.h>
#include <mbedtls/md5.h>
#include <algorithm>
#include <Minitel1B_Hard.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESP32Ping.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <utils/utils.h>
#include <applications/shell.h>
#include <applications/shell_apps/shell_edit.h>
#include <applications/shell_apps/shell_fs.h>
#include <applications/shell_apps/shell_sys.h>
#include <applications/shell_apps/shell_grep.h>
#include <applications/shell_apps/shell_script.h>
#include <applications/cron/cron.h>
#include <applications/ssh/sshClient.h>

//OPTIONAL


//System
extern Minitel minitel;
extern Preferences preferences;
extern WiFiClientSecure client;
extern HTTPClient http;
extern String OSVersion;

// Session
extern String sessionUsername;
extern String sessionPassword;
extern bool sessionIsLoggedIn;
extern bool permitLeftRight;
extern String sessionAccessLevel;

// SSH
extern SSHClient sshClient;
extern String sshUser;
extern String sshHost;
extern String sshPassword;

// WiFi
extern String wifiSSID;
extern String wifiPassword;

//CRON
extern bool cron_paused;

#endif // GLOBALS_H
