# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MinitelOS is an Arduino/C++ operating system for ESP32 that drives a retro French Minitel terminal (1B/2). It provides a Unix-like shell experience over a serial Minitel connection with file system access (LittleFS), user authentication, WiFi networking, SSH, scripting, and cron scheduling.

## Build & Flash

**Toolchain**: PlatformIO (required)

```bash
# Compile and flash to connected ESP32
pio run -t upload

# Compile only (no flash)
pio run

# Serial monitor (115200 baud)
pio device monitor

# Clean build
pio run -t clean
```

There is no automated test framework — testing is manual via the Minitel terminal or serial monitor.

**Debug logging**: Enable with `#define DEBUG_MON_APP` in relevant source files.

## Architecture

### Execution Flow

`main.cpp` initializes hardware (Minitel serial, LittleFS, WiFi) and user DB, then enters the shell loop in `shell.cpp`.

### Command Dispatch

All shell commands are registered as `ShellCommand { const char *name; void (*func)(const String &); }` entries in a static array in `shell.cpp`. Adding a new command means implementing a function and appending an entry to that array.

### Module Breakdown

| Module | Role |
|--------|------|
| `shell.h/cpp` | Command-line parsing, history (deque), dispatch loop |
| `shell_sys.h/cpp` | User auth (MD5), session/privilege management, system info |
| `shell_fs.h/cpp` | File operations (ls, cat, mkdir, rm, cp, mv) over LittleFS |
| `shell_edit.h/cpp` | Vi-like in-terminal text editor |
| `shell_grep.h/cpp` | Pattern matching and file search |
| `shell_script.h/cpp` | Script interpreter: variables, conditionals, loops, `run` command |
| `cron.h/cpp` | Task scheduler for timed/repeated command execution |
| `ssh/sshClient.h/cpp` | SSH client over WiFi using LibSSH-ESP32 |
| `utils.h/cpp` | Minitel I/O helpers, WiFi connect, text input primitives |
| `globals.h/cpp` | Shared global state (current user, dir, vars, session flags) |

### State & Storage

- **User database**: `/root/.users` on LittleFS (MD5-hashed passwords, privilege levels)
- **Home directories**: `/home/<user>/` per user, `/root` for root
- **Shell state**: `shell_current_dir`, `shell_vars` (string map), `shell_int_vars` (int map), command history deque — all declared in `globals.h`
- **WiFi credentials**: persisted in NVS (non-volatile storage)
- **SPIFFS/LittleFS**: 1 MB partition for user files

### Privilege Model

Three levels: `user` → `admin` → `root`. Enforced per-command in `shell_sys.cpp`. `su` and `login` switch sessions; privilege is tracked in a global session variable.

### Partition Layout

Defined in `partitions.csv`: NVS (24 KB), PHY init (4 KB), factory app (2.9 MB), SPIFFS (1 MB).

## Key Dependencies

- [`Minitel1B_Hard`](https://github.com/iodeo/Minitel1B_Hard) — Minitel terminal control (display, keyboard input)
- `ewpa/LibSSH-ESP32` — SSH client
- `marian-craciunescu/ESP32Ping` — ICMP ping

---

## Minitel1B_Hard — Référence API

**Toute interaction avec le terminal Minitel doit passer par cette librairie.** L'objet global `minitel` (instancié dans `main.cpp`) est l'unique point d'accès à l'écran et au clavier.

Librairie installée dans : `.pio/libdeps/MinitelOS/Minitel1B_Hard/`
Spec officielle : [Spécifications Techniques d'Utilisation du Minitel 1B](http://543210.free.fr/TV/stum1b.pdf)

### Initialisation (ESP32)

```cpp
#include "Minitel1B_Hard.h"
// Constructeur ESP32 avec pins RX/TX custom :
Minitel minitel(Serial2, rxPin, txPin);
// Vitesse par défaut à l'init : 1200 bauds (SERIAL_7E1)
minitel.changeSpeed(9600);  // Passer en 9600 bauds (Minitel 2 uniquement)
```

### Affichage

```cpp
minitel.print("texte");          // Affiche une String (UTF-8 → codes Minitel)
minitel.println("texte");        // Affiche + saut de ligne
minitel.println();               // Saut de ligne seul
minitel.printChar('A');          // Caractère du jeu G0
minitel.printSpecialChar(DEGRE); // Caractère du jeu G2 (voir constantes spéciales)
minitel.repeat(n);               // Répète le dernier caractère n fois
minitel.bip();                   // Signal sonore
```

### Curseur

```cpp
minitel.moveCursorXY(x, y);     // Positionnement direct (col x, rangée y)
minitel.moveCursorLeft(n);
minitel.moveCursorRight(n);
minitel.moveCursorUp(n);
minitel.moveCursorDown(n);
minitel.moveCursorReturn(n);    // Retour début de ligne + descend n rangées
minitel.getCursorX();           // Colonne courante
minitel.getCursorY();           // Rangée courante
minitel.cursor();               // Affiche le curseur
minitel.noCursor();             // Cache le curseur
```

### Effacement

```cpp
minitel.newScreen();            // Efface tout + curseur en (1,1) — reset les attributs
minitel.clearScreen();          // Efface tout (curseur non déplacé)
minitel.clearLine();            // Efface la rangée courante
minitel.clearLineFromCursor();  // Efface de la position curseur à la fin de rangée
minitel.cancel();               // Efface de la position curseur à la fin de rangée (sans déplacer)
minitel.newXY(x, y);           // Efface écran + positionne en (x,y) — reset les attributs
```

### Attributs visuels

```cpp
minitel.attributs(CARACTERE_BLANC);  // Couleur texte
minitel.attributs(FOND_BLEU);        // Couleur fond (déclenché par un espace)
minitel.attributs(DOUBLE_HAUTEUR);   // Taille
minitel.attributs(INVERSION_FOND);   // Fond inversé (évite l'espace déclencheur)
minitel.attributs(FOND_NORMAL);
minitel.attributs(CLIGNOTEMENT);
minitel.attributs(FIXE);
```

**Couleurs texte** : `CARACTERE_NOIR`, `CARACTERE_ROUGE`, `CARACTERE_VERT`, `CARACTERE_JAUNE`, `CARACTERE_BLEU`, `CARACTERE_MAGENTA`, `CARACTERE_CYAN`, `CARACTERE_BLANC`

**Couleurs fond** : `FOND_NOIR`, `FOND_ROUGE`, `FOND_VERT`, `FOND_JAUNE`, `FOND_BLEU`, `FOND_MAGENTA`, `FOND_CYAN`, `FOND_BLANC`

**Tailles** (mode texte uniquement) : `GRANDEUR_NORMALE`, `DOUBLE_HAUTEUR`, `DOUBLE_LARGEUR`, `DOUBLE_GRANDEUR`

### Mode graphique (semi-graphique G1)

```cpp
minitel.graphicMode();          // Bascule en jeu G1 (semi-graphique)
minitel.textMode();             // Retour jeu G0 (alphanumérique)
minitel.graphic(byte b, x, y); // Affiche un bloc graphique en (x,y)
// b = bitmask 6 bits 0b000000 → coins supérieur-gauche à inférieur-droit
```

### Clavier

```cpp
unsigned long key = minitel.getKeyCode();  // Lit une touche (bloquant), retourne Unicode
// Touches de fonction (comparer avec ces constantes) :
// ENVOI, RETOUR, REPETITION, GUIDE, ANNULATION, SOMMAIRE, CORRECTION, SUITE
// TOUCHE_FLECHE_HAUT, TOUCHE_FLECHE_BAS, TOUCHE_FLECHE_GAUCHE, TOUCHE_FLECHE_DROITE
// HOME, SUPPRESSION_LIGNE, INSERTION_LIGNE, SUPRESSION_CARACTERE
// CR (0x0D) = Entrée, DEL (0x7F) = Suppression

minitel.extendedKeyboard();     // Active le clavier étendu (flèches, etc.)
minitel.smallMode();            // Clavier en minuscules
minitel.capitalMode();          // Clavier en majuscules
minitel.echo(false);            // Désactive l'écho clavier à l'écran
```

### Géométrie

```cpp
minitel.rect(x1, y1, x2, y2);              // Rectangle semi-graphique
minitel.hLine(x1, y, x2, CENTER);          // Ligne horizontale (CENTER/TOP/BOTTOM)
minitel.vLine(x, y1, y2, CENTER, DOWN);    // Ligne verticale (LEFT/CENTER/RIGHT, DOWN/UP)
```

### Modes et protocole

```cpp
minitel.pageMode();                 // Mode page (pas de défilement)
minitel.scrollMode();               // Mode rouleau (défilement)
minitel.modeMixte();                // Mode Mixte 80 colonnes
minitel.modeVideotex();             // Mode Vidéotex 40 colonnes (défaut)
minitel.standardTeleinformatique(); // Standard téléinformatique 80 col
minitel.standardTeletel();          // Standard Télétel (défaut)
minitel.identifyDevice();           // Identifie le modèle de Minitel
minitel.reset();                    // Reset du Minitel
```

### Caractères spéciaux (jeu G2 — via `printSpecialChar`)

`LIVRE`, `DOLLAR`, `DIESE`, `PARAGRAPHE`, `FLECHE_GAUCHE`, `FLECHE_HAUT`, `FLECHE_DROITE`, `FLECHE_BAS`, `DEGRE`, `PLUS_OU_MOINS`, `DIVISION`, `UN_QUART`, `UN_DEMI`, `TROIS_QUART`, `OE_MAJUSCULE`, `OE_MINUSCULE`, `BETA`

### Constantes de mise en page bas niveau

`BS` (gauche), `HT` (droite), `LF` (bas), `VT` (haut), `CR` (début rangée), `FF` (efface écran + curseur origine), `RS` (curseur en (1,1) sans effacement), `CAN` (efface jusqu'en fin de rangée)

### Règles d'usage importantes

- **Changement de couleur de fond** : le fond ne change qu'à partir d'un espace (`0x20`). Pour éviter d'afficher cet espace, utiliser `INVERSION_FOND` + `FOND_NORMAL`.
- **`newScreen()` et `newXY()`** réinitialisent les attributs de visualisation — ne pas les appeler en cours d'affichage si des attributs doivent persister.
- **Mode graphique** : les attributs taille (`DOUBLE_HAUTEUR`, etc.) et `INVERSION_FOND`/`FOND_NORMAL` ne sont pas utilisables en mode graphique.
- **Vitesse série** : à l'allumage le Minitel est à 1200 bauds. `changeSpeed(9600)` est disponible sur Minitel 2 uniquement. Si les vitesses ne concordent pas, la liaison est perdue.
- **Écriture bas niveau** : préférer `print()`/`println()` pour du texte UTF-8. Utiliser `writeByte()`, `writeWord()`, `writeCode()` uniquement pour envoyer des codes de contrôle bruts.
