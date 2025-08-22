#include "globals.h"

void shell_cat(const String &args)
{
    String filename = shell_abspath(args);
    filename.trim();
    if (sessionAccessLevel != "root")
    {
        if (filename == "/root" || filename.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (filename.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (filename != allowed && !filename.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    if (!LittleFS.exists(filename))
    {
        shell_println_wrapped("Fichier introuvable : " + filename);
        return;
    }
    File file = LittleFS.open(filename, "r");
    if (!file)
    {
        shell_println_wrapped("Erreur ouverture : " + filename);
        return;
    }
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        shell_println_wrapped(line);
    }
    file.close();
}

void shell_cd(const String &args)
{
    String dir = args;
    dir.trim();
    if (dir.length() == 0)
    {
        if (sessionUsername == "root")
            shell_current_dir = "/root";
        else
            shell_current_dir = "/home/" + sessionUsername;
        return;
    }
    String newdir = shell_abspath(dir);

    if (sessionAccessLevel != "root")
    {
        if (newdir == "/root" || newdir.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (newdir.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (newdir != allowed && !newdir.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    File f = LittleFS.open(newdir);
    if (!f || !f.isDirectory())
    {
        shell_println_wrapped("Dossier introuvable : " + newdir);
        return;
    }
    shell_current_dir = newdir;
}

void shell_cp(const String &args)
{
    int sep = args.indexOf(' ');
    if (sep == -1)
    {
        shell_println_wrapped("Usage: cp <src> <dst>");
        return;
    }
    String src = shell_abspath(args.substring(0, sep));
    String dst = shell_abspath(args.substring(sep + 1));
    src.trim();
    dst.trim();
    if (sessionAccessLevel != "root")
    {
        if (src == "/root" || src.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (src.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (src != allowed && !src.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
        if (dst == "/root" || dst.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (dst.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (dst != allowed && !dst.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    if (!LittleFS.exists(src))
    {
        shell_println_wrapped("Fichier introuvable : " + src);
        return;
    }
    File fsrc = LittleFS.open(src, "r");
    File fdst = LittleFS.open(dst, "w");
    if (!fsrc || !fdst)
    {
        shell_println_wrapped("Erreur ouverture fichier");
        return;
    }
    while (fsrc.available())
    {
        fdst.write(fsrc.read());
    }
    fsrc.close();
    fdst.close();
    shell_println_wrapped("Copie : " + src + " -> " + dst);
}

void shell_create(const String &args)
{
    String filename = shell_abspath(args);
    filename.trim();
    if (filename.length() == 0)
    {
        shell_println_wrapped("Usage: create <nom_fichier>");
        return;
    }
    if (sessionAccessLevel != "root")
    {
        if (filename == "/root" || filename.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (filename.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (filename != allowed && !filename.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file)
    {
        shell_println_wrapped("Erreur creation : " + filename);
        return;
    }
    file.println("");
    file.close();
    shell_println_wrapped("Fichier cree : " + filename);
}
void shell_ls(const String &args)
{
    String dir = shell_current_dir;
    bool showHidden = args.indexOf("-h") != -1;
    if (sessionAccessLevel != "root")
    {
        if (dir == "/root" || dir.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (dir.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (dir != allowed && !dir.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    File root = LittleFS.open(dir);
    if (!root || !root.isDirectory())
    {
        shell_println_wrapped("Impossible d'ouvrir le dossier : " + dir);
        return;
    }
    File file = root.openNextFile();
    while (file)
    {
        String name = String(file.name());
        if (name.startsWith(dir) && dir != "/")
            name = name.substring(dir.length());
        if (name.startsWith("/"))
            name = name.substring(1);
        if (name.startsWith(".") && !showHidden)
        {
            file = root.openNextFile();
            continue;
        }
        if (file.isDirectory())
            shell_println_wrapped(name + " (repertoire)");
        else
            shell_println_wrapped(name + " (" + file.size() + " octets)");
        file = root.openNextFile();
    }
}

void shell_mkdir(const String &args)
{
    String dirname = shell_abspath(args);
    dirname.trim();
    if (dirname.length() == 0)
    {
        shell_println_wrapped("Usage: mkdir <nom_dossier>");
        return;
    }
    if (sessionAccessLevel != "root")
    {
        if (dirname == "/root" || dirname.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (dirname.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (dirname != allowed && !dirname.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    if (LittleFS.mkdir(dirname))
    {
        shell_println_wrapped("Dossier cree : " + dirname);
    }
    else
    {
        shell_println_wrapped("Erreur creation dossier : " + dirname);
    }
}

void shell_mv(const String &args)
{
    int sep = args.indexOf(' ');
    if (sep == -1)
    {
        shell_println_wrapped("Usage: mv <src> <dst>");
        return;
    }
    String src = shell_abspath(args.substring(0, sep));
    String dst = shell_abspath(args.substring(sep + 1));
    src.trim();
    dst.trim();
    if (sessionAccessLevel != "root")
    {
        if (src == "/root" || src.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (src.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (src != allowed && !src.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
        if (dst == "/root" || dst.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (dst.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (dst != allowed && !dst.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    if (!LittleFS.exists(src))
    {
        shell_println_wrapped("Fichier/dossier introuvable : " + src);
        return;
    }
    if (LittleFS.rename(src, dst))
    {
        shell_println_wrapped("Renomme/deplace : " + src + " -> " + dst);
    }
    else
    {
        shell_println_wrapped("Erreur renommage/deplacement");
    }
}

void shell_rm_recursive(const String &path)
{
    File entry = LittleFS.open(path);
    if (!entry)
        return;
    if (entry.isDirectory())
    {
        File file = entry.openNextFile();
        while (file)
        {
            String child = String(file.name());
            shell_rm_recursive(child);
            file = entry.openNextFile();
        }
        entry.close();
        LittleFS.rmdir(path);
    }
    else
    {
        entry.close();
        LittleFS.remove(path);
    }
}

void shell_rm(const String &args)
{
    String filename = shell_abspath(args);
    filename.trim();
    if (filename == "/")
    {
        shell_println_wrapped("Suppression interdite");
        return;
    }
    if (sessionAccessLevel != "root")
    {
        if (filename == "/root" || filename.startsWith("/root/"))
        {
            shell_println_wrapped("Acces refuse");
            return;
        }
        else if (filename.startsWith("/home/"))
        {
            String allowed = "/home/" + sessionUsername;
            if (filename != allowed && !filename.startsWith(allowed + "/"))
            {
                shell_println_wrapped("Acces refuse");
                return;
            }
        }
    }
    if (!LittleFS.exists(filename))
    {
        shell_println_wrapped("Fichier ou dossier introuvable : " + filename);
        return;
    }
    File entry = LittleFS.open(filename);
    if (!entry)
    {
        shell_println_wrapped("Erreur : " + filename);
        return;
    }
    bool isDir = entry.isDirectory();
    entry.close();
    if (isDir)
    {
        shell_rm_recursive(filename);
        shell_println_wrapped("Deleted : " + filename);
    }
    else
    {
        if (LittleFS.remove(filename))
            shell_println_wrapped("Deleted : " + filename);
        else
            shell_println_wrapped("Erreur suppression : " + filename);
    }
}
