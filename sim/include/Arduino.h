#pragma once
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>

// Arduino basic types
typedef uint8_t  byte;
typedef uint16_t word;
typedef bool     boolean;

#define HEX 16
#define DEC 10
#define OCT 8
#define BIN 2

#define LOW  0
#define HIGH 1
#define INPUT  0
#define OUTPUT 1

#define PROGMEM
#define F(x) (x)
#define PSTR(x) (x)

#define PI 3.14159265358979323846f

// Time
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void yield();
void delayMicroseconds(unsigned int us);

// Math
long map(long x, long in_min, long in_max, long out_min, long out_max);
inline long min(long a, long b) { return a < b ? a : b; }
inline long max(long a, long b) { return a > b ? a : b; }
// abs is already defined by system headers on macOS with noexcept; only define here if not already available
#if !defined(__APPLE__) && !defined(_LIBCPP_STDLIB_H)
inline long abs(long x) { return x < 0 ? -x : x; }
#endif
inline long constrain(long x, long lo, long hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float sq(float x) { return x * x; }
char* dtostrf(double val, signed char width, unsigned char prec, char* sout);

// Bit ops
#define bitRead(v,b)   (((v)>>(b))&1)
#define bitWrite(v,b,n) (n ? ((v)|=(1UL<<(b))) : ((v)&=~(1UL<<(b))))
#define bit(b) (1UL << (b))

// --- String class ---
class String {
public:
    std::string _s;

    String() {}
    String(const String& o) : _s(o._s) {}
    String(String&& o) noexcept : _s(std::move(o._s)) {}
    String(const char* s) : _s(s ? s : "") {}
    String(const std::string& s) : _s(s) {}
    String(char c) : _s(1, c) {}
    String(char c, unsigned int n) : _s(n, c) {}   // String(' ', 80)

    // Integer constructors with optional base
    String(int val, unsigned char base = 10)           { _fromInt((long)val, base); }
    String(unsigned int val, unsigned char base = 10)  { _fromUInt((unsigned long)val, base); }
    String(long val, unsigned char base = 10)          { _fromInt(val, base); }
    String(unsigned long val, unsigned char base = 10) { _fromUInt(val, base); }
    String(unsigned char val, unsigned char base = 10) { _fromUInt((unsigned long)val, base); }

    // Float constructors
    String(float val, unsigned char decimals = 2)  { _fromFloat((double)val, decimals); }
    String(double val, unsigned char decimals = 2) { _fromFloat(val, decimals); }

    // Assignments
    String& operator=(const String& o)  { _s = o._s; return *this; }
    String& operator=(String&& o) noexcept { _s = std::move(o._s); return *this; }
    String& operator=(const char* s)    { _s = s ? s : ""; return *this; }
    String& operator=(char c)           { _s = std::string(1, c); return *this; }

    // Concatenation
    String& operator+=(const String& o)  { _s += o._s; return *this; }
    String& operator+=(const char* s)    { if (s) _s += s; return *this; }
    String& operator+=(char c)           { _s += c; return *this; }
    String  operator+(const String& o)  const { return String(_s + o._s); }
    String  operator+(const char* s)    const { return String(_s + (s ? s : "")); }
    String  operator+(char c)           const { return String(_s + c); }
    friend String operator+(const char* lhs, const String& rhs) { return String(std::string(lhs ? lhs : "") + rhs._s); }

    // Comparison
    bool operator==(const String& o) const { return _s == o._s; }
    bool operator==(const char* s)   const { return _s == (s ? s : ""); }
    bool operator!=(const String& o) const { return _s != o._s; }
    bool operator!=(const char* s)   const { return _s != (s ? s : ""); }
    bool operator< (const String& o) const { return _s <  o._s; }
    bool operator<=(const String& o) const { return _s <= o._s; }
    bool operator> (const String& o) const { return _s >  o._s; }
    bool operator>=(const String& o) const { return _s >= o._s; }

    // Bool conversion
    explicit operator bool() const { return !_s.empty(); }

    // Info
    unsigned int length() const { return (unsigned int)_s.size(); }
    bool isEmpty() const        { return _s.empty(); }
    const char* c_str() const   { return _s.c_str(); }
    char charAt(unsigned int i) const { return (i < _s.size()) ? _s[i] : 0; }
    char operator[](unsigned int i) const { return charAt(i); }
    char& operator[](unsigned int i) { return _s[i]; }

    // Search
    int indexOf(char ch, unsigned int from = 0) const {
        auto p = _s.find(ch, from);
        return (p == std::string::npos) ? -1 : (int)p;
    }
    int indexOf(const String& s, unsigned int from = 0) const {
        auto p = _s.find(s._s, from);
        return (p == std::string::npos) ? -1 : (int)p;
    }
    int lastIndexOf(char ch) const {
        auto p = _s.rfind(ch);
        return (p == std::string::npos) ? -1 : (int)p;
    }
    int lastIndexOf(const String& s) const {
        auto p = _s.rfind(s._s);
        return (p == std::string::npos) ? -1 : (int)p;
    }
    int lastIndexOf(char ch, unsigned int from) const {
        auto p = _s.rfind(ch, from);
        return (p == std::string::npos) ? -1 : (int)p;
    }
    bool startsWith(const String& s) const {
        return _s.size() >= s._s.size() && _s.compare(0, s._s.size(), s._s) == 0;
    }
    bool endsWith(const String& s) const {
        if (s._s.size() > _s.size()) return false;
        return _s.compare(_s.size() - s._s.size(), s._s.size(), s._s) == 0;
    }

    // Modify
    void trim() {
        size_t b = _s.find_first_not_of(" \t\r\n");
        size_t e = _s.find_last_not_of(" \t\r\n");
        _s = (b == std::string::npos) ? "" : _s.substr(b, e - b + 1);
    }
    void toLowerCase() { for (auto& c : _s) c = tolower(c); }
    void toUpperCase() { for (auto& c : _s) c = toupper(c); }
    void replace(const String& from, const String& to) {
        if (from._s.empty()) return;
        size_t pos = 0;
        while ((pos = _s.find(from._s, pos)) != std::string::npos) {
            _s.replace(pos, from._s.size(), to._s);
            pos += to._s.size();
        }
    }
    void replace(const char* from, const char* to) {
        replace(String(from), String(to));
    }
    void remove(unsigned int index, unsigned int count = (unsigned int)-1) {
        if (index >= _s.size()) return;
        if (count == (unsigned int)-1) _s.erase(index);
        else _s.erase(index, count);
    }

    // Extract
    String substring(unsigned int from) const {
        if (from >= _s.size()) return String();
        return String(_s.substr(from));
    }
    String substring(unsigned int from, unsigned int to) const {
        if (from >= _s.size()) return String();
        return String(_s.substr(from, to - from));
    }

    // Convert
    long   toInt()   const { return strtol(_s.c_str(), nullptr, 10); }
    float  toFloat() const { return strtof(_s.c_str(), nullptr); }
    double toDouble()const { return strtod(_s.c_str(), nullptr); }

    // Reserve (noop in sim)
    bool reserve(unsigned int) { return true; }

private:
    void _fromInt(long val, unsigned char base) {
        char buf[34];
        if (base == 16) snprintf(buf, sizeof(buf), "%lx", val);
        else if (base == 8) snprintf(buf, sizeof(buf), "%lo", val);
        else snprintf(buf, sizeof(buf), "%ld", val);
        _s = buf;
    }
    void _fromUInt(unsigned long val, unsigned char base) {
        char buf[34];
        if (base == 16) snprintf(buf, sizeof(buf), "%lx", val);
        else if (base == 8) snprintf(buf, sizeof(buf), "%lo", val);
        else snprintf(buf, sizeof(buf), "%lu", val);
        _s = buf;
    }
    void _fromFloat(double val, unsigned char dec) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", (int)dec, val);
        _s = buf;
    }
};

// HardwareSerial stub
class HardwareSerial {
public:
    void begin(int baud) {}
    void begin(int baud, int config) {}
    void begin(int baud, int config, int rx, int tx) {}
    void print(const String& s) { fputs(s.c_str(), stderr); }
    void println(const String& s) { fprintf(stderr, "%s\n", s.c_str()); }
    void println() { fputc('\n', stderr); }
    int  available() { return 0; }
    int  read() { return -1; }
    size_t write(uint8_t) { return 1; }
};

extern HardwareSerial Serial;
extern HardwareSerial Serial2;

// ESP class stub (simulated heap values matching typical ESP32)
class EspClass {
public:
    uint32_t getFreeHeap()    const { return 180 * 1024; }
    uint32_t getHeapSize()    const { return 320 * 1024; }
    uint32_t getMinFreeHeap() const { return 120 * 1024; }
    uint32_t getMaxAllocHeap()const { return 160 * 1024; }
    void restart() { exit(0); }
};
extern EspClass ESP;

#include "WString.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
