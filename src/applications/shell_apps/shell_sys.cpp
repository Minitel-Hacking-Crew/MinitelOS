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
        File usersFile = LittleFS.open("/root/.users", "r");
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
    shell();
}

void shell_reboot()
{
    shell_println_wrapped("Redemarrage...");
    esp_restart();
}

void shell_clear()
{
    for (int i = 0; i < 24; ++i)
    {
        minitel.println(String(' ', 80));
    }
    minitel.moveCursorXY(0, 0);
}

void shell_df(const String &)
{
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    shell_println_wrapped("/ : " + String(used / 1024) + " Ko utilises / " + String(total / 1024) + " Ko");
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

    File usersFile = LittleFS.open("/root/.users", "r");
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
    usersFile = LittleFS.open("/root/.users", FILE_WRITE);
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
    shell_println_wrapped("MinitelOS" + OSVersion);
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

    shell_println_wrapped("\n--- CTF CHALLENGES ---");
    shell_println_wrapped("  ctf              - Menu des challenges");
    shell_println_wrapped("  ctf <challenge>  - Lance un challenge spécifique");

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

    // Cherche l'utilisateur dans /root/.users
    File usersFile = LittleFS.open("/root/.users", "r");
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
    File usersFile = LittleFS.open("/root/.users", "r");
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
    usersFile = LittleFS.open("/root/.users", "a");
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
    File usersFile = LittleFS.open("/root/.users", "r");
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
    usersFile = LittleFS.open("/root/.users", "w");
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