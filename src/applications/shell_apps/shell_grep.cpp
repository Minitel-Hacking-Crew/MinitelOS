#include "globals.h"

void shell_grep(const String &args)
{
    int before = 0, after = 0;
    String motif, fichier;
    std::vector<String> tokens;
    int i = 0;
    String tmp = args;
    while (tmp.length() > 0)
    {
        int sp = tmp.indexOf(' ');
        if (sp == -1)
        {
            tokens.push_back(tmp);
            break;
        }
        tokens.push_back(tmp.substring(0, sp));
        tmp = tmp.substring(sp + 1);
        tmp.trim();
    }
    while (i < tokens.size())
    {
        if (tokens[i] == "-b" && i + 1 < tokens.size())
        {
            before = tokens[i + 1].toInt();
            i += 2;
        }
        else if (tokens[i] == "-a" && i + 1 < tokens.size())
        {
            after = tokens[i + 1].toInt();
            i += 2;
        }
        else
        {
            break;
        }
    }
    if (i + 1 >= tokens.size())
    {
        shell_println_wrapped("Usage: grep [-b x] [-a x] <motif_regex> <fichier>");
        return;
    }
    motif = tokens[i];
    fichier = tokens[i + 1];
    motif.trim();
    fichier.trim();
    fichier = shell_abspath(fichier);
    if (!LittleFS.exists(fichier))
    {
        shell_println_wrapped("Fichier introuvable : " + fichier);
        return;
    }
    File f = LittleFS.open(fichier, "r");
    if (!f)
    {
        shell_println_wrapped("Erreur ouverture : " + fichier);
        return;
    }
    std::regex re(motif.c_str());
    std::vector<String> lines;
    while (f.available())
    {
        lines.push_back(f.readStringUntil('\n'));
    }
    f.close();
    int n = lines.size();
    std::vector<bool> toPrint(n, false);
    for (int l = 0; l < n; ++l)
    {
        if (std::regex_search(lines[l].c_str(), re))
        {
            int start = l - before;
            int end = l + after;
            if (start < 0)
                start = 0;
            if (end >= n)
                end = n - 1;
            for (int k = start; k <= end; ++k)
                toPrint[k] = true;
        }
    }
    for (int l = 0; l < n; ++l)
    {
        if (toPrint[l])
        {
            shell_println_wrapped(String(l + 1) + ": " + lines[l]);
        }
    }
}

void shell_head(const String &args)
{
    int n = 10;
    String fichier;
    String tmp = args;
    tmp.trim();
    if (tmp.startsWith("-n "))
    {
        int sp = tmp.indexOf(' ', 3);
        if (sp == -1)
        {
            shell_println_wrapped("Usage: head [-n x] <fichier>");
            return;
        }
        n = tmp.substring(3, sp).toInt();
        fichier = tmp.substring(sp + 1);
    }
    else
    {
        fichier = tmp;
    }
    fichier.trim();
    fichier = shell_abspath(fichier);
    if (!LittleFS.exists(fichier))
    {
        shell_println_wrapped("Fichier introuvable : " + fichier);
        return;
    }
    File f = LittleFS.open(fichier, "r");
    if (!f)
    {
        shell_println_wrapped("Erreur ouverture : " + fichier);
        return;
    }
    int count = 0;
    while (f.available() && count < n)
    {
        String line = f.readStringUntil('\n');
        shell_println_wrapped(line);
        count++;
    }
    f.close();
}

void shell_tail(const String &args)
{
    int n = 10;
    String fichier;
    String tmp = args;
    tmp.trim();
    if (tmp.startsWith("-n "))
    {
        int sp = tmp.indexOf(' ', 3);
        if (sp == -1)
        {
            shell_println_wrapped("Usage: tail [-n x] <fichier>");
            return;
        }
        n = tmp.substring(3, sp).toInt();
        fichier = tmp.substring(sp + 1);
    }
    else
    {
        fichier = tmp;
    }
    fichier.trim();
    fichier = shell_abspath(fichier);
    if (!LittleFS.exists(fichier))
    {
        shell_println_wrapped("Fichier introuvable : " + fichier);
        return;
    }
    File f = LittleFS.open(fichier, "r");
    if (!f)
    {
        shell_println_wrapped("Erreur ouverture : " + fichier);
        return;
    }
    std::vector<String> lines;
    while (f.available())
    {
        lines.push_back(f.readStringUntil('\n'));
    }
    f.close();
    int start = (int)lines.size() - n;
    if (start < 0)
        start = 0;
    for (int i = start; i < (int)lines.size(); ++i)
    {
        shell_println_wrapped(lines[i]);
    }
}
