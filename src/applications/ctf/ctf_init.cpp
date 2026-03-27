#ifdef CTF_MODE

#include <FS.h>
#include <LittleFS.h>
#include <Arduino.h>
#include "ctf_init.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void ctf_mkdir(const char *path) {
    if (!LittleFS.exists(path))
        LittleFS.mkdir(path);
}

static void ctf_write(const char *path, const char *content) {
    File f = LittleFS.open(path, "w");
    if (f) { f.print(content); f.close(); }
}

static void ctf_remove(const char *path) {
    if (LittleFS.exists(path))
        LittleFS.remove(path);
}

// Supprime tous les fichiers contenus dans un répertoire (non récursif).
// Utilisé pour le reset propre entre équipes — quel que soit le nom des artefacts.
static void ctf_clear_dir(const char *dirpath) {
    File dir = LittleFS.open(dirpath);
    if (!dir || !dir.isDirectory()) return;
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String fullpath = String(dirpath) + "/" + entry.name();
            entry.close();
            LittleFS.remove(fullpath);
        } else {
            entry.close();
        }
        entry = dir.openNextFile();
    }
    dir.close();
}

// ---------------------------------------------------------------------------
// CTF filesystem init
//
// Chaîne :  stagiaire/1234 ──[C1 hash]──► admin/4815162342 ──[C2 flag]──[C3 motd]──► FLAG root
//
//   C1 (pas de flag) :
//     cat /etc/shadow → hash admin crackable (4815162342) → su admin
//     /etc/shadow est world-readable (misconfiguration volontaire)
//     → hash root incrackable (mot de passe aléatoire)
//
//   C2 : cat ~/user.txt  → base64 flag 1 (décoder hors appli)
//
//   C3 : edit motd_perso.txt → motd exécute en root → base64 flag 2 (décoder hors appli)
// ---------------------------------------------------------------------------

void ctf_fs_init() {

    // Arborescence
    ctf_mkdir("/root");
    ctf_mkdir("/home");
    ctf_mkdir("/tmp");
    ctf_mkdir("/etc");
    ctf_mkdir("/scripts");
    ctf_mkdir("/home/stagiaire");
    ctf_mkdir("/home/admin");

    // ── Reset complet entre équipes ───────────────────────────────────────────
    // Purge tous les fichiers des répertoires accessibles en écriture,
    // sans cibler de noms spécifiques (les joueurs peuvent créer n'importe quoi).
    // Les fichiers CTF sont réécrits ensuite par ctf_write.
    ctf_clear_dir("/tmp");
    ctf_clear_dir("/home/stagiaire");
    ctf_clear_dir("/home/admin");

    // ── Comptes utilisateurs — /etc/shadow est le fichier d'auth système ─────
    // Passwords (MD5) :
    //   root      = V1Oz5Re8G41EVmqWXl76 → 2260a49226afcd3bb784cb3e3888ea91)
    //   stagiaire = 1234                 → 81dc9bdb52d04dc20036dbd8313ed055 (donné sur postit caché sous le clavier)
    //   admin     = 4815162342           → f7b16af5588f9654862e4aefcec8b0de (rockyou)

    ctf_write("/etc/shadow",
        "root:2260a49226afcd3bb784cb3e3888ea91:root\n"
        "stagiaire:81dc9bdb52d04dc20036dbd8313ed055:user\n"
        "admin:f7b16af5588f9654862e4aefcec8b0de:admin\n");

    // ── Flags (base64 — décoder hors application) ────────────────────────────
    ctf_write("/home/admin/user.txt", "bWluaXRlbCgwbGQzNTdfN3IxY2tfMW5fN2gzX2IwMGsp\n");
    ctf_write("/root/root.txt",       "bWluaXRlbChnMDdfcjAwN18wbl8wbGRfbTFuMTczbCk=\n");

    // ── Cron — tourne en root ────────────────────────────────────
    ctf_write("/root/.crontab",
        "# Format: intervalle_secondes commande\n"
        "30 run /scripts/maintenance.msh\n");

    // ── Config CTF ───────────────────────────────────────────────────────────
    ctf_write("/root/.ctf_config", "900\n");

    // ── Métadonnées de permissions ────────────────────────────────────────────
    ctf_write("/root/.fsmeta",
        "/etc/shadow rw-r--r-- root root\n"
        "/root/root.txt rw------- root root\n"
        "/home/admin/user.txt rw-r----- admin admin\n"
        "/home/admin/notes_perso.txt rw-r--r-- admin admin\n"
        "/home/admin/README.txt rw-r--r-- admin admin\n"
        "/scripts/maintenance.msh rwxrwxrwx root root\n"
        "/scripts/maintenance.log rw-rw-rw- root root\n");

    // ── Script de maintenance (tâche cron de fond) ────────────────────────────
    ctf_write("/scripts/maintenance.msh",
        "# Script de check uptime automatique\n"
        "date >> /scripts/maintenance.log\n");

    ctf_write("/scripts/maintenance.log",
        "2026-01-14 09:12:03\n"
        "2026-01-14 09:12:33\n"
        "2026-01-14 09:13:03\n"
        "2026-01-14 09:13:33\n");

    // ── Fichiers admin ────────────────────────────────────────────────────────
    ctf_write("/home/admin/notes_perso.txt",
        "Memo perso admin - CONFIDENTIEL\n\n"
        "Rien de particulier a signaler.\n");

    ctf_write("/home/admin/README.txt",
        "Notes admin - 2026-01-14\n\n"
        "Maintenance systeme effectuee.\n");

    // ── Bannière ──────────────────────────────────────────────────────────────
    ctf_write("/.motd",
        "Acces restreint - personnel autorise uniquement\n"
        "Derniere connexion : 2026-01-14 09:08:41\n");

}

#endif // CTF_MODE
