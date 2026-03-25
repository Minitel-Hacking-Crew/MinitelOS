#include "shell_extra.h"
#include "applications/shell_apps/shell_edit.h"
#include "applications/cron/cron.h"
#include <ctime>

extern void shell_println_wrapped(const String &text);
extern String shell_abspath(const String &path);
extern std::map<String, String> shell_vars;
extern std::map<String, int> shell_int_vars;
extern std::map<String, float> shell_float_vars;
extern std::map<String, String> shell_string_vars;
extern std::map<String, bool> shell_bool_vars;
extern TaskHandle_t cronTaskHandle;

// ─── helpers metadata ────────────────────────────────────────────────────────
// Format /root/.fsmeta : une entrée par ligne  "<path> <perms> <owner> <group>"

static const char *META_FILE = "/root/.fsmeta";

struct FileMeta { String perms; String owner; String group; };

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
    if (!LittleFS.exists(path)) {
        File f = LittleFS.open(path, "w");
        if (!f) { shell_println_wrapped("Erreur creation : " + path); return; }
        f.close();
    }
    // Met à jour les métadonnées (simule l'horodatage de modification)
    FileMeta m = read_meta(path);
    write_meta(path, m.perms, m.owner.isEmpty() ? sessionUsername : m.owner,
               m.group.isEmpty() ? sessionUsername : m.group);
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
    write_meta(path, perms, m.owner, m.group);
    shell_println_wrapped(path + " -> " + perms);
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
    String owner = ownership;
    String group = ownership;
    int colon = ownership.indexOf(':');
    if (colon >= 0) {
        owner = ownership.substring(0, colon);
        group = ownership.substring(colon + 1);
    }
    FileMeta m = read_meta(path);
    write_meta(path, m.perms, owner, group);
    shell_println_wrapped(path + " -> " + owner + ":" + group);
}
