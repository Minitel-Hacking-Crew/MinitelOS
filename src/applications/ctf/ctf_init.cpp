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
// Chaîne : stagiaire → (cron) → user → (backup) → admin → (motd) → root
// ---------------------------------------------------------------------------

void ctf_fs_init() {

    // Arborescence
    ctf_mkdir("/root");
    ctf_mkdir("/home");
    ctf_mkdir("/tmp");
    ctf_mkdir("/home/stagiaire");
    ctf_mkdir("/home/user");
    ctf_mkdir("/home/admin");

    // ── Comptes utilisateurs ─────────────────────────────────────────────────
    // Passwords (MD5) :
    //   root      = T3lm1n1   → 06698cd46a5585ec7a10f99d74d675fd
    //   stagiaire = 1234      → 81dc9bdb52d04dc20036dbd8313ed055 (donné sur fiche)
    //   user      = linux     → e206a54e97690cce50cc872dd70ee896 (obtenu via C1)
    //   admin     = minitel   → bb3c3e98175d33c8300fbb0e84bf9e9f (cracké via C2)
    ctf_write("/root/.users",
        "root:06698cd46a5585ec7a10f99d74d675fd:root\n"
        "stagiaire:81dc9bdb52d04dc20036dbd8313ed055:user\n"
        "user:e206a54e97690cce50cc872dd70ee896:user\n"
        "admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin\n");

    // ── Flags ────────────────────────────────────────────────────────────────
    ctf_write("/root/flag1.txt",          "FLAG{cr0n_2_p1v0t}\n");      // C1 — lu via cron
    ctf_write("/home/admin/flag2.txt",    "FLAG{4dm1n_4cc3ss}\n");      // C2 — lu en admin
    ctf_write("/root/flag3.txt",          "FLAG{r00t_m0td_pwn3d}\n");   // C3 — lu via motd

    // ── Cron ────────────────────────────────────────────────────────────────
    ctf_write("/root/.crontab",
        "# Taches planifiees systeme - NE PAS MODIFIER\n"
        "# Format: intervalle_secondes commande\n"
        "30 run /tmp/maintenance.msh\n");

    // ── Config CTF ──────────────────────────────────────────────────────────
    ctf_write("/root/.ctf_config", "900\n");   // 15 minutes

    // ── Métadonnées de permissions ──────────────────────────────────────────
    ctf_write("/root/.fsmeta",
        "/tmp/maintenance.msh rwxrwxrwx root root\n"
        "/tmp/maintenance.log rw-rw-rw- root root\n"
        "/root/flag1.txt rw------- root root\n"
        "/root/flag3.txt rw------- root root\n"
        "/home/admin/flag2.txt rw-r----- admin admin\n");

    // ── Challenge 1 — Backdoor Cron (stagiaire → user) ──────────────────────
    // maintenance.msh world-writable, exécuté par root toutes les 30s
    // Injection attendue : cat /root/flag1.txt > /tmp/loot.txt
    //                      grep "^user:" /root/.users >> /tmp/loot.txt
    // → /tmp/loot.txt contiendra flag1 + hash MD5 de user (linux)
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
        "TODO perso :\n"
        "- demander a l'admin de desactiver le script de\n"
        "  maintenance apres l'incident de janvier\n"
        "  (droits mal configures sur /tmp/maintenance.msh)\n"
        "- voir pourquoi le cron tourne encore en root\n");

    // ── Challenge 2 — Backup Oublié (user → admin) ──────────────────────────
    // users.bak visible depuis /tmp (world-readable)
    // Contient user + admin — wordlist C1 permet de cracker user, wordlist C2 permet admin
    ctf_write("/tmp/users.bak",
        "user:e206a54e97690cce50cc872dd70ee896:user\n"
        "admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin\n");

    ctf_write("/home/user/TODO.txt",
        "- rappeler l'admin de supprimer le backup dans /tmp\n"
        "  (comptes exportes pour la migration, jamais effaces)\n"
        "- changer mon mot de passe (trop simple)\n");

    // flag2 dans /home/admin/ — lisible uniquement après su admin
    ctf_write("/home/admin/notes_perso.txt",
        "Memo perso admin - CONFIDENTIEL\n\n"
        "Compte : admin / minitel (a changer ASAP !)\n\n"
        "Note de service : le systeme de motd personnalise\n"
        "a ete active pour les comptes utilisateurs afin\n"
        "d'afficher des informations dynamiques au login.\n"
        "Voir README dans le repertoire home pour details.\n");

    // ── Challenge 3 — Editeur Piege (admin → root via motd) ─────────────────
    // admin lit README.txt → crée motd_perso.txt → motd l'exécute en root
    // Injection attendue : cat /root/flag3.txt > /tmp/pwned.txt
    ctf_write("/home/admin/README.txt",
        "Guide administrateur — MOTD personnalise\n\n"
        "Pour configurer votre message du jour personnel :\n"
        "  1. Creez motd_perso.txt dans votre repertoire home\n"
        "  2. Les commandes shell sont executees en tant que\n"
        "     SYSTEME pour afficher des informations dynamiques\n"
        "     (date, charge CPU, espace disque...)\n"
        "  3. Exemple : echo \"Bonjour\" > motd_perso.txt\n"
        "  4. Tapez 'motd' pour visualiser le resultat\n\n"
        "Note : cette fonctionnalite necessite des droits\n"
        "eleves pour acceder aux metriques systeme.\n");

    // ── Bannière CTF ────────────────────────────────────────────────────────
    ctf_write("/.motd",
        "MINITEL SECURITE INDUSTRIELLE v2.3\n"
        "Acces restreint - personnel autorise uniquement\n"
        "Tapez 'ctftime' pour voir le temps restant\n"
        "Objectif : obtenez les 3 flags et soumettez-les\n");

    // ── Nettoyage des artefacts des sessions précédentes ────────────────────
    ctf_remove("/tmp/loot.txt");
    ctf_remove("/tmp/pwned.txt");
    ctf_remove("/home/user/motd_perso.txt");
    ctf_remove("/home/admin/motd_perso.txt");
}

#endif // CTF_MODE
