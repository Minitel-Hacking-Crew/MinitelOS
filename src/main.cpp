#include "globals.h"

Minitel minitel(Serial2);

void PreferencesSetup();

void setup()
{
  Serial.begin(115200);
  minitel.changeSpeed(minitel.searchSpeed());
  minitel.clearScreen();
  passerEnMixte();
  minitel.extendedKeyboard();
  minitel.smallMode();
  minitel.textMode();
  minitel.echo(false);

  minitel.println();
  minitel.println();
  minitel.println();
  minitel.println();
  minitel.println(">>========================================================================<<");
  minitel.println("||                                                                        ||");
  minitel.println("||  __  __ _       _ _       _  ___  ____   __     ___   ___  _           ||");
  minitel.println("|| |  \\/  (_)_ __ (_| |_ ___| |/ _ \\/ ___|  \\ \\   / / | / _ \\| |__        ||");
  minitel.println("|| | |\\/| | | '_ \\| | __/ _ | | | | \\___ \\   \\ \\ / /| || | | | '_ \\       ||");
  minitel.println("|| | |  | | | | | | | ||  __| | |_| |___) |   \\ V _ | || |_| | |_) |      ||");
  minitel.println("|| |_|  |_|_|_| |_|_|\\__\\___|_|\\___/|____/     \\_(_)|_(_\\___/|_.__/       ||");
  minitel.println("||                                                                        ||");
  minitel.println("||  ____             _     _                _      ____  _    ____  ____  ||");
  minitel.println("|| | __ )  __ _ _ __| |__ | |__   __ _  ___| | __ |___ \\| | _|___ \\| ___| ||");
  minitel.println("|| |  _ \\ / _`  | '__| '_ \\| '_ \\ / _`  |/ __||/ /    __) | |/ / __) |___ \\ ||");
  minitel.println("|| | |_) | (_| | |  | |_) | | | | (_| | (__|   <   / __/|   < / __/ ___) |||");
  minitel.println("|| |____/ \\__,_|_|  |_.__/|_| |_|\\__,_|\\___|_|\\_\\ |_____|_|\\_|_____|____/ ||");
  minitel.println("||                                                                        ||");
  minitel.println("||                                                                        ||");
  minitel.println(">>========================================================================<<");
  minitel.println();
  minitel.println("Minitel Hacking Crew - Version " + String(OSVersion));
  minitel.println();
  minitel.println();

  int guideCount = 0;

  minitel.println("Appuyez sur une touche pour continuer...");
  while (true)
  {
    uint32_t k = minitel.getKeyCode(false);
    if (k == 0)
    {
      delay(50);
      continue;
    }
    if (k == CORRECTION)
    {
      guideCount++;
      if (guideCount >= 7)
      {
        minitel.println("Reinitialisation des utilisateurs...");
        break;
      }
    }
    else if (k == ENVOI)
    {
      permitLeftRight = !permitLeftRight;
      if (permitLeftRight)
      {
        minitel.println("[INFO] - Deplacement curseur active.");
      }
      else
      {
        minitel.println("[INFO] - Deplacement curseur desactive.");
      }
    }
    else
    {
      break;
    }
  }

  if (!LittleFS.begin(true))
  {
    minitel.println("Erreur FS : systeme de fichier non monte");
    while (1)
      delay(1000);
  }

  if (!LittleFS.exists("/root"))
    LittleFS.mkdir("/root");

  if (guideCount >= 6)
  {
    File f = LittleFS.open("/root/.users", "w");
    if (f)
    {
      // username:hashedPassword:accessLevel 
      f.println("root:63a9f0ea7bb98050796b649e85481845:root"); //root:root
      f.close();
    }
    else
    {
      minitel.println("Erreur creation /root/.users");
    }
  }
  preferences.begin("MHC-OS", false);
  String savedSSID = preferences.getString("ssid");
  String savedPass = preferences.getString("pass");
  if (!savedSSID.isEmpty() && !savedPass.isEmpty())
  {
    minitel.println("Connexion WiFi en cours...");
    WiFi.disconnect();
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    int retryCount = 0;
    bool connected = false;
    while (!connected && retryCount < 5)
    {
      unsigned long startTime = millis();
      while (millis() - startTime < 8000)
      {
        if (WiFi.status() == WL_CONNECTED)
        {
          connected = true;
          break;
        }
        minitel.print(".");
        delay(500);
      }
      if (!connected)
      {
        retryCount++;
        WiFi.disconnect();
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
      }
    }
    if (connected)
    {
      minitel.println();
      minitel.println("Connexion WiFi reussie");
    }
    else
    {
      minitel.println();
      minitel.println("Connexion WiFi ignoree");
    }
  }
  else
  {
    minitel.println("Aucun WiFi enregistre, connexion ignoree.");
  }
  minitel.println("Chargement des parametres OK");
  if (!LittleFS.exists("/.motd"))
  {
    File motdFile = LittleFS.open("/.motd", "w");
    if (motdFile)
    {
      motdFile.println("Bienvenue sur MinitelOS !");
      motdFile.close();
    }
    else
    {
      minitel.println("Erreur lors de la creation du fichier /.motd");
    }
  }
  preferences.end();
  minitel.println("[Minitel] Link speed : " + String(minitel.currentSpeed()));
  minitel.println("[Minitel] DeviceID : " + String(minitel.identifyDevice()));
  delay(750);

  shell();
}

void loop()
{
}