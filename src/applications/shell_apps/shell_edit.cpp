#include "globals.h"

void shell_edit(const String &args)
{
    String filename = shell_abspath(args);
    filename.trim();
    if (filename.length() == 0)
    {
        shell_println_wrapped("Usage: edit <fichier>");
        return;
    }
    if (filename == "/root/.users")
    {
        shell_println_wrapped("Edition du fichier /root/.users interdite.");
        return;
    }
    std::vector<String> lines;
    if (LittleFS.exists(filename))
    {
        File file = LittleFS.open(filename, "r");
        while (file && file.available())
        {
            lines.push_back(file.readStringUntil('\n'));
        }
        file.close();
    }
    int current = 0;
    bool modified = false;
    while (true)
    {
        shell_clear();
        minitel.println("--- mShell Edit > " + filename + " ---");
        minitel.println("");
        int start = current - 9;
        if (start < 0)
            start = 0;
        int end = start + 20;
        if (end > (int)lines.size())
            end = lines.size();
        minitel.moveCursorXY(0, 3);
        for (int i = start; i < end; ++i)
        {
            String prefix = (i == current) ? "> " : "  ";
            minitel.println(prefix + String(i + 1) + ": " + (i < (int)lines.size() ? lines[i] : ""));
        }

        minitel.moveCursorXY(0, 23);
        minitel.println("haut/bas:prec/suiv e[x]:edit a/rc:newline x:sup g[x]:goto w:save q!:quit h:aide");
        minitel.moveCursorXY(0, 24);
        minitel.print(String(' ', 80));
        minitel.moveCursorXY(0, 24);
        String cmd;
        while (true)
        {
            uint32_t k = minitel.getKeyCode(false);
            if (k == 0)
                continue;
            if (k == TOUCHE_FLECHE_BAS)
            {
                cmd = "n1";
                break;
            }
            else if (k == TOUCHE_FLECHE_HAUT)
            {
                cmd = "p1";
                break;
            }
            else if (k == ':')
            {
                cmd = saisirTexte(":");
                cmd.trim();
                break;
            }
            else if (k == CR)
            {
                cmd = "a";
                break;
            }
        }

        if (cmd == "n" || cmd == "+")
        {
            if (current < (int)lines.size() - 1)
                current++;
        }
        else if (cmd == "p" || cmd == "-")
        {
            if (current > 0)
                current--;
        }
        else if (cmd.startsWith("e"))
        {
            int editLine = -1;
            if (cmd.length() > 1)
            {
                String val = cmd.substring(1);
                val.trim();
                editLine = val.toInt();
            }
            int idx = (editLine > 0 && editLine <= (int)lines.size()) ? (editLine - 1) : (cmd == "e" ? current : -1);
            if (idx >= 0 && idx < (int)lines.size())
            {
                minitel.moveCursorXY(0, 24);
                minitel.print(String(' ', 80));
                minitel.moveCursorXY(0, 24);
                String newLine = saisirTexte("> ", false, 128, lines[idx]);
                int fill = 80 - newLine.length() - 2;
                if (fill > 0)
                    minitel.print(String(' ', fill));
                minitel.moveCursorXY(0, 24);
                minitel.print(String(' ', 80));
                lines[idx] = newLine;
                modified = true;
            }
            else
            {
                shell_println_wrapped("Numero de ligne invalide.");
                attendreToucheMore();
            }
        }
        else if (cmd == "b")
        {
            String newLine = saisirTexte("b: ", false, 128, "");
            lines.insert(lines.begin() + current, newLine);
            modified = true;
        }
        else if (cmd == "a" || cmd == "")
        {
            String newLine = saisirTexte("a: ", false, 128, "");
            lines.insert(lines.begin() + current + 1, newLine);
            current++;
            modified = true;
        }
        else if (cmd.startsWith("n"))
        {
            int offset = 1;
            if (cmd.length() > 1)
            {
                String val = cmd.substring(1);
                val.trim();
                int v = val.toInt();
                if (v > 0)
                    offset = v;
            }
            if (current + offset < (int)lines.size())
                current += offset;
            else
                current = lines.size() - 1;
        }
        else if (cmd.startsWith("p"))
        {
            int offset = 1;
            if (cmd.length() > 1)
            {
                String val = cmd.substring(1);
                val.trim();
                int v = val.toInt();
                if (v > 0)
                    offset = v;
            }
            if (current - offset >= 0)
                current -= offset;
            else
                current = 0;
        }
        else if (cmd.startsWith("g"))
        {
            int num = -1;
            if (cmd.length() > 1)
            {
                String val = cmd.substring(1);
                val.trim();
                num = val.toInt();
            }
            if (num == -1)
            {
                minitel.println();
                String numStr = saisirTexte("Aller a la ligne #: ", false, 4, String(current + 1));
                numStr.trim();
                num = numStr.toInt();
            }
            if (num > 0 && num <= (int)lines.size())
            {
                current = num - 1;
            }
            else
            {
                shell_println_wrapped("Numero de ligne invalide.");
                attendreToucheMore();
            }
        }
        else if (cmd.startsWith("x"))
        {
            int num = -1;
            if (cmd.length() > 1)
            {
                String val = cmd.substring(1);
                val.trim();
                num = val.toInt();
            }
            if (num > 0 && num <= (int)lines.size())
            {
                lines.erase(lines.begin() + num - 1);
                if (current >= (int)lines.size() && current > 0)
                    current--;
                modified = true;
            }
            else if (lines.size() > 0 && num == -1)
            {
                lines.erase(lines.begin() + current);
                if (current >= (int)lines.size() && current > 0)
                    current--;
                modified = true;
            }
        }
        else if (cmd == "w")
        {
            File file = LittleFS.open(filename, FILE_WRITE);
            if (!file)
            {
                minitel.println();
                shell_println_wrapped("Erreur ecriture : " + filename);
            }
            else
            {
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    file.print(lines[i]);
                    if (i < lines.size() - 1)
                        file.print("\n");
                }
                file.close();
                minitel.println();
                shell_println_wrapped("Fichier sauvegarde.");
                modified = false;
            }
        }
        else if (cmd == "wq")
        {
            File file = LittleFS.open(filename, FILE_WRITE);
            if (!file)
            {
                minitel.println();
                shell_println_wrapped("Erreur ecriture : " + filename);
            }
            else
            {
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    file.print(lines[i]);
                    if (i < lines.size() - 1)
                        file.print("\n");
                }
                file.close();
                minitel.println();
                shell_println_wrapped("Fichier sauvegarde.");
                modified = false;
            }
            break;
        }
        else if (cmd == "q!")
        {
            if (modified)
            {
                minitel.println();
                shell_println_wrapped("Modifications annulees.");
            }
            break;
        }
        else if (cmd == "h")
        {
            shell_clear();
            minitel.println("Commandes editeur:");
            minitel.println("n[nb] : ligne suivante (+nb)");
            minitel.println("p[nb] : ligne precedente (-nb)");
            minitel.println("g[nb] : aller a la ligne nb");
            minitel.println("e[nb] : editer la ligne nb (ou courante)");
            minitel.println("b     : inserer avant la ligne");
            minitel.println("a     : ajouter apres la ligne");
            minitel.println("x[nb] : supprimer la ligne nb (ou courante)");
            minitel.println("st    : aller a la premiere ligne");
            minitel.println("fi    : aller a la derniere ligne");
            minitel.println("w     : sauvegarder");
            minitel.println("wq    : sauvegarder et quitter");
            minitel.println("q!    : quitter sans sauvegarder");
            minitel.println("h     : aide");
            attendreToucheMore();
        }
        else if (cmd == "st")
        {
            current = 0;
        }
        else if (cmd == "fi")
        {
            if (!lines.empty())
                current = lines.size() - 1;
        }
        else if (cmd == "")
        {
        }
        else
        {
            minitel.println();
            shell_println_wrapped("Commande inconnue: " + cmd);
            delay(500);
        }
    }
    minitel.println();
    shell_println_wrapped("Sortie de l'editeur.");
}
