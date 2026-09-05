
//compile
gcc -o countryp main.c gui.c src/database.c lib/cJSON/cJSON.c src/config.c -Ilib/cJSON -Isrc `pkg-config --cflags --libs gtk+-3.0` -lm


// exe
./countryp.exe

...



























MonProjetC/
├── src/
│ ├── main.c
│ ├── database.c
│ ├── database.h
│ ├── gui.c
│ ├── gui.h
│ └── config.c
│
├── data/
│ ├── countries_index.json
│ ├── countries/
│ │ ├── france.json
│ │ ├── spain.json
│ │ ├── germany.json
│ │ └── italy.json
│ │
│ ├── images/
│ │ ├── france.png
│ │ ├── spain.png
│ │ ├── germany.png
│ │ └── default.png
│ │
│ ├── news/
│ │ ├── france.json
│ │ └── spain.json
│ │
│ └── search_history.json
│
├── config/
│ └── config.txt
│
├── lib/
│ └── cJSON/
│ ├── cJSON.c
│ └── cJSON.h
│
└── README.md

🔹 data/images/

Images affichées dans GTK :

GtkImage

fallback sur default.png si manquante

🔹 src/database.c

Responsabilités :

charger countries_index.json

stocker :

nom

code

chemin du fichier

charger un pays précis à la demande

👉 Il ne connaît pas GTK

🔹 src/gui.c

Responsabilités :

fenêtre GTK

liste cliquable des pays

affichage texte + image

champ de recherche

👉 Il ne parse pas le JSON

🔹 src/main.c

Responsabilité :

initialiser UTF-8

charger l’index

lancer la GUI

fait et pa sfait :::::::::::::
✅ Ce qui a été fait jusqu’à présent

1. Gestion de la base de données

Création des structures Country et CountryIndex.

Fichiers JSON par pays et index général index.json.

Fonctions de chargement des pays et de l’index (load_country_from_file, load_countries_index).

Fonctions CRUD complètes pour les pays :

Ajouter un pays (add_country_index)

Modifier un pays (update_country_index)

Supprimer un pays (remove_country_index)

Gestion correcte des fichiers JSON pour chaque pays.

Sauvegarde de l’index (save_index) et d’un pays (save_country_to_file).

Gestion des erreurs simples (existe déjà, fichier introuvable…).

2. Interface console

Menu principal avec options :

Lister tous les pays

Rechercher un pays par nom

Ajouter un pays

Modifier un pays

Supprimer un pays

Quitter

Affichage des informations d’un pays (print_country).

Recherche par nom complet.

Validation de base pour l’entrée utilisateur (entiers, fgets pour textes).

CRUD testé en console : ajout, modification, suppression.

Bloc description + image pour chaque pays affiché dans la console.

3. Logiciels et compilation

Projet compilable avec GCC sous Windows.

Bibliothèque cJSON intégrée pour manipuler JSON.

Gestion des problèmes Windows avec fichiers ouverts.

🟡 Améliorations réalisées / prévues sur la logique du CRUD

Empêche l’ajout de doublons (nom ou code pays déjà existant).

Validation basique des champs (population et superficie ≥ 0).

Logique cohérente : modification ou suppression uniquement si le pays existe.

Fichiers JSON nommés avec le code du pays pour cohérence et éviter les noms longs ou accents.

Recherche à compléter pour accepter nom et code simultanément.

⬜ Ce qu’il reste à faire

1. Améliorations du CRUD

Gestion avancée des erreurs et messages clairs pour chaque action.

Validation complète de tous les champs (par ex. devise et continent non vides, image existante ou par défaut).

Recherche intelligente : accepter nom ou code pour retrouver un pays.

Support des majuscules/minuscules et des accents dans la recherche.

Vérification de l’intégrité de la base (ex : fichiers JSON manquants ou corrompus).

2. Interface graphique (GTK)

Créer une interface graphique pour remplacer le menu console.

Afficher la liste des pays dans un tableau ou liste.

Afficher les informations complètes d’un pays avec image.

Ajouter les boutons pour CRUD (ajout, modification, suppression).

Support des traductions et liens dans un bloc texte (GtkTextView ou similaire).

Rechercher un pays par nom ou code directement depuis GTK.

3. Fonctions avancées / extras

Gestion des liens dans la description (clic pour ouvrir le navigateur).

Fonction de filtrage ou tri (continent, population, superficie).

Ajout d’une image par défaut si aucune image fournie.

Export ou sauvegarde automatique avant fermeture.
