#ifndef DATABASE_H
#define DATABASE_H

typedef struct {
    char name[100];
    char capital[100];
    long population;
    float area;
    char currency[50];
    char continent[50];

    char image[256];
    char description[2048];   
} Country;


typedef struct {
    char code[8];
    char name[100];
    char file[256];
} CountryIndex;

int load_countries_index(const char *filename);
int get_index_count(void);
const CountryIndex *get_country_index(int index);

int load_country_from_file(const char *filename, Country *country);

void print_country(const Country *country);

const CountryIndex* search_country_in_index(const char *name);

// CRUD COMPLET
// CRUD complet par index + JSON
int add_country_index(Country c, const char *code);       // Create
int update_country_index(const char *code, Country c);    // Update
int remove_country_index(const char *code);               // Delete
int save_index(const char *filename);                     // Sauvegarde index.json
int load_countries_index(const char *filename);           // Load index.json

// Fonction pour sauvegarder un pays dans son fichier JSON
int save_country_to_file(const char *filename, Country *c);


// Nettoyage
void free_database(void);


#endif
