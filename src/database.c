#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include "cJSON.h"

// Variables globales pour l'index
static CountryIndex *country_index = NULL;
static int index_count = 0;
static int index_capacity = 0;

int load_countries_index(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("❌ Impossible d'ouvrir %s\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return -1;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        free(buffer);
        return -1;
    }

    // Libérer l'ancien index si existe
    if (country_index) {
        free(country_index);
    }

    cJSON *countries = cJSON_GetObjectItem(json, "countries");
    if (!cJSON_IsArray(countries)) {
        cJSON_Delete(json);
        free(buffer);
        return -1;
    }

    index_count = cJSON_GetArraySize(countries);
    index_capacity = index_count;
    country_index = malloc(index_capacity * sizeof(CountryIndex));

    for (int i = 0; i < index_count; i++) {
        cJSON *item = cJSON_GetArrayItem(countries, i);
        cJSON *code = cJSON_GetObjectItem(item, "code");
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *file = cJSON_GetObjectItem(item, "file");

        if (code && name && file) {
            strncpy(country_index[i].code, code->valuestring, sizeof(country_index[i].code)-1);
            strncpy(country_index[i].name, name->valuestring, sizeof(country_index[i].name)-1);
            strncpy(country_index[i].file, file->valuestring, sizeof(country_index[i].file)-1);
        }
    }

    cJSON_Delete(json);
    free(buffer);
    
    printf("✅ Index chargé: %d pays\n", index_count);
    return 0;
}

int get_index_count(void) {
    return index_count;
}

const CountryIndex *get_country_index(int index) {
    if (index < 0 || index >= index_count) {
        return NULL;
    }
    return &country_index[index];
}

const CountryIndex* find_country_by_code(const char *code) {
    if (!code || !country_index) return NULL;
    
    for (int i = 0; i < index_count; i++) {
        if (strcmp(country_index[i].code, code) == 0) {
            return &country_index[i];
        }
    }
    return NULL;
}

int load_country_from_file(const char *filename, Country *country) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("❌ Impossible d'ouvrir %s\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return -1;
    }

    fread(buffer, 1, length, file);
    fclose(file);
    buffer[length] = '\0';

    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        free(buffer);
        return -1;
    }

    // Charger les champs de base
    cJSON *name = cJSON_GetObjectItem(json, "name");
    cJSON *capital = cJSON_GetObjectItem(json, "capital");
    cJSON *population = cJSON_GetObjectItem(json, "population");
    cJSON *area = cJSON_GetObjectItem(json, "area");
    cJSON *currency = cJSON_GetObjectItem(json, "currency");
    cJSON *continent = cJSON_GetObjectItem(json, "continent");
    cJSON *image = cJSON_GetObjectItem(json, "image");
    cJSON *description = cJSON_GetObjectItem(json, "description");

    if (name) strncpy(country->name, name->valuestring, sizeof(country->name)-1);
    if (capital) strncpy(country->capital, capital->valuestring, sizeof(country->capital)-1);
    if (population) country->population = population->valueint;
    if (area) country->area = area->valuedouble;
    if (currency) strncpy(country->currency, currency->valuestring, sizeof(country->currency)-1);
    if (continent) strncpy(country->continent, continent->valuestring, sizeof(country->continent)-1);
    if (image) strncpy(country->image, image->valuestring, sizeof(country->image)-1);
    if (description) strncpy(country->description, description->valuestring, sizeof(country->description)-1);

    // ============ CHARGEMENT DES NOUVEAUX CHAMPS ============
    
    // 1. Pays frontaliers
    country->border_count = 0;
    cJSON *borders = cJSON_GetObjectItem(json, "borders");
    if (cJSON_IsArray(borders)) {
        int i = 0;
        cJSON *border_item = NULL;
        
        cJSON_ArrayForEach(border_item, borders) {
            if (i < 10 && cJSON_IsString(border_item)) {
                strncpy(country->borders[i], border_item->valuestring, 3);
                country->borders[i][3] = '\0';
                i++;
            }
        }
        country->border_count = i;
    }
    
    // 2. Gouvernement
    cJSON *government = cJSON_GetObjectItem(json, "government");
    if (cJSON_IsString(government)) {
        strncpy(country->government, government->valuestring, sizeof(country->government)-1);
    } else {
        strcpy(country->government, "Non spécifié");
    }
    
    // 3. Chef d'État
    cJSON *head_of_state = cJSON_GetObjectItem(json, "head_of_state");
    if (cJSON_IsString(head_of_state)) {
        strncpy(country->head_of_state, head_of_state->valuestring, sizeof(country->head_of_state)-1);
    } else {
        strcpy(country->head_of_state, "Non spécifié");
    }
    
    // 4. Langue officielle
    cJSON *official_language = cJSON_GetObjectItem(json, "official_language");
    if (cJSON_IsString(official_language)) {
        strncpy(country->official_language, official_language->valuestring, sizeof(country->official_language)-1);
    } else {
        strcpy(country->official_language, "Non spécifié");
    }
    // ========================================================

    cJSON_Delete(json);
    free(buffer);

    printf("✅ Pays chargé: %s (%d frontières)\n", country->name, country->border_count);
    return 0;
}

void print_country(const Country *country) {
    printf("=== %s ===\n", country->name);
    printf("Capitale: %s\n", country->capital);
    printf("Population: %ld\n", country->population);
    printf("Superficie: %.0f km²\n", country->area);
    printf("Devise: %s\n", country->currency);
    printf("Continent: %s\n", country->continent);
    printf("Image: %s\n", country->image);
    printf("Description: %s\n", country->description);
    
    // Afficher les nouveaux champs
    printf("Gouvernement: %s\n", country->government);
    printf("Chef d'État: %s\n", country->head_of_state);
    printf("Langue: %s\n", country->official_language);
    
    if (country->border_count > 0) {
        printf("Pays frontaliers (%d): ", country->border_count);
        for (int i = 0; i < country->border_count; i++) {
            printf("%s ", country->borders[i]);
        }
        printf("\n");
    }
}

const CountryIndex* search_country_in_index(const char *name) {
    if (!name || !country_index) return NULL;
    
    for (int i = 0; i < index_count; i++) {
        if (strcasecmp(country_index[i].name, name) == 0) {
            return &country_index[i];
        }
    }
    return NULL;
}

// CRUD functions
int add_country_index(Country c, const char *code) {
    if (!code || strlen(code) != 3) return -1;
    
    // Vérifier si le code existe déjà
    for (int i = 0; i < index_count; i++) {
        if (strcmp(country_index[i].code, code) == 0) {
            return -1;
        }
    }
    
    // Agrandir le tableau si nécessaire
    if (index_count >= index_capacity) {
        index_capacity = index_capacity == 0 ? 10 : index_capacity * 2;
        country_index = realloc(country_index, index_capacity * sizeof(CountryIndex));
    }
    
    // Ajouter le nouveau pays
    strcpy(country_index[index_count].code, code);
    strcpy(country_index[index_count].name, c.name);
    snprintf(country_index[index_count].file, sizeof(country_index[index_count].file),
             "data/countries/%s.json", code);
    
    index_count++;
    
    // Sauvegarder le pays dans son fichier
    char filename[256];
    snprintf(filename, sizeof(filename), "data/countries/%s.json", code);
    save_country_to_file(filename, &c);
    
    // Sauvegarder l'index
    save_index("data/index.json");
    
    return 0;
}

int update_country_index(const char *code, Country c) {
    if (!code) return -1;
    
    // Trouver le pays
    for (int i = 0; i < index_count; i++) {
        if (strcmp(country_index[i].code, code) == 0) {
            // Mettre à jour le nom si changé
            strcpy(country_index[i].name, c.name);
            
            // Sauvegarder dans le fichier
            save_country_to_file(country_index[i].file, &c);
            
            // Sauvegarder l'index
            save_index("data/index.json");
            
            return 0;
        }
    }
    
    return -1;
}

int remove_country_index(const char *code) {
    if (!code) return -1;
    
    // Trouver l'index
    int found_index = -1;
    for (int i = 0; i < index_count; i++) {
        if (strcmp(country_index[i].code, code) == 0) {
            found_index = i;
            break;
        }
    }
    
    if (found_index == -1) return -1;
    
    // Supprimer le fichier du pays
    remove(country_index[found_index].file);
    
    // Décaler les éléments
    for (int i = found_index; i < index_count - 1; i++) {
        country_index[i] = country_index[i + 1];
    }
    
    index_count--;
    
    // Sauvegarder l'index
    save_index("data/index.json");
    
    return 0;
}

int save_index(const char *filename) {
    cJSON *root = cJSON_CreateObject();
    cJSON *countries_array = cJSON_CreateArray();
    
    for (int i = 0; i < index_count; i++) {
        cJSON *country_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(country_obj, "code", country_index[i].code);
        cJSON_AddStringToObject(country_obj, "name", country_index[i].name);
        cJSON_AddStringToObject(country_obj, "file", country_index[i].file);
        cJSON_AddItemToArray(countries_array, country_obj);
    }
    
    cJSON_AddItemToObject(root, "countries", countries_array);
    
    char *json_str = cJSON_Print(root);
    FILE *file = fopen(filename, "w");
    if (file) {
        fputs(json_str, file);
        fclose(file);
    }
    
    free(json_str);
    cJSON_Delete(root);
    
    return 0;
}

int save_country_to_file(const char *filename, Country *c) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "name", c->name);
    cJSON_AddStringToObject(root, "capital", c->capital);
    cJSON_AddNumberToObject(root, "population", c->population);
    cJSON_AddNumberToObject(root, "area", c->area);
    cJSON_AddStringToObject(root, "currency", c->currency);
    cJSON_AddStringToObject(root, "continent", c->continent);
    cJSON_AddStringToObject(root, "image", c->image);
    cJSON_AddStringToObject(root, "description", c->description);
    
    // ============ SAUVEGARDER LES NOUVEAUX CHAMPS ============
    
    // 1. Pays frontaliers
    cJSON *borders_array = cJSON_CreateArray();
    for (int i = 0; i < c->border_count; i++) {
        cJSON_AddItemToArray(borders_array, cJSON_CreateString(c->borders[i]));
    }
    cJSON_AddItemToObject(root, "borders", borders_array);
    
    // 2. Gouvernement
    cJSON_AddStringToObject(root, "government", c->government);
    
    // 3. Chef d'État
    cJSON_AddStringToObject(root, "head_of_state", c->head_of_state);
    
    // 4. Langue officielle
    cJSON_AddStringToObject(root, "official_language", c->official_language);
    // ========================================================
    
    char *json_str = cJSON_Print(root);
    FILE *file = fopen(filename, "w");
    if (file) {
        fputs(json_str, file);
        fclose(file);
    }
    
    free(json_str);
    cJSON_Delete(root);
    
    return 0;
}

void free_database(void) {
    if (country_index) {
        free(country_index);
        country_index = NULL;
    }
    index_count = 0;
    index_capacity = 0;
}