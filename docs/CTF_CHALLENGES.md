# MinitelOS CTF — Guide des Challenges

Trois challenges d'exploitation jouables sur terminal Minitel (ou simulateur natif).
Chaque challenge est indépendant. Les participants reçoivent une **fiche papier** avec le login de départ et des indices progressifs.

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

# Lancer (CTF_MODE actif : reset automatique au démarrage)
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

> `CTF_MODE` est activé dans les deux envs par défaut (`platformio.ini`).
> Au boot, tous les fichiers CTF sont recréés et les artefacts des sessions précédentes supprimés.

---

## Reset entre équipes

### Simulateur

```bash
# Option A — reset complet (recommandé)
rm -rf sim_fs/ && .pio/build/native/program

# Option B — reset partiel (plus rapide, conserve l'arborescence)
git checkout -- sim_fs/
rm -f sim_fs/tmp/loot.txt sim_fs/tmp/d_flag.txt sim_fs/home/user/motd_perso.txt
.pio/build/native/program
```

> Les deux options sont équivalentes : `CTF_MODE` recrée tout au démarrage.

### ESP32

```bash
# Simple reboot (CTF_MODE recrée tout au démarrage)
# Via bouton RESET de la carte, ou :
pio run -e MinitelOS -t upload   # reflash complet si nécessaire
```

---

## Configuration

| Paramètre | Emplacement | Valeur par défaut |
|-----------|-------------|-------------------|
| Durée CTF | `sim_fs/root/.ctf_config` | `720` (12 min) |
| Intervalle cron | `sim_fs/root/.crontab` | `30` (secondes) |
| Activer CTF_MODE | `platformio.ini` → `build_flags` | `-D CTF_MODE` |

**Ajuster la durée** (ex. 10 min) :
```bash
echo "600" > sim_fs/root/.ctf_config
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

## Informations communes

| Élément | Valeur |
|---------|--------|
| Branche git | `ctf/scenario-3-backdoor-cron` |
| Timer | commande `ctftime` — durée dans `/root/.ctf_config` |
| Soumission | Flag à rentrer sur la plateforme web CTF |

---

## Challenge 1 — La Backdoor Cron

**Difficulté** : ⭐⭐ Intermédiaire
**Durée recommandée** : 12 min
**Catégorie** : Privilege escalation via cron mal sécurisé

### Accès de départ

```
Login    : stagiaire
Password : 1234
```

### Objectif

Devenir root et lire `/root/flag.txt`.

### Chemin de résolution

```
1. ps
   → Voir : [1] 30s run /tmp/maintenance.msh

2. cat /tmp/maintenance.msh
   → Script légitime, tourne toutes les 30s en ROOT

3. edit /tmp/maintenance.msh
   → Injecter : cat /root/flag.txt > /tmp/loot.txt

4. (attendre ~30 secondes)

5. cat /tmp/loot.txt
   → FLAG{cr0n_b4ckd00r_w4s_h3r3}
```

### Indices papier (à donner progressivement)

1. *"Un processus tourne en arrière-plan en tant que root..."*
2. *"Le script de maintenance est dans /tmp. Qui peut modifier des fichiers dans /tmp ?"*
3. *"Vous pouvez écrire dans le script. Que se passe-t-il si vous y mettez une commande ?"*

### Mécanique exploitée

Script world-writable exécuté périodiquement par root (cron privilege escalation).
Équivalent réel : cron jobs avec chemins non protégés (CVE classique Unix).

---

## Challenge 2 — Le Backup Oublié

**Difficulté** : ⭐ Débutant/Intermédiaire
**Durée recommandée** : 10 min
**Catégorie** : Credential exposure + hash cracking + lateral movement

### Accès de départ

```
Login    : user
Password : 1234
```

### Objectif

Devenir root en deux étapes (user → admin → root) et lire `/root/flag_a.txt`.

### Chemin de résolution

```
1. cat TODO.txt
   → "rappeler l'admin de supprimer l'ancien backup dans /tmp"

2. cd /tmp && cat users.bak
   → Fichier de comptes : admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin

3. Craquer le hash MD5 de admin
   → Wordlist fournie sur fiche papier : [minitel, password, admin, 1234, telmini]
   → Résultat : "minitel"

4. su admin  →  password : minitel

5. cat notes_perso.txt
   → "root : T3lm1n1"

6. su root  →  password : T3lm1n1

7. cat /root/flag_a.txt
   → FLAG{b4ckup_3xp0s3d_4dm1n}
```

### Indices papier (à donner progressivement)

1. *"Il y a des fichiers intéressants dans /tmp..."*
2. *"Le format du fichier de comptes est user:MD5(password):niveau"*
3. *"L'admin a les mêmes mauvaises habitudes pour tous ses mots de passe..."*

### Mécanique exploitée

Backup de base d'utilisateurs laissé dans un répertoire world-accessible.
MD5 non salé crackable par dictionnaire. Credentials root en clair dans les notes.

### Wordlist à distribuer sur fiche

```
minitel  password  admin  1234  telmini  root  azerty  qwerty
```

---

## Challenge 3 — L'Éditeur Piégé

**Difficulté** : ⭐⭐⭐ Avancé
**Durée recommandée** : 15 min
**Catégorie** : Code injection via fonctionnalité vulnérable (logic bug)

### Accès de départ

```
Login    : user
Password : 1234
```

### Objectif

Exploiter une vulnérabilité dans la commande `motd` pour exécuter du code en root,
sans jamais connaître le mot de passe root. Lire `/root/flag_d.txt`.

### Chemin de résolution

```
1. cat README.txt
   → "Créez motd_perso.txt — les commandes shell sont exécutées automatiquement"

2. edit motd_perso.txt
   → Écrire : cat /root/flag_d.txt > /tmp/d_flag.txt

3. motd
   → La commande lit motd_perso.txt et l'exécute EN ROOT (bug d'implémentation)
   → /tmp/d_flag.txt est créé

4. cat /tmp/d_flag.txt
   → FLAG{m0td_c0d3_3x3c_ft_w}
```

### Indices papier (à donner progressivement)

1. *"Lisez attentivement votre répertoire personnel..."*
2. *"La fonctionnalité de motd personnalisé exécute les commandes. Dans quel contexte ?"*
3. *"Une commande qui 'redirige' vers /tmp peut créer un fichier lisible par tous."*

### Mécanique exploitée

La commande `motd` élève les privilèges en root pour "afficher des infos système dynamiques"
puis exécute les lignes du fichier utilisateur sans restriction.
Équivalent réel : scripts SUID mal implémentés, plugins exécutés en root.

---

## Tableau récapitulatif

| # | Nom | Login départ | Flag | Difficulté | Temps |
|---|-----|-------------|------|-----------|-------|
| 1 | Backdoor Cron | stagiaire / 1234 | `FLAG{cr0n_b4ckd00r_w4s_h3r3}` | ⭐⭐ | 12 min |
| 2 | Backup Oublié | user / 1234 | `FLAG{b4ckup_3xp0s3d_4dm1n}` | ⭐ | 10 min |
| 3 | Éditeur Piégé | user / 1234 | `FLAG{m0td_c0d3_3x3c_ft_w}` | ⭐⭐⭐ | 15 min |

---

## Arborescence sim_fs

```
sim_fs/
├── .motd                        # Bannière CTF affichée à la connexion
├── home/
│   ├── stagiaire/               # Home du joueur — Challenge 1
│   │   └── note.txt             # Indice : "droits mal configurés"
│   ├── user/                    # Home du joueur — Challenges 2 & 3
│   │   ├── TODO.txt             # Indice C2 : "backup dans /tmp"
│   │   └── README.txt           # Indice C3 : "motd supporte les commandes"
│   └── admin/                   # Home admin — accessible après su admin (C2)
│       └── notes_perso.txt      # Mot de passe root en clair
├── root/                        # Accessible root uniquement
│   ├── .users                   # Base de comptes (MD5)
│   ├── .crontab                 # Tâche cron : 30s run /tmp/maintenance.msh
│   ├── .ctf_config              # Durée CTF en secondes (720 = 12 min)
│   ├── .fsmeta                  # Permissions : maintenance.msh = rwxrwxrwx
│   ├── flag.txt                 # FLAG Challenge 1
│   ├── flag_a.txt               # FLAG Challenge 2
│   └── flag_d.txt               # FLAG Challenge 3
└── tmp/                         # World-accessible
    ├── maintenance.msh          # Script cron légitime (world-writable — C1)
    ├── maintenance.log          # Log d'exécution
    └── users.bak                # Backup de comptes exposé (C2)
```

---

## Comptes utilisateurs

| User | Password | Level | Rôle |
|------|----------|-------|------|
| `root` | T3lm1n1 | root | Admin système (flag caché) |
| `stagiaire` | 1234 | user | Joueur — Challenge 1 |
| `user` | 1234 | user | Joueur — Challenges 2 & 3 |
| `admin` | minitel | admin | Pivot — Challenge 2 |

---

## Notes organisateur

- **Rotation entre équipes** : `git checkout -- sim_fs/ && rm -f sim_fs/tmp/loot.txt sim_fs/tmp/d_flag.txt`
- **Ajuster le timer** : modifier `sim_fs/root/.ctf_config` (valeur en secondes)
- **Mode silencieux** : la commande `ctftime` est accessible à tous — les équipes peuvent consulter le temps restant à tout moment
- **Cron interval** : modifiable dans `sim_fs/root/.crontab` (valeur en secondes) — mettre `5` pour les démos rapides
