#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "database.h"

void menu() {
    printf("\n=== COUNTRYPEDIA ===\n");
    printf("1. Lister tous les pays\n");
    printf("2. Rechercher un pays par nom ou code\n");
    printf("3. Ajouter un pays\n");
    printf("4. Modifier un pays\n");
    printf("5. Supprimer un pays\n");
    printf("6. Quitter\n");
    printf("Choix : ");
}

int main(void) {
    if (load_countries_index("data/index.json") != 0) {
        printf("Erreur chargement index\n");
        return 1;
    }

    int choix;

    do {
        menu();
        if (scanf("%d", &choix) != 1) {
            while (getchar() != '\n'); // vider le buffer
            printf("Erreur : choix invalide\n");
            continue;
        }
        getchar(); // enlever le retour chariot

        if (choix == 1) {
            printf("\nPays disponibles :\n");
            for (int i = 0; i < get_index_count(); i++) {
                const CountryIndex *idx = get_country_index(i);
                printf("%d. %s (%s)\n", i + 1, idx->name, idx->code);
            }
        }
        else if (choix == 2) {
            char recherche[100];
            printf("Nom ou code du pays : ");
            fgets(recherche, sizeof(recherche), stdin);
            recherche[strcspn(recherche, "\n")] = 0;

            const CountryIndex *found = search_country_in_index(recherche);
            if (found) {
                Country c;
                if (load_country_from_file(found->file, &c) == 0)
                    print_country(&c);
                else
                    printf("Erreur chargement du pays\n");
            } else {
                printf("Pays non trouvé\n");
            }
        }
        else if (choix == 3) {
            Country c;
            char code[10];

            printf("Code pays (ex: FR) : ");
            fgets(code, sizeof(code), stdin);
            code[strcspn(code, "\n")] = 0;
            if (strlen(code) == 0) { printf("Code obligatoire\n"); continue; }

            printf("Nom : "); fgets(c.name, sizeof(c.name), stdin); c.name[strcspn(c.name, "\n")] = 0;
            if (strlen(c.name) == 0) { printf("Nom obligatoire\n"); continue; }

            printf("Capitale : "); fgets(c.capital, sizeof(c.capital), stdin); c.capital[strcspn(c.capital, "\n")] = 0;

            printf("Population : "); 
            if (scanf("%ld", &c.population) != 1 || c.population <= 0) {
                while (getchar() != '\n');
                printf("Population invalide\n");
                continue;
            }
            getchar();

            printf("Superficie : "); 
            if (scanf("%ld", &c.area) != 1 || c.area <= 0) {
                while (getchar() != '\n');
                printf("Superficie invalide\n");
                continue;
            }
            getchar();

            printf("Devise : "); fgets(c.currency, sizeof(c.currency), stdin); c.currency[strcspn(c.currency, "\n")] = 0;
            printf("Continent : "); fgets(c.continent, sizeof(c.continent), stdin); c.continent[strcspn(c.continent, "\n")] = 0;
            printf("Image (chemin) : "); fgets(c.image, sizeof(c.image), stdin); c.image[strcspn(c.image, "\n")] = 0;
            printf("Description : "); fgets(c.description, sizeof(c.description), stdin); c.description[strcspn(c.description, "\n")] = 0;

            if (add_country_index(c, code) == 0)
                printf("Pays ajouté ✅\n");
            else
                printf("Erreur ajout : ce pays ou code existe déjà. Vous pouvez le modifier ou le supprimer.\n");
        }
        else if (choix == 4) {
            char code[10];
            printf("Code du pays à modifier : ");
            fgets(code, sizeof(code), stdin);
            code[strcspn(code, "\n")] = 0;

            const CountryIndex *idx = search_country_in_index(code);
            if (!idx) { printf("Code inconnu\n"); continue; }

            Country c;
            printf("Nom : "); fgets(c.name, sizeof(c.name), stdin); c.name[strcspn(c.name, "\n")] = 0;
            printf("Capitale : "); fgets(c.capital, sizeof(c.capital), stdin); c.capital[strcspn(c.capital, "\n")] = 0;
            printf("Population : "); scanf("%ld", &c.population); getchar();
            if (c.population <= 0) { printf("Population invalide\n"); continue; }
            printf("Superficie : "); scanf("%ld", &c.area); getchar();
            if (c.area <= 0) { printf("Superficie invalide\n"); continue; }
            printf("Devise : "); fgets(c.currency, sizeof(c.currency), stdin); c.currency[strcspn(c.currency, "\n")] = 0;
            printf("Continent : "); fgets(c.continent, sizeof(c.continent), stdin); c.continent[strcspn(c.continent, "\n")] = 0;
            printf("Image (chemin) : "); fgets(c.image, sizeof(c.image), stdin); c.image[strcspn(c.image, "\n")] = 0;
            printf("Description : "); fgets(c.description, sizeof(c.description), stdin); c.description[strcspn(c.description, "\n")] = 0;

            if (update_country_index(code, c) == 0)
                printf("Pays modifié ✅\n");
            else
                printf("Erreur modification\n");
        }
        else if (choix == 5) {
            char code[10];
            printf("Code du pays à supprimer : ");
            fgets(code, sizeof(code), stdin);
            code[strcspn(code, "\n")] = 0;

            if (remove_country_index(code) == 0)
                printf("Pays supprimé ✅\n");
            else
                printf("Erreur suppression : code inconnu\n");
        }

    } while (choix != 6);

    free_database();
    return 0;
}
