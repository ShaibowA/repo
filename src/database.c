#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/cJSON/cJSON.h"
#include "database.h"

static CountryIndex *index_list = NULL;
static int index_count = 0;

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) { fclose(file); return NULL; }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

/* ================================
   CHARGER L’INDEX DES PAYS
   ================================ */
int load_countries_index(const char *filename) {
    char *json_data = read_file(filename);
    if (!json_data) return -1;

    cJSON *root = cJSON_Parse(json_data);
    free(json_data);
    if (!root) return -1;

    cJSON *countries = cJSON_GetObjectItem(root, "countries");
    if (!cJSON_IsArray(countries)) { cJSON_Delete(root); return -1; }

    index_count = cJSON_GetArraySize(countries);
    index_list = malloc(sizeof(CountryIndex) * index_count);
    if (!index_list) { cJSON_Delete(root); return -1; }

    for (int i = 0; i < index_count; i++) {
        cJSON *item = cJSON_GetArrayItem(countries, i);

        strncpy(index_list[i].code,
                cJSON_GetObjectItem(item, "code")->valuestring,
                sizeof(index_list[i].code)-1);
        index_list[i].code[sizeof(index_list[i].code)-1] = '\0';

        strncpy(index_list[i].name,
                cJSON_GetObjectItem(item, "name")->valuestring,
                sizeof(index_list[i].name)-1);
        index_list[i].name[sizeof(index_list[i].name)-1] = '\0';

        strncpy(index_list[i].file,
                cJSON_GetObjectItem(item, "file")->valuestring,
                sizeof(index_list[i].file)-1);
        index_list[i].file[sizeof(index_list[i].file)-1] = '\0';
    }

    cJSON_Delete(root);
    return 0;
}

/* ================================
   ACCÈS À L’INDEX
   ================================ */
int get_index_count(void) { return index_count; }

const CountryIndex *get_country_index(int index) {
    if (index < 0 || index >= index_count) return NULL;
    return &index_list[index];
}

/* ================================
   CHARGER UN PAYS (JSON PAR PAYS)
   ================================ */
int load_country_from_file(const char *filename, Country *country) {
    if (!country) return -1;

    char *json_data = read_file(filename);
    if (!json_data) return -1;

    cJSON *root = cJSON_Parse(json_data);
    free(json_data);
    if (!root) return -1;

    #define COPY_STR(field, key, size)                     \
        do {                                               \
            cJSON *it = cJSON_GetObjectItem(root, key);    \
            if (cJSON_IsString(it)) {                      \
                strncpy(field, it->valuestring, size-1);  \
                field[size-1]='\0';                        \
            } else { field[0]='\0'; }                     \
        } while(0)

    COPY_STR(country->name, "name", sizeof(country->name));
    COPY_STR(country->capital, "capital", sizeof(country->capital));
    COPY_STR(country->currency,"currency",sizeof(country->currency));
    COPY_STR(country->continent,"continent",sizeof(country->continent));
    COPY_STR(country->image,"image",sizeof(country->image));
    COPY_STR(country->description,"description",sizeof(country->description));

    cJSON *pop = cJSON_GetObjectItem(root, "population");
    cJSON *area = cJSON_GetObjectItem(root, "area");

    country->population = cJSON_IsNumber(pop)?pop->valuedouble:0;
    country->area       = cJSON_IsNumber(area)?area->valuedouble:0;

    if (strlen(country->image)==0)
        strcpy(country->image,"data/images/default.png");

    cJSON_Delete(root);
    return 0;
}

/* ================================
   DEBUG CONSOLE
   ================================ */
void print_country(const Country *c) {
    if (!c) return;
    printf("\n==============================\n");
    printf("%s\n", c->name);
    printf("==============================\n");
    printf("Capitale   : %s\n", c->capital);
    printf("Population : %ld\n", c->population);
    printf("Superficie : %ld km²\n", c->area);
    printf("Devise     : %s\n", c->currency);
    printf("Continent  : %s\n", c->continent);
    printf("\n%s\n", c->description);
}

/* ================================
   RECHERCHE DANS L’INDEX
   ================================ */
const CountryIndex* search_country_in_index(const char *query) {
    if (!query) return NULL;

    for (int i = 0; i < index_count; i++) {
        if (strcasecmp(index_list[i].name, query) == 0 || 
            strcasecmp(index_list[i].code, query) == 0) {
            return &index_list[i];
        }
    }
    return NULL;
}


/* ================================
   SAUVEGARDE JSON
   ================================ */
int save_country_to_file(const char *filename, Country *c) {
    if(!c) return -1;

    cJSON *root=cJSON_CreateObject();
    cJSON_AddStringToObject(root,"name",c->name);
    cJSON_AddStringToObject(root,"capital",c->capital);
    cJSON_AddNumberToObject(root,"population",c->population);
    cJSON_AddNumberToObject(root,"area",c->area);
    cJSON_AddStringToObject(root,"currency",c->currency);
    cJSON_AddStringToObject(root,"continent",c->continent);
    cJSON_AddStringToObject(root,"image",c->image);
    cJSON_AddStringToObject(root,"description",c->description);

    char *json_str=cJSON_Print(root);
    if(!json_str){ cJSON_Delete(root); return -1; }

    FILE *f=fopen(filename,"w");
    if(!f){ free(json_str); cJSON_Delete(root); return -1; }

    fprintf(f,"%s",json_str);
    fclose(f);
    free(json_str);
    cJSON_Delete(root);
    return 0;
}

int save_index(const char *filename) {
    cJSON *root=cJSON_CreateObject();
    cJSON *array=cJSON_CreateArray();

    for(int i=0;i<index_count;i++){
        cJSON *item=cJSON_CreateObject();
        cJSON_AddStringToObject(item,"code",index_list[i].code);
        cJSON_AddStringToObject(item,"name",index_list[i].name);
        cJSON_AddStringToObject(item,"file",index_list[i].file);
        cJSON_AddItemToArray(array,item);
    }

    cJSON_AddItemToObject(root,"countries",array);

    FILE *f=fopen(filename,"w");
    if(!f) { cJSON_Delete(root); return -1; }

    char *json_str=cJSON_Print(root);
    fprintf(f,"%s",json_str);
    fclose(f);
    free(json_str);
    cJSON_Delete(root);
    return 0;
}

/* ================================
   CRUD PAYS AVEC VALIDATION
   ================================ */

int add_country_index(Country c, const char *code){
    if(strlen(c.name)==0 || strlen(code)==0){
        fprintf(stderr,"Erreur : nom ou code vide\n");
        return -1;
    }
    if(c.population<=0){ fprintf(stderr,"Erreur : population > 0\n"); return -1;}
    if(c.area<=0){ fprintf(stderr,"Erreur : superficie > 0\n"); return -1;}

    // verifie doublon
    for(int i=0;i<index_count;i++){
        if(strcasecmp(index_list[i].name,c.name)==0 ||
           strcasecmp(index_list[i].code,code)==0){
            fprintf(stderr,"Erreur : pays deja existant (%s)\n",c.name);
            return -1;
        }
    }

    char filename[256];
    sprintf(filename,"data/countries/%s.json",code);

    if(save_country_to_file(filename,&c)!=0){
        fprintf(stderr,"Erreur : impossible de creer le fichier JSON\n");
        return -1;
    }

    CountryIndex *tmp=realloc(index_list,sizeof(CountryIndex)*(index_count+1));
    if(!tmp){ fprintf(stderr,"Erreur allocation memoire\n"); return -1;}
    index_list=tmp;

    strncpy(index_list[index_count].name,c.name,sizeof(index_list[index_count].name)-1);
    strncpy(index_list[index_count].code,code,sizeof(index_list[index_count].code)-1);
    strncpy(index_list[index_count].file,filename,sizeof(index_list[index_count].file)-1);
    index_count++;

    return save_index("data/index.json");
}

int update_country_index(const char *code, Country updated){
    if(updated.population<=0){ fprintf(stderr,"Erreur : population > 0\n"); return -1;}
    if(updated.area<=0){ fprintf(stderr,"Erreur : superficie > 0\n"); return -1;}

    CountryIndex *idx=NULL;
    for(int i=0;i<index_count;i++)
        if(strcasecmp(index_list[i].code,code)==0){ idx=&index_list[i]; break;}
    if(!idx){ fprintf(stderr,"Erreur : code pays introuvable\n"); return -1;}

    return save_country_to_file(idx->file,&updated);
}

int remove_country_index(const char *code){
    int index=-1;
    for(int i=0;i<index_count;i++)
        if(strcasecmp(index_list[i].code,code)==0){ index=i; break;}
    if(index==-1){ fprintf(stderr,"Erreur : code pays introuvable\n"); return -1;}

    if(remove(index_list[index].file)!=0)
        fprintf(stderr,"Attention : fichier JSON non trouve pour suppression\n");

    for(int i=index;i<index_count-1;i++) index_list[i]=index_list[i+1];
    index_count--;
    index_list=realloc(index_list,sizeof(CountryIndex)*index_count);

    return save_index("data/index.json");
}



/* ================================
   LIBÉRATION MÉMOIRE
   ================================ */
void free_database(void){
    free(index_list);
    index_list=NULL;
    index_count=0;
}
