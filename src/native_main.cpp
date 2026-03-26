#ifdef NATIVE_SIM
// =============================================================================
// MinitelOS — Native simulation entry point
// =============================================================================
#include "globals.h"
#include "applications/firstboot.h"
#include "applications/ctf/ctf_init.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

extern void sim_set_baud(int baud);

// Define the global Minitel instance (replaces main.cpp's Minitel minitel(Serial2))
Minitel minitel;

static void sim_mount_fs() {
    LittleFS.begin(true);
    if (!LittleFS.exists("/root")) LittleFS.mkdir("/root");
    if (!LittleFS.exists("/home")) LittleFS.mkdir("/home");
}

static int parse_baud(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        // --baud=1200
        if (strncmp(argv[i], "--baud=", 7) == 0)
            return atoi(argv[i] + 7);
        // -b 1200
        if ((strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--baud") == 0) && i + 1 < argc)
            return atoi(argv[++i]);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int baud = parse_baud(argc, argv);
    if (baud > 0) {
        sim_set_baud(baud);
        fprintf(stderr, "[sim] baud rate: %d (%d ms/char)\n", baud, 10 * 1000 / baud);
    }
    sim_mount_fs();
    minitel.newScreen();

#ifdef CTF_MODE
    ctf_fs_init();  // doit précéder is_first_boot() — crée .users et l'arborescence
#else
    if (is_first_boot()) {
        run_first_boot_setup();
    } else {
        // Assure que le MOTD existe pour les boots suivants
        if (!LittleFS.exists("/.motd")) {
            File f = LittleFS.open("/.motd", "w");
            if (f) { f.println("Bienvenue sur MinitelOS !"); f.close(); }
        }
    }
#endif

    shell();
    return 0;
}
#endif // NATIVE_SIM
