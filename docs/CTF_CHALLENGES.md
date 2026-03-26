# MinitelOS CTF — Guide des Challenges

Créneau : **15 minutes** par équipe pour enchaîner les 3 challenges.
Chaque challenge pivote vers le suivant — il n'y a qu'une seule session de jeu.

```
stagiaire/1234 → [C1 cron] → user/linux → [C2 backup] → admin/minitel → [C3 motd] → FLAG final
```

---

## Prérequis

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/) (`pip install platformio`)
- Branche : `ctf/scenario-3-backdoor-cron`

```bash
git clone <repo>
cd MinitelOS
git checkout ctf/scenario-3-backdoor-cron
```

---

## Build & lancement

### Simulateur natif (poste organisateur / démo)

```bash
# Compiler
pio run -e native

# Lancer — CTF_MODE actif : reset complet au démarrage
.pio/build/native/program
```

### ESP32 (Minitel physique)

```bash
# Compiler et flasher le firmware
pio run -e MinitelOS -t upload

# Flasher le filesystem initial (première fois seulement)
pio run -e MinitelOS -t uploadfs

# Monitor série (debug)
pio device monitor
```

> Au boot, `CTF_MODE` recrée tous les fichiers CTF et supprime les artefacts des sessions précédentes.

---

## Reset entre équipes

### Simulateur

```bash
rm -rf sim_fs/ && .pio/build/native/program
```

### ESP32

```bash
# Simple reboot (bouton RESET sur la carte)
# — CTF_MODE recrée tout au démarrage automatiquement
```

---

## Configuration

| Paramètre | Emplacement | Valeur par défaut |
|-----------|-------------|-------------------|
| Durée CTF | `sim_fs/root/.ctf_config` | `900` (15 min) |
| Intervalle cron | `sim_fs/root/.crontab` | `30` (secondes) |
| Activer CTF_MODE | `platformio.ini` → `build_flags` | `-D CTF_MODE` |

**Ajuster la durée** (ex. 12 min) :
```bash
echo "720" > sim_fs/root/.ctf_config
```

**Accélérer le cron pour démo** (5s au lieu de 30s) :
```bash
# Éditer sim_fs/root/.crontab, remplacer 30 par 5
```

**Désactiver CTF_MODE** (build MinitelOS standard) :
```bash
# Dans platformio.ini, retirer -D CTF_MODE des build_flags
```

---

## Challenge 1 — La Backdoor Cron

**Difficulté** : ⭐⭐ Intermédiaire
**Durée recommandée** : 5 min
**Pivot** : stagiaire → **user**

### Accès de départ

```
Login    : stagiaire
Password : 1234
```

### Objectif

Exploiter le cron world-writable pour lire `/root/flag1.txt` et obtenir les credentials du compte suivant.

### Chemin de résolution

```
1. ps
   → [2] 30s run /tmp/maintenance.msh  (root)

2. cat /tmp/maintenance.msh
   → Script légitime, world-writable

3. edit /tmp/maintenance.msh
   → Injecter : cat /root/flag1.txt > /tmp/loot.txt

4. (attendre ~30 secondes)

5. cat /tmp/loot.txt
   → FLAG{cr0n_2_p1v0t}

6. cat /tmp/users.bak
   → user:e206a54e97690cce50cc872dd70ee896:user

7. Craquer le hash MD5 de user (wordlist C1)
   → "linux"

8. su user  →  password : linux
```

### Indices papier (progressifs)

1. *"Un processus tourne en arrière-plan en tant que root..."*
2. *"Le script de maintenance est dans /tmp. Qui peut le modifier ?"*
3. *"Vous pouvez écrire dans le script. Que se passe-t-il si vous y mettez une commande ?"*

### Wordlist C1

```
minitel  password  linux  1234  telmini  root  azerty
```

---

## Challenge 2 — Le Backup Oublié

**Difficulté** : ⭐ Débutant/Intermédiaire
**Durée recommandée** : 4 min
**Pivot** : user → **admin**

### Accès de départ

Credentials obtenus à la fin de C1 (`su user` / `linux`).

### Objectif

Cracker le hash admin depuis le backup exposé, devenir admin, lire `/home/admin/flag2.txt`.

### Chemin de résolution

```
1. cat TODO.txt
   → "rappeler l'admin de supprimer le backup dans /tmp"

2. cat /tmp/users.bak
   → admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin

3. Craquer le hash MD5 de admin (wordlist C2)
   → "minitel"

4. su admin  →  password : minitel

5. cat flag2.txt
   → FLAG{4dm1n_4cc3ss}
```

### Indices papier (progressifs)

1. *"Il y a des fichiers intéressants dans /tmp..."*
2. *"Le format est user:MD5(password):niveau"*
3. *"L'admin a de mauvaises habitudes de mot de passe..."*

### Wordlist C2

```
minitel  password  admin  1234  telmini  root  azerty  qwerty
```

---

## Challenge 3 — L'Éditeur Piégé

**Difficulté** : ⭐⭐⭐ Avancé
**Durée recommandée** : 6 min
**Pivot** : admin → **root** (exécution)

### Accès de départ

Credentials obtenus à la fin de C2 (`su admin` / `minitel`).

### Objectif

Exploiter la fonctionnalité `motd` qui exécute les commandes en root pour lire `/root/flag3.txt` — sans jamais connaître le mot de passe root.

### Chemin de résolution

```
1. cat README.txt
   → "Les commandes shell sont exécutées en tant que SYSTEME
      pour afficher des informations dynamiques"

2. edit motd_perso.txt
   → Écrire : cat /root/flag3.txt > /tmp/pwned.txt
   → Sauvegarder (w)

3. motd
   → Exécute motd_perso.txt EN ROOT (bug d'implémentation)

4. cat /tmp/pwned.txt
   → FLAG{r00t_m0td_pwn3d}
```

### Indices papier (progressifs)

1. *"Lisez attentivement votre répertoire personnel..."*
2. *"La commande motd exécute les commandes. Dans quel contexte ?"*
3. *"Une commande qui 'redirige' vers /tmp peut créer un fichier lisible par tous."*

---

## Tableau récapitulatif

| # | Pivot | Login départ | Flag | Durée |
|---|-------|-------------|------|-------|
| 1 | stagiaire → user | stagiaire / 1234 | `FLAG{cr0n_2_p1v0t}` | ~5 min |
| 2 | user → admin | user / linux | `FLAG{4dm1n_4cc3ss}` | ~4 min |
| 3 | admin → root exec | admin / minitel | `FLAG{r00t_m0td_pwn3d}` | ~6 min |

---

## Arborescence sim_fs

```
sim_fs/
├── .motd                        # Bannière CTF
├── home/
│   ├── stagiaire/
│   │   └── note.txt             # Indice C1 : droits mal configurés
│   ├── user/
│   │   └── TODO.txt             # Indice C2 : backup dans /tmp
│   └── admin/
│       ├── flag2.txt            # FLAG Challenge 2 (admin only)
│       ├── notes_perso.txt      # Infos admin (hint motd)
│       └── README.txt           # Indice C3 : motd exécute en root
├── root/                        # Accessible root uniquement
│   ├── .users                   # Base de comptes (MD5)
│   ├── .crontab                 # Tâche cron : 30s run /tmp/maintenance.msh
│   ├── .ctf_config              # Durée CTF en secondes (900 = 15 min)
│   ├── .fsmeta                  # Permissions fichiers
│   ├── flag1.txt                # FLAG Challenge 1
│   └── flag3.txt                # FLAG Challenge 3
└── tmp/                         # World-accessible
    ├── maintenance.msh          # Script cron world-writable (vecteur C1)
    ├── maintenance.log          # Log d'exécution
    └── users.bak                # Backup comptes exposé (vecteur C2)
```

---

## Comptes utilisateurs

| User | Password | Level | Obtenu |
|------|----------|-------|--------|
| `stagiaire` | 1234 | user | Donné sur fiche papier |
| `user` | linux | user | Cracké via C1 (hash dans users.bak) |
| `admin` | minitel | admin | Cracké via C2 (hash dans users.bak) |
| `root` | T3lm1n1 | root | Jamais nécessaire — C3 bypasse le password |

---

## Notes organisateur

- **Reset entre équipes** : `rm -rf sim_fs/ && .pio/build/native/program` (simulateur) ou reboot ESP32
- **Ajuster le timer** : modifier `sim_fs/root/.ctf_config` (secondes)
- **Cron démo rapide** : mettre `5` dans `.crontab` au lieu de `30`
- **Vérifier la progression** : `ctftime` affiche le temps restant à tout moment
- **Artefacts à nettoyer** (gérés automatiquement par CTF_MODE au boot) :
  - `/tmp/loot.txt` — flag C1
  - `/tmp/pwned.txt` — flag C3
  - `/home/admin/motd_perso.txt` — injection C3
