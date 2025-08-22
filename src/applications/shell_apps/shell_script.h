#ifndef SHELL_SCRIPT_H
#define SHELL_SCRIPT_H

#include "globals.h"

void shell_run(const String &args);
void shell_eval_line(const String &line);
bool shell_process_line(const String &line);
void shell_execute_block(const std::vector<String> &block);

#endif // SHELL_SCRIPT_H
