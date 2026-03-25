#ifdef NATIVE_SIM
// =============================================================================
// MinitelOS — Native simulation entry point
// =============================================================================
#include "globals.h"
#include "applications/firstboot.h"

// Define the global Minitel instance (replaces main.cpp's Minitel minitel(Serial2))
Minitel minitel;

static void sim_mount_fs() {
    LittleFS.begin(true);
    if (!LittleFS.exists("/root")) LittleFS.mkdir("/root");
    if (!LittleFS.exists("/home")) LittleFS.mkdir("/home");
}

int main() {
    sim_mount_fs();
    minitel.newScreen();

    if (is_first_boot()) {
        run_first_boot_setup();
    } else {
        // Assure que le MOTD existe pour les boots suivants
        if (!LittleFS.exists("/.motd")) {
            File f = LittleFS.open("/.motd", "w");
            if (f) { f.println("Bienvenue sur MinitelOS !"); f.close(); }
        }
    }

    shell();
    return 0;
}
#endif // NATIVE_SIM
