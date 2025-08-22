#include "globals.h"
#include "utils.h"

void newPage(const String &titre)
{
    minitel.newScreen();
    minitel.println(titre);
    minitel.println("-------------------");
    minitel.moveCursorReturn(1);
}

void attendreTouche()
{
    while (minitel.getKeyCode(false) == 0)
        delay(50);
}

void attendreRetour(bool backToMainMenu)
{
    minitel.println();
    minitel.println("Appuyez sur une touche...");
    attendreTouche();
    if (backToMainMenu)
    {
        shell();
    }
}

String saisirTexte(const String &invite, bool masque, size_t maxLen, const String &placeholder)
{
    minitel.print(invite);
    String entree = "";
    size_t placeholderLen = placeholder.length();
    size_t placeholderRest = placeholderLen;
    bool placeholderVisible = placeholderLen > 0;
    if (placeholderVisible)
    {
        minitel.print(placeholder);
    }
    int cursorPos = 0;
    if (placeholderLen > 0)
    {
        entree = placeholder;
        cursorPos = entree.length();
        placeholderVisible = false;
        placeholderRest = 0;
    }
    while (true)
    {
        uint32_t k = minitel.getKeyCode(true);
        if (k == 0)
            continue;
        if (k == CR)
            break;
        else if (k == TOUCHE_FLECHE_GAUCHE)
        {
            if (permitLeftRight && cursorPos > 0)
            {
                cursorPos--;
                minitel.moveCursorLeft(1);
            }
        }
        else if (k == TOUCHE_FLECHE_DROITE)
        {
            if (permitLeftRight && cursorPos < entree.length())
            {
                int totalLen = (placeholderVisible ? placeholderRest : 0) + entree.length();
                minitel.moveCursorLeft(cursorPos);
                if (placeholderVisible && placeholderRest > 0)
                    minitel.print(placeholder.substring(0, placeholderRest));
                String toShow = "";
                if (masque)
                {
                    for (size_t i = 0; i < entree.length(); ++i)
                        toShow += '*';
                }
                else
                {
                    toShow = entree;
                }
                minitel.print(toShow);
                cursorPos++;
                minitel.moveCursorLeft(entree.length() - cursorPos);
            }
        }
        else if (k == CORRECTION)
        {
            if (cursorPos > 0)
            {
                entree.remove(cursorPos - 1, 1);
                cursorPos--;
                minitel.moveCursorLeft(1);
                String after = entree.substring(cursorPos);
                String maskStr = "";
                if (masque)
                {
                    for (size_t i = 0; i < after.length(); ++i)
                        maskStr += '*';
                    minitel.print(maskStr + " ");
                }
                else
                {
                    minitel.print(after + " ");
                }
                for (int i = 0; i < after.length() + 1; ++i)
                    minitel.moveCursorLeft(1);
            }
            else if (placeholderVisible && placeholderRest > 0)
            {
                placeholderRest--;
                minitel.moveCursorLeft(1);
                minitel.print(" ");
                minitel.moveCursorLeft(1);
                if (placeholderRest == 0)
                    placeholderVisible = false;
            }
        }
        else if (k == ANNULATION)
        {
            while (entree.length() > 0)
            {
                entree.remove(entree.length() - 1);
                minitel.moveCursorLeft(1);
                minitel.print(" ");
                minitel.moveCursorLeft(1);
            }
            while (placeholderVisible && placeholderRest > 0)
            {
                placeholderRest--;
                minitel.moveCursorLeft(1);
                minitel.print(" ");
                minitel.moveCursorLeft(1);
                if (placeholderRest == 0)
                    placeholderVisible = false;
            }
            cursorPos = 0;
        }
        else if (k == SOMMAIRE)
        {
            shell(true);
            return "";
        }
        else if (k >= 32 && k <= 126 && entree.length() < maxLen)
        {
            entree = entree.substring(0, cursorPos) + (char)k + entree.substring(cursorPos);
            cursorPos++;
            int totalLen = (placeholderVisible ? placeholderRest : 0) + entree.length();
            minitel.moveCursorLeft(cursorPos - 1);
            for (int i = 0; i < totalLen + 1; ++i)
            {
                minitel.print(" ");
            }
            minitel.moveCursorLeft(totalLen + 1);
            if (placeholderVisible && placeholderRest > 0)
                minitel.print(placeholder.substring(0, placeholderRest));
            String toShow = "";
            if (masque)
            {
                for (size_t i = 0; i < entree.length(); ++i)
                    toShow += '*';
            }
            else
            {
                toShow = entree;
            }
            minitel.print(toShow);
            minitel.moveCursorLeft(entree.length() - cursorPos);
        }
    }
    String result = "";
    if (placeholderLen > 0 && placeholderRest > 0)
        result += placeholder.substring(0, placeholderRest);
    result += entree;
    return result;
}

void passerEnVideotex()
{
    minitel.modeVideotex();
    minitel.pageMode();
    minitel.clearScreen();
    minitel.smallMode();
    minitel.cursor();
}
void passerEnMixte()
{
    minitel.modeMixte();
    minitel.smallMode();
    minitel.cursor();
}

// FONCTIONS WIFI
void connecterWiFi()
{
    minitel.println();
    minitel.println("Recherche des reseaux en cours...");

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);

    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        minitel.println("Aucun reseau trouve.");
        minitel.println("Verifiez que vous etes a portee d'un reseau WiFi.");
        attendreRetour(true);
        return;
    }

    minitel.println(String(n) + " reseau(x) trouve(s) :");
    minitel.println("-------------------");

    int indices[n];
    for (int i = 0; i < n; i++)
    {
        indices[i] = i;
    }

    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (WiFi.RSSI(indices[j]) < WiFi.RSSI(indices[j + 1]))
            {
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }
    for (int i = 0; i < n; ++i)
    {
        int idx = indices[i];
        String ssid = WiFi.SSID(idx);
        String signal = getSignalStrength(WiFi.RSSI(idx));
        String security = WiFi.encryptionType(idx) == WIFI_AUTH_OPEN ? "Ouvert" : "Securise";

        minitel.print(String(i + 1) + ". ");
        minitel.print(ssid);
        minitel.print(" [");
        minitel.print(signal);
        minitel.print("] - ");
        minitel.println(security);
    }

    String choixStr = saisirTexte("Choix (num) ou 'R' pour rafraichir : ", false, 64, "");

    if (choixStr == "R" || choixStr == "r")
    {
        WiFi.scanDelete();
        connecterWiFi();
        return;
    }

    if (choixStr == "")
        return;

    int choix = choixStr.toInt();
    if (choix > 0 && choix <= n)
    {
        connecterWiFi(WiFi.SSID(indices[choix - 1]));
    }
    else
    {
        minitel.println("Choix invalide.");
        delay(1000);
        connecterWiFi();
    }
}
void connecterWiFi(const String &ssid)
{
    minitel.println();
    minitel.println("Reseau : " + ssid);

    String pass = saisirTexte("Mot de passe: ", true, 64, "");
    if (pass == "")
    {
        minitel.println("Connexion annulee.");
        delay(1000);
        return;
    }

    minitel.println();
    minitel.println("Connexion en cours...");

    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long startTime = millis();
    int retryCount = 0;
    bool connected = false;

    while (!connected && retryCount < MAX_RETRIES)
    {
        while (millis() - startTime < CONNECTION_TIMEOUT)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                connected = true;
                break;
            }
        }

        if (!connected)
        {
            retryCount++;
            if (retryCount < MAX_RETRIES)
            {
                minitel.println();
                minitel.println("Tentative " + String(retryCount + 1) + "/" + String(MAX_RETRIES));
                WiFi.disconnect();
                WiFi.begin(ssid.c_str(), pass.c_str());
                startTime = millis();
            }
        }
    }

    if (connected)
    {
        minitel.println();
        minitel.println("=== Connexion Reussie ===");
        minitel.println();
        minitel.println("SSID : " + ssid);
        minitel.println("IP   : " + WiFi.localIP().toString());
        minitel.println("MAC  : " + WiFi.macAddress());
        minitel.println("Gateway : " + WiFi.gatewayIP().toString());
        minitel.println("Subnet : " + WiFi.subnetMask().toString());
        minitel.println("RSSI : " + String(WiFi.RSSI()) + " dBm");

        preferences.begin("MHC-OS", false);
        preferences.putString("ssid", ssid);
        preferences.putString("pass", pass);
        preferences.end();
    }
    else
    {
        minitel.println();
        minitel.println("echec de connexion apres " + String(MAX_RETRIES) + " tentatives.");
        minitel.println("Verifiez le mot de passe et la portee du reseau.");
    }

    attendreRetour(true);
}
String getSignalStrength(int rssi)
{
    if (rssi >= -50)
        return "Excellent";
    if (rssi >= -60)
        return "Tres bon";
    if (rssi >= -70)
        return "Bon";
    if (rssi >= -80)
        return "Moyen";
    return "Faible";
}