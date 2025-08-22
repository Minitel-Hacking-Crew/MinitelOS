// =========================
// 0. INCLUDES & PROTOTYPES
// =========================
#include "globals.h"
#include "shell.h"

#define HISTORY_SIZE 24
std::deque<String> shell_history;
int shell_history_index = -1;

void sshTask(void *pvParameters);

// =========================
// 1. WRAPPERS ET TABLEAU DES COMMANDES
// =========================

void shell_version_wrapper(const String &) { shell_version(); }
void shell_clear_wrapper(const String &) { shell_clear(); }
void shell_logout_wrapper(const String &) { shell_logout(); }
void shell_reboot_wrapper(const String &) { shell_reboot(); }
void shell_whoami_wrapper(const String &) { shell_whoami(); }
void shell_passwd_wrapper(const String &) { shell_passwd(); }
void shell_wifi_wrapper(const String &) { connecterWiFi(); }
void shell_ifconfig_wrapper(const String &) { shell_ifconfig(""); }
void shell_run_wrapper(const String &args) { shell_run(args); }
void shell_adduser(const String &);


void shell_history_clear(const String &)
{
    shell_history.clear();
    save_shell_history(sessionUsername);
    // Efface le fichier sur disque
    String histfile = "/home/" + sessionUsername + "/.msh_history";
    if (LittleFS.exists(histfile))
    {
        LittleFS.remove(histfile);
    }
    shell_println_wrapped("Historique de commande vide pour l'utilisateur :" + sessionUsername + " (niveau: " + sessionAccessLevel + ")");
}
extern void shell_motd();

void shell_su_wrapper(const String &args) { shell_su(args); }

ShellCommand commands[] = {
    {"echo", shell_echo},
    {"ssh", shell_sshlauncher},
    {"whoami", shell_whoami_wrapper},
    {"passwd", shell_passwd_wrapper},
    {"clear", shell_clear_wrapper},
    {"reboot", shell_reboot_wrapper},
    {"logout", shell_logout_wrapper},
    {"version", shell_version_wrapper},
    {"ping", shell_ping},
    {"wifi", shell_wifi_wrapper},
    {"ifconfig", shell_ifconfig_wrapper},
    {"curl", shell_curl},
    {"ls", shell_ls},
    {"cat", shell_cat},
    {"rm", shell_rm},
    {"create", shell_create},
    {"mkdir", shell_mkdir},
    {"mv", shell_mv},
    {"cp", shell_cp},
    {"df", shell_df},
    {"set", shell_set},
    {"cd", shell_cd},
    {"pwd", shell_pwd},
    {"run", shell_run_wrapper},
    {"edit", shell_edit},
    {"wait", shell_wait},
    {"help", shell_help},
    {"history", shell_history_cmd},
    {"grep", shell_grep},
    {"head", shell_head},
    {"tail", shell_tail},
    {"motd", shell_motd},
    {"clearh", shell_history_clear},
    {"su", shell_su_wrapper},
    {"adduser", shell_adduser},
    {"deluser", shell_deluser},
    {"cronpause", shell_cronpause},
    {"cronresume", shell_cronresume},
};

int numCommands = sizeof(commands) / sizeof(commands[0]);

// =========================
// 2. VARIABLES GLOBALES
// =========================

String shell_output_buffer;
bool shell_redirect_mode = false;
std::map<String, String> shell_vars;
String shell_current_dir = "/";

std::map<String, int> shell_int_vars;
std::map<String, float> shell_float_vars;
std::map<String, String> shell_string_vars;
std::map<String, bool> shell_bool_vars;

// Variables SSH
String ssh_host;
String ssh_username;
String ssh_password;

bool in_ssh_session = false;
bool shell_break_flag = false;

void write_to_file(const String &filename, const String &content, bool append)
{
    File file = LittleFS.open(filename, append ? FILE_APPEND : FILE_WRITE);
    if (!file)
    {
        shell_println_wrapped("Erreur ouverture : " + filename);
        return;
    }
    file.print(content);
    file.close();
    shell_println_wrapped(String(append ? "Ajout dans : " : "Ecrit dans : ") + filename);
}

void attendreToucheMore()
{
    minitel.print(">>");
    while (minitel.getKeyCode(false) == 0)
        delay(50);
}

static int shell_println_line_count = 0;

void reset_shell_println_line_count()
{
    shell_println_line_count = 0;
}

void shell_println_wrapped(const String &text)
{
    const int maxLineLen = 80;
    int start = 0;
    int len = text.length();
    while (start < len)
    {
        int end = start + maxLineLen;
        if (end > len)
            end = len;
        String line = text.substring(start, end);
        if (shell_redirect_mode)
            shell_output_buffer += line + "\n";
        else
        {
            minitel.println(line);
            shell_println_line_count++;
            if (shell_println_line_count >= 24)
            {
                attendreToucheMore();
                shell_println_line_count = 0;
            }
        }
        start = end;
    }
}

String shell_abspath(const String &path)
{
    String p = path;
    p.trim();
    if (p == ".")
    {
        return shell_abspath(shell_current_dir);
    }
    if (p == "..")
    {
        String base = shell_current_dir;
        int last = base.lastIndexOf('/', base.length() - 2);
        if (last >= 0)
            base = base.substring(0, last + 1);
        else
            base = "/";
        if (base.length() > 1 && base.endsWith("/"))
            base.remove(base.length() - 1);
        return base;
    }
    String base = shell_current_dir;
    if (p.startsWith("/"))
        base = "/";
    while (p.startsWith("../"))
    {
        int last = base.lastIndexOf('/', base.length() - 2);
        if (last >= 0)
            base = base.substring(0, last + 1);
        else
            base = "/";
        p = p.substring(3);
    }
    if (p.startsWith("./"))
        p = p.substring(2);
    String full = base;
    if (!full.endsWith("/"))
        full += "/";
    full += p;
    while (full.indexOf("//") != -1)
        full.replace("//", "/");
    while (true)
    {
        int idx = full.indexOf("/../");
        if (idx <= 0)
            break;
        int prev = full.lastIndexOf('/', idx - 1);
        if (prev >= 0)
            full.remove(prev, idx + 4 - prev);
        else
            full.remove(0, idx + 4);
    }
    full.replace("/./", "/");
    if (full.length() > 1 && full.endsWith("/"))
        full.remove(full.length() - 1);
    return full;
}

// =========================
// 4. COMMANDES
// =========================

void shell_curl(const String &args)
{
    if (args.length() == 0)
    {
        shell_println_wrapped("Usage: curl <url> ou curl -d \"data\" <url>");
        return;
    }
    String url = args;
    String data = "";
    bool isPost = false;

    if (args.startsWith("-d "))
    {
        int quote1 = args.indexOf('"');
        int quote2 = args.indexOf('"', quote1 + 1);
        if (quote1 != -1 && quote2 != -1)
        {
            data = args.substring(quote1 + 1, quote2);
            url = args.substring(quote2 + 1);
            url.trim();
            isPost = true;
        }
    }

    client.setInsecure();
    shell_println_wrapped(String(isPost ? "POST " : "GET "));
    shell_println_wrapped(url);

    if (http.begin(client, url))
    {
        int httpCode;
        if (isPost)
        {
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            httpCode = http.POST(data);
        }
        else
        {
            httpCode = http.GET();
        }
        minitel.print("HTTP ");
        shell_println_wrapped(String(httpCode));
        String payload = http.getString();
        shell_println_wrapped(payload);
        http.end();
    }
    else
    {
        shell_println_wrapped("Erreur d'initialisation HTTP");
    }
}

void shell_ifconfig(const String &args)
{
    minitel.println("");
    if (WiFi.status() == WL_CONNECTED)
    {
        shell_println_wrapped("SSID     : " + WiFi.SSID());
        shell_println_wrapped("IP       : " + WiFi.localIP().toString());
        shell_println_wrapped("Gateway  : " + WiFi.gatewayIP().toString());
        shell_println_wrapped("DNS      : " + WiFi.dnsIP().toString());
        shell_println_wrapped("MAC      : " + WiFi.macAddress());
    }
    else
    {
        shell_println_wrapped("Non connecte au WiFi.");
    }
}

void shell_ping(const String &args)
{
    if (args.length() == 0)
    {
        shell_println_wrapped("Usage: ping [-t x] <host>");
        return;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        shell_println_wrapped("Erreur : pas de connexion WiFi");
        return;
    }
    String host = args;
    int count = 4;
    int arg_t = args.indexOf("-t ");
    if (arg_t != -1)
    {
        int space_after_t = args.indexOf(' ', arg_t + 3);
        String countStr;
        if (space_after_t == -1)
        {
            countStr = args.substring(arg_t + 3);
            host = args.substring(0, arg_t);
            host.trim();
        }
        else
        {
            countStr = args.substring(arg_t + 3, space_after_t);
            host = args.substring(space_after_t + 1);
            host.trim();
        }
        countStr.trim();
        if (countStr.length() > 0)
        {
            int c = countStr.toInt();
            if (c > 0 && c < 100)
                count = c;
        }
        host.trim();
        if (host.length() == 0)
        {
            shell_println_wrapped("Usage: ping [-t x] <host>");
            return;
        }
    }
    else
    {
        host = args;
        host.trim();
    }
    shell_println_wrapped("PING " + host + " :");
    int sent = 0, received = 0;
    int minTime = 9999, maxTime = 0, totalTime = 0;
    for (int i = 0; i < count; ++i)
    {
        unsigned long start = millis();
        bool ok = Ping.ping(host.c_str(), 1);
        unsigned long elapsed = millis() - start;
        sent++;
        if (ok)
        {
            received++;
            if ((int)elapsed < minTime)
                minTime = elapsed;
            if ((int)elapsed > maxTime)
                maxTime = elapsed;
            totalTime += elapsed;
            shell_println_wrapped("Reply from " + host + ": time=" + String(elapsed) + " ms");
        }
        else
        {
            shell_println_wrapped("Request timeout for icmp_seq " + String(i + 1));
        }
        delay(500);
    }
    shell_println_wrapped("");
    shell_println_wrapped("--- " + host + " ping statistics ---");
    shell_println_wrapped(String(sent) + " packets transmitted, " + String(received) + " received, " + String(100 - (received * 100 / sent)) + "% packet loss");
    if (received > 0)
    {
        int avg = totalTime / received;
        shell_println_wrapped("rtt min/avg/max = " + String(minTime) + "/" + String(avg) + "/" + String(maxTime) + " ms");
    }
}

void shell_sshlauncher(const String &args)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        shell_println_wrapped("Erreur : pas de connexion WiFi");
        return;
    }
    int atIdx = args.indexOf('@');
    int pIdx = args.indexOf(" -p ");
    if (atIdx == -1)
    {
        shell_println_wrapped("Syntaxe : ssh user@host -p pass");
        return;
    }
    ssh_username = args.substring(0, atIdx);
    if (pIdx == -1)
    {
        ssh_host = args.substring(atIdx + 1);
        ssh_host.trim();
        ssh_password = saisirTexte("Mot de passe SSH : ", true, 64, "");
    }
    else
    {
        ssh_host = args.substring(atIdx + 1, pIdx);
        ssh_password = args.substring(pIdx + 4);
    }

    minitel.println("");
    shell_println_wrapped(" [SSH] Connexion SSH  " + ssh_username + "@" + ssh_host + "...");
    BaseType_t xReturned;
    xReturned = xTaskCreatePinnedToCore(sshTask, "sshTask", 51200, NULL, (configMAX_PRIORITIES - 1), NULL, ARDUINO_RUNNING_CORE);
    if (xReturned != pdPASS)
    {
        shell_println_wrapped("Erreur lors de la creation de la tache SSH");
    }
}

// =========================
// 7. BOUCLE PRINCIPALE DU SHELL
// =========================

void shell(bool skipInitScreen)
{
    if (!skipInitScreen)
    {
        passerEnMixte();
        shell_clear();
        minitel.println("");
    }

    while (!sessionIsLoggedIn)
    {
        shell_login();
    }

    load_shell_history(sessionUsername);
    start_cronTask();

    while (true)
    {
        while (in_ssh_session)
        {
            vTaskDelay(60000 / portTICK_PERIOD_MS);
        }

        String histfile = "/home/" + sessionUsername + "/.msh_history";
        if (!LittleFS.exists(histfile) && !shell_history.empty())
        {
            shell_history.clear();
        }

        String prompt = "";
        String homeDir = "/home/" + sessionUsername;
        if (shell_current_dir.startsWith(homeDir))
        {
            String subdir = shell_current_dir.substring(homeDir.length());
            if (subdir.length() == 0)
                prompt += "# >";
            else
                prompt += "#" + subdir + " >";
        }
        else if (shell_current_dir != "/")
        {
            prompt += "(" + shell_current_dir + ") >";
        }
        else
        {
            prompt += "/ >";
        }
        String cmdLine = "";
        shell_history_index = shell_history.size();
        minitel.print(prompt);
        while (true)
        {
            uint32_t k = minitel.getKeyCode(false);
            if (k == 0)
                continue;
            if (k == CR)
                break;
            else if (k == GUIDE)
            {
                shell_help("");
                minitel.print(prompt);
                continue;
            }
            else if (k == TOUCHE_FLECHE_HAUT)
            {
                if (shell_history_index > 0 && !shell_history.empty())
                {
                    shell_history_index--;
                    for (int i = 0; i < cmdLine.length(); ++i)
                    {
                        minitel.moveCursorLeft(1);
                        minitel.print(" ");
                        minitel.moveCursorLeft(1);
                    }
                    cmdLine = shell_history[shell_history_index];
                    minitel.print(cmdLine);
                }
            }
            else if (k == TOUCHE_FLECHE_BAS)
            {
                if (shell_history_index < (int)shell_history.size() - 1)
                {
                    shell_history_index++;
                    for (int i = 0; i < cmdLine.length(); ++i)
                    {
                        minitel.moveCursorLeft(1);
                        minitel.print(" ");
                        minitel.moveCursorLeft(1);
                    }
                    cmdLine = shell_history[shell_history_index];
                    minitel.print(cmdLine);
                }
                else if (shell_history_index == (int)shell_history.size() - 1)
                {
                    shell_history_index++;
                    for (int i = 0; i < cmdLine.length(); ++i)
                    {
                        minitel.moveCursorLeft(1);
                        minitel.print(" ");
                        minitel.moveCursorLeft(1);
                    }
                    cmdLine = "";
                }
            }
            else if (k == 0x1347)
            {
                if (cmdLine.length() > 0)
                {
                    cmdLine.remove(cmdLine.length() - 1);
                    minitel.moveCursorLeft(1);
                    minitel.print(" ");
                    minitel.moveCursorLeft(1);
                }
            }
            else if (k >= 32 && k <= 126)
            {
                cmdLine += (char)k;
                minitel.print(String((char)k));
            }
        }
        minitel.println("");
        cmdLine.trim();
        if (cmdLine.length() > 0 && (shell_history.empty() || shell_history.back() != cmdLine))
        {
            add_shell_history(sessionUsername, cmdLine);
        }

        for (auto &kv : shell_int_vars)
        {
            cmdLine.replace("$" + kv.first, String(kv.second));
        }
        for (auto &kv : shell_float_vars)
        {
            char buf[16];
            dtostrf(kv.second, 0, 2, buf);
            cmdLine.replace("$" + kv.first, String(buf));
        }
        for (auto &kv : shell_string_vars)
        {
            cmdLine.replace("$" + kv.first, kv.second);
        }
        for (auto &kv : shell_bool_vars)
        {
            cmdLine.replace("$" + kv.first, kv.second ? "true" : "false");
        }
        for (auto &kv : shell_vars)
        {
            cmdLine.replace("$" + kv.first, kv.second);
        }

        if (cmdLine.length() == 0)
        {
            continue;
        }
        int redirectIdx = cmdLine.indexOf('>');
        int appendIdx = cmdLine.indexOf(">>");
        bool appendMode = false;
        if (appendIdx != -1)
        {
            redirectIdx = appendIdx;
            appendMode = true;
        }
        String redirectFile = "";
        String realCmdLine = cmdLine;
        if (redirectIdx != -1)
        {
            realCmdLine = cmdLine.substring(0, redirectIdx);
            redirectFile = cmdLine.substring(redirectIdx + (appendMode ? 2 : 1));
            realCmdLine.trim();
            redirectFile.trim();
        }
        int spaceIdx = realCmdLine.indexOf(' ');
        String cmd = (spaceIdx == -1) ? realCmdLine : realCmdLine.substring(0, spaceIdx);
        String args = (spaceIdx == -1) ? "" : realCmdLine.substring(spaceIdx + 1);
        shell_output_buffer = "";
        shell_redirect_mode = (redirectFile.length() > 0);
        bool found = false;
        for (int i = 0; i < numCommands; ++i)
        {
            if (cmd == commands[i].name)
            {
                commands[i].func(args);
                found = true;
                break;
            }
        }
        if (!found)
        {
            shell_println_wrapped("Commande inconnue : " + cmd);
        }
        if (shell_redirect_mode)
        {
            String absRedirectFile = shell_abspath(redirectFile);
            write_to_file(absRedirectFile, shell_output_buffer, appendMode);
            shell_redirect_mode = false;
        }
    }
}

void sshTask(void *pvParameters)
{
    minitel.println("[SSH] Connexion en cours...");
    in_ssh_session = true;

    bool isOpen = sshClient.begin(ssh_host.c_str(), ssh_username.c_str(), ssh_password.c_str());

    if (!isOpen)
    {
        minitel.println("[SSH] Connexion SSH echouee, retour au menu principal...");
        delay(1000);
        in_ssh_session = false;
        vTaskDelete(NULL);
        return;
    }
    minitel.println("[SSH] Session ouverte");
    shell_clear();

    while (true)
    {
        bool cancel = false;
        bool quit = false;
        if (!sshClient.available())
        {
            break;
        }

        int nbytes = sshClient.receive();
        if (nbytes < 0)
        {
            break;
        }
        if (nbytes > 0)
        {
            int index = 0;
            while (index < nbytes)
            {
                char b = sshClient.readIndex(index++);
                minitel.writeByte(b);
            }
        }

        uint32_t key = minitel.getKeyCode(false);
        if (key == 0)
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }
        switch (key)
        {
        case SOMMAIRE:
            quit = true;
            sshClient.end();
            in_ssh_session = false;
            vTaskDelete(NULL);
            break;
        case GUIDE:
            key = 0x07;
            break;
        case ANNULATION:
            key = 0x0515;
            break;
        case CORRECTION:
            key = 0x7F;
            break;
        case RETOUR:
            key = 0x01;
            break;
        case SUITE:
            key = 0x05;
            break;
        case REPETITION:
            key = 0x0C;
            break;
        case ENVOI:
            key = 0x0D;
            break;
        case 0x03:
            cancel = true;
            break;
        }
        if (quit)
        {
            break;
        }
        uint8_t payload[4];
        size_t len = 0;
        for (len = 0; key != 0 && len < 4; len++)
        {
            payload[3 - len] = uint8_t(key);
            key = key >> 8;
        }
        if (sshClient.send(payload + 4 - len, len) < 0)
        {
            break;
        }
        if (cancel)
        {
            int nbyte = sshClient.flushReceiving();
            minitel.println();
            minitel.println();
            minitel.println("\r\r * ctrl+C * ");
            minitel.println("Warning: received data ignored to avoid long diplaying time");
            minitel.print("number of bytes ignored : ");
            minitel.println(String(nbyte));
            uint8_t cr = 0x0D;
            sshClient.send(&cr, 1);
        }
    }
    sshClient.end();
    in_ssh_session = false;
    vTaskDelete(NULL);
}