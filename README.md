# MinitelOS

![Version](https://img.shields.io/badge/version-2.3-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-green)
![Framework](https://img.shields.io/badge/framework-Arduino%20%2F%20PlatformIO-orange)

Système d'exploitation Unix-like pour ESP32 + terminal Minitel 1B/2, développé par la **Minitel Hacking Crew**.

---

## Description

MinitelOS transforme un ESP32 couplé à un Minitel en un vrai terminal interactif : shell complet, système de fichiers, gestion d'utilisateurs, réseau WiFi, SSH, scripting, cron, et simulateur natif pour développer sans matériel.

---

## Démarrage rapide

### Matériel requis
- ESP32 (devboard compatible Minitel)
- Minitel 1B ou 2

### Prérequis logiciel
- [PlatformIO](https://platformio.org/) (CLI ou extension VSCode)

### Flash ESP32
```bash
pio run -t upload       # compile + flash
pio device monitor      # moniteur série (115200 bauds)
```

### Simulateur natif (sans matériel)
```bash
pio run -e native                         # compile le simulateur
.pio/build/native/program                 # vitesse normale
.pio/build/native/program --baud=1200     # simulation vitesse Minitel 1B (1200 bauds)
.pio/build/native/program --baud=4800     # simulation vitesse Minitel 2
```

Le simulateur monte `sim_fs/` comme système de fichiers, simule le WiFi (internet réel via libcurl), et émule le terminal Minitel dans le terminal ANSI.

L'argument `--baud` (ou `-b`) ralentit l'affichage des caractères pour reproduire fidèlement la vitesse série d'un vrai Minitel. Seuls les caractères visibles sont throttlés — les séquences de contrôle (curseur, effacement) restent instantanées.

---

## Premier démarrage

Au premier boot (ou après réinitialisation), un assistant de configuration se lance :

1. **Mot de passe root** — définit le compte administrateur
2. **Création d'utilisateur** — nom, mot de passe, niveau (user / admin)
3. **WiFi** — scan et connexion optionnelle
4. **MOTD** — message affiché à chaque connexion

> Réinitialisation : appuyer 7× sur `CORRECTION` au démarrage (efface `/etc/shadow` et redémarre).

---

## Shell — Référence des commandes

### Système
| Commande | Description |
|----------|-------------|
| `help` | Aide complète |
| `whoami` | Utilisateur actuel |
| `passwd` | Changer son mot de passe |
| `su <user>` | Changer d'utilisateur |
| `sudo <cmd>` | Élévation temporaire root (sudoers ou mot de passe root) |
| `adduser` | Créer un utilisateur (root) |
| `deluser` | Supprimer un utilisateur (root) |
| `groupadd <nom>` | Créer un groupe (root) |
| `groupdel <nom>` | Supprimer un groupe (root) |
| `groupmem <groupe> <user>` | Ajouter un utilisateur à un groupe (root) |
| `groupmem -d <groupe> <user>` | Retirer un utilisateur d'un groupe (root) |
| `groups [user]` | Lister les groupes d'un utilisateur |
| `sudoedit list` | Afficher les règles sudoers (root) |
| `sudoedit add <sujet> <cmds>` | Ajouter une règle sudoers (root) |
| `sudoedit del <num>` | Supprimer une règle sudoers (root) |
| `login` / `logout` / `exit` | Connexion / déconnexion |
| `reboot` | Redémarrer l'ESP32 |
| `clear` | Effacer l'écran |
| `version` | Version du système |
| `env` | Variables d'environnement |
| `export VAR=val` | Définir une variable |
| `date` | Date et heure système |
| `uptime` | Temps de fonctionnement |
| `free` | Mémoire heap ESP32 |
| `ps` | Processus actifs + tâches cron |
| `kill <index>` | Supprimer une tâche cron |
| `motd` | Afficher le message du jour (+ exécute `~/motd_perso.txt` si présent) |
| `motd --help` | Aide détaillée sur la commande motd |
| `motd -s <msg>` | Définir le message du jour système (root) |

### Fichiers
| Commande | Description |
|----------|-------------|
| `ls` | Lister le répertoire courant |
| `ls -l` | Listing long : permissions, propriétaire, taille |
| `cd <dir>` | Changer de répertoire |
| `pwd` | Répertoire courant |
| `cat <fichier>` | Afficher un fichier |
| `edit <fichier>` | Éditeur de texte intégré |
| `mkdir <dir>` | Créer un dossier |
| `rm <fichier>` | Supprimer un fichier |
| `cp <src> <dst>` | Copier |
| `mv <src> <dst>` | Déplacer / renommer |
| `touch <fichier>` | Créer un fichier vide |
| `grep <motif>` | Recherche dans les fichiers |
| `head` / `tail` | Début / fin d'un fichier |
| `chmod <mode> <fichier>` | Changer les permissions (`644`, `rwxr-xr-x`…) |
| `chown <user>[:<group>] <fichier>` | Changer le propriétaire |
| `df` | Espace disque LittleFS (barre de progression) |
| `du [path]` | Taille d'un fichier ou répertoire |


### Réseau
| Commande | Description |
|----------|-------------|
| `ifconfig` | IP, masque, gateway, DNS, MAC, RSSI |
| `ping <host>` | Test de connectivité ICMP |
| `nslookup <host>` | Résolution DNS |
| `curl <url>` | Requête HTTP/HTTPS |
| `wifi <ssid> <pass>` | Connexion WiFi |
| `ssh <user@host>` | Client SSH |
| `sleep <sec>` | Pause (float accepté, ex: `sleep 0.5`) |

### Automatisation
| Commande | Description |
|----------|-------------|
| `crontab` | Éditer les tâches cron (`/root/.crontab`) |
| `cronpause` / `cronresume` | Suspendre / reprendre le cron |

Format crontab :
```
# intervalle_secondes  commande
60 echo tick
300 curl http://exemple.fr/ping
```

---

## Scripting — mShell (.msh)

Exécution : `run monscript.msh`

### Variables
```bash
int compteur = 0
float temp = 20.5
string nom = "alice"
bool actif = true
x = 42              # auto-typage
```

### Arithmétique (left op right correct)
```bash
int res = $a + $b   # avec $
int res = a + b     # ou sans $ (rétrocompat)
# opérateurs : + - * / %
```

### Conditions
```bash
if score ge 90
  echo excellent
else
  if score ge 60
    echo bien
  endif
endif
# opérateurs int : eq ne gt lt ge le
# opérateurs string : seq sne
```

### Boucles
```bash
for i 1 10          # for i $debut $fin aussi supporté
  echo $i
endfor

while cnt lt 100
  cnt = $cnt * 2
  if cnt gt 50
    break
  endif
endwhile
```

### Redirection
```bash
echo rapport > /root/rapport.txt
date >> /root/rapport.txt
cat /root/rapport.txt
```

### Fonctions

Les fonctions peuvent être déclarées n'importe où dans le fichier (avant ou après leur appel).

```bash
func saluer
    echo Bonjour $1 !
    echo Vous avez $2 ans.
endfunc

func max
    if $1 gt $2
        echo $1
    else
        echo $2
    endif
endfunc

call saluer Alice 30
call max 42 17
```

**Paramètres** : disponibles en `$1`, `$2`, … `$argc` (nombre d'arguments).
Les variables globales sont partagées avec les fonctions. `$1`…`$9` et `$argc` sont sauvegardés/restaurés automatiquement entre appels.

### Mots-clés spéciaux
- `break` — sort d'une boucle
- `exit` — arrête le script
- `return` — sort de la fonction courante (ne stoppe pas le script parent)
- `read VAR` — lecture utilisateur (no-op en mode pipe)

---

## Architecture

```
src/
├── main.cpp                   # Entrée ESP32
├── native_main.cpp            # Entrée simulateur natif
├── globals.h / globals.cpp    # État global partagé
├── applications/
│   ├── shell.h / shell.cpp    # Dispatch commandes, historique
│   ├── firstboot.h / .cpp     # Assistant premier démarrage
│   ├── shell_apps/
│   │   ├── shell_sys          # Auth, users, session, help
│   │   ├── shell_fs           # Fichiers (ls, cat, cp…)
│   │   ├── shell_edit         # Éditeur vi-like
│   │   ├── shell_grep         # Recherche
│   │   ├── shell_script       # Interpréteur .msh
│   │   └── shell_extra        # env, date, ps, sudo, df, du…
│   ├── cron/                  # Planificateur de tâches
│   └── ssh/                   # Client SSH (LibSSH-ESP32)
└── utils/                     # I/O Minitel, WiFi, saisie

sim/
├── include/                   # Stubs POSIX : Arduino, LittleFS, WiFi…
└── src/sim_impl.cpp           # Implémentation simulateur

sim_fs/                        # Système de fichiers simulé (monté à la racine)
├── etc/
│   └── shadow                 # Base d'authentification
├── root/
│   ├── demo.msh               # Script de démonstration
│   ├── test_script.msh        # Suite de tests
│   ├── .crontab               # Tâches cron
│   └── .fsmeta                # Permissions chmod/chown
└── home/
```

### Modèle de privilèges

#### Niveaux d'accès

| Niveau | Description |
|--------|-------------|
| `user` | Utilisateur standard — accès à son home uniquement |
| `admin` | Accès étendu — peut gérer certains fichiers système |
| `root` | Accès total — toutes les commandes, tous les fichiers |

#### Changement de session

| Commande | Comportement |
|----------|-------------|
| `su <user>` | Switch permanent vers un autre utilisateur (demande son mot de passe, sauf si déjà root) |
| `sudo <cmd>` | Élévation temporaire root pour une commande — vérifie `/etc/sudoers` en premier (sans mot de passe si règle match), sinon demande le mot de passe root |

#### Permissions fichiers

Les permissions suivent le modèle Unix (`rwxrwxrwx` : propriétaire / groupe / autres).

- `chmod <mode> <fichier>` — modifiable par le propriétaire ou root (`644`, `rwxr-xr-x`…)
- `chown <user>[:<group>] <fichier>` — changer le propriétaire (root) ou le groupe (propriétaire, si membre du groupe cible)
- Les métadonnées sont stockées dans `/root/.fsmeta`

#### Groupes

Les groupes contrôlent les bits groupe des permissions fichiers.

Groupes intégrés (non modifiables) :
| Groupe | Membres |
|--------|---------|
| `user` | Tous les utilisateurs connectés |
| `admin` | Utilisateurs de niveau `admin` et `root` |
| `root` | Uniquement `root` |
| `<username>` | Groupe personnel — uniquement cet utilisateur |

Groupes personnalisés : stockés dans `/root/.groups` (format : `nom:user1,user2,...`), gérés via `groupadd` / `groupdel` / `groupmem` (root uniquement).

#### Sudoers

Format de `/etc/sudoers` — une règle par ligne :
```
# Autoriser alice à exécuter cat et ls sans mot de passe
alice cat,ls
# Autoriser tous les membres du groupe devs à tout faire
%devs ALL
# Autoriser bob à tout faire
bob ALL
```

Géré via `sudoedit add` / `sudoedit del` / `sudoedit list` (root uniquement).

### Stockage
| Fichier | Contenu |
|---------|---------|
| `/etc/shadow` | `user:MD5(pass):level` |
| `/etc/sudoers` | Règles sudo : `user\|%group ALL\|cmd1,cmd2` |
| `/root/.groups` | Groupes personnalisés : `nom:user1,user2,...` |
| `/root/.crontab` | Tâches cron |
| `/root/.fsmeta` | Permissions chmod/chown |
| `/.motd` | Message du jour |
| `/home/<user>/.msh_history` | Historique commandes |

---

## Ajouter une commande

1. **Déclarer** dans le header approprié (`shell_extra.h`, `shell_sys.h`…) :
```cpp
void shell_ma_commande(const String &args);
```

2. **Implémenter** dans le `.cpp` correspondant :
```cpp
void shell_ma_commande(const String &args) {
    if (args.isEmpty()) { shell_println_wrapped("Usage: ma_commande <arg>"); return; }
    shell_println_wrapped("Résultat : " + args);
}
```

3. **Enregistrer** dans `shell.cpp` :
```cpp
{"ma_commande", shell_ma_commande},
```

---

## Dépendances

| Bibliothèque | Rôle |
|-------------|------|
| `Minitel1B_Hard` | Contrôle terminal Minitel (affichage, clavier) |
| `LibSSH-ESP32` | Client SSH |
| `ESP32Ping` | Ping ICMP |
| `mbedTLS` | Hash MD5 (mots de passe) |
| `libcurl` | HTTP/HTTPS (simulateur uniquement) |

---

## Contributeurs

- LL7Baucarré
- 0b3ud

## Licence

GPL-3.0
