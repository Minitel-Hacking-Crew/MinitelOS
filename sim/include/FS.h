#pragma once
#include "Arduino.h"
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <memory>

#define FILE_READ   "r"
#define FILE_WRITE  "w"
#define FILE_APPEND "a"

struct FileImpl {
    FILE*       fp   = nullptr;
    DIR*        dp   = nullptr;
    std::string path;       // full host path
    std::string virtPath;   // virtual path (as seen by shell)
    bool        isDir = false;
};

class File {
public:
    File() {}
    File(std::shared_ptr<FileImpl> impl) : _impl(impl) {}

    operator bool() const { return _impl && (_impl->fp || _impl->dp); }

    void close() {
        if (!_impl) return;
        if (_impl->fp) { fclose(_impl->fp); _impl->fp = nullptr; }
        if (_impl->dp) { closedir(_impl->dp); _impl->dp = nullptr; }
    }

    bool available() {
        if (!_impl || !_impl->fp) return false;
        return !feof(_impl->fp);
    }

    String readStringUntil(char delim) {
        if (!_impl || !_impl->fp) return String();
        std::string s;
        int c;
        while ((c = fgetc(_impl->fp)) != EOF) {
            if ((char)c == delim) break;
            s += (char)c;
        }
        return String(s.c_str());
    }

    int read() {
        if (!_impl || !_impl->fp) return -1;
        return fgetc(_impl->fp);
    }

    size_t write(uint8_t b) {
        if (!_impl || !_impl->fp) return 0;
        return fputc(b, _impl->fp) != EOF ? 1 : 0;
    }

    size_t write(const uint8_t* buf, size_t size) {
        if (!_impl || !_impl->fp) return 0;
        return fwrite(buf, 1, size, _impl->fp);
    }

    size_t print(const String& s) {
        if (!_impl || !_impl->fp) return 0;
        return fwrite(s.c_str(), 1, s.length(), _impl->fp);
    }

    size_t println(const String& s) {
        size_t n = print(s);
        if (_impl && _impl->fp) { fputc('\n', _impl->fp); n++; }
        return n;
    }

    size_t println() {
        if (!_impl || !_impl->fp) return 0;
        fputc('\n', _impl->fp);
        return 1;
    }

    size_t println(const char* s) { return println(String(s)); }

    bool isDirectory() const { return _impl && _impl->isDir; }

    size_t size() const {
        if (!_impl) return 0;
        if (_impl->fp) {
            long pos = ftell(_impl->fp);
            fseek(_impl->fp, 0, SEEK_END);
            long sz = ftell(_impl->fp);
            fseek(_impl->fp, pos, SEEK_SET);
            return (size_t)sz;
        }
        struct stat st;
        if (stat(_impl->path.c_str(), &st) == 0) return (size_t)st.st_size;
        return 0;
    }

    const char* name() const {
        return _impl ? _impl->virtPath.c_str() : "";
    }

    // Returns next entry in directory, or empty File when done
    File openNextFile();

private:
    std::shared_ptr<FileImpl> _impl;
};
