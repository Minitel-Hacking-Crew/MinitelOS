#include "shell_extra.h"
#include "applications/shell_apps/shell_edit.h"
#include "applications/cron/cron.h"
#ifdef CTF_MODE
#include "applications/ctf/ctf_init.h"
#endif
#include <ctime>
#include <mbedtls/md5.h>

extern void shell_println_wrapped(const String &text);
extern String shell_abspath(const String &path);
extern String shell_current_dir;
extern void shell_eval_line(const String &line);
extern std::map<String, String> shell_vars;
extern std::map<String, int> shell_int_vars;
extern std::map<String, float> shell_float_vars;
extern std::map<String, String> shell_string_vars;
extern std::map<String, bool> shell_bool_vars;
extern TaskHandle_t cronTaskHandle;
extern void shell_logout();

// ─── helpers metadata ────────────────────────────────────────────────────────
// Format /root/.fsmeta : une entrée par ligne  "<path> <perms> <owner> <group>"

static const char *META_FILE = "/root/.fsmeta";

static FileMeta read_meta(const String &path) {
    FileMeta m{"rw-r--r--", "root", "root"};
    if (!LittleFS.exists(META_FILE)) return m;
    File f = LittleFS.open(META_FILE, "r");
    if (!f) return m;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        int s1 = line.indexOf(' ');
        if (s1 < 0) continue;
        if (line.substring(0, s1) != path) continue;
        int s2 = line.indexOf(' ', s1 + 1);
        int s3 = line.indexOf(' ', s2 + 1);
        if (s2 < 0 || s3 < 0) continue;
        m.perms = line.substring(s1 + 1, s2);
        m.owner = line.substring(s2 + 1, s3);
        m.group = line.substring(s3 + 1);
        break;
    }
    f.close();
    return m;
}

FileMeta get_file_meta(const String &path) { return read_meta(path); }

static void write_meta(const String &path, const String &perms,
                        const String &owner, const String &group) {
    // Rewrite file, updating or inserting the entry
    String newContent = path + " " + perms + " " + owner + " " + group + "\n";
    if (LittleFS.exists(META_FILE)) {
        File f = LittleFS.open(META_FILE, "r");
        String rebuilt = "";
        bool found = false;
        if (f) {
            while (f.available()) {
                String line = f.readStringUntil('\n');
                line.trim();
                int s1 = line.indexOf(' ');
                if (s1 > 0 && line.substring(0, s1) == path) {
                    rebuilt += newContent;
                    found = true;
                } else if (line.length() > 0) {
                    rebuilt += line + "\n";
                }
            }
            f.close();
        }
        if (!found) rebuilt += newContent;
        File out = LittleFS.open(META_FILE, "w");
        if (out) { out.print(rebuilt); out.close(); }
    } else {
        File out = LittleFS.open(META_FILE, "w");
        if (out) { out.print(newContent); out.close(); }
    }
}

void set_file_meta(const String &path, const String &perms,
                   const String &owner, const String &group) {
    write_meta(path, perms, owner, group);
}

// Vérifie si la session appartient au groupe du fichier.
// Hiérarchie : "user" = tout le monde, "admin" = admin+root, "root" = root seul.
// Un nom d'utilisateur exact correspond à ce user uniquement.
// Les groupes personnalisés sont stockés dans /root/.groups : "nom:user1,user2,..."
static bool user_in_group(const String &group) {
    if (group == sessionUsername)    return true;
    if (group == "user")             return true;
    if (group == "admin")            return sessionAccessLevel == "admin" ||
                                            sessionAccessLevel == "root";
    if (group == "root")             return sessionAccessLevel == "root";
    // Groupes personnalisés
    static const char *GROUPS_FILE = "/root/.groups";
    if (LittleFS.exists(GROUPS_FILE)) {
        File f = LittleFS.open(GROUPS_FILE, "r");
        if (f) {
            while (f.available()) {
                String line = f.readStringUntil('\n');
                line.trim();
                int colon = line.indexOf(':');
                if (colon < 0) continue;
                if (line.substring(0, colon) != group) continue;
                String members = line.substring(colon + 1);
                // members = "user1,user2,..."
                int pos = 0;
                while (pos <= (int)members.length()) {
                    int comma = members.indexOf(',', pos);
                    String member = (comma < 0)
                        ? members.substring(pos)
                        : members.substring(pos, comma);
                    member.trim();
                    if (member == sessionUsername) { f.close(); return true; }
                    if (comma < 0) break;
                    pos = comma + 1;
                }
                break;
            }
            f.close();
        }
    }
    return false;
}

bool fs_can_access(const String &path, char perm) {
    if (sessionAccessLevel == "root") return true;
    FileMeta m = get_file_meta(path);
    int offset = (perm == 'r') ? 0 : (perm == 'w') ? 1 : 2;

    if (m.owner == sessionUsername) {
        // Bits propriétaire (0-2)
        char bit = ((int)m.perms.length() > offset) ? m.perms[offset] : '-';
        return bit == perm;
    }
    if (user_in_group(m.group)) {
        // Bits groupe (3-5)
        char bit = ((int)m.perms.length() > 3 + offset) ? m.perms[3 + offset] : '-';
        return bit == perm;
    }
    // Bits autres (6-8)
    char bit = ((int)m.perms.length() > 6 + offset) ? m.perms[6 + offset] : '-';
    return bit == perm;
}

static String numeric_to_perms(const String &modeStr) {
    // Parse en octal : "644" → 0644 → rw-r--r--
    int mode = (int)strtol(modeStr.c_str(), nullptr, 8);
    String out = "";
    for (int shift = 6; shift >= 0; shift -= 3) {
        int bits = (mode >> shift) & 7;
        out += (bits & 4) ? 'r' : '-';
        out += (bits & 2) ? 'w' : '-';
        out += (bits & 1) ? 'x' : '-';
    }
    return out;
}

// ─── touch ───────────────────────────────────────────────────────────────────
void shell_touch(const String &args) {
    if (args.isEmpty()) { shell_println_wrapped("Usage: touch <fichier>"); return; }
    String path = shell_abspath(args);
    bool isNew = !LittleFS.exists(path);
    if (isNew) {
        File f = LittleFS.open(path, "w");
        if (!f) { shell_println_wrapped("Erreur creation : " + path); return; }
        f.close();
    }
    // Nouveau fichier : owner = utilisateur courant ; existant : on conserve l'owner
    FileMeta m = read_meta(path);
    String owner = isNew ? sessionUsername : m.owner;
    String group = isNew ? sessionUsername : m.group;
    write_meta(path, m.perms, owner, group);
}

// ─── env ─────────────────────────────────────────────────────────────────────
void shell_env(const String &) {
    // Variables système implicites
    shell_println_wrapped("USER=" + sessionUsername);
    shell_println_wrapped("HOME=" + String(sessionAccessLevel == "root" ? "/root" : "/home/" + sessionUsername));
    shell_println_wrapped("LEVEL=" + sessionAccessLevel);
    shell_println_wrapped("PWD=" + shell_current_dir);
    shell_println_wrapped("OS=MinitelOS");
    // Variables utilisateur
    for (auto &kv : shell_string_vars)
        shell_println_wrapped(kv.first + "=" + kv.second);
    for (auto &kv : shell_int_vars)
        shell_println_wrapped(kv.first + "=" + String(kv.second));
    for (auto &kv : shell_float_vars) {
        char buf[16]; dtostrf(kv.second, 0, 2, buf);
        shell_println_wrapped(kv.first + "=" + String(buf));
    }
    for (auto &kv : shell_bool_vars)
        shell_println_wrapped(kv.first + "=" + String(kv.second ? "true" : "false"));
}

// ─── export ──────────────────────────────────────────────────────────────────
void shell_export(const String &args) {
    if (args.isEmpty()) { shell_env(""); return; }
    int eq = args.indexOf('=');
    if (eq == -1) {
        // Affiche la variable
        String var = args;
        var.trim();
        if (shell_string_vars.count(var))
            shell_println_wrapped(var + "=" + shell_string_vars[var]);
        else if (shell_int_vars.count(var))
            shell_println_wrapped(var + "=" + String(shell_int_vars[var]));
        else
            shell_println_wrapped(var + ": non definie");
        return;
    }
    String var = args.substring(0, eq);
    String val = args.substring(eq + 1);
    var.trim(); val.trim();
    if (val.startsWith("\"") && val.endsWith("\""))
        val = val.substring(1, val.length() - 1);
    shell_string_vars[var] = val;
}

// ─── date ────────────────────────────────────────────────────────────────────
void shell_date(const String &) {
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    shell_println_wrapped(String(buf));
}

// ─── uptime ──────────────────────────────────────────────────────────────────
void shell_uptime(const String &) {
    unsigned long ms = millis();
    unsigned long s  = ms / 1000;
    unsigned long m  = s / 60;
    unsigned long h  = m / 60;
    unsigned long d  = h / 24;
    s %= 60; m %= 60; h %= 24;
    String out = "up ";
    if (d > 0) out += String(d) + "j ";
    char buf[32];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, s);
    out += String(buf);
    out += "  user: " + sessionUsername;
    shell_println_wrapped(out);
}

// ─── free ────────────────────────────────────────────────────────────────────
void shell_free_cmd(const String &) {
    uint32_t total = ESP.getHeapSize();
    uint32_t free_ = ESP.getFreeHeap();
    uint32_t used  = total - free_;
    uint32_t minFr = ESP.getMinFreeHeap();
    shell_println_wrapped("Memoire heap (octets) :");
    shell_println_wrapped("  Total : " + String(total));
    shell_println_wrapped("  Libre : " + String(free_));
    shell_println_wrapped("  Utilise: " + String(used));
    shell_println_wrapped("  Min libre: " + String(minFr));
}

// ─── ps ──────────────────────────────────────────────────────────────────────
void shell_ps(const String &) {
    shell_println_wrapped("PID  USER   STAT COMMAND");
    shell_println_wrapped("1    " + sessionUsername + "  S    shell");
    if (cronTaskHandle != NULL) {
        shell_println_wrapped("2    root   S    cron (" +
                              String(cronTasks.size()) + " taches)");
        for (size_t i = 0; i < cronTasks.size(); i++) {
            String interval = String(cronTasks[i].intervalMs / 1000) + "s";
            shell_println_wrapped("  [" + String(i + 1) + "] " +
                                  interval + " " + cronTasks[i].command);
        }
    }
}

// ─── kill ────────────────────────────────────────────────────────────────────
void shell_kill_cmd(const String &args) {
    if (args.isEmpty()) {
        shell_println_wrapped("Usage: kill <index>");
        shell_println_wrapped("Voir 'ps' pour les indices.");
        return;
    }
    int idx = args.toInt() - 1;
    if (idx < 0 || idx >= (int)cronTasks.size()) {
        shell_println_wrapped("Index invalide.");
        return;
    }
    String cmd = cronTasks[idx].command;
    cronTasks.erase(cronTasks.begin() + idx);
    shell_println_wrapped("Tache supprimee : " + cmd);
}

// ─── crontab ─────────────────────────────────────────────────────────────────
void shell_crontab_cmd(const String &args) {
    if (sessionAccessLevel != "root" && sessionAccessLevel != "admin") {
        shell_println_wrapped("Permission refusee.");
        return;
    }
    shell_edit("/root/.crontab");
    // Recharge le crontab après édition
    load_crontab();
}

// ─── chmod ───────────────────────────────────────────────────────────────────
void shell_chmod(const String &args) {
    // Usage: chmod <mode> <path>   (mode: 644 ou rw-r--r--)
    int sp = args.indexOf(' ');
    if (sp < 0) { shell_println_wrapped("Usage: chmod <mode> <fichier>"); return; }
    String mode = args.substring(0, sp);
    String path = shell_abspath(args.substring(sp + 1));
    if (!LittleFS.exists(path)) {
        shell_println_wrapped("Introuvable : " + path);
        return;
    }
    String perms;
    // Mode numérique (ex: 644, 755)
    bool isNumeric = true;
    for (int i = 0; i < mode.length(); i++)
        if (!isdigit((unsigned char)mode[i])) { isNumeric = false; break; }
    if (isNumeric)
        perms = numeric_to_perms(mode);
    else
        perms = mode;  // accepte directement "rw-r--r--"

    FileMeta m = read_meta(path);
    if (sessionAccessLevel != "root" && m.owner != sessionUsername) {
        shell_println_wrapped("Acces refuse : vous n'etes pas proprietaire");
        return;
    }
    write_meta(path, perms, m.owner, m.group);
    shell_println_wrapped(path + " -> " + perms);
}

// ─── du ──────────────────────────────────────────────────────────────────────
static size_t du_recursive(const String &path) {
    File f = LittleFS.open(path);
    if (!f) return 0;
    if (!f.isDirectory()) {
        size_t sz = f.size();
        f.close();
        return sz;
    }
    size_t total = 0;
    File child = f.openNextFile();
    while (child) {
        total += du_recursive(String(child.name()));
        child.close();
        child = f.openNextFile();
    }
    f.close();
    return total;
}

void shell_du(const String &args) {
    String path = args.isEmpty() ? shell_current_dir : shell_abspath(args);
    if (!LittleFS.exists(path)) {
        shell_println_wrapped("Introuvable : " + path); return;
    }
    size_t sz = du_recursive(path);
    char buf[48];
    if (sz < 1024)
        snprintf(buf, sizeof(buf), "%u o\t%s", (unsigned)sz, path.c_str());
    else
        snprintf(buf, sizeof(buf), "%u Ko\t%s", (unsigned)(sz / 1024), path.c_str());
    shell_println_wrapped(String(buf));
}

// ─── nslookup ────────────────────────────────────────────────────────────────
void shell_nslookup(const String &args) {
    if (args.isEmpty()) { shell_println_wrapped("Usage: nslookup <host>"); return; }
    if (WiFi.status() != WL_CONNECTED) {
        shell_println_wrapped("Erreur : pas de connexion WiFi"); return;
    }
    String host = args; host.trim();
    shell_println_wrapped("Resolution de " + host + " ...");
    IPAddress ip;
    if (WiFi.hostByName(host.c_str(), ip)) {
        shell_println_wrapped(host + " -> " + ip.toString());
    } else {
        shell_println_wrapped("Echec : hote introuvable.");
    }
}

// ─── sleep ───────────────────────────────────────────────────────────────────
void shell_sleep(const String &args) {
    if (args.isEmpty()) { shell_println_wrapped("Usage: sleep <secondes>"); return; }
    float secs = args.toFloat();
    if (secs > 0) delay((unsigned long)(secs * 1000.0f));
}

// ─── sudo ────────────────────────────────────────────────────────────────────
// Vérifie si l'utilisateur courant peut exécuter cmd via /etc/sudoers.
// Format : "user ALL" | "user cmd1,cmd2" | "%group ALL" | "%group cmd1,cmd2"
static bool sudoers_allows(const String &cmd) {
    static const char *SUDOERS_FILE = "/etc/sudoers";
    if (!LittleFS.exists(SUDOERS_FILE)) return false;
    File f = LittleFS.open(SUDOERS_FILE, "r");
    if (!f) return false;
    // Extrait le nom de la commande (premier token)
    String cmdName = cmd;
    int sp = cmd.indexOf(' ');
    if (sp > 0) cmdName = cmd.substring(0, sp);
    cmdName.trim();
    bool allowed = false;
    while (f.available() && !allowed) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;
        // Sépare sujet et commandes
        int sep = line.indexOf(' ');
        if (sep < 0) continue;
        String subject = line.substring(0, sep);
        String cmds    = line.substring(sep + 1);
        cmds.trim();
        // Vérifie si le sujet correspond
        bool match = false;
        if (subject.startsWith("%")) {
            // Groupe
            match = user_in_group(subject.substring(1));
        } else {
            match = (subject == sessionUsername);
        }
        if (!match) continue;
        // Vérifie si la commande est autorisée
        if (cmds == "ALL") { allowed = true; break; }
        int pos = 0;
        while (pos <= (int)cmds.length()) {
            int comma = cmds.indexOf(',', pos);
            String entry = (comma < 0) ? cmds.substring(pos) : cmds.substring(pos, comma);
            entry.trim();
            if (entry == cmdName || entry == "ALL") { allowed = true; break; }
            if (comma < 0) break;
            pos = comma + 1;
        }
    }
    f.close();
    return allowed;
}

void shell_sudo(const String &args) {
    if (args.isEmpty()) { shell_println_wrapped("Usage: sudo <commande>"); return; }
    // Deja root : execution directe
    if (sessionAccessLevel == "root") {
        shell_eval_line(args); return;
    }
    // Vérification sudoers : si autorisé, pas besoin de mot de passe
    if (sudoers_allows(args)) {
        String savedLevel = sessionAccessLevel;
        String savedUser  = sessionUsername;
        sessionAccessLevel = "root";
        sessionUsername    = "root";
        shell_eval_line(args);
        if (sessionUsername == "root" && savedUser != "root") {
            sessionAccessLevel = savedLevel;
            sessionUsername    = savedUser;
        }
        return;
    }
    // Demande le mot de passe root
    String pass = saisirTexte("Mot de passe root : ", true, 64, "");
    minitel.println();
    // Lecture du hash root dans .users
    File uf = LittleFS.open("/etc/shadow", "r");
    if (!uf) { shell_println_wrapped("Erreur : fichier utilisateurs introuvable."); return; }
    String rootHash = "";
    while (uf.available()) {
        String line = uf.readStringUntil('\n'); line.trim();
        int c1 = line.indexOf(':'), c2 = line.lastIndexOf(':');
        if (c1 > 0 && c2 > c1 && line.substring(0, c1) == "root") {
            rootHash = line.substring(c1 + 1, c2); break;
        }
    }
    uf.close();
    // Hash MD5 du mot de passe saisi
    unsigned char digest[16];
    mbedtls_md5((const unsigned char*)pass.c_str(), pass.length(), digest);
    String hash = "";
    for (int i = 0; i < 16; i++) {
        if (digest[i] < 16) hash += "0";
        hash += String(digest[i], HEX);
    }
    if (hash != rootHash) { shell_println_wrapped("Mot de passe incorrect."); return; }
    // Élévation temporaire root (username + level)
    // Si la commande change la session définitivement (ex: su), on ne restaure pas.
    String savedLevel = sessionAccessLevel;
    String savedUser  = sessionUsername;
    sessionAccessLevel = "root";
    sessionUsername    = "root";
    shell_eval_line(args);
    // Restaure uniquement si su (ou autre) n'a pas changé la session
    if (sessionUsername == "root" && savedUser != "root") {
        // La commande n'a pas fait de su vers un autre user → restaure
        sessionAccessLevel = savedLevel;
        sessionUsername    = savedUser;
    }
    // Sinon : su a changé vers un autre user ou on était déjà root → garde
}

// ─── chown ───────────────────────────────────────────────────────────────────
void shell_chown(const String &args) {
    // Usage: chown <user>[:<group>] <path>
    int sp = args.indexOf(' ');
    if (sp < 0) { shell_println_wrapped("Usage: chown <user>[:<group>] <fichier>"); return; }
    String ownership = args.substring(0, sp);
    String path = shell_abspath(args.substring(sp + 1));
    if (!LittleFS.exists(path)) {
        shell_println_wrapped("Introuvable : " + path);
        return;
    }
    String newOwner = ownership;
    String newGroup = ownership;
    int colon = ownership.indexOf(':');
    if (colon >= 0) {
        newOwner = ownership.substring(0, colon);
        newGroup = ownership.substring(colon + 1);
    }
    FileMeta m = read_meta(path);
    bool isOwner = (m.owner == sessionUsername);
    bool isRoot  = (sessionAccessLevel == "root");

    // Changer le propriétaire : root uniquement
    if (newOwner != m.owner && !isRoot) {
        shell_println_wrapped("Acces refuse : seul root peut changer le proprietaire");
        return;
    }
    // Changer le groupe : propriétaire ou root
    // Le propriétaire ne peut assigner qu'un groupe auquel il appartient lui-même
    if (newGroup != m.group && !isRoot) {
        if (!isOwner) {
            shell_println_wrapped("Acces refuse : vous n'etes pas proprietaire");
            return;
        }
        if (!user_in_group(newGroup)) {
            shell_println_wrapped("Acces refuse : vous n'appartenez pas au groupe " + newGroup);
            return;
        }
    }
    write_meta(path, m.perms, newOwner, newGroup);
    shell_println_wrapped(path + " -> " + newOwner + ":" + newGroup);
}

// ─── ctftime ─────────────────────────────────────────────────────────────────
void shell_ctftime(const String &) {
    // Durée configurée dans /root/.ctf_config (en secondes), défaut 720s (12min)
    unsigned long dur_s = 720;
    if (LittleFS.exists("/root/.ctf_config")) {
        File f = LittleFS.open("/root/.ctf_config", "r");
        if (f) {
            String line = f.readStringUntil('\n'); line.trim();
            if (line.toInt() > 0) dur_s = (unsigned long)line.toInt();
            f.close();
        }
    }
    unsigned long elapsed_s  = millis() / 1000;
    unsigned long remain_s   = (elapsed_s < dur_s) ? (dur_s - elapsed_s) : 0;
    char buf[64];
    snprintf(buf, sizeof(buf), "Ecoule  : %02lu:%02lu", elapsed_s / 60, elapsed_s % 60);
    shell_println_wrapped(String(buf));
    snprintf(buf, sizeof(buf), "Restant : %02lu:%02lu", remain_s / 60, remain_s % 60);
    shell_println_wrapped(String(buf));
    int pct = (int)(100UL * elapsed_s / dur_s);
    if (pct > 100) pct = 100;
    String bar = "[";
    for (int i = 0; i < 20; i++) bar += (i < pct / 5) ? '#' : '.';
    bar += "] " + String(pct) + "%";
    shell_println_wrapped(bar);
    if (remain_s == 0) {
        shell_println_wrapped("!!! TEMPS ECOULE — Reinitialisation en cours... !!!");
#ifdef CTF_MODE
        delay(2000);
        ctf_fs_init();
#endif
        shell_logout();
    }
}
