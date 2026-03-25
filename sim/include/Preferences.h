#pragma once
#include "Arduino.h"
#include <map>
#include <fstream>
#include <string>

class Preferences {
    std::string _ns;
    std::string _file;
    std::map<std::string, std::string> _data;

    void _load() {
        _data.clear();
        std::ifstream f(_file);
        if (!f) return;
        std::string line;
        bool inNs = false;
        while (std::getline(f, line)) {
            if (line == "[" + _ns + "]") { inNs = true; continue; }
            if (line.size() > 0 && line[0] == '[') { inNs = false; continue; }
            if (!inNs) continue;
            auto eq = line.find('=');
            if (eq != std::string::npos)
                _data[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }

    void _save() {
        // Read all sections, replace ours
        std::map<std::string, std::map<std::string, std::string>> all;
        {
            std::ifstream f(_file);
            std::string line, cur;
            while (std::getline(f, line)) {
                if (line.size() > 1 && line[0] == '[') {
                    cur = line.substr(1, line.size() - 2);
                } else {
                    auto eq = line.find('=');
                    if (eq != std::string::npos && !cur.empty())
                        all[cur][line.substr(0, eq)] = line.substr(eq + 1);
                }
            }
        }
        all[_ns] = _data;
        std::ofstream f(_file);
        for (auto& [sec, kvs] : all) {
            f << "[" << sec << "]\n";
            for (auto& [k, v] : kvs)
                f << k << "=" << v << "\n";
        }
    }

public:
    bool begin(const char* ns, bool readOnly = false) {
        _ns   = ns;
        _file = std::string("sim_prefs.ini");
        _load();
        return true;
    }

    void end() { _save(); }

    bool putString(const char* key, const String& val) {
        _data[key] = val.c_str();
        return true;
    }

    String getString(const char* key, const String& def = "") {
        auto it = _data.find(key);
        return it != _data.end() ? String(it->second.c_str()) : def;
    }

    bool putInt(const char* key, int val) {
        _data[key] = std::to_string(val);
        return true;
    }

    int getInt(const char* key, int def = 0) {
        auto it = _data.find(key);
        return it != _data.end() ? std::stoi(it->second) : def;
    }

    bool isKey(const char* key) {
        return _data.count(key) > 0;
    }

    void clear() { _data.clear(); }
    void remove(const char* key) { _data.erase(key); }
};
