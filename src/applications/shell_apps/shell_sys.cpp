#include "globals.h"

void shell_cronpause(const String &)
{
    cron_paused = true;
    shell_println_wrapped("[CRON] Pause activee.");
}

void shell_cronresume(const String &)
{
    cron_paused = false;
    shell_println_wrapped("[CRON] Reprise activee.");
}
void shell_login()
{
    minitel.println();
    minitel.println("=== Minitel OS V.1 : Connexion ===");
    minitel.println();
    shell_motd();
    minitel.println();

    auto md5 = [](const String &str) -> String
    {
        unsigned char md5sum[16];
        mbedtls_md5((const unsigned char *)str.c_str(), str.length(), md5sum);
        String out = "";
        for (int i = 0; i < 16; ++i)
        {
            if (md5sum[i] < 16)
                out += "0";
            out += String(md5sum[i], HEX);
        }
        out.toLowerCase();
        return out;
    };

    bool found = false;
    String accessLevel = "user";
    while (!found)
    {
        String inputUser = saisirTexte("Nom d'utilisateur :", false, 20, "");
        minitel.println();
        String inputPass = saisirTexte("Mot de passe :", true, 20, "");
        minitel.println();

        String userHash = md5(inputPass);
        File usersFile = LittleFS.open("/etc/shadow", "r");
        String debugHashAttendu = "";
        bool debugUserFound = false;
        found = false;
        if (usersFile)
        {
            while (usersFile.available())
            {
                String line = usersFile.readStringUntil('\n');
                line.trim();
                if (line.length() == 0 || line.startsWith("#"))
                    continue;
                int c1 = line.indexOf(':');
                int c2 = line.lastIndexOf(':');
                if (c1 == -1 || c2 == -1 || c2 <= c1)
                    continue;
                String u = line.substring(0, c1);
                String h = line.substring(c1 + 1, c2);
                String lvl = line.substring(c2 + 1);
                // (DEBUG supprime)
                if (u == inputUser)
                {
                    debugHashAttendu = h;
                    debugUserFound = true;
                }
                if (u == inputUser && h == userHash)
                {
                    sessionUsername = u;
                    sessionPassword = userHash;
                    accessLevel = lvl;
                    found = true;
                    break;
                }
            }
            usersFile.close();
        }

        if (!found)
        {
            minitel.println("Echec de connexion : utilisateur ou mot de passe incorrect.");
            delay(1000);
            minitel.println();
        }
    }
    sessionIsLoggedIn = true;
    sessionAccessLevel = accessLevel;
    minitel.println();
    minitel.println("Connexion reussie !");
    if (sessionUsername == "root")
        shell_current_dir = "/root";
    else
        shell_current_dir = "/home/" + sessionUsername;
    shell();
}

void shell_logout()
{
    shell_println_wrapped("Deconnexion...");
    delay(500);
    sessionIsLoggedIn = false;
    sessionAccessLevel = "user";
    sessionUsername = "";
    sessionPassword = "";
    shell_current_dir = "/";
    shell_vars.clear();
    shell_int_vars.clear();
    shell_float_vars.clear();
    shell_string_vars.clear();
    shell_bool_vars.clear();
    shell();
}

void shell_reboot()
{
    shell_println_wrapped("Redemarrage...");
    esp_restart();
}

void shell_clear()
{
    minitel.clearScreen();
    minitel.moveCursorXY(1, 1);
}

void shell_df(const String &)
{
    size_t total  = LittleFS.totalBytes();
    size_t used   = LittleFS.usedBytes();
    size_t free_  = total - used;
    int    pct    = total ? (int)(100UL * used / total) : 0;
    String bar    = "[";
    for (int i = 0; i < 20; i++) bar += (i < pct / 5) ? '#' : '.';
    bar += "]";
    shell_println_wrapped("Filesystem  Taille   Utilise  Libre    Usage");
    char line[64];
    snprintf(line, sizeof(line), "LittleFS    %4uKo   %4uKo   %4uKo   %d%%",
             (unsigned)(total/1024), (unsigned)(used/1024),
             (unsigned)(free_/1024), pct);
    shell_println_wrapped(String(line));
    shell_println_wrapped(bar + " " + String(pct) + "%");
}

void shell_echo(const String &args)
{
    if (args.startsWith("$"))
    {
        String var = args.substring(1);
        if (shell_vars.count(var))
        {
            shell_println_wrapped(shell_vars[var]);
        }
        else
        {
            shell_println_wrapped("Variable non definie : " + var);
        }
    }
    else
    {
        shell_println_wrapped(args);
        File f = LittleFS.open("/cron.log", "a");
        if (f)
        {
            f.println(args);
            f.close();
        }
    }
}

void shell_passwd()
{
    shell_println_wrapped("Changement de mot de passe");

    // Demande l'ancien mot de passe
    String oldPass = saisirTexte("Ancien mot de passe : ", true, 64, "");
    auto md5 = [](const String &str) -> String
    {
        unsigned char md5sum[16];
        mbedtls_md5((const unsigned char *)str.c_str(), str.length(), md5sum);
        String out = "";
        for (int i = 0; i < 16; ++i)
        {
            if (md5sum[i] < 16)
                out += "0";
            out += String(md5sum[i], HEX);
        }
        out.toLowerCase();
        return out;
    };
    String oldHash = md5(oldPass);

    if (oldHash != sessionPassword)
    {
        minitel.println();
        shell_println_wrapped("Mot de passe actuel incorrect.");
        return;
    }
    minitel.println();

    // Demande le nouveau mot de passe
    String newPass1 = saisirTexte("Nouveau mot de passe : ", true, 64, "");
    minitel.println();
    String newPass2 = saisirTexte("Confirmer le nouveau : ", true, 64, "");
    minitel.println();
    if (newPass1 != newPass2)
    {
        shell_println_wrapped("Les mots de passe ne correspondent pas.");
        return;
    }
    String newHash = md5(newPass1);

    File usersFile = LittleFS.open("/etc/shadow", "r");
    if (!usersFile)
    {
        shell_println_wrapped("Impossible d'acceder au fichier des utilisateurs.");
        return;
    }
    std::vector<String> lines;
    while (usersFile.available())
    {
        String line = usersFile.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#"))
        {
            lines.push_back(line);
            continue;
        }
        int c1 = line.indexOf(':');
        int c2 = line.lastIndexOf(':');
        if (c1 == -1 || c2 == -1 || c2 <= c1)
        {
            lines.push_back(line);
            continue;
        }
        String u = line.substring(0, c1);
        String h = line.substring(c1 + 1, c2);
        String lvl = line.substring(c2 + 1);
        if (u == sessionUsername)
        {
            // Remplace le hash
            line = u + ":" + newHash + ":" + lvl;
        }
        lines.push_back(line);
    }
    usersFile.close();

    // Reecrit le fichier
    usersFile = LittleFS.open("/etc/shadow", FILE_WRITE);
    if (!usersFile)
    {
        shell_println_wrapped("Erreur lors de la mise a jour du mot de passe.");
        return;
    }
    for (auto &l : lines)
        usersFile.println(l);
    usersFile.close();

    // Met a jour la session
    sessionPassword = newHash;
    minitel.println();
    shell_println_wrapped("Mot de passe change avec succes.");
}

void shell_set(const String &args)
{
    int sep = args.indexOf(' ');
    if (sep == -1)
    {
        shell_println_wrapped("Usage: set <VAR> <valeur>");
        return;
    }
    String var = args.substring(0, sep);
    String val = args.substring(sep + 1);
    var.trim();
    val.trim();
    shell_vars[var] = val;
}

void shell_version()
{
    shell_println_wrapped(String("MinitelOS ") + OSVersion);
}

void shell_wait(const String &args)
{
    int ms = args.toInt();
    if (ms <= 0)
    {
        shell_println_wrapped("Usage: wait <millisecondes>");
        return;
    }
    delay(ms);
}

void shell_pwd(const String &)
{
    shell_println_wrapped(shell_abspath(shell_current_dir));
}

void shell_help(const String &)
{
    shell_println_wrapped("Commandes disponibles :");

    shell_println_wrapped("\n--- SYSTEME ---");
    shell_println_wrapped("  reboot           - Redémarre le système");
    shell_println_wrapped("  logout           - Déconnexion");
    shell_println_wrapped("  clear            - Efface l'écran");
    shell_println_wrapped("  version          - Affiche la version du système");
    shell_println_wrapped("  whoami           - Affiche l'utilisateur actuel");
    shell_println_wrapped("  passwd           - Change le mot de passe");
    shell_println_wrapped("  set <var> <val>  - Définit une variable d'environnement");
    shell_println_wrapped("  echo <texte>     - Affiche le texte ou la valeur d'une variable");
    shell_println_wrapped("  wait <ms>        - Attend un certain nombre de millisecondes");
    shell_println_wrapped("  history          - Affiche l'historique des commandes");
    shell_println_wrapped("  clearh           - Efface l'historique des commandes");
    shell_println_wrapped("  motd             - Affiche ou modifie le message du jour (MOTD)");
    shell_println_wrapped("  su <user>        - Change d'utilisateur après vérification du mot de passe");
    shell_println_wrapped("  adduser          - Ajoute un nouvel utilisateur (root uniquement)");
    shell_println_wrapped("  deluser          - Supprime un utilisateur (root uniquement)");
    shell_println_wrapped("  cronpause        - Met en pause les tâches cron");
    shell_println_wrapped("  cronresume       - Reprend les tâches cron");

    shell_println_wrapped("\n--- NAVIGATION & FICHIERS ---");
    shell_println_wrapped("  pwd              - Affiche le répertoire courant");
    shell_println_wrapped("  cd <répertoire>  - Change de répertoire");
    shell_println_wrapped("  ls               - Liste les fichiers du répertoire courant");
    shell_println_wrapped("  mkdir <nom>      - Crée un nouveau dossier");
    shell_println_wrapped("  rm <fichier>     - Supprime un fichier ou dossier");
    shell_println_wrapped("  cp <src> <dst>   - Copie un fichier");
    shell_println_wrapped("  mv <src> <dst>   - Renomme ou déplace un fichier");
    shell_println_wrapped("  cat <fichier>    - Affiche le contenu d'un fichier");
    shell_println_wrapped("  create <nom>     - Crée un fichier vide");
    shell_println_wrapped("  df               - Affiche l'espace disque");
    shell_println_wrapped("  edit <fichier>   - Ouvre un fichier dans l'éditeur de texte");
    shell_println_wrapped("  run <fichier>    - Exécute un script");

    shell_println_wrapped("\n--- RÉSEAU ---");
    shell_println_wrapped("  ifconfig         - Affiche les informations réseau");
    shell_println_wrapped("  ping <host>      - Ping un hôte");
    shell_println_wrapped("  ping -t <x> <host> - Ping un hôte avec un nombre de requêtes spécifié");
    shell_println_wrapped("  curl <url>       - Effectue une requête HTTP GET ou POST");
    shell_println_wrapped("  curl -d 'data' <url> - Effectue une requête HTTP POST avec des données");
    shell_println_wrapped("  ssh <user@host>  - Lance un client SSH sur le port par défaut (22)");

    shell_println_wrapped("\n--- DIVERS ---");
    shell_println_wrapped("  help             - Affiche cette aide");
    shell_println_wrapped("  saisirTexte      - Saisie de texte avancée (utilisée dans l'éditeur)");
    shell_println_wrapped("  (Touche REPETITION) - Active/désactive le déplacement gauche/droite dans la saisie texte");
    shell_println_wrapped("  (Touche GUIDE x6)   - Réinitialise les utilisateurs au démarrage");
    minitel.println();
}

void load_shell_history(const String &user)
{
    shell_history.clear();
    String histfile = "/home/" + user + "/.msh_history";
    if (!LittleFS.exists(histfile))
    {
        // Cree le fichier vide si absent
        File f = LittleFS.open(histfile, FILE_WRITE);
        if (f)
            f.close();
        return;
    }
    File f = LittleFS.open(histfile, "r");
    while (f && f.available())
    {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0)
            shell_history.push_back(line);
        if (shell_history.size() > HISTORY_SIZE)
            shell_history.pop_front();
    }
    f.close();
}

void save_shell_history(const String &user)
{
    String histfile = "/home/" + user + "/.msh_history";
    File f = LittleFS.open(histfile, FILE_WRITE);
    if (!f)
        return;
    for (auto &cmd : shell_history)
    {
        f.println(cmd);
    }
    f.close();
}

void add_shell_history(const String &user, const String &cmd)
{
    if (cmd.length() == 0)
        return;
    if (!shell_history.empty() && shell_history.back() == cmd)
        return;
    shell_history.push_back(cmd);
    if (shell_history.size() > HISTORY_SIZE)
        shell_history.pop_front();
    save_shell_history(user);
}

void shell_history_cmd(const String &)
{
    int idx = 1;
    for (auto &cmd : shell_history)
    {
        minitel.println(String(idx) + ": " + cmd);
        idx++;
    }
}

void shell_motd(const String &args)
{
    String motdFile = "/.motd";
    String trimmed = args;
    trimmed.trim();
    if (trimmed == "--help" || trimmed == "-h")
    {
        shell_println_wrapped("Usage: motd [--help] [-s <message>]");
        shell_println_wrapped("");
        shell_println_wrapped("Affiche le message du jour systeme.");
        shell_println_wrapped("Si ~/motd_perso.txt existe, les commandes");
        shell_println_wrapped("qu'il contient sont executees et leur");
        shell_println_wrapped("sortie est ajoutee a l'affichage.");
        shell_println_wrapped("");
        shell_println_wrapped("Options:");
        shell_println_wrapped("  -s <msg>   Definir le motd systeme (root)");
        shell_println_wrapped("  --help     Afficher cette aide");
        shell_println_wrapped("");
        shell_println_wrapped("Exemple de ~/motd_perso.txt :");
        shell_println_wrapped("  echo Bonjour !");
        shell_println_wrapped("  date");
        return;
    }
    if (trimmed.startsWith("-s "))
    {
        String msg = trimmed.substring(3);
        File f = LittleFS.open(motdFile, FILE_WRITE);
        if (f)
        {
            f.println(msg);
            f.close();
            shell_println_wrapped("motd mis a jour.");
        }
        else
        {
            shell_println_wrapped("Erreur ecriture motd.");
        }
        return;
    }
    if (LittleFS.exists(motdFile))
    {
        File f = LittleFS.open(motdFile, "r");
        while (f && f.available())
        {
            String line = f.readStringUntil('\n');
            shell_println_wrapped(line);
        }
        f.close();
    }
    // Affichage du motd personnalise — les commandes shell sont executees
    // pour permettre l'affichage dynamique (date, uptime, echo...)
    String personal = "/home/" + sessionUsername + "/motd_perso.txt";
    if (LittleFS.exists(personal))
    {
        shell_println_wrapped("--- Message personnel ---");
        String savedUser  = sessionUsername;
        String savedLevel = sessionAccessLevel;
        sessionUsername    = "root";
        sessionAccessLevel = "root";
        File pf = LittleFS.open(personal, "r");
        if (pf)
        {
            while (pf.available())
            {
                String line = pf.readStringUntil('\n');
                line.trim();
                if (line.length() > 0) shell_eval_line(line);
            }
            pf.close();
        }
        sessionUsername    = savedUser;
        sessionAccessLevel = savedLevel;
    }
}

void shell_motd()
{
    shell_motd("");
}

void shell_whoami()
{
    shell_println_wrapped(sessionUsername);
}

void shell_su(const String &args)
{
    String targetUser = args;
    targetUser.trim();
    if (targetUser.length() == 0)
    {
        shell_println_wrapped("Usage: su <user>");
        return;
    }

    // On ne peut pas su vers l'utilisateur deja courant
    if (targetUser == sessionUsername)
    {
        shell_println_wrapped("Deja connecte en tant que " + targetUser);
        return;
    }

    // Cherche l'utilisateur dans /etc/shadow
    File usersFile = LittleFS.open("/etc/shadow", "r");
    if (!usersFile)
    {
        shell_println_wrapped("Impossible d'acceder a la base des utilisateurs.");
        return;
    }
    String foundHash = "";
    String foundLevel = "user";
    bool userFound = false;
    while (usersFile.available())
    {
        String line = usersFile.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#"))
            continue;
        int c1 = line.indexOf(':');
        int c2 = line.lastIndexOf(':');
        if (c1 == -1 || c2 == -1 || c2 <= c1)
            continue;
        String u = line.substring(0, c1);
        String h = line.substring(c1 + 1, c2);
        String lvl = line.substring(c2 + 1);
        if (u == targetUser)
        {
            foundHash = h;
            foundLevel = lvl;
            userFound = true;
            break;
        }
    }
    usersFile.close();
    if (!userFound)
    {
        minitel.println();
        shell_println_wrapped("Utilisateur inconnu : " + targetUser);
        return;
    }

    // Root peut changer d'utilisateur sans mot de passe (comme sur Linux)
    if (sessionAccessLevel == "root") {
        sessionUsername    = targetUser;
        sessionPassword    = foundHash;
        sessionAccessLevel = foundLevel;
        minitel.println();
        shell_println_wrapped("Changement d'utilisateur : " + targetUser);
        if (targetUser == "root")
            shell_current_dir = "/root";
        else
            shell_current_dir = "/home/" + targetUser;
        return;
    }

    // Demande le mot de passe
    minitel.println();
    String inputPass = saisirTexte("Mot de passe pour " + targetUser + " :", true, 20, "");
    auto md5 = [](const String &str) -> String
    {
        unsigned char md5sum[16];
        mbedtls_md5((const unsigned char *)str.c_str(), str.length(), md5sum);
        String out = "";
        for (int i = 0; i < 16; ++i)
        {
            if (md5sum[i] < 16)
                out += "0";
            out += String(md5sum[i], HEX);
        }
        out.toLowerCase();
        return out;
    };
    String inputHash = md5(inputPass);
    if (inputHash != foundHash)
    {
        minitel.println();
        shell_println_wrapped("Mot de passe incorrect.");
        return;
    }

    // Change la session
    sessionUsername = targetUser;
    sessionPassword = inputHash;
    sessionAccessLevel = foundLevel;
    // On ne change pas sessionIsLoggedIn (reste true)
    minitel.println();
    shell_println_wrapped("Changement d'utilisateur : " + targetUser);
    // Change le repertoire courant
    if (targetUser == "root")
        shell_current_dir = "/root";
    else
        shell_current_dir = "/home/" + targetUser;
    // Recharge l'historique
    load_shell_history(targetUser);
    // Affiche le prompt du nouvel utilisateur
    shell_motd();
}

void shell_adduser(const String &)
{
    if (sessionAccessLevel != "root")
    {
        minitel.println();
        shell_println_wrapped("Seul root peut ajouter un utilisateur.");
        return;
    }
    minitel.println("=== Ajouter un nouvel utilisateur ===");
    minitel.println();
    String username = saisirTexte("Nom d'utilisateur : ", false, 20, "");
    minitel.println();
    username.trim();
    if (username.length() == 0)
    {
        minitel.println();
        shell_println_wrapped("Nom d'utilisateur vide.");
        return;
    }
    File usersFile = LittleFS.open("/etc/shadow", "r");
    if (usersFile)
    {
        while (usersFile.available())
        {
            String line = usersFile.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("#"))
                continue;
            int c1 = line.indexOf(':');
            if (c1 == -1)
                continue;
            String u = line.substring(0, c1);
            if (u == username)
            {
                usersFile.close();
                minitel.println();
                shell_println_wrapped("Utilisateur deja existant.");
                return;
            }
        }
        usersFile.close();
    }
    minitel.println();

    String password = saisirTexte("Mot de passe : ", true, 64, "");
    password.trim();
    if (password.length() == 0)
    {
        minitel.println();
        shell_println_wrapped("Mot de passe vide.");
        return;
    }

    // Prompt pour le niveau administratif
    minitel.println();
    minitel.println("Choisir le niveau d'acces :");
    minitel.println("  1 - root");
    minitel.println("  2 - admin");
    minitel.println("  3 - user");
    String accessInput = saisirTexte("Niveau (1/2/3) : ", false, 1, "3");
    accessInput.trim();
    String accessLevel = "user";
    if (accessInput == "1")
        accessLevel = "root";
    else if (accessInput == "2")
        accessLevel = "admin";
    else
        accessLevel = "user";

    auto md5 = [](const String &str) -> String
    {
        unsigned char md5sum[16];
        mbedtls_md5((const unsigned char *)str.c_str(), str.length(), md5sum);
        String out = "";
        for (int i = 0; i < 16; ++i)
        {
            if (md5sum[i] < 16)
                out += "0";
            out += String(md5sum[i], 16);
        }
        out.toLowerCase();
        return out;
    };
    String hash = md5(password);
    usersFile = LittleFS.open("/etc/shadow", "a");
    if (!usersFile)
    {
        minitel.println();
        shell_println_wrapped("Erreur d'acces au fichier des utilisateurs.");
        return;
    }
    usersFile.println(username + ":" + hash + ":" + accessLevel);
    usersFile.close();
    String userDir = "/home/" + username;
    if (!LittleFS.exists(userDir))
    {
        LittleFS.mkdir(userDir);
    }
    minitel.println();
    shell_println_wrapped("Utilisateur ajoute : " + username + " (niveau: " + accessLevel + ")");
}

void shell_deluser(const String &)
{
    if (sessionAccessLevel != "root")
    {
        minitel.println();

        shell_println_wrapped("Seul root peut supprimer un utilisateur.");
        return;
    }
    minitel.println();

    String username = saisirTexte("Utilisateur a supprimer : ", false, 20, "");
    username.trim();
    if (username.length() == 0)
    {
        minitel.println();

        shell_println_wrapped("Nom d'utilisateur vide.");
        return;
    }
    if (username == "root")
    {
        minitel.println();

        shell_println_wrapped("Impossible de supprimer root.");
        return;
    }
    File usersFile = LittleFS.open("/etc/shadow", "r");
    if (!usersFile)
    {
        minitel.println();
        shell_println_wrapped("Erreur d'acces au fichier des utilisateurs.");
        return;
    }
    std::vector<String> lines;
    bool found = false;
    while (usersFile.available())
    {
        String line = usersFile.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#"))
        {
            lines.push_back(line);
            continue;
        }
        int c1 = line.indexOf(':');
        if (c1 == -1)
        {
            lines.push_back(line);
            continue;
        }
        String u = line.substring(0, c1);
        if (u == username)
        {
            found = true;
            continue;
        }
        lines.push_back(line);
    }
    usersFile.close();
    if (!found)
    {
        minitel.println();
        shell_println_wrapped("Utilisateur introuvable : " + username);
        return;
    }
    usersFile = LittleFS.open("/etc/shadow", "w");
    if (!usersFile)
    {
        minitel.println();
        shell_println_wrapped("Erreur d'acces au fichier des utilisateurs.");
        return;
    }
    for (auto &l : lines)
        usersFile.println(l);
    usersFile.close();
    String userDir = "/home/" + username;
    if (LittleFS.exists(userDir))
    {
        LittleFS.rmdir(userDir);
    }
    minitel.println();
    shell_println_wrapped("Utilisateur supprime : " + username);
}

// ─── Gestion des groupes ─────────────────────────────────────────────────────
// Format /root/.groups : "nomgroupe:user1,user2,..." (une ligne par groupe)

static const char *GROUPS_FILE = "/root/.groups";

// Lit tous les groupes → map nom → liste membres
static std::vector<std::pair<String, String>> read_groups() {
    std::vector<std::pair<String, String>> result;
    if (!LittleFS.exists(GROUPS_FILE)) return result;
    File f = LittleFS.open(GROUPS_FILE, "r");
    if (!f) return result;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        int colon = line.indexOf(':');
        if (colon < 0) continue;
        result.push_back({line.substring(0, colon), line.substring(colon + 1)});
    }
    f.close();
    return result;
}

// Écrit les groupes → fichier
static void write_groups(const std::vector<std::pair<String, String>> &groups) {
    File f = LittleFS.open(GROUPS_FILE, "w");
    if (!f) return;
    for (auto &g : groups) f.println(g.first + ":" + g.second);
    f.close();
}

void shell_groupadd(const String &args) {
    if (sessionAccessLevel != "root") {
        shell_println_wrapped("Acces refuse : root uniquement");
        return;
    }
    String name = args;
    name.trim();
    if (name.length() == 0) { shell_println_wrapped("Usage: groupadd <nom>"); return; }
    auto groups = read_groups();
    for (auto &g : groups) {
        if (g.first == name) {
            shell_println_wrapped("Groupe deja existant : " + name);
            return;
        }
    }
    groups.push_back({name, ""});
    write_groups(groups);
    shell_println_wrapped("Groupe cree : " + name);
}

void shell_groupdel(const String &args) {
    if (sessionAccessLevel != "root") {
        shell_println_wrapped("Acces refuse : root uniquement");
        return;
    }
    String name = args;
    name.trim();
    if (name.length() == 0) { shell_println_wrapped("Usage: groupdel <nom>"); return; }
    auto groups = read_groups();
    bool found = false;
    std::vector<std::pair<String, String>> newGroups;
    for (auto &g : groups) {
        if (g.first == name) { found = true; continue; }
        newGroups.push_back(g);
    }
    if (!found) { shell_println_wrapped("Groupe introuvable : " + name); return; }
    write_groups(newGroups);
    shell_println_wrapped("Groupe supprime : " + name);
}

// groupmem <group> <user>      — ajoute user au groupe
// groupmem -d <group> <user>   — retire user du groupe
void shell_groupmem(const String &args) {
    if (sessionAccessLevel != "root") {
        shell_println_wrapped("Acces refuse : root uniquement");
        return;
    }
    String tmp = args;
    tmp.trim();
    bool remove = false;
    if (tmp.startsWith("-d ")) {
        remove = true;
        tmp = tmp.substring(3);
        tmp.trim();
    }
    int sp = tmp.indexOf(' ');
    if (sp < 0) {
        shell_println_wrapped("Usage: groupmem [-d] <groupe> <user>");
        return;
    }
    String groupName = tmp.substring(0, sp);
    String userName  = tmp.substring(sp + 1);
    groupName.trim(); userName.trim();

    auto groups = read_groups();
    bool found = false;
    for (auto &g : groups) {
        if (g.first != groupName) continue;
        found = true;
        // Parse members
        std::vector<String> members;
        String raw = g.second;
        int pos = 0;
        while (pos <= (int)raw.length()) {
            int c = raw.indexOf(',', pos);
            String m = (c < 0) ? raw.substring(pos) : raw.substring(pos, c);
            m.trim();
            if (m.length() > 0) members.push_back(m);
            if (c < 0) break;
            pos = c + 1;
        }
        if (remove) {
            std::vector<String> updated;
            for (auto &m : members) if (m != userName) updated.push_back(m);
            String newMembers = "";
            for (size_t i = 0; i < updated.size(); i++) {
                if (i > 0) newMembers += ",";
                newMembers += updated[i];
            }
            g.second = newMembers;
            shell_println_wrapped(userName + " retire de " + groupName);
        } else {
            for (auto &m : members) {
                if (m == userName) {
                    shell_println_wrapped(userName + " est deja dans " + groupName);
                    write_groups(groups);
                    return;
                }
            }
            g.second = (g.second.length() > 0) ? g.second + "," + userName : userName;
            shell_println_wrapped(userName + " ajoute a " + groupName);
        }
        break;
    }
    if (!found) { shell_println_wrapped("Groupe introuvable : " + groupName); return; }
    write_groups(groups);
}

// groups [user]  — liste les groupes d'un utilisateur (défaut : session courante)
void shell_groups_cmd(const String &args) {
    String targetUser = args;
    targetUser.trim();
    if (targetUser.length() == 0) targetUser = sessionUsername;

    String result = targetUser + " : user";
    // Groupes hiérarchiques
    if (targetUser == sessionUsername) {
        if (sessionAccessLevel == "admin" || sessionAccessLevel == "root")
            result += " admin";
        if (sessionAccessLevel == "root")
            result += " root";
        result += " " + sessionUsername;
    }
    // Groupes personnalisés
    auto groups = read_groups();
    for (auto &g : groups) {
        // Cherche dans les membres
        String raw = g.second;
        int pos = 0;
        while (pos <= (int)raw.length()) {
            int c = raw.indexOf(',', pos);
            String m = (c < 0) ? raw.substring(pos) : raw.substring(pos, c);
            m.trim();
            if (m == targetUser) { result += " " + g.first; break; }
            if (c < 0) break;
            pos = c + 1;
        }
    }
    shell_println_wrapped(result);
}

// ─── Gestion sudoers ─────────────────────────────────────────────────────────
// Format /etc/sudoers : "user ALL" | "user cmd1,cmd2" | "%group ALL"

static const char *SUDOERS_FILE = "/etc/sudoers";

void shell_sudoedit(const String &args) {
    if (sessionAccessLevel != "root") {
        shell_println_wrapped("Acces refuse : root uniquement");
        return;
    }
    String tmp = args;
    tmp.trim();
    if (tmp == "list") {
        // Affiche les règles existantes
        if (!LittleFS.exists(SUDOERS_FILE)) {
            shell_println_wrapped("(aucune regle)");
            return;
        }
        File f = LittleFS.open(SUDOERS_FILE, "r");
        int n = 0;
        while (f && f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("#")) continue;
            n++;
            shell_println_wrapped(String(n) + ": " + line);
        }
        if (f) f.close();
        if (n == 0) shell_println_wrapped("(aucune regle)");
        return;
    }
    if (tmp.startsWith("add ")) {
        // sudoedit add <sujet> <cmds>
        String rule = tmp.substring(4);
        rule.trim();
        if (rule.length() == 0) {
            shell_println_wrapped("Usage: sudoedit add <user|%group> <ALL|cmd1,cmd2>");
            return;
        }
        File f = LittleFS.open(SUDOERS_FILE, "a");
        if (!f) { shell_println_wrapped("Erreur ecriture sudoers"); return; }
        f.println(rule);
        f.close();
        shell_println_wrapped("Regle ajoutee : " + rule);
        return;
    }
    if (tmp.startsWith("del ")) {
        // sudoedit del <numero>
        int num = tmp.substring(4).toInt();
        if (num <= 0) { shell_println_wrapped("Usage: sudoedit del <numero>"); return; }
        if (!LittleFS.exists(SUDOERS_FILE)) { shell_println_wrapped("Aucune regle"); return; }
        File f = LittleFS.open(SUDOERS_FILE, "r");
        std::vector<String> lines;
        while (f && f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("#")) continue;
            lines.push_back(line);
        }
        if (f) f.close();
        if (num > (int)lines.size()) { shell_println_wrapped("Numero invalide"); return; }
        String removed = lines[num - 1];
        lines.erase(lines.begin() + num - 1);
        File out = LittleFS.open(SUDOERS_FILE, "w");
        if (out) { for (auto &l : lines) out.println(l); out.close(); }
        shell_println_wrapped("Regle supprimee : " + removed);
        return;
    }
    shell_println_wrapped("Usage: sudoedit list | add <sujet> <cmds> | del <num>");
}