#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

// Constantes
const int MAX_RETRIES = 3;
const int CONNECTION_TIMEOUT = 10000;

// Fonctions
void newPage(const String &titre);
void passerEnVideotex();
void passerEnMixte();
void attendreRetour(bool backToMainMenu = true);
void attendreTouche();
String saisirTexte(const String &invite,bool masque = false,size_t maxLen = 64, const String &placeholder = "");
String getSignalStrength(int rssi);
void connecterWiFi();
void connecterWiFi(const String &ssid);
void rebootToSSH(Minitel &minitel);
#endif // UTILS_H