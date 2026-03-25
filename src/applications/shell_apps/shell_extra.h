#ifndef SHELL_EXTRA_H
#define SHELL_EXTRA_H

#include "globals.h"

void shell_touch(const String &args);
void shell_env(const String &args);
void shell_export(const String &args);
void shell_date(const String &args);
void shell_uptime(const String &args);
void shell_free_cmd(const String &args);
void shell_ps(const String &args);
void shell_kill_cmd(const String &args);
void shell_crontab_cmd(const String &args);
void shell_chmod(const String &args);
void shell_chown(const String &args);
void shell_du(const String &args);
void shell_nslookup(const String &args);
void shell_sleep(const String &args);
void shell_sudo(const String &args);
void shell_ctftime(const String &args);

#endif // SHELL_EXTRA_H
