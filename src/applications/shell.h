#ifndef SHELL_H
#define SHELL_H

#include "globals.h"

// Fonctions utilitaires
void write_to_file(const String &filename, const String &content, bool append = false);
String shell_abspath(const String &path);
void attendreToucheMore();
void shell_println_wrapped(const String &text);

//externs
extern String shell_current_dir;
extern std::map<String, String> shell_vars;

//Fonctions shell
void shell(bool skipInitScreen = false);
void shell_sshlauncher(const String &args);
void shell_ping(const String &args);
void shell_ifconfig(const String &args);
void shell_curl(const String &args);
void shell_run(const String &args);
void shell_eval_line(const String &line);
bool shell_process_line(const String &line);
void shell_execute_block(const std::vector<String> &block);
void reset_shell_println_line_count();
void shell_cronpause(const String &);
void shell_cronresume(const String &);

struct ShellCommand
{
    const char *name;
    void (*func)(const String &);
};

extern int numCommands;
extern ShellCommand commands[];

#endif // SHELL_H