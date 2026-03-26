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

// ---------------------------------------------------------------------------
// CTF filesystem init
//
// Chaîne stricte (chaque pivot n'est accessible qu'en ayant fait le précédent) :
//
//   stagiaire/1234 ──[C1 cron]──► user/linux ──[C2 backup]──► admin/minitel ──[C3 motd]──► FLAG3
//
//   C1 : inject maintenance.msh → cron (root) écrit flag1 + hash user dans /tmp/loot.txt
//         → /root/.ctf_pivot1 (root-only) contient le hash user
//         → stagiaire ne peut PAS le lire directement
//
//   C2 : /home/user/backup.bak (user-only) contient le hash admin
//         → stagiaire ne peut PAS lire /home/user/
//
//   C3 : /home/admin/motd_perso.txt exécuté en root via motd
//         → admin ne connaît pas le mot de passe root
// ---------------------------------------------------------------------------

void ctf_fs_init() {

    // Arborescence
    ctf_mkdir("/root");
    ctf_mkdir("/home");
    ctf_mkdir("/tmp");
    ctf_mkdir("/scripts");
    ctf_mkdir("/home/stagiaire");
    ctf_mkdir("/home/user");
    ctf_mkdir("/home/admin");

    // ── Comptes utilisateurs ─────────────────────────────────────────────────
    // Passwords (MD5) :
    //   root      = T3lm1n1  → 06698cd46a5585ec7a10f99d74d675fd
    //   stagiaire = 1234     → 81dc9bdb52d04dc20036dbd8313ed055 (sur fiche)
    //   user      = linux    → e206a54e97690cce50cc872dd70ee896 (C1 → /tmp/loot.txt)
    //   admin     = minitel  → bb3c3e98175d33c8300fbb0e84bf9e9f (C2 → /home/user/backup.bak)
    ctf_write("/root/.users",
        "root:06698cd46a5585ec7a10f99d74d675fd:root\n"
        "stagiaire:81dc9bdb52d04dc20036dbd8313ed055:user\n"
        "user:e206a54e97690cce50cc872dd70ee896:user\n"
        "admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin\n");

    // ── Pivot C1 → C2 : hash user (root-only, extrait via cron) ─────────────
    // La cron injection attendue :
    //   cat /root/flag1.txt > /tmp/loot.txt
    //   cat /root/.ctf_pivot1 >> /tmp/loot.txt
    // → /tmp/loot.txt contiendra flag1 puis "user:<hash>:user"
    ctf_write("/root/.ctf_pivot1",
        "user:e206a54e97690cce50cc872dd70ee896:user\n");

    // ── Flags ────────────────────────────────────────────────────────────────
    ctf_write("/root/flag1.txt",       "FLAG{cr0n_2_p1v0t}\n");
    ctf_write("/home/admin/flag2.txt", "FLAG{4dm1n_4cc3ss}\n");
    ctf_write("/root/flag3.txt",       "FLAG{r00t_m0td_pwn3d}\n");

    // ── Cron ─────────────────────────────────────────────────────────────────
    ctf_write("/root/.crontab",
        "# Taches planifiees systeme - NE PAS MODIFIER\n"
        "# Format: intervalle_secondes commande\n"
        "30 run /scripts/maintenance.msh\n");

    // ── Config CTF ───────────────────────────────────────────────────────────
    ctf_write("/root/.ctf_config", "900\n");   // 15 minutes

    // ── Métadonnées de permissions ────────────────────────────────────────────
    ctf_write("/root/.fsmeta",
        "/scripts/maintenance.msh rwxrwxrwx root root\n"
        "/scripts/maintenance.log rw-rw-rw- root root\n"
        "/root/flag1.txt         rw------- root root\n"
        "/root/flag3.txt         rw------- root root\n"
        "/root/.ctf_pivot1       rw------- root root\n"
        "/home/admin/flag2.txt   rw-r----- admin admin\n"
        "/home/user/backup.bak   rw-r----- user  user\n");

    // ── Challenge 1 — Backdoor Cron (stagiaire → user) ───────────────────────
    // /scripts/maintenance.msh world-writable, exécuté par root toutes les 30s
    // Injection attendue (2 lignes via edit) :
    //   cat /root/flag1.txt > /tmp/loot.txt
    //   cat /root/.ctf_pivot1 >> /tmp/loot.txt
    ctf_write("/scripts/maintenance.msh",
        "# Script de maintenance automatique v1.2\n"
        "# ATTENTION: ce script tourne en tache de fond toutes les 30s\n"
        "echo [MAINT] Debut verification systeme...\n"
        "date >> /scripts/maintenance.log\n"
        "echo [MAINT] Verification terminee.\n");

    ctf_write("/scripts/maintenance.log",
        "2026-01-14 09:12:03\n"
        "2026-01-14 09:12:33\n"
        "2026-01-14 09:13:03\n"
        "2026-01-14 09:13:33\n");

    ctf_write("/home/stagiaire/note.txt",
        "TODO perso :\n"
        "- demander a l'admin de corriger les droits sur\n"
        "  /scripts/maintenance.msh (incident de janvier)\n"
        "- voir pourquoi le cron tourne encore en root\n");

    // ── Challenge 2 — Backup Oublié (user → admin) ───────────────────────────
    // /home/user/backup.bak readable uniquement en tant que user
    // (stagiaire bloqué par l'access control /home/<other>/)
    ctf_write("/home/user/backup.bak",
        "admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin\n");

    ctf_write("/home/user/TODO.txt",
        "- effacer backup.bak (migration terminee depuis longtemps)\n"
        "- changer mon mot de passe\n");

    ctf_write("/home/admin/notes_perso.txt",
        "Memo perso admin - CONFIDENTIEL\n\n"
        "Compte : admin / minitel (a changer ASAP !)\n\n"
        "Note : le systeme de motd personnalise a ete active\n"
        "pour afficher des informations dynamiques au login.\n"
        "Voir README.txt pour les details d'utilisation.\n");

    // ── Challenge 3 — Editeur Piege (admin → root via motd) ──────────────────
    // admin crée motd_perso.txt → motd l'exécute en root (bug SUID-like)
    // Injection attendue : cat /root/flag3.txt > /tmp/pwned.txt
    ctf_write("/home/admin/README.txt",
        "Guide administrateur — MOTD personnalise\n\n"
        "Pour configurer votre message du jour personnel :\n"
        "  1. Creez motd_perso.txt dans votre repertoire home\n"
        "  2. Les commandes shell sont executees en tant que\n"
        "     SYSTEME pour afficher des informations dynamiques\n"
        "     (date, charge CPU, espace disque...)\n"
        "  3. Exemple : echo Bonjour > motd_perso.txt\n"
        "  4. Tapez 'motd' pour visualiser le resultat\n\n"
        "Note : cette fonctionnalite necessite des droits\n"
        "eleves pour acceder aux metriques systeme.\n");

    // ── Bannière CTF ─────────────────────────────────────────────────────────
    ctf_write("/.motd",
        "MINITEL SECURITE INDUSTRIELLE v2.3\n"
        "Acces restreint - personnel autorise uniquement\n"
        "Tapez 'ctftime' pour voir le temps restant\n"
        "Objectif : obtenez les 3 flags et soumettez-les\n");

    // ── Nettoyage des artefacts des sessions précédentes ─────────────────────
    ctf_remove("/tmp/loot.txt");
    ctf_remove("/tmp/pwned.txt");
    ctf_remove("/home/user/motd_perso.txt");
    ctf_remove("/home/admin/motd_perso.txt");
}

#endif // CTF_MODE
