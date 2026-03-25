#include "globals.h"

// --- Interfaces systeme ---
Preferences preferences;

// Session
String sessionPassword = "";
String sessionUsername = "";

String sessionAccessLevel = "user";

bool sessionIsLoggedIn = false;
bool permitLeftRight = true;

SSHClient sshClient;

const char* OSVersion = "2.0";