#ifndef SHELL_SYS_H
#define SHELL_SYS_H

#include "Arduino.h"

void shell_login();
void shell_logout();
void shell_reboot();
void shell_clear();
void shell_df(const String &);
void shell_echo(const String &args);
void shell_passwd();
void shell_set(const String &args);
void shell_version();
void shell_whoami();
void shell_wait(const String &args);
void shell_pwd(const String &);
void shell_help(const String &);
void shell_motd();
void shell_su(const String &args);
void shell_motd(const String &args);

#define HISTORY_SIZE 24
extern std::deque<String> shell_history;
extern int shell_history_index;
void load_shell_history(const String &user);
void save_shell_history(const String &user);
void add_shell_history(const String &user, const String &cmd);
void shell_history_cmd(const String &);
void shell_adduser(const String &);
void shell_deluser(const String &);

extern bool cron_paused;

#endif // SHELL_SYS_H