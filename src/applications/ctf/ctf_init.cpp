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
// ---------------------------------------------------------------------------

void ctf_fs_init() {

    // Arborescence
    ctf_mkdir("/root");
    ctf_mkdir("/home");
    ctf_mkdir("/tmp");
    ctf_mkdir("/home/stagiaire");
    ctf_mkdir("/home/user");
    ctf_mkdir("/home/admin");

    // ── Comptes utilisateurs ────────────────────────────────────────────────
    // Passwords (MD5) : root=T3lm1n1, stagiaire/user=1234, admin=minitel
    ctf_write("/root/.users",
        "root:06698cd46a5585ec7a10f99d74d675fd:root\n"
        "stagiaire:81dc9bdb52d04dc20036dbd8313ed055:user\n"
        "user:81dc9bdb52d04dc20036dbd8313ed055:user\n"
        "admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin\n");

    // ── Flags ────────────────────────────────────────────────────────────────
    ctf_write("/root/flag.txt",   "FLAG{cr0n_b4ckd00r_w4s_h3r3}\n");
    ctf_write("/root/flag_a.txt", "FLAG{b4ckup_3xp0s3d_4dm1n}\n");
    ctf_write("/root/flag_d.txt", "FLAG{m0td_c0d3_3x3c_ft_w}\n");

    // ── Cron ────────────────────────────────────────────────────────────────
    ctf_write("/root/.crontab",
        "# Taches planifiees systeme - NE PAS MODIFIER\n"
        "# Format: intervalle_secondes commande\n"
        "30 run /tmp/maintenance.msh\n");

    // ── Config CTF ──────────────────────────────────────────────────────────
    ctf_write("/root/.ctf_config", "720\n");   // durée en secondes (12 min)

    // ── Métadonnées de permissions ──────────────────────────────────────────
    ctf_write("/root/.fsmeta",
        "/tmp/maintenance.msh rwxrwxrwx root root\n"
        "/tmp/maintenance.log rw-rw-rw- root root\n"
        "/root/flag.txt rw------- root root\n");

    // ── Challenge 1 — Backdoor Cron ─────────────────────────────────────────
    ctf_write("/tmp/maintenance.msh",
        "# Script de maintenance automatique v1.2\n"
        "# Auteur: admin@minitel-securite.fr\n"
        "# ATTENTION: ce script tourne en tache de fond toutes les 30s\n"
        "echo [MAINT] Debut verification systeme...\n"
        "date >> /tmp/maintenance.log\n"
        "echo [MAINT] Verification terminee.\n");

    ctf_write("/tmp/maintenance.log",
        "2026-01-14 09:12:03\n"
        "2026-01-14 09:12:33\n"
        "2026-01-14 09:13:03\n"
        "2026-01-14 09:13:33\n");

    ctf_write("/home/stagiaire/note.txt",
        "TODO perso:\n"
        "- demander a l'admin de desactiver le script de maintenance\n"
        "  apres l'incident de janvier (droits mal configures apparemment)\n"
        "- voir pourquoi le cron tourne encore\n");

    // ── Challenge 2 — Backup Oublié ─────────────────────────────────────────
    ctf_write("/tmp/users.bak",
        "root:06698cd46a5585ec7a10f99d74d675fd:root\n"
        "admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin\n"
        "user:81dc9bdb52d04dc20036dbd8313ed055:user\n");

    ctf_write("/home/user/TODO.txt",
        "- rappeler l'admin pour supprimer l'ancien backup dans /tmp\n"
        "  (il a exporte les comptes la semaine derniere pour la migration)\n"
        "- changer mon mot de passe\n");

    ctf_write("/home/admin/notes_perso.txt",
        "Memo perso - CONFIDENTIEL\n\n"
        "Mots de passe systeme (a changer ASAP) :\n"
        "  root : T3lm1n1\n"
        "  admin : minitel\n"
        "  backup FTP : backup42\n\n"
        "Contact urgence : 06 XX XX XX XX\n");

    // ── Challenge 3 — Éditeur Piégé ─────────────────────────────────────────
    ctf_write("/home/user/README.txt",
        "Aide memoire utilisateur :\n\n"
        "Pour personnaliser votre message du jour :\n"
        "  1. Creez le fichier motd_perso.txt dans votre repertoire\n"
        "  2. Ajoutez vos lignes de texte\n"
        "  3. Les commandes shell (echo, date, uptime...) sont\n"
        "     executees automatiquement pour l'affichage dynamique\n"
        "  4. Tapez 'motd' pour voir le resultat\n");

    // ── Bannière CTF ────────────────────────────────────────────────────────
    ctf_write("/.motd",
        "MINITEL SECURITE INDUSTRIELLE v2.3\n"
        "Acces restreint - personnel autorise uniquement\n"
        "Tapez 'ctftime' pour voir le temps restant\n"
        "Objectif : trouvez le flag et soumettez-le sur la plateforme\n");

    // ── Nettoyage des artefacts des sessions précédentes ────────────────────
    ctf_remove("/tmp/loot.txt");
    ctf_remove("/tmp/d_flag.txt");
    ctf_remove("/home/user/motd_perso.txt");
}

#endif // CTF_MODE
