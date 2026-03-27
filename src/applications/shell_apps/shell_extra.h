#ifndef SHELL_EXTRA_H
#define SHELL_EXTRA_H

#include "globals.h"

struct FileMeta { String perms; String owner; String group; };
FileMeta get_file_meta(const String &path);
void set_file_meta(const String &path, const String &perms,
                   const String &owner, const String &group);
// Vérifie si la session courante a le droit perm ('r','w','x') sur path.
// Root passe toujours. Sinon : bits owner si propriétaire, bits other sinon.
bool fs_can_access(const String &path, char perm);

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
