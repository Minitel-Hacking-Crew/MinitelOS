#pragma once
#include "Arduino.h"

// ── Attribute constants (from real library) ──────────────────────────────────
#define CARACTERE_NOIR    0x40
#define CARACTERE_ROUGE   0x41
#define CARACTERE_VERT    0x42
#define CARACTERE_JAUNE   0x43
#define CARACTERE_BLEU    0x44
#define CARACTERE_MAGENTA 0x45
#define CARACTERE_CYAN    0x46
#define CARACTERE_BLANC   0x47

#define FOND_NOIR         0x50
#define FOND_ROUGE        0x51
#define FOND_VERT         0x52
#define FOND_JAUNE        0x53
#define FOND_BLEU         0x54
#define FOND_MAGENTA      0x55
#define FOND_CYAN         0x56
#define FOND_BLANC        0x57

#define GRANDEUR_NORMALE  0x4C
#define DOUBLE_HAUTEUR    0x4D
#define DOUBLE_LARGEUR    0x4E
#define DOUBLE_GRANDEUR   0x4F
#define CLIGNOTEMENT      0x48
#define FIXE              0x49
#define MASQUAGE          0x58
#define DEMASQUAGE        0x5F
#define FIN_LIGNAGE       0x59
#define DEBUT_LIGNAGE     0x5A
#define FOND_NORMAL       0x5C
#define INVERSION_FOND    0x5D

// ── Special characters ────────────────────────────────────────────────────────
#define LIVRE             0x23
#define DOLLAR            0x24
#define DIESE             0x26
#define PARAGRAPHE        0x27
#define FLECHE_GAUCHE     0x2C
#define FLECHE_HAUT       0x2D
#define FLECHE_DROITE     0x2E
#define FLECHE_BAS        0x2F
#define DEGRE             0x30
#define PLUS_OU_MOINS     0x31
#define DIVISION          0x38
#define UN_QUART          0x3C
#define UN_DEMI           0x3D
#define TROIS_QUART       0x3E
#define OE_MAJUSCULE      0x6A
#define OE_MINUSCULE      0x7A
#define BETA              0x7B
#define ACCENT_GRAVE      0x41
#define ACCENT_AIGU       0x42
#define ACCENT_CIRCONFLEXE 0x43
#define TREMA             0x48
#define CEDILLE           0x4B

// ── Layout codes ──────────────────────────────────────────────────────────────
#define BS   0x08
#define HT   0x09
#define LF   0x0A
#define VT   0x0B
#define CR   0x0D
#define RS   0x1E
#define FF   0x0C
#define US   0x1F
#define CAN  0x18
#define REP  0x12
#define NUL  0x00
#define SP   0x20
#define DEL  0x7F
#define BEL  0x07
#define SO   0x0E
#define SI   0x0F
#define SS2  0x19
#define ESC  0x1B
#define CON  0x11
#define COFF 0x14

// ── Keyboard codes ─────────────────────────────────────────────────────────────
#define ENVOI          0x1341
#define RETOUR         0x1342
#define REPETITION     0x1343
#define GUIDE          0x1344
#define ANNULATION     0x1345
#define SOMMAIRE       0x1346
#define CORRECTION     0x1347
#define SUITE          0x1348
#define CONNEXION_FIN  0x1359

#define TOUCHE_FLECHE_HAUT         0x1B5B41
#define SUPPRESSION_LIGNE          0x1B5B4D
#define TOUCHE_FLECHE_BAS          0x1B5B42
#define INSERTION_LIGNE            0x1B5B4C
#define TOUCHE_FLECHE_DROITE       0x1B5B43
#define DEBUT_INSERTION_CARACTERE  0x1B5B3468
#define FIN_INSERTION_CARACTERE    0x1B5B346C
#define TOUCHE_FLECHE_GAUCHE       0x1B5B44
#define SUPRESSION_CARACTERE       0x1B5B50
#define HOME                       0x1B5B4B
#define EFFACEMENT_PAGE            0x1B5B324A

// ── Protocol constants ────────────────────────────────────────────────────────
#define CODE_EMISSION_ECRAN        0x50
#define CODE_EMISSION_CLAVIER      0x51
#define CODE_EMISSION_MODEM        0x52
#define CODE_EMISSION_PRISE        0x53
#define CODE_RECEPTION_ECRAN       0x58
#define CODE_RECEPTION_CLAVIER     0x59
#define CODE_RECEPTION_MODEM       0x5A
#define CODE_RECEPTION_PRISE       0x5B
#define AIGUILLAGE_OFF             0x60
#define AIGUILLAGE_ON              0x61
#define TO                         0x62
#define FROM                       0x63
#define ENQROM                     0x7B
#define CONNEXION                  0x68
#define DECONNEXION                0x67
#define PROG                       0x6B
#define STATUS_VITESSE             0x74
#define ETEN                       0x41
#define ROULEAU                    0x43
#define START                      0x69
#define STOP                       0x6A
#define MINUSCULES                 0x45
#define MIXTE1                     0x327D
#define MIXTE2                     0x327E
#define TELINFO                    0x317D
#define RESET                      0x7F

// ── Geometry helpers ─────────────────────────────────────────────────────────
#define CENTER  0
#define TOP     1
#define BOTTOM  2
#define LEFT    3
#define RIGHT   4
#define UP      5
#define DOWN    6

class HardwareSerial;

class Minitel {
public:
    // Constructors
    Minitel() {}
    Minitel(HardwareSerial&) {}
    Minitel(HardwareSerial&, int8_t, int8_t) {}

    // Initialization / mode (no-ops or ANSI equivalents)
    int  changeSpeed(int baud)    { return baud; }
    int  currentSpeed()           { return 9600; }
    int  searchSpeed()            { return 9600; }
    byte pageMode()               { return 0; }
    byte scrollMode()             { return 0; }
    byte modeMixte()              { return 0; }
    byte modeVideotex()           { return 0; }
    byte standardTeleinformatique() { return 0; }
    byte standardTeletel()        { return 0; }
    byte smallMode()              { return 0; }
    byte capitalMode()            { return 0; }
    byte extendedKeyboard()       { return 0; }
    byte standardKeyboard()       { return 0; }
    byte echo(boolean)            { return 0; }
    byte reset()                  { return 0; }
    unsigned long identifyDevice() { return 0x35; }
    byte aiguillage(boolean, byte, byte) { return 0; }
    byte statusAiguillage(byte)   { return 0; }
    byte connexion(boolean)       { return 0; }

    // Text modes (no-ops in ANSI terminal)
    void textMode()    {}
    void graphicMode() {}
    void cursor()   { _ansi("\033[?25h"); }
    void noCursor() { _ansi("\033[?25l"); }

    // Screen / cursor
    void newScreen();
    void newXY(int x, int y);
    void clearScreen();
    void clearScreenFromCursor();
    void clearScreenToCursor();
    void clearLine();
    void clearLineFromCursor();
    void clearLineToCursor();
    void cancel();
    void deleteChars(int n);
    void insertChars(int n);
    void startInsert()  {}
    void stopInsert()   {}
    void deleteLines(int n);
    void insertLines(int n);

    void moveCursorXY(int x, int y);
    void moveCursorLeft(int n);
    void moveCursorRight(int n);
    void moveCursorUp(int n);
    void moveCursorDown(int n);
    void moveCursorReturn(int n);
    int  getCursorX();
    int  getCursorY();

    // Output
    void attributs(byte attr);
    void print(String s);
    void println(String s);
    void println();
    void printChar(char c);
    void printSpecialChar(byte b);
    void writeByte(byte b);
    void writeWord(word w);
    void writeCode(unsigned long code);
    void repeat(int n);
    void bip() { _ansi("\007"); }

    // Graphics
    void graphic(byte b, int x, int y);
    void graphic(byte b);
    void rect(int x1, int y1, int x2, int y2);
    void hLine(int x1, int y, int x2, int pos);
    void vLine(int x, int y1, int y2, int pos, int sens);

    // Keyboard
    unsigned long getKeyCode(bool unicode = true);

    // String helpers
    String getString(unsigned long code) { return String((char)code); }
    int    getNbBytes(unsigned long)     { return 1; }
    byte   getCharByte(char c)           { return (byte)c; }

private:
    int _curX = 1, _curY = 1;
    char _lastChar = ' ';

    void _ansi(const char* seq) { fputs(seq, stdout); fflush(stdout); }
};
