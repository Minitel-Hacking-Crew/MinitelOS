#include "globals.h"

// --- Interfaces systeme ---
Preferences preferences;
WiFiClientSecure client;
HTTPClient http;

// Session
String sessionPassword = "";
String sessionUsername = "";

String sessionAccessLevel = "user";

bool sessionIsLoggedIn = false;
bool permitLeftRight = true;

// SSH
String sshUser = "";
String sshHost = "";
String sshPassword = "";

// WiFi
String wifiSSID = "";
String wifiPassword = "";

SSHClient sshClient;

String OSVersion = "1.0a";