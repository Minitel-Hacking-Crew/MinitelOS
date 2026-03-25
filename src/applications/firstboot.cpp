#include "firstboot.h"
#include "globals.h"
#include "utils/utils.h"
#include <mbedtls/md5.h>

// ------------------------------------------------------------------ helpers

static String fb_md5(const String &str) {
    unsigned char digest[16];
    mbedtls_md5((const unsigned char *)str.c_str(), str.length(), digest);
    String out = "";
    for (int i = 0; i < 16; i++) {
        if (digest[i] < 16) out += "0";
        out += String(digest[i], HEX);
    }
    return out;
}

static void fb_header(const String &step, const String &title) {
    minitel.newScreen();
    minitel.println("MinitelOS - Premier demarrage");
    minitel.println("------------------------------");
    minitel.println(step);
    minitel.println(title);
    minitel.println("------------------------------");
    minitel.println();
}

static String fb_ask_password(const String &label) {
    while (true) {
        String p1 = saisirTexte(label, true, 32, "");
        minitel.println();
        if (p1.isEmpty()) {
            minitel.println("Mot de passe obligatoire.");
            minitel.println();
            continue;
        }
        String p2 = saisirTexte("Confirmation      : ", true, 32, "");
        minitel.println();
        if (p1 == p2) return p1;
        minitel.println();
        minitel.println("Les mots de passe different,");
        minitel.println("recommencez.");
        minitel.println();
    }
}

// ------------------------------------------------------------------ WiFi (inline, sans attendreRetour)

static void fb_wifi_setup() {
    minitel.println("Recherche des reseaux...");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();

    if (n <= 0) {
        minitel.println("Aucun reseau WiFi detecte.");
        minitel.println("Configuration ignoree.");
        minitel.println("(configurable via : wifi)");
        delay(1500);
        return;
    }

    minitel.println(String(n) + " reseau(x) trouve(s) :");
    minitel.println();

    // Trier par RSSI
    int indices[n];
    for (int i = 0; i < n; i++) indices[i] = i;
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (WiFi.RSSI(indices[j]) < WiFi.RSSI(indices[j + 1])) {
                int t = indices[j]; indices[j] = indices[j + 1]; indices[j + 1] = t;
            }

    int shown = (n > 8) ? 8 : n;
    for (int i = 0; i < shown; i++) {
        int idx = indices[i];
        minitel.println(String(i + 1) + ". " + WiFi.SSID(idx) +
                        " [" + getSignalStrength(WiFi.RSSI(idx)) + "]");
    }
    minitel.println("0. Passer");
    minitel.println();

    String choixStr = saisirTexte("Choix : ", false, 2, "");
    int choix = choixStr.toInt();
    if (choix <= 0 || choix > shown) {
        minitel.println("WiFi ignore.");
        delay(1000);
        return;
    }

    String ssid = WiFi.SSID(indices[choix - 1]);
    minitel.println();
    minitel.println("Reseau : " + ssid);
    String pass = saisirTexte("Mot de passe : ", true, 64, "");

    minitel.println();
    minitel.println("Connexion en cours...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), pass.c_str());

    bool connected = false;
    for (int i = 0; i < 20 && !connected; i++) {
        delay(500);
        minitel.print(".");
        if (WiFi.status() == WL_CONNECTED) connected = true;
    }
    minitel.println();

    if (connected) {
        minitel.println("Connecte ! IP : " + WiFi.localIP().toString());
        preferences.begin("MHC-OS", false);
        preferences.putString("ssid", ssid);
        preferences.putString("pass", pass);
        preferences.end();
    } else {
        minitel.println("Echec de connexion.");
        minitel.println("Configurable via : wifi");
    }
    delay(1500);
}

// ------------------------------------------------------------------ public API

bool is_first_boot() {
    return !LittleFS.exists("/root/.users");
}

void run_first_boot_setup() {

    // --- Ecran d'accueil ---
    minitel.newScreen();
    minitel.println("==============================");
    minitel.println("   MinitelOS " + String(OSVersion));
    minitel.println("   Premier demarrage");
    minitel.println("==============================");
    minitel.println();
    minitel.println("Bienvenue !");
    minitel.println();
    minitel.println("Ce assistant va configurer");
    minitel.println("votre systeme en 4 etapes :");
    minitel.println();
    minitel.println("  1. Mot de passe root");
    minitel.println("  2. Creation utilisateur");
    minitel.println("  3. Connexion WiFi");
    minitel.println("  4. Message du jour");
    minitel.println();
    minitel.println("Appuyez sur une touche...");
    attendreTouche();

    // --- Etape 1 : root ---
    fb_header("Etape 1/4", "Compte administrateur");
    minitel.println("Definissez le mot de passe");
    minitel.println("du compte root.");
    minitel.println();
    String rootPass = fb_ask_password("Mot de passe root : ");

    // --- Etape 2 : premier utilisateur ---
    fb_header("Etape 2/4", "Creer un utilisateur");
    String userName = "";
    String userPass = "";
    while (true) {
        userName = saisirTexte("Nom d'utilisateur : ", false, 16, "");
        if (userName.isEmpty() || userName == "root") {
            minitel.println();
            minitel.println("Nom invalide, recommencez.");
            minitel.println();
            continue;
        }
        minitel.println();
        userPass = fb_ask_password("Mot de passe      : ");
        break;
    }
    minitel.println();
    minitel.println("Niveau d'acces :");
    minitel.println("  1. Utilisateur standard");
    minitel.println("  2. Administrateur");
    minitel.println();
    String lvl = saisirTexte("Choix (1/2) : ", false, 1, "1");
    String userLevel = (lvl == "2") ? "admin" : "user";

    // --- Etape 3 : WiFi ---
    fb_header("Etape 3/4", "Configuration WiFi");
    minitel.println("Voulez-vous configurer");
    minitel.println("le WiFi maintenant ?");
    minitel.println();
    minitel.println("  O. Oui, scanner");
    minitel.println("  N. Non, passer");
    minitel.println();
    String wc = saisirTexte("Choix (O/N) : ", false, 1, "N");
    if (wc == "O" || wc == "o") {
        minitel.println();
        fb_wifi_setup();
    }

    // --- Etape 4 : MOTD ---
    fb_header("Etape 4/4", "Message du jour (MOTD)");
    minitel.println("Affiche a chaque connexion.");
    minitel.println("Laissez vide pour le defaut.");
    minitel.println();
    String motd = saisirTexte("> ", false, 60, "");

    // --- Ecriture des fichiers ---
    if (!LittleFS.exists("/root")) LittleFS.mkdir("/root");
    if (!LittleFS.exists("/home")) LittleFS.mkdir("/home");
    String userHome = "/home/" + userName;
    if (!LittleFS.exists(userHome)) LittleFS.mkdir(userHome);

    File usersFile = LittleFS.open("/root/.users", "w");
    if (usersFile) {
        usersFile.println("root:" + fb_md5(rootPass) + ":root");
        usersFile.println(userName + ":" + fb_md5(userPass) + ":" + userLevel);
        usersFile.close();
    }

    File motdFile = LittleFS.open("/.motd", "w");
    if (motdFile) {
        motdFile.println(motd.isEmpty() ? "Bienvenue sur MinitelOS !" : motd);
        motdFile.close();
    }

    // --- Recapitulatif ---
    minitel.newScreen();
    minitel.println("=== Configuration terminee ===");
    minitel.println();
    minitel.println("Comptes crees :");
    minitel.println("  root          [root]");
    minitel.println("  " + userName + " [" + userLevel + "]");
    minitel.println();
    if (WiFi.status() == WL_CONNECTED)
        minitel.println("WiFi : connecte");
    else
        minitel.println("WiFi : non configure");
    minitel.println();
    minitel.println("Systeme pret !");
    minitel.println();
    minitel.println("Appuyez sur une touche...");
    attendreTouche();
    minitel.newScreen();
}
