#pragma once

#ifdef CTF_MODE
// Initialise (ou réinitialise) l'intégralité du filesystem CTF.
// À appeler au boot, après LittleFS.begin().
// Écrase tous les fichiers CTF et supprime les artefacts des sessions précédentes.
void ctf_fs_init();
#endif // CTF_MODE
