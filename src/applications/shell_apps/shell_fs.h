#ifndef SHELL_FS_H
#define SHELL_FS_H

#include "globals.h"

void shell_cat(const String &args);
void shell_cd(const String &args);
void shell_cp(const String &args);
void shell_create(const String &args);
void shell_ls(const String &args);
void shell_mkdir(const String &args);
void shell_mv(const String &args);
void shell_rm(const String &args);
void shell_rm_recursive(const String &path);

#endif