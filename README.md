# MHC-OS - Système d'Exploitation pour ESP32 et Minitel

![Version](https://img.shields.io/badge/version-1.0a-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-green)
![Framework](https://img.shields.io/badge/framework-Arduino-orange)

Statistiques PlatformIO

<img width="356" height="161" alt="image" src="https://github.com/user-attachments/assets/e7d5be02-ca35-4fc4-a4f9-71047e82ea4f" />


## 📖 Description

**MHC-OS** est un système d'exploitation personnalisé développé par la **Minitel Hacking Crew** pour ESP32, spécialement conçu pour être utilisé avec un Minitel. Ce projet transforme votre ESP32 en un terminal interactif rétro avec de nombreuses fonctionnalités modernes.

Le système offre une interface shell complète et des outils de connectivité réseau, le tout affiché sur l'écran iconique du Minitel.

## ✨ Fonctionnalités principales

### 🖥️ Interface Shell (mShell)
- **Shell interactif** avec invite de commandes personnalisable
- **Système de fichiers** complet avec navigation
- **Gestion des utilisateurs** avec authentification et niveaux de privilèges
- **Historique des commandes** persistant
- **Variables d'environnement** et exécution de scripts avec interpréteur
- **Éditeur de texte** intégré avec commandes vi-like

### 🌐 Connectivité réseau
- **Client SSH** intégré pour connexions à distance
- **Commandes réseau** : ping, curl, ifconfig
- **Configuration WiFi** avec connexion automatique en cas de reboot
- **Client HTTP** pour requêtes web

### 🔧 Outils système
- **Gestionnaire de tâches CRON** pour l'automatisation
- **Système de fichiers LittleFS** avec partitionnement
- **Monitoring de la mémoire**
- **Commandes de manipulation de fichiers** (ls, cat, grep, etc.)

## 🛠️ Installation et configuration

### Prérequis
- **ESP32** sur une devboard compatible minitel
- **Minitel** compatible
- **PlatformIO** installé sur IDE (VSCode)

### Dépendances

Les dépendences sont intégrées au **platformio.ini**

### Compilation et flash
```bash
# Cloner le repository
git clone <repository_url>

# Compiler et flasher
pio run -t upload

```

### Configuration du partitionnement
Le projet utilise un partitionnement personnalisé défini dans `partitions.csv` :
- **NVS** : Stockage des préférences (24 KB)
- **PHY** : Initialisation WiFi (4 KB)
- **APP** : Application principale (2.9 MB)
- **SPIFFS** : Système de fichiers (1 MB)

## 🎯 Utilisation

### Démarrage
1. Réglez la vitesse d'affichage série du minitel (touche Fonction + P puis 4 (pour 4800 bauds) ou 9 (9600 bauds, minitel 2 uniquement))
2. Connectez votre ESP32 Devboard au Minitel
3. Le splash screen MHC-OS s'affiche (sur minitel 2 il faudra effectuer la combinaison de touches Fonction + SOMMAIRE)
4. Utilisez les touches spéciales pour la configuration initiale :
   - **GUIDE x7** : Réinitialise les utilisateurs (conformément au users renseignés dans main.cpp)
   - **ENVOI** : Active/désactive le déplacement curseur (incompatible avec Minitel 1B, désactivé par défaut)

### Commandes principales

#### Système
```bash
help              # Affiche l'aide complète
whoami            # Utilisateur actuel
passwd            # Changer le mot de passe
su <user>         # Changer d'utilisateur
login/logout      # Connexion/déconnexion
reboot            # Redémarrage
clear             # Efface l'écran
```

#### Fichiers et navigation
```bash
ls                # Liste les fichiers
cd <dossier>      # Change de répertoire
pwd               # Répertoire actuel
cat <fichier>     # Affiche le contenu
edit <fichier>    # Éditeur de texte
mkdir <dossier>   # Crée un dossier
rm <fichier>      # Supprime un fichier
grep <motif>      # Recherche dans les fichiers
```

#### Réseau
```bash
wifi <ssid> <password>  # Configuration WiFi
ping <host>             # Test de connectivité
ssh <user@host>         # Connexion SSH
curl <url>              # Requête HTTP
ifconfig               # Configuration réseau
```


#### Tâches automatisées
```bash
cronpause         # Met en pause les tâches cron
cronresume        # Reprend les tâches cron
```

### Éditeur de texte intégré
L'éditeur utilise des commandes similaires à vi/ed :
```
n[nb]     # Ligne suivante (+nb)
p[nb]     # Ligne précédente (-nb)
g[nb]     # Aller à la ligne nb
e[nb]     # Éditer la ligne nb
a         # Ajouter après la ligne
b         # Insérer avant la ligne
x[nb]     # Supprimer la ligne nb
w         # Sauvegarder
wq        # Sauvegarder et quitter
q!        # Quitter sans sauvegarder
```

## 🏗️ Architecture du code

```
src/
├── main.cpp              # Point d'entrée principal
├── globals.h/cpp         # Variables et includes globaux
├── applications/
│   ├── shell.h/cpp       # Shell principal
│   ├── cron/            # Gestionnaire de tâches
│   ├── shell_apps/      # Applications shell
│   └── ssh/             # Client SSH
└── utils/               # Utilitaires
```

### Structure des applications
- **Shell** : Interface principale avec système de commandes modulaire
- **SSH** : Client SSH complet pour connexions distantes
- **CRON** : Gestionnaire de tâches périodiques

## 🔐 Système de sécurité

### Gestion des utilisateurs
- Système d'authentification par utilisateur/mot de passe
- Niveaux d'accès différenciés (user, admin, root)
- Stockage sécurisé des credentials
- Sessions avec timeout automatique

## 🎨 Interface Minitel

### Affichage
- Mode texte 40x25 caractères
- Support des caractères spéciaux Minitel
- Interface adaptée aux limitations du terminal
- Gestion optimisée de l'affichage

### Contrôles
- Navigation avec les touches fléchées
- Touches de fonction Minitel intégrées
- Saisie de texte avec correction
- Gestion des touches spéciales (ENVOI, CORRECTION, etc.)

## 🔧 Configuration avancée

### Variables d'environnement
Le système supporte des variables shell persistantes :
```bash
set VARIABLE=valeur      # Définir une variable
echo $VARIABLE          # Afficher une variable
```

### Scripts personnalisés
Support de scripts avec :
- Conditions et boucles
- Variables locales et globales
- Exécution en arrière-plan
- Gestion d'erreurs

### Tâches CRON
Configuration de tâches automatisées :
```bash
# Format : intervalle_ms commande
# Exemple dans le code CRON
60000 echo "Message périodique"
```

## 🐛 Dépannage

### Problèmes courants
1. **Écran vide ou pas d'affichage** : Vérifier la vitesse de communication Minitel
2. **Touches non reconnues** : Contrôler le mode clavier étendu
3. **Connexion WiFi** : Vérifier les credentials et la portée

### Debug
```bash
# Vérifier l'état système
df                  # Espace disque
version            # Version du système
whoami             # Session actuelle
```

## 📝 Développement

### 🔧 Ajouter une nouvelle commande simple

Pour ajouter une commande basique au shell :

#### 1. Déclarer la fonction
Dans le fichier header approprié (ex: `src/applications/shell_apps/shell_sys.h`) :
```cpp
void shell_ma_commande(const String &args);
```

#### 2. Implémenter la fonction
Dans le fichier source correspondant (ex: `src/applications/shell_apps/shell_sys.cpp`) :
```cpp
void shell_ma_commande(const String &args) {
    // Validation des arguments
    if (args.length() == 0) {
        shell_println_wrapped("Usage: ma_commande <parametre>");
        return;
    }
    
    // Logique de la commande
    shell_println_wrapped("Résultat: " + args);
}
```

#### 3. Enregistrer la commande
Dans `src/applications/shell.cpp`, ajouter à la liste `commands[]` :
```cpp
ShellCommand commands[] = {
    // ... commandes existantes ...
    {"ma_commande", shell_ma_commande},
};
```

#### 4. Documenter dans l'aide
Dans `src/applications/shell_apps/shell_sys.cpp`, fonction `shell_help()` :
```cpp
shell_println_wrapped("  ma_commande      - Description de ma commande");
```

### 🏗️ Créer une nouvelle application complexe

Pour développer une application complète :

#### Structure recommandée
```
src/applications/mon_app/
├── mon_app.h              # Headers et déclarations
├── mon_app.cpp            # Logique principale
├── mon_app_ui.cpp         # Interface utilisateur
├── mon_app_handlers.cpp   # Gestionnaires d'événements
└── mon_app_utils.cpp      # Utilitaires spécifiques
```

#### 1. Créer les fichiers de base

**mon_app.h** :
```cpp
#ifndef MON_APP_H
#define MON_APP_H

#include "../globals.h"

// Structures de données
struct MonAppState {
    int niveau;
    bool actif;
    String donnees;
};

// Fonctions principales
void mon_app_init();
void mon_app_run();
void mon_app_cleanup();

// Fonctions utilitaires
void mon_app_afficher_interface();
void mon_app_traiter_input(uint32_t key);

// Variables globales
extern MonAppState appState;

#endif // MON_APP_H
```

**mon_app.cpp** :
```cpp
#include "mon_app.h"

MonAppState appState = {0, false, ""};

void shell_mon_app_wrapper(const String &args) {
    mon_app_run();
}

void mon_app_init() {
    appState.niveau = 1;
    appState.actif = true;
    appState.donnees = "Données initiales";
}

void mon_app_run() {
    mon_app_init();
    
    // Boucle principale
    while (appState.actif) {
        mon_app_afficher_interface();
        
        uint32_t key = minitel.getKeyCode(false);
        if (key != 0) {
            mon_app_traiter_input(key);
        }
        
        delay(50); // Éviter la surcharge CPU
    }
    
    mon_app_cleanup();
}

void mon_app_cleanup() {
    // Nettoyage et retour au shell
    shell_clear();
    shell_println_wrapped("Sortie de Mon App");
}
```

**mon_app_ui.cpp** :
```cpp
#include "mon_app.h"

void mon_app_afficher_interface() {
    // Effacer et repositionner
    minitel.moveCursorXY(0, 0);
    
    // Afficher l'interface
    minitel.println("=== MON APPLICATION ===");
    minitel.println("Niveau: " + String(appState.niveau));
    minitel.println("Données: " + appState.donnees);
    minitel.println();
    minitel.println("[ESC] Quitter [SPACE] Action");
    
    // Positionner le curseur
    minitel.moveCursorXY(0, 10);
}
```

**mon_app_handlers.cpp** :
```cpp
#include "mon_app.h"

void mon_app_traiter_input(uint32_t key) {
    switch (key) {
        case 0x1B: // ESC
            appState.actif = false;
            break;
            
        case ' ': // SPACE
            appState.niveau++;
            appState.donnees = "Niveau " + String(appState.niveau);
            break;
            
        case 'h':
        case 'H':
            minitel.println("Aide: [ESC] quitter [SPACE] monter niveau");
            break;
            
        default:
            // Touche non gérée
            break;
    }
}
```

#### 2. Intégrer l'application

**Inclure les headers** dans `globals.h` :
```cpp
#include "applications/mon_app/mon_app.h"
```

**Ajouter la commande** dans `shell.cpp` :
```cpp
ShellCommand commands[] = {
    // ... autres commandes ...
    {"monapp", shell_mon_app_wrapper},
};
```

#### 3. Gestion des ressources

**Utilisation du système de fichiers** :
```cpp
// Sauvegarder des données
void mon_app_sauvegarder() {
    File file = LittleFS.open("/mon_app/config.txt", FILE_WRITE);
    if (file) {
        file.println("niveau=" + String(appState.niveau));
        file.println("donnees=" + appState.donnees);
        file.close();
    }
}

// Charger des données
void mon_app_charger() {
    if (LittleFS.exists("/mon_app/config.txt")) {
        File file = LittleFS.open("/mon_app/config.txt", FILE_READ);
        while (file.available()) {
            String line = file.readStringUntil('\n');
            // Parser la ligne...
        }
        file.close();
    }
}
```

#### Gestion des touches spéciales
```cpp
void traiter_touches_speciales(uint32_t key) {
    switch (key) {
        case TOUCHE_FLECHE_HAUT:    // Navigation
        case TOUCHE_FLECHE_BAS:
        case TOUCHE_FLECHE_GAUCHE:
        case TOUCHE_FLECHE_DROITE:
            // Logique de navigation
            break;
            
        case ENVOI:                 // Validation
            // Action de validation
            break;
            
        case CORRECTION:            // Annulation/Retour
            // Action d'annulation
            break;
            
        case GUIDE:                 // Aide
            afficher_aide();
            break;
    }
}
```

### 🧪 Tests et debug

#### Debug de base
```cpp
#define DEBUG_MON_APP 1

void debug_log(const String &message) {
    #if DEBUG_MON_APP
    Serial.println("[MON_APP] " + message);
    #endif
}
```

#### Tests des fonctionnalités
```cpp
void mon_app_test() {
    debug_log("Test d'initialisation");
    mon_app_init();
    
    debug_log("État: niveau=" + String(appState.niveau));
    debug_log("État: actif=" + String(appState.actif));
    
    // Tests unitaires simples
    assert(appState.niveau == 1);
    assert(appState.actif == true);
}
```

Cette structure modulaire permet de créer des applications robustes et maintenables dans l'écosystème MHC-OS.

## 👥 Contributeurs

- LL7Baucarre
- 0b3ud

## 📄 Licence

GPL-3.0

---
