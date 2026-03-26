#pragma once
#include "FS.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>

// Root directory for simulation filesystem
#ifndef LITTLEFS_SIM_ROOT
#define LITTLEFS_SIM_ROOT "sim_fs"
#endif

class LittleFS_class {
    std::string _root;

    std::string _hostPath(const String& virtPath) const {
        std::string vp = virtPath.c_str();
        if (vp.empty() || vp[0] != '/') vp = "/" + vp;
        return _root + vp;
    }

    void _mkdirs(const std::string& path) const {
        std::string p;
        for (size_t i = 1; i < path.size(); ++i) {
            if (path[i] == '/' || i == path.size() - 1) {
                p = path.substr(0, i + 1);
                struct stat st;
                if (stat(p.c_str(), &st) != 0)
                    ::mkdir(p.c_str(), 0755);
            }
        }
    }

public:
    bool begin(bool formatOnFail = false) {
        _root = LITTLEFS_SIM_ROOT;
        struct stat st;
        if (stat(_root.c_str(), &st) != 0)
            ::mkdir(_root.c_str(), 0755);
        return true;
    }

    void end() {}

    bool exists(const String& path) const {
        struct stat st;
        return stat(_hostPath(path).c_str(), &st) == 0;
    }

    bool mkdir(const String& path) {
        std::string hp = _hostPath(path);
        _mkdirs(hp);
        struct stat st;
        return stat(hp.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    }

    bool rmdir(const String& path) {
        return ::rmdir(_hostPath(path).c_str()) == 0;
    }

    bool remove(const String& path) {
        return ::unlink(_hostPath(path).c_str()) == 0;
    }

    bool rename(const String& from, const String& to) {
        return ::rename(_hostPath(from).c_str(), _hostPath(to).c_str()) == 0;
    }

    File open(const String& path, const char* mode = "r") {
        std::string hp = _hostPath(path);
        struct stat st;
        auto impl = std::make_shared<FileImpl>();
        impl->path = hp;
        impl->virtPath = path.c_str();

        if (stat(hp.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            impl->dp = opendir(hp.c_str());
            impl->isDir = true;
            return File(impl);
        }

        // For write modes, create parent dirs
        if (mode[0] == 'w' || mode[0] == 'a') {
            size_t slash = hp.rfind('/');
            if (slash != std::string::npos)
                _mkdirs(hp.substr(0, slash));
        }

        impl->fp = fopen(hp.c_str(), mode);
        impl->isDir = false;
        return File(impl);
    }

    // Simule la partition LittleFS ESP32 (1 Mo fixe)
    size_t totalBytes() const { return 1024 * 1024; }

    size_t usedBytes() const { return _dir_size(_root); }

private:
    static size_t _dir_size(const std::string &path) {
        DIR *d = opendir(path.c_str());
        if (!d) return 0;
        size_t total = 0;
        struct dirent *e;
        while ((e = readdir(d)) != nullptr) {
            if (e->d_name[0] == '.') continue;
            std::string child = path + "/" + e->d_name;
            struct stat st;
            if (stat(child.c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) total += _dir_size(child);
                else total += (size_t)st.st_size;
            }
        }
        closedir(d);
        return total;
    }
public:
};

extern LittleFS_class LittleFS;
