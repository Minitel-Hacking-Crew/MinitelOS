// =============================================================================
// MinitelOS — Native simulation layer implementation
// =============================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <curl/curl.h>

#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ESP32Ping.h"
#include "mbedtls/md5.h"

// =============================================================================
// Global instances
// =============================================================================
HardwareSerial Serial;
HardwareSerial Serial2;
WiFiClass      WiFi;
ESP32PingClass Ping;
LittleFS_class LittleFS;
EspClass       ESP;

// =============================================================================
// Arduino runtime
// =============================================================================
unsigned long millis() {
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return (unsigned long)duration_cast<milliseconds>(steady_clock::now() - start).count();
}

unsigned long micros() {
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return (unsigned long)duration_cast<microseconds>(steady_clock::now() - start).count();
}

void delay(unsigned long ms) { usleep((useconds_t)ms * 1000); }
void yield() { usleep(1000); }
void delayMicroseconds(unsigned int us) { usleep(us); }

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

char* dtostrf(double val, signed char width, unsigned char prec, char* sout) {
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%%d.%df", (int)width, (int)prec);
    snprintf(sout, 32, fmt, val);
    return sout;
}

// =============================================================================
// FS — File::openNextFile
// =============================================================================
File File::openNextFile() {
    if (!_impl || !_impl->dp) return File();
    struct dirent* ent;
    while ((ent = readdir(_impl->dp)) != nullptr) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        auto child = std::make_shared<FileImpl>();
        child->path = _impl->path + "/" + ent->d_name;

        // Build virtual path = parent virt path + "/" + name
        std::string vp = _impl->virtPath;
        if (vp.empty() || vp.back() != '/') vp += '/';
        vp += ent->d_name;
        child->virtPath = vp;

        struct stat st;
        if (stat(child->path.c_str(), &st) != 0) continue;
        child->isDir = S_ISDIR(st.st_mode);
        if (child->isDir) {
            child->dp = opendir(child->path.c_str());
        } else {
            child->fp = fopen(child->path.c_str(), "r");
        }
        return File(child);
    }
    return File();
}

// =============================================================================
// MD5 — public domain implementation (Alexei Kravchenko / RFC 1321)
// =============================================================================
extern "C" {

#define MD5_F(x,y,z) ((z)^((x)&((y)^(z))))
#define MD5_G(x,y,z) ((y)^((z)&((x)^(y))))
#define MD5_H(x,y,z) ((x)^(y)^(z))
#define MD5_I(x,y,z) ((y)^((x)|(~(z))))

#define MD5_STEP(f,a,b,c,d,x,t,s) \
    (a) += f((b),(c),(d)) + (x) + (uint32_t)(t); \
    (a)  = (((a)<<(s))|((a)>>(32-(s)))); \
    (a) += (b);

static void md5_transform(uint32_t state[4], const uint8_t* block) {
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3];
    uint32_t x[16];
    for (int i=0;i<16;i++) {
        x[i]=((uint32_t)block[i*4])|(((uint32_t)block[i*4+1])<<8)|
             (((uint32_t)block[i*4+2])<<16)|(((uint32_t)block[i*4+3])<<24);
    }
    MD5_STEP(MD5_F,a,b,c,d,x[ 0],0xd76aa478, 7) MD5_STEP(MD5_F,d,a,b,c,x[ 1],0xe8c7b756,12)
    MD5_STEP(MD5_F,c,d,a,b,x[ 2],0x242070db,17) MD5_STEP(MD5_F,b,c,d,a,x[ 3],0xc1bdceee,22)
    MD5_STEP(MD5_F,a,b,c,d,x[ 4],0xf57c0faf, 7) MD5_STEP(MD5_F,d,a,b,c,x[ 5],0x4787c62a,12)
    MD5_STEP(MD5_F,c,d,a,b,x[ 6],0xa8304613,17) MD5_STEP(MD5_F,b,c,d,a,x[ 7],0xfd469501,22)
    MD5_STEP(MD5_F,a,b,c,d,x[ 8],0x698098d8, 7) MD5_STEP(MD5_F,d,a,b,c,x[ 9],0x8b44f7af,12)
    MD5_STEP(MD5_F,c,d,a,b,x[10],0xffff5bb1,17) MD5_STEP(MD5_F,b,c,d,a,x[11],0x895cd7be,22)
    MD5_STEP(MD5_F,a,b,c,d,x[12],0x6b901122, 7) MD5_STEP(MD5_F,d,a,b,c,x[13],0xfd987193,12)
    MD5_STEP(MD5_F,c,d,a,b,x[14],0xa679438e,17) MD5_STEP(MD5_F,b,c,d,a,x[15],0x49b40821,22)
    MD5_STEP(MD5_G,a,b,c,d,x[ 1],0xf61e2562, 5) MD5_STEP(MD5_G,d,a,b,c,x[ 6],0xc040b340, 9)
    MD5_STEP(MD5_G,c,d,a,b,x[11],0x265e5a51,14) MD5_STEP(MD5_G,b,c,d,a,x[ 0],0xe9b6c7aa,20)
    MD5_STEP(MD5_G,a,b,c,d,x[ 5],0xd62f105d, 5) MD5_STEP(MD5_G,d,a,b,c,x[10],0x02441453, 9)
    MD5_STEP(MD5_G,c,d,a,b,x[15],0xd8a1e681,14) MD5_STEP(MD5_G,b,c,d,a,x[ 4],0xe7d3fbc8,20)
    MD5_STEP(MD5_G,a,b,c,d,x[ 9],0x21e1cde6, 5) MD5_STEP(MD5_G,d,a,b,c,x[14],0xc33707d6, 9)
    MD5_STEP(MD5_G,c,d,a,b,x[ 3],0xf4d50d87,14) MD5_STEP(MD5_G,b,c,d,a,x[ 8],0x455a14ed,20)
    MD5_STEP(MD5_G,a,b,c,d,x[13],0xa9e3e905, 5) MD5_STEP(MD5_G,d,a,b,c,x[ 2],0xfcefa3f8, 9)
    MD5_STEP(MD5_G,c,d,a,b,x[ 7],0x676f02d9,14) MD5_STEP(MD5_G,b,c,d,a,x[12],0x8d2a4c8a,20)
    MD5_STEP(MD5_H,a,b,c,d,x[ 5],0xfffa3942, 4) MD5_STEP(MD5_H,d,a,b,c,x[ 8],0x8771f681,11)
    MD5_STEP(MD5_H,c,d,a,b,x[11],0x6d9d6122,16) MD5_STEP(MD5_H,b,c,d,a,x[14],0xfde5380c,23)
    MD5_STEP(MD5_H,a,b,c,d,x[ 1],0xa4beea44, 4) MD5_STEP(MD5_H,d,a,b,c,x[ 4],0x4bdecfa9,11)
    MD5_STEP(MD5_H,c,d,a,b,x[ 7],0xf6bb4b60,16) MD5_STEP(MD5_H,b,c,d,a,x[10],0xbebfbc70,23)
    MD5_STEP(MD5_H,a,b,c,d,x[13],0x289b7ec6, 4) MD5_STEP(MD5_H,d,a,b,c,x[ 0],0xeaa127fa,11)
    MD5_STEP(MD5_H,c,d,a,b,x[ 3],0xd4ef3085,16) MD5_STEP(MD5_H,b,c,d,a,x[ 6],0x04881d05,23)
    MD5_STEP(MD5_H,a,b,c,d,x[ 9],0xd9d4d039, 4) MD5_STEP(MD5_H,d,a,b,c,x[12],0xe6db99e5,11)
    MD5_STEP(MD5_H,c,d,a,b,x[15],0x1fa27cf8,16) MD5_STEP(MD5_H,b,c,d,a,x[ 2],0xc4ac5665,23)
    MD5_STEP(MD5_I,a,b,c,d,x[ 0],0xf4292244, 6) MD5_STEP(MD5_I,d,a,b,c,x[ 7],0x432aff97,10)
    MD5_STEP(MD5_I,c,d,a,b,x[14],0xab9423a7,15) MD5_STEP(MD5_I,b,c,d,a,x[ 5],0xfc93a039,21)
    MD5_STEP(MD5_I,a,b,c,d,x[12],0x655b59c3, 6) MD5_STEP(MD5_I,d,a,b,c,x[ 3],0x8f0ccc92,10)
    MD5_STEP(MD5_I,c,d,a,b,x[10],0xffeff47d,15) MD5_STEP(MD5_I,b,c,d,a,x[ 1],0x85845dd1,21)
    MD5_STEP(MD5_I,a,b,c,d,x[ 8],0x6fa87e4f, 6) MD5_STEP(MD5_I,d,a,b,c,x[15],0xfe2ce6e0,10)
    MD5_STEP(MD5_I,c,d,a,b,x[ 2],0xa3014314,15) MD5_STEP(MD5_I,b,c,d,a,x[ 9],0x4e0811a1,21)
    MD5_STEP(MD5_I,a,b,c,d,x[ 6],0xf7537e82, 6) MD5_STEP(MD5_I,d,a,b,c,x[13],0xbd3af235,10)
    MD5_STEP(MD5_I,c,d,a,b,x[10],0x2ad7d2bb,15) MD5_STEP(MD5_I,b,c,d,a,x[ 7],0xeb86d391,21)
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
}

void _md5_init(_MD5_CTX* ctx) {
    ctx->a=0x67452301; ctx->b=0xefcdab89; ctx->c=0x98badcfe; ctx->d=0x10325476;
    ctx->lo=ctx->hi=0;
}

void _md5_update(_MD5_CTX* ctx, const uint8_t* data, size_t size) {
    uint32_t saved_lo = ctx->lo;
    if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo) ctx->hi++;
    ctx->hi += (uint32_t)(size >> 29);
    uint32_t used = saved_lo & 0x3f;
    if (used) {
        uint32_t available = 64 - used;
        if (size < available) { memcpy(&ctx->buffer[used], data, size); return; }
        memcpy(&ctx->buffer[used], data, available);
        data += available; size -= available;
        uint32_t state[4]={ctx->a,ctx->b,ctx->c,ctx->d};
        md5_transform(state, ctx->buffer);
        ctx->a=state[0]; ctx->b=state[1]; ctx->c=state[2]; ctx->d=state[3];
    }
    while (size >= 64) {
        uint32_t state[4]={ctx->a,ctx->b,ctx->c,ctx->d};
        md5_transform(state, data);
        ctx->a=state[0]; ctx->b=state[1]; ctx->c=state[2]; ctx->d=state[3];
        data += 64; size -= 64;
    }
    memcpy(ctx->buffer, data, size);
}

void _md5_final(uint8_t* result, _MD5_CTX* ctx) {
    uint32_t used = ctx->lo & 0x3f;
    ctx->buffer[used++] = 0x80;
    uint32_t available = 64 - used;
    if (available < 8) {
        memset(&ctx->buffer[used], 0, available);
        uint32_t state[4]={ctx->a,ctx->b,ctx->c,ctx->d};
        md5_transform(state, ctx->buffer);
        ctx->a=state[0]; ctx->b=state[1]; ctx->c=state[2]; ctx->d=state[3];
        used=0; available=64;
    }
    memset(&ctx->buffer[used], 0, available - 8);
    ctx->lo <<= 3;
    ctx->buffer[56]=ctx->lo; ctx->buffer[57]=ctx->lo>>8;
    ctx->buffer[58]=ctx->lo>>16; ctx->buffer[59]=ctx->lo>>24;
    ctx->buffer[60]=ctx->hi; ctx->buffer[61]=ctx->hi>>8;
    ctx->buffer[62]=ctx->hi>>16; ctx->buffer[63]=ctx->hi>>24;
    uint32_t state[4]={ctx->a,ctx->b,ctx->c,ctx->d};
    md5_transform(state, ctx->buffer);
    ctx->a=state[0]; ctx->b=state[1]; ctx->c=state[2]; ctx->d=state[3];
    result[0]=ctx->a; result[1]=ctx->a>>8; result[2]=ctx->a>>16; result[3]=ctx->a>>24;
    result[4]=ctx->b; result[5]=ctx->b>>8; result[6]=ctx->b>>16; result[7]=ctx->b>>24;
    result[8]=ctx->c; result[9]=ctx->c>>8; result[10]=ctx->c>>16; result[11]=ctx->c>>24;
    result[12]=ctx->d; result[13]=ctx->d>>8; result[14]=ctx->d>>16; result[15]=ctx->d>>24;
}

} // extern "C"

// =============================================================================
// Minitel ANSI mock
// =============================================================================
#include "Minitel1B_Hard.h"

static struct termios _orig_termios;
static bool _raw_mode = false;

static void _restore_terminal() {
    if (_raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &_orig_termios);
        fputs("\033[?25h", stdout); // show cursor
        fflush(stdout);
        _raw_mode = false;
    }
}

static void _enable_raw_mode() {
    if (_raw_mode) return;
    if (!isatty(STDIN_FILENO)) return; // pipe/redirect: skip raw mode
    tcgetattr(STDIN_FILENO, &_orig_termios);
    atexit(_restore_terminal);
    struct termios raw = _orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    _raw_mode = true;
}

void Minitel::newScreen() {
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
    _curX = 1; _curY = 1;
}

void Minitel::newXY(int x, int y) {
    fputs("\033[2J", stdout);
    moveCursorXY(x, y);
}

void Minitel::clearScreen()         { fputs("\033[2J", stdout); fflush(stdout); }
void Minitel::clearScreenFromCursor(){ fputs("\033[J",  stdout); fflush(stdout); }
void Minitel::clearScreenToCursor() { fputs("\033[1J", stdout); fflush(stdout); }
void Minitel::clearLine()           { fputs("\033[2K", stdout); fflush(stdout); }
void Minitel::clearLineFromCursor() { fputs("\033[K",  stdout); fflush(stdout); }
void Minitel::clearLineToCursor()   { fputs("\033[1K", stdout); fflush(stdout); }
void Minitel::cancel()              { fputs("\033[K",  stdout); fflush(stdout); }

void Minitel::deleteChars(int n) {
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%dP", n);
    fputs(buf, stdout); fflush(stdout);
}
void Minitel::insertChars(int n) {
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%d@", n);
    fputs(buf, stdout); fflush(stdout);
}
void Minitel::deleteLines(int n) {
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%dM", n);
    fputs(buf, stdout); fflush(stdout);
}
void Minitel::insertLines(int n) {
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%dL", n);
    fputs(buf, stdout); fflush(stdout);
}

void Minitel::moveCursorXY(int x, int y) {
    char buf[24]; snprintf(buf, sizeof(buf), "\033[%d;%dH", y, x);
    fputs(buf, stdout); fflush(stdout);
    _curX = x; _curY = y;
}

void Minitel::moveCursorLeft(int n) {
    if (n <= 0) return;
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%dD", n);
    fputs(buf, stdout); fflush(stdout);
    _curX -= n; if (_curX < 1) _curX = 1;
}

void Minitel::moveCursorRight(int n) {
    if (n <= 0) return;
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%dC", n);
    fputs(buf, stdout); fflush(stdout);
    _curX += n;
}

void Minitel::moveCursorUp(int n) {
    if (n <= 0) return;
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%dA", n);
    fputs(buf, stdout); fflush(stdout);
    _curY -= n; if (_curY < 1) _curY = 1;
}

void Minitel::moveCursorDown(int n) {
    if (n <= 0) return;
    char buf[16]; snprintf(buf, sizeof(buf), "\033[%dB", n);
    fputs(buf, stdout); fflush(stdout);
    _curY += n;
}

void Minitel::moveCursorReturn(int n) {
    fputc('\r', stdout);
    _curX = 1;
    moveCursorDown(n);
}

int Minitel::getCursorX() { return _curX; }
int Minitel::getCursorY() { return _curY; }

void Minitel::attributs(byte attr) {
    char buf[16];
    if (attr >= 0x40 && attr <= 0x47) {
        snprintf(buf, sizeof(buf), "\033[%dm", 30 + (attr - 0x40));
    } else if (attr >= 0x50 && attr <= 0x57) {
        snprintf(buf, sizeof(buf), "\033[%dm", 40 + (attr - 0x50));
    } else {
        switch(attr) {
            case 0x4C: strcpy(buf, "\033[0m");  break; // GRANDEUR_NORMALE
            case 0x4D: strcpy(buf, "\033[1m");  break; // DOUBLE_HAUTEUR
            case 0x4E: strcpy(buf, "\033[1m");  break; // DOUBLE_LARGEUR
            case 0x4F: strcpy(buf, "\033[1m");  break; // DOUBLE_GRANDEUR
            case 0x48: strcpy(buf, "\033[5m");  break; // CLIGNOTEMENT
            case 0x49: strcpy(buf, "\033[25m"); break; // FIXE
            case 0x5C: strcpy(buf, "\033[27m"); break; // FOND_NORMAL
            case 0x5D: strcpy(buf, "\033[7m");  break; // INVERSION_FOND
            case 0x5A: strcpy(buf, "\033[4m");  break; // DEBUT_LIGNAGE
            case 0x59: strcpy(buf, "\033[24m"); break; // FIN_LIGNAGE
            default:   return;
        }
    }
    fputs(buf, stdout); fflush(stdout);
}

void Minitel::print(String s) {
    fputs(s.c_str(), stdout); fflush(stdout);
    _curX += (int)s.length();
    _lastChar = s.isEmpty() ? ' ' : s.c_str()[s.length()-1];
}

void Minitel::println(String s) {
    fputs(s.c_str(), stdout);
    fputc('\n', stdout);
    fflush(stdout);
    _curX = 1; _curY++;
    _lastChar = '\n';
}

void Minitel::println() {
    fputc('\n', stdout); fflush(stdout);
    _curX = 1; _curY++;
}

void Minitel::printChar(char c) {
    fputc(c, stdout); fflush(stdout);
    _curX++; _lastChar = c;
}

void Minitel::printSpecialChar(byte b) {
    // Map a few common special chars
    const char* s = "?";
    switch(b) {
        case 0x30: s = "\xc2\xb0"; break;  // degree sign (UTF-8)
        case 0x31: s = "\xc2\xb1"; break;  // plus-minus
        case 0x38: s = "\xc3\xb7"; break;  // division
        case 0x3C: s = "\xc2\xbc"; break;  // 1/4
        case 0x3D: s = "\xc2\xbd"; break;  // 1/2
        case 0x3E: s = "\xc2\xbe"; break;  // 3/4
        case 0x23: s = "\xc2\xa3"; break;  // pound
        default: break;
    }
    fputs(s, stdout); fflush(stdout);
    _curX++;
}

void Minitel::writeByte(byte b)        { fputc(b, stdout); fflush(stdout); }
void Minitel::writeWord(word w)        { fputc((w>>8)&0xFF, stdout); fputc(w&0xFF, stdout); fflush(stdout); }
void Minitel::writeCode(unsigned long) {}

void Minitel::repeat(int n) {
    for (int i = 0; i < n; i++) fputc(_lastChar, stdout);
    fflush(stdout);
    _curX += n;
}

void Minitel::graphic(byte b, int x, int y) { moveCursorXY(x, y); graphic(b); }
void Minitel::graphic(byte b) {
    // Print a braille-like representation of the 6-bit block
    const char* blocks[] = {" ","\xe2\x96\x97","\xe2\x96\x96","\xe2\x96\x84",
                             "\xe2\x96\x9d","\xe2\x96\x90","\xe2\x96\x9e","\xe2\x96\x9f",
                             "\xe2\x96\x98","\xe2\x96\x9a","\xe2\x96\x8c","\xe2\x96\x99",
                             "\xe2\x96\x80","\xe2\x96\x9c","\xe2\x96\x9b","\xe2\x96\x88"};
    int idx = ((b>>5)&1)|((b>>3)&2)|((b>>1)&4)|((b<<1)&8);
    fputs(blocks[idx & 0xF], stdout); fflush(stdout);
}

void Minitel::rect(int x1, int y1, int x2, int y2) {
    for (int y = y1; y <= y2; y++) {
        moveCursorXY(x1, y);
        for (int x = x1; x <= x2; x++)
            fputc((y==y1||y==y2||x==x1||x==x2) ? '+' : ' ', stdout);
    }
    fflush(stdout);
}

void Minitel::hLine(int x1, int y, int x2, int) {
    moveCursorXY(x1, y);
    for (int x = x1; x <= x2; x++) fputc('-', stdout);
    fflush(stdout);
}

void Minitel::vLine(int x, int y1, int y2, int, int) {
    for (int y = y1; y <= y2; y++) { moveCursorXY(x, y); fputc('|', stdout); }
    fflush(stdout);
}

// ── Keyboard ─────────────────────────────────────────────────────────────────
// Key mapping guide (printed on startup):
//   Arrow keys     → Minitel arrows
//   Enter          → ENVOI (0x1341)
//   Backspace/Del  → CORRECTION (0x1347)
//   ESC            → SOMMAIRE (0x1346)
//   Tab            → GUIDE (0x1344)
//   F1             → ENVOI
//   F2             → RETOUR
//   F5             → ANNULATION
//   Ctrl+C         → exit simulator

unsigned long Minitel::getKeyCode(bool unicode) {
    _enable_raw_mode();

    unsigned char c;
    int n = (int)read(STDIN_FILENO, &c, 1);
    if (n < 0) return 0;
    if (n == 0) {
        // EOF on pipe/redirect: exit cleanly
        if (!isatty(STDIN_FILENO)) { _restore_terminal(); exit(0); }
        return 0;
    }

    if (c == 0x03) { _restore_terminal(); exit(0); } // Ctrl+C
    if (c == '\r' || c == '\n') return (unsigned long)CR;
    if (c == 127 || c == 8)     return (unsigned long)CORRECTION;
    if (c == '\t')               return (unsigned long)GUIDE;

    if (c == 0x1B) {
        // Read rest of escape sequence with short timeout
        if (!isatty(STDIN_FILENO)) return (unsigned long)SOMMAIRE;
        struct termios t = _orig_termios;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN] = 0; t.c_cc[VTIME] = 1; // 100ms timeout
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);

        unsigned char seq[6] = {0};
        int sn = (int)read(STDIN_FILENO, seq, sizeof(seq));

        // Restore raw mode
        struct termios raw = _orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG);
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        if (sn == 0) return (unsigned long)SOMMAIRE; // bare ESC

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return (unsigned long)TOUCHE_FLECHE_HAUT;
                case 'B': return (unsigned long)TOUCHE_FLECHE_BAS;
                case 'C': return (unsigned long)TOUCHE_FLECHE_DROITE;
                case 'D': return (unsigned long)TOUCHE_FLECHE_GAUCHE;
                case 'H': return (unsigned long)HOME;
                case '3': return (unsigned long)SUPRESSION_CARACTERE;
                // Function keys: \033[11~ F1, \033[12~ F2...
                case '1': if(seq[2]=='1'&&seq[3]=='~') return (unsigned long)ENVOI;
                          if(seq[2]=='2'&&seq[3]=='~') return (unsigned long)RETOUR;
                          if(seq[2]=='5'&&seq[3]=='~') return (unsigned long)ANNULATION;
                          if(seq[2]=='7'&&seq[3]=='~') return (unsigned long)SOMMAIRE;
                          if(seq[2]=='8'&&seq[3]=='~') return (unsigned long)SUITE;
                          break;
            }
        }
        if (seq[0] == 'O') { // SS3 sequences
            switch (seq[1]) {
                case 'P': return (unsigned long)ENVOI;       // F1
                case 'Q': return (unsigned long)RETOUR;      // F2
                case 'R': return (unsigned long)REPETITION;  // F3
                case 'S': return (unsigned long)GUIDE;       // F4
            }
        }
        return 0;
    }

    if (c >= 32 && c <= 126) return (unsigned long)c;
    return 0;
}

// =============================================================================
// HTTPClient — pont libcurl pour le simulateur natif
// =============================================================================
#include "HTTPClient.h"

size_t HTTPClient::_write_cb(void *data, size_t size, size_t nmemb, void *userp) {
    auto *buf = reinterpret_cast<std::string *>(userp);
    buf->append(reinterpret_cast<char *>(data), size * nmemb);
    return size * nmemb;
}

HTTPClient::~HTTPClient() {
    end();
}

bool HTTPClient::begin(WiFiClientSecure &, const String &url) {
    return begin(url);
}

bool HTTPClient::begin(const String &url) {
    _url = url.c_str();
    _response.clear();
    _postData.clear();
    if (_headers) { curl_slist_free_all(_headers); _headers = nullptr; }
    return !_url.empty();
}

void HTTPClient::addHeader(const String &name, const String &value) {
    std::string h = std::string(name.c_str()) + ": " + value.c_str();
    _headers = curl_slist_append(_headers, h.c_str());
}

int HTTPClient::_perform(bool isPost) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    curl_easy_setopt(curl, CURLOPT_URL,            _url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  _write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &_response);

    if (_headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, _headers);

    if (isPost) {
        curl_easy_setopt(curl, CURLOPT_POST,          1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    _postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)_postData.size());
    }

    CURLcode res = curl_easy_perform(curl);
    long code = -1;
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_easy_cleanup(curl);
    _lastCode = (int)code;
    return _lastCode;
}

int HTTPClient::GET() {
    return _perform(false);
}

int HTTPClient::POST(const String &data) {
    _postData = data.c_str();
    return _perform(true);
}

String HTTPClient::getString() {
    return String(_response.c_str());
}

void HTTPClient::end() {
    if (_headers) { curl_slist_free_all(_headers); _headers = nullptr; }
    _response.clear();
    _postData.clear();
}
