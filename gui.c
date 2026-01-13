// gui.c - Interface GTK complete et fonctionnelle (SANS problemes de scroll)
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h> 
#include <glib/gstdio.h>
#include "database.h"

static GtkWidget *current_details_dialog = NULL;

typedef struct {
    GtkWidget *window;
    GtkWidget *treeview;
    GtkListStore *list_store;
    GtkWidget *search_entry;
    GtkWidget *status_label;
    
    GtkWidget *detail_nom_entry;
    GtkWidget *detail_capitale_entry;
    GtkWidget *detail_surface_entry;
    GtkWidget *detail_population_entry;
    GtkWidget *detail_devise_entry;
    GtkWidget *detail_continent_combo;
    GtkWidget *detail_description_text;
    
    // Pour la gestion des images
    GtkWidget *detail_image_label;  // Label pour afficher le chemin de l'image
    char image_path[256];           // Chemin de l'image temporaire
    
    // Donnees temporaires
    Country current_country;
    char current_code[10];
    int edit_mode; // 0: ajout, 1: modification
} AppData;
// ============ DÉCLARATIONS ANTICIPÉES ============
void afficher_details_pays(AppData *app_data, const char *code);
// =================================================

void afficher_details_pays(AppData *app_data, const char *code);

void on_border_country_clicked(GtkWidget *widget, gpointer data) {
    printf("\n🎯 CLIC SUR PAYS FRONTALIER 🎯\n");
    
    char *country_code = (char*)data;
    printf("Code demandé: %s\n", country_code);
    
    // Trouver l'AppData (stocké dans les données du widget)
    AppData *app_data = g_object_get_data(G_OBJECT(widget), "app_data");
    if (!app_data) {
        printf("❌ AppData non trouvé!\n");
        g_free(country_code);
        return;
    }
    
    printf("AppData valide, ouverture de %s...\n", country_code);
    
    // Fermer la fenêtre de détails actuelle si elle existe
    if (current_details_dialog != NULL && GTK_IS_WIDGET(current_details_dialog)) {
        printf("Fermeture fenêtre précédente...\n");
        gtk_widget_destroy(current_details_dialog);
        current_details_dialog = NULL;
    }
    
    // Ouvrir la fiche du pays frontalier
    afficher_details_pays(app_data, country_code);
    
    g_free(country_code);
}

// Créer un widget pour afficher les pays frontaliers
GtkWidget* create_borders_widget(AppData *app_data, Country *country) {
    if (country->border_count == 0) {
        GtkWidget *label = gtk_label_new("Ce pays n'a pas de frontières terrestres.");
        gtk_widget_set_margin_top(label, 10);
        gtk_widget_set_margin_bottom(label, 10);
        return label;
    }
    
    GtkWidget *frame = gtk_frame_new("🧭 Pays limitrophes");
    gtk_widget_set_margin_top(frame, 15);
    gtk_widget_set_margin_bottom(frame, 15);
    
    GtkWidget *flowbox = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flowbox), GTK_SELECTION_NONE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flowbox), 4);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flowbox), 10);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flowbox), 10);
    
    gtk_container_set_border_width(GTK_CONTAINER(flowbox), 10);
    
    // Parcourir tous les pays frontaliers
    for (int i = 0; i < country->border_count; i++) {
        const char *border_code = country->borders[i];
        
        // Chercher le nom complet du pays frontalier
        const CountryIndex *border_idx = find_country_by_code(border_code);
        if (!border_idx) {
            printf("⚠️  Pays frontalier non trouvé: %s\n", border_code);
            continue;
        }
        
        // Charger le pays pour avoir son nom
        Country border_country;
        if (load_country_from_file(border_idx->file, &border_country) != 0) {
            continue;
        }
        
        // Créer un bouton pour ce pays frontalier
        GtkWidget *btn = gtk_button_new();
        
        // Créer un conteneur vertical pour le bouton
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
        
        // Code du pays (petit)
        GtkWidget *code_label = gtk_label_new(NULL);
        char code_text[50];
        snprintf(code_text, sizeof(code_text), "<small>%s</small>", border_code);
        gtk_label_set_markup(GTK_LABEL(code_label), code_text);
        
        // Nom du pays
        GtkWidget *name_label = gtk_label_new(NULL);
        char name_text[100];
        snprintf(name_text, sizeof(name_text), "<b>%s</b>", border_country.name);
        gtk_label_set_markup(GTK_LABEL(name_label), name_text);
        gtk_label_set_line_wrap(GTK_LABEL(name_label), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(name_label), 15);
        
        // Ajouter au conteneur
        gtk_box_pack_start(GTK_BOX(vbox), code_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
        
        // Style du bouton
        gtk_widget_set_size_request(btn, 100, 70);
        gtk_container_add(GTK_CONTAINER(btn), vbox);
        
        // Stocker les données
        g_object_set_data(G_OBJECT(btn), "app_data", app_data);
        
        // Connecter le clic
        g_signal_connect_data(btn, "clicked", 
                             G_CALLBACK(on_border_country_clicked), 
                             g_strdup(border_code), 
                             (GClosureNotify)g_free, 0);
        
        // Ajouter au flowbox
        gtk_container_add(GTK_CONTAINER(flowbox), btn);

        printf("✅ Bouton créé pour: %s (%s)\n", border_country.name, border_code);
    }
    
    gtk_container_add(GTK_CONTAINER(frame), flowbox);
    return frame;
}

// ============ FONCTIONS POUR LES INFOS GOUVERNEMENT ============

GtkWidget* create_government_widget(Country *country) {
    GtkWidget *frame = gtk_frame_new("🏛️ Gouvernement & Langue");
    gtk_widget_set_margin_top(frame, 10);
    gtk_widget_set_margin_bottom(frame, 10);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    
    int row = 0;
    
    // Type de gouvernement
    GtkWidget *gov_label = gtk_label_new("Gouvernement:");
    gtk_widget_set_halign(gov_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), gov_label, 0, row, 1, 1);
    
    GtkWidget *gov_value = gtk_label_new(country->government);
    gtk_widget_set_halign(gov_value, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), gov_value, 1, row, 1, 1);
    row++;
    
    // Chef d'État
    GtkWidget *head_label = gtk_label_new("Chef d'État:");
    gtk_widget_set_halign(head_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), head_label, 0, row, 1, 1);
    
    GtkWidget *head_value = gtk_label_new(country->head_of_state);
    gtk_widget_set_halign(head_value, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), head_value, 1, row, 1, 1);
    row++;
    
    // Langue officielle
    GtkWidget *lang_label = gtk_label_new("Langue officielle:");
    gtk_widget_set_halign(lang_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), lang_label, 0, row, 1, 1);
    
    GtkWidget *lang_value = gtk_label_new(country->official_language);
    gtk_widget_set_halign(lang_value, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), lang_value, 1, row, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(frame), grid);
    return frame;
}
// ajout recent
/* ===================================
   FONCTIONS UTILITAIRES POUR LES IMAGES
   =================================== */

// Fonction pour charger et redimensionner une image
GtkWidget* create_scaled_image(const char *filename, int max_width, int max_height) {
    if (!filename || !g_file_test(filename, G_FILE_TEST_EXISTS)) {
        // Image par défaut si le fichier n'existe pas
        return gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);
    }
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    
    if (!pixbuf) {
        // Si échec du chargement
        printf("⚠️  Impossible de charger l'image: %s\n", filename);
        return gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);
    }
    
    // Obtenir les dimensions originales
    int orig_width = gdk_pixbuf_get_width(pixbuf);
    int orig_height = gdk_pixbuf_get_height(pixbuf);
    
    printf("📏 Image originale: %s - %dx%d\n", filename, orig_width, orig_height);
    
    // Si l'image est déjà plus petite que la taille max, on garde la taille originale
    if (orig_width <= max_width && orig_height <= max_height) {
        GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
        printf("🔄 Image conservée à sa taille originale\n");
        return image;
    }
    
    // Calculer les nouvelles dimensions en conservant les proportions
    double width_ratio = (double)max_width / orig_width;
    double height_ratio = (double)max_height / orig_height;
double ratio = fmin(width_ratio, height_ratio);
    
    int new_width = (int)(orig_width * ratio);
    int new_height = (int)(orig_height * ratio);
    
    // Assurer une taille minimale
    if (new_width < 50) new_width = 50;
    if (new_height < 50) new_height = 50;
    
    printf("🔄 Redimensionnement: %dx%d -> %dx%d (ratio: %.2f)\n", 
           orig_width, orig_height, new_width, new_height, ratio);
    
    // Redimensionner l'image
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, 
                                               new_width, new_height, 
                                               GDK_INTERP_BILINEAR);
    GtkWidget *image = gtk_image_new_from_pixbuf(scaled);
    
    // Nettoyer la mémoire
    g_object_unref(pixbuf);
    g_object_unref(scaled);
    
    return image;
}
// fin ajout recent

/* ===================================
   FONCTIONS UTILITAIRES
   =================================== */

   void afficher_details_pays(AppData *app_data, const char *code);

// Charger et afficher la liste des pays
void charger_liste_pays(AppData *app_data) {
    if (!app_data->list_store) return;
    
    gtk_list_store_clear(app_data->list_store);
    
    if (load_countries_index("data/index.json") != 0) {
        gtk_label_set_text(GTK_LABEL(app_data->status_label), 
                          "Erreur de chargement de l'index");
        return;
    }
    
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(app_data->search_entry));
    int total = get_index_count();
    int affiches = 0;
    
    for (int i = 0; i < total; i++) {
        const CountryIndex *idx = get_country_index(i);
        
        // Filtre de recherche
        if (strlen(search_text) > 0) {
            char search_lower[100];
            char name_lower[100];
            
            strcpy(search_lower, search_text);
            strcpy(name_lower, idx->name);
            
            for (int j = 0; search_lower[j]; j++) 
                search_lower[j] = tolower(search_lower[j]);
            for (int j = 0; name_lower[j]; j++) 
                name_lower[j] = tolower(name_lower[j]);
            
            if (strstr(name_lower, search_lower) == NULL) {
                continue;
            }
        }
        
        // Charger les details du pays
        Country country;
        if (load_country_from_file(idx->file, &country) == 0) {
            GtkTreeIter iter;
            char population_str[50], surface_str[50];
            
            snprintf(population_str, sizeof(population_str), "%ld", country.population);
            snprintf(surface_str, sizeof(surface_str), "%.0f km²", country.area);
            
            gtk_list_store_append(app_data->list_store, &iter);
            gtk_list_store_set(app_data->list_store, &iter,
                              0, idx->code,
                              1, country.name,
                              2, country.capital,
                              3, population_str,
                              4, surface_str,
                              5, country.continent,
                              6, country.currency,
                              -1);
            affiches++;
        }
    }
    
    char status[100];
    snprintf(status, sizeof(status), "%d pays sur %d affiches", affiches, total);
    gtk_label_set_text(GTK_LABEL(app_data->status_label), status);
}
// Callback pour les liens cliquables dans la description
gboolean on_description_link_clicked(GtkWidget *label, gchar *uri, gpointer data)
{
    AppData *app_data = (AppData *)data;
    
    // 🔹 DEBUG : Affiche dans la console
    printf("=== LIEN CLIQUE DETECTE ===\n");
    printf("URI reçu: %s\n", uri);
    printf("AppData valide: %s\n", app_data ? "OUI" : "NON");
    
    if (!uri || strlen(uri) == 0) {
        printf("ERREUR: URI vide!\n");
        return TRUE;
    }
    
    // uri = code du pays (ex: "ESP")
    printf("Ouverture de la fiche pour le code: %s\n", uri);
    
    printf("=== FIN DU CLIC ===\n\n");
    return TRUE; // Empêche GTK d'ouvrir un navigateur
}
// Variable globale pour garder la reference de la fenêtre




void afficher_details_pays(AppData *app_data, const char *code) {
    // 🔹 DEBUG COMPLET
    printf("\n🎯🎯🎯 NOUVELLE FENÊTRE DEMANDÉE 🎯🎯🎯\n");
    printf("🎯 Code demande: '%s'\n", code);
    printf("🎯 AppData valide: %s\n", app_data ? "OUI" : "NON");
    printf("🎯 Nombre total de pays indexes: %d\n", get_index_count());
    
    // Afficher tous les pays indexes
    for (int j = 0; j < get_index_count(); j++) {
        const CountryIndex *idx_test = get_country_index(j);
        printf("  [%d] Code: '%s' -> Nom: '%s'\n", j, idx_test->code, idx_test->name);
    }
    
    for (int i = 0; i < get_index_count(); i++) {
        const CountryIndex *idx = get_country_index(i);
        printf("\n🎯 Comparaison %d: '%s' vs '%s'\n", i, idx->code, code);
        
        if (strcmp(idx->code, code) == 0) {
            printf("🎯🎯 MATCH TROUVÉ ! 🎯🎯\n");
            printf("🎯 Fichier a charger: %s\n", idx->file);
            
            if (load_country_from_file(idx->file, &app_data->current_country) == 0) {
                printf("🎯 Fichier charge avec succes!\n");
                printf("🎯 Nom du pays: %s\n", app_data->current_country.name);
                printf("🎯 Image: %s\n", app_data->current_country.image); // Debug image
                strcpy(app_data->current_code, code);
                
                // 🔹 DEBUG
                printf("=== OUVRIR FICHE PAYS ===\n");
                printf("Code: %s, Nom: %s\n", code, app_data->current_country.name);
                
                // Fermer la fenêtre precedente si elle existe
                if (current_details_dialog != NULL && GTK_IS_WIDGET(current_details_dialog)) {
                    printf("Fermeture fenêtre precedente (widget valide)\n");
                    gtk_widget_destroy(current_details_dialog);
                    current_details_dialog = NULL;
                } else {
                    printf("Pas de fenêtre precedente a fermer\n");
                }
                
                // Creer une fenêtre de details
                GtkWidget *dialog = gtk_dialog_new_with_buttons("Details du pays",
                    GTK_WINDOW(app_data->window),
                    GTK_DIALOG_DESTROY_WITH_PARENT,  // PAS MODAL
                    "Fermer", GTK_RESPONSE_CLOSE,
                    NULL);
                
                gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 500); // 🔹 +100 pixels pour l'image
                
                // Stocker la reference
                current_details_dialog = dialog;
                
                GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
                GtkWidget *grid = gtk_grid_new();
                gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
                gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
                gtk_container_set_border_width(GTK_CONTAINER(grid), 20);

                // Titre
                GtkWidget *title = gtk_label_new(NULL);
                char title_text[200];
                snprintf(title_text, sizeof(title_text), 
                        "<b>%s</b>\n%s",
                        app_data->current_country.name,
                        app_data->current_country.capital);
                gtk_label_set_markup(GTK_LABEL(title), title_text);
                gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
                gtk_grid_attach(GTK_GRID(grid), title, 0, 0, 2, 1);
                
                // 🔹 IMAGE DU PAYS (redimensionnée automatiquement)
GtkWidget *image = NULL;
const char *default_image = "data/images/default.jpg";

printf("🎯 Image JSON: '%s'\n", app_data->current_country.image);

// Définir la taille maximale souhaitée pour l'image
const int MAX_IMAGE_WIDTH = 300;
const int MAX_IMAGE_HEIGHT = 200;

// 1. Priorité: image du JSON
if (strlen(app_data->current_country.image) > 0 && 
    g_file_test(app_data->current_country.image, G_FILE_TEST_EXISTS)) {
    image = create_scaled_image(app_data->current_country.image, 
                                MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT);
    printf("✅ Image du pays chargée et redimensionnée: %s\n", 
           app_data->current_country.image);
} 
// 2. Fallback: image par défaut
else if (g_file_test(default_image, G_FILE_TEST_EXISTS)) {
    image = create_scaled_image(default_image, 
                                MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT);
    printf("🔄 Image par défaut chargée et redimensionnée: %s\n", default_image);
} 
// 3. Dernier recours: icône GTK
else {
    printf("⚠️  Aucune image disponible\n");
    image = gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);
}
                
                // Centrer l'image
                GtkWidget *image_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
                gtk_widget_set_halign(image_box, GTK_ALIGN_CENTER);
                gtk_box_pack_start(GTK_BOX(image_box), image, FALSE, FALSE, 0);
                gtk_grid_attach(GTK_GRID(grid), image_box, 0, 1, 2, 1);
                
                // Separateur
                GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
                gtk_grid_attach(GTK_GRID(grid), separator, 0, 2, 2, 1);

                
                // Informations (TOUTES DÉCALÉES DE +2)
                // Population (ligne 3 au lieu de 2)
                GtkWidget *pop_label = gtk_label_new("Population:");
                gtk_widget_set_halign(pop_label, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), pop_label, 0, 3, 1, 1); // 🔹 2→3
                
                char pop_str[50];
                snprintf(pop_str, sizeof(pop_str), "%ld habitants", 
                        app_data->current_country.population);
                GtkWidget *pop_value = gtk_label_new(pop_str);
                gtk_widget_set_halign(pop_value, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), pop_value, 1, 3, 1, 1); // 🔹 2→3
                
                // Surface (ligne 4 au lieu de 3)
                GtkWidget *area_label = gtk_label_new("Surface:");
                gtk_widget_set_halign(area_label, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), area_label, 0, 4, 1, 1); // 🔹 3→4
                
                char area_str[50];
                snprintf(area_str, sizeof(area_str), "%.0f km²", 
                        app_data->current_country.area);
                GtkWidget *area_value = gtk_label_new(area_str);
                gtk_widget_set_halign(area_value, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), area_value, 1, 4, 1, 1); // 🔹 3→4
                
                // Devise (ligne 5 au lieu de 4)
                GtkWidget *curr_label = gtk_label_new("Devise:");
                gtk_widget_set_halign(curr_label, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), curr_label, 0, 5, 1, 1); // 🔹 4→5
                
                GtkWidget *curr_value = gtk_label_new(app_data->current_country.currency);
                gtk_widget_set_halign(curr_value, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), curr_value, 1, 5, 1, 1); // 🔹 4→5
                
                // Continent (ligne 6 au lieu de 5)
                GtkWidget *cont_label = gtk_label_new("Continent:");
                gtk_widget_set_halign(cont_label, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), cont_label, 0, 6, 1, 1); // 🔹 5→6
                
                GtkWidget *cont_value = gtk_label_new(app_data->current_country.continent);
                gtk_widget_set_halign(cont_value, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), cont_value, 1, 6, 1, 1); // 🔹 5→6
                
                // Description (ligne 7 au lieu de 6)
                GtkWidget *desc_label = gtk_label_new("Description:");
                gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
                gtk_widget_set_valign(desc_label, GTK_ALIGN_START);
                gtk_grid_attach(GTK_GRID(grid), desc_label, 0, 7, 1, 1); // 🔹 6→7
                
                GtkWidget *desc_text = gtk_label_new(NULL);

                // Activer le markup (HTML-like)
                gtk_label_set_use_markup(GTK_LABEL(desc_text), TRUE);
                gtk_label_set_line_wrap(GTK_LABEL(desc_text), TRUE);
                gtk_label_set_max_width_chars(GTK_LABEL(desc_text), 60);
                gtk_label_set_justify(GTK_LABEL(desc_text), GTK_JUSTIFY_FILL);

                // 🔹 CRITIQUE : Desactiver le tracking des liens visites
                gtk_label_set_track_visited_links(GTK_LABEL(desc_text), FALSE);

                // 🔹 Permettre la focalisation (pour les clics)
                gtk_widget_set_can_focus(desc_text, TRUE);

                // Texte avec liens
                gtk_label_set_markup(GTK_LABEL(desc_text),
                                     app_data->current_country.description);

                // 🔹 Connecter le clic sur les liens
                g_signal_connect(desc_text, "activate-link", 
                                G_CALLBACK(on_description_link_clicked), 
                                app_data);

                // 🔹 ATTACHE DIRECTEMENT AU GRID
                gtk_grid_attach(GTK_GRID(grid), desc_text, 1, 7, 1, 1); // 🔹 6→7
                
                // ============ NOUVEAUX WIDGETS AJOUTÉS ICI ============
                
                // ��� PAYS LIMITROPHES (ligne 8)
                GtkWidget *borders_widget = create_borders_widget(app_data, &app_data->current_country);
                gtk_grid_attach(GTK_GRID(grid), borders_widget, 0, 8, 2, 1);
                
                // ��� GOUVERNEMENT & LANGUE (ligne 9)
                GtkWidget *gov_widget = create_government_widget(&app_data->current_country);
                gtk_grid_attach(GTK_GRID(grid), gov_widget, 0, 9, 2, 1);
                
                // ======================================================
                gtk_container_add(GTK_CONTAINER(content), grid);
                
                // 🔹 AFFICHER LA FENÊTRE
                gtk_widget_show_all(dialog);
                
                // 🔹 DEBUG
                printf("🎯 Fenêtre affichee pour: %s\n", app_data->current_country.name);
                printf("🎯 Taille: 500x500 (avec image)\n");
                printf("🎯 Dialog address: %p\n", dialog);
                printf("🎯 ===========================\n\n");
                
                // 🔹 Connecter le bouton "Fermer"
                g_signal_connect_swapped(dialog, "response", 
                                        G_CALLBACK(gtk_widget_destroy), 
                                        dialog);
            } else {
                printf("❌ ERREUR: Impossible de charger le fichier %s\n", idx->file);
            }
            break;
        }
    }
    
    printf("🎯 Fin de la fonction afficher_details_pays\n");
}

/* ===================================
   FONCTIONS POUR LA GESTION DES IMAGES
   =================================== */

// Callback pour selectionner une image (version pour ajout)
void on_select_image_add(GtkWidget *button, gpointer data) {
    GtkWidget *image_label = (GtkWidget *)data;
    
    // On recupere la fenêtre parente via le widget
    GtkWidget *toplevel = gtk_widget_get_toplevel(button);
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Selectionner une image",
                                                   GTK_IS_WINDOW(toplevel) ? GTK_WINDOW(toplevel) : NULL,
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "Annuler", GTK_RESPONSE_CANCEL,
                                                   "Ouvrir", GTK_RESPONSE_ACCEPT,
                                                   NULL);
    
    // Filtrer pour les images
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.gif");
    gtk_file_filter_add_pattern(filter, "*.bmp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Definir le dossier par defaut
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "data/images");
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        if (filename) {
            // Copier l'image dans le dossier du projet
            char dest_path[256];
            char *basename = g_path_get_basename(filename);
            
            // Creer le dossier images s'il n'existe pas
            g_mkdir_with_parents("data/images", 0755);
            
            // Chemin de destination
            snprintf(dest_path, sizeof(dest_path), "data/images/%s", basename);
            
            // Verifier si le fichier existe deja
            if (!g_file_test(dest_path, G_FILE_TEST_EXISTS)) {
                // Copier le fichier en utilisant FILE* au lieu de GFile
                FILE *src = fopen(filename, "rb");
                FILE *dst = fopen(dest_path, "wb");
                
                if (src && dst) {
                    char buffer[4096];
                    size_t n;
                    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                        fwrite(buffer, 1, n, dst);
                    }
                    fclose(src);
                    fclose(dst);
                    printf("✅ Image copiee vers : %s\n", dest_path);
                } else {
                    printf("❌ Erreur ouverture fichier pour copie\n");
                    // Si la copie echoue, utiliser le chemin absolu
                    strcpy(dest_path, filename);
                    printf("🔄 Utilisation chemin absolu: %s\n", dest_path);
                }
            } else {
                printf("ℹ️  Image deja existante: %s\n", dest_path);
            }
            
            // Mettre a jour le label avec le chemin relatif
            gtk_label_set_text(GTK_LABEL(image_label), dest_path);
            
            g_free(basename);
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Callback pour selectionner une image (version pour modification)
void on_select_image_edit(GtkWidget *button, gpointer data) {
    AppData *app_data = (AppData *)data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Selectionner une image",
                                                   GTK_WINDOW(app_data->window),
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "Annuler", GTK_RESPONSE_CANCEL,
                                                   "Ouvrir", GTK_RESPONSE_ACCEPT,
                                                   NULL);
    
    // Filtrer pour les images
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.gif");
    gtk_file_filter_add_pattern(filter, "*.bmp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Definir le dossier par defaut
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "data/images");
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        if (filename) {
            // Copier l'image dans le dossier du projet
            char dest_path[256];
            char *basename = g_path_get_basename(filename);
            
            // Creer le dossier images s'il n'existe pas
            g_mkdir_with_parents("data/images", 0755);
            
            // Chemin de destination
            snprintf(dest_path, sizeof(dest_path), "data/images/%s", basename);
            
            // Verifier si le fichier existe deja
            if (!g_file_test(dest_path, G_FILE_TEST_EXISTS)) {
                // Copier le fichier en utilisant FILE* au lieu de GFile
                FILE *src = fopen(filename, "rb");
                FILE *dst = fopen(dest_path, "wb");
                
                if (src && dst) {
                    char buffer[4096];
                    size_t n;
                    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                        fwrite(buffer, 1, n, dst);
                    }
                    fclose(src);
                    fclose(dst);
                    printf("✅ Image copiee vers : %s\n", dest_path);
                } else {
                    printf("❌ Erreur ouverture fichier pour copie\n");
                    // Si la copie echoue, utiliser le chemin absolu
                    strcpy(dest_path, filename);
                    printf("🔄 Utilisation chemin absolu: %s\n", dest_path);
                }
            } else {
                printf("ℹ️  Image deja existante: %s\n", dest_path);
            }
            
            // Mettre a jour le label avec le chemin relatif
            gtk_label_set_text(GTK_LABEL(app_data->detail_image_label), dest_path);
            strcpy(app_data->image_path, dest_path);
            
            g_free(basename);
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(dialog);
}
///////////////////////////////////////////////////////
// Ouvrir la fenêtre de modification - VERSION AVEC IMAGE
void ouvrir_fenetre_modification(AppData *app_data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Modifier le pays",
        GTK_WINDOW(app_data->window),
        GTK_DIALOG_MODAL,
        "Sauvegarder", GTK_RESPONSE_OK,
        "Annuler", GTK_RESPONSE_CANCEL,
        NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 600); // Augmente pour l'image
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    // Utiliser une GtkBox
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    int current_row = 0;
    
    // Code (lecture seule si modification)
    GtkWidget *code_label = gtk_label_new("Code:");
    gtk_widget_set_halign(code_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), code_label, 0, current_row, 1, 1);
    
    GtkWidget *code_display = gtk_label_new(app_data->current_code);
    gtk_widget_set_halign(code_display, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), code_display, 1, current_row, 1, 1);
    current_row++;
    
    // Nom
    GtkWidget *nom_label = gtk_label_new("Nom:");
    gtk_widget_set_halign(nom_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), nom_label, 0, current_row, 1, 1);
    
    app_data->detail_nom_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app_data->detail_nom_entry), app_data->current_country.name);
    gtk_grid_attach(GTK_GRID(grid), app_data->detail_nom_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Capitale
    GtkWidget *cap_label = gtk_label_new("Capitale:");
    gtk_widget_set_halign(cap_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), cap_label, 0, current_row, 1, 1);
    
    app_data->detail_capitale_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app_data->detail_capitale_entry), app_data->current_country.capital);
    gtk_grid_attach(GTK_GRID(grid), app_data->detail_capitale_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Population
    GtkWidget *pop_label = gtk_label_new("Population:");
    gtk_widget_set_halign(pop_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), pop_label, 0, current_row, 1, 1);
    
    app_data->detail_population_entry = gtk_entry_new();
    char pop_str[50];
    snprintf(pop_str, sizeof(pop_str), "%ld", app_data->current_country.population);
    gtk_entry_set_text(GTK_ENTRY(app_data->detail_population_entry), pop_str);
    gtk_entry_set_input_purpose(GTK_ENTRY(app_data->detail_population_entry), GTK_INPUT_PURPOSE_NUMBER);
    gtk_grid_attach(GTK_GRID(grid), app_data->detail_population_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Surface
    GtkWidget *area_label = gtk_label_new("Surface (km²):");
    gtk_widget_set_halign(area_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), area_label, 0, current_row, 1, 1);
    
    app_data->detail_surface_entry = gtk_entry_new();
    char area_str[50];
    snprintf(area_str, sizeof(area_str), "%.0f", app_data->current_country.area);
    gtk_entry_set_text(GTK_ENTRY(app_data->detail_surface_entry), area_str);
    gtk_entry_set_input_purpose(GTK_ENTRY(app_data->detail_surface_entry), GTK_INPUT_PURPOSE_NUMBER);
    gtk_grid_attach(GTK_GRID(grid), app_data->detail_surface_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Devise
    GtkWidget *curr_label = gtk_label_new("Devise:");
    gtk_widget_set_halign(curr_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), curr_label, 0, current_row, 1, 1);
    
    app_data->detail_devise_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app_data->detail_devise_entry), app_data->current_country.currency);
    gtk_grid_attach(GTK_GRID(grid), app_data->detail_devise_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Continent
    GtkWidget *cont_label = gtk_label_new("Continent:");
    gtk_widget_set_halign(cont_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), cont_label, 0, current_row, 1, 1);
    
    app_data->detail_continent_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app_data->detail_continent_combo), "Europe");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app_data->detail_continent_combo), "Asie");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app_data->detail_continent_combo), "Amerique");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app_data->detail_continent_combo), "Afrique");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app_data->detail_continent_combo), "Oceanie");
    
    // Selectionner le continent actuel
    const gchar *continents[] = {"Europe", "Asie", "Amerique", "Afrique", "Oceanie"};
    for (int j = 0; j < 5; j++) {
        if (strcmp(continents[j], app_data->current_country.continent) == 0) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(app_data->detail_continent_combo), j);
            break;
        }
    }
    
    gtk_grid_attach(GTK_GRID(grid), app_data->detail_continent_combo, 1, current_row, 1, 1);
    current_row++;
    
    // Description
    GtkWidget *desc_label = gtk_label_new("Description:");
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    gtk_widget_set_valign(desc_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), desc_label, 0, current_row, 1, 1);
    
    app_data->detail_description_text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app_data->detail_description_text), GTK_WRAP_WORD);
    gtk_widget_set_size_request(app_data->detail_description_text, 250, 80);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app_data->detail_description_text));
    gtk_text_buffer_set_text(buffer, app_data->current_country.description, -1);
    gtk_grid_attach(GTK_GRID(grid), app_data->detail_description_text, 1, current_row, 1, 1);
    current_row++;
    
    // 🔹 IMAGE (AJOUTÉ ICI)
    GtkWidget *img_label = gtk_label_new("Image:");
    gtk_widget_set_halign(img_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), img_label, 0, current_row, 1, 1);
    
    GtkWidget *img_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    // Label pour afficher le chemin
    app_data->detail_image_label = gtk_label_new(app_data->current_country.image);
    gtk_label_set_ellipsize(GTK_LABEL(app_data->detail_image_label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_max_width_chars(GTK_LABEL(app_data->detail_image_label), 30);
    gtk_box_pack_start(GTK_BOX(img_box), app_data->detail_image_label, TRUE, TRUE, 0);
    
    // Bouton pour selectionner l'image
    GtkWidget *img_button = gtk_button_new_with_label("Parcourir...");
    gtk_box_pack_start(GTK_BOX(img_box), img_button, FALSE, FALSE, 0);
    
    gtk_grid_attach(GTK_GRID(grid), img_box, 1, current_row, 1, 1);
    
    // Stocker le chemin actuel
    strcpy(app_data->image_path, app_data->current_country.image);
    
    // Connecter le bouton
    g_signal_connect(img_button, "clicked", G_CALLBACK(on_select_image_edit), app_data);
    
    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        // Recuperer les nouvelles valeurs
        const gchar *new_nom = gtk_entry_get_text(GTK_ENTRY(app_data->detail_nom_entry));
        const gchar *new_capitale = gtk_entry_get_text(GTK_ENTRY(app_data->detail_capitale_entry));
        const gchar *new_population = gtk_entry_get_text(GTK_ENTRY(app_data->detail_population_entry));
        const gchar *new_surface = gtk_entry_get_text(GTK_ENTRY(app_data->detail_surface_entry));
        const gchar *new_devise = gtk_entry_get_text(GTK_ENTRY(app_data->detail_devise_entry));
        gchar *new_continent = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(app_data->detail_continent_combo));
        
        GtkTextBuffer *new_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app_data->detail_description_text));
        GtkTextIter new_start, new_end;
        gtk_text_buffer_get_start_iter(new_buffer, &new_start);
        gtk_text_buffer_get_end_iter(new_buffer, &new_end);
        gchar *new_description = gtk_text_buffer_get_text(new_buffer, &new_start, &new_end, FALSE);
        
        // Recuperer le chemin de l'image
        const gchar *new_image = gtk_label_get_text(GTK_LABEL(app_data->detail_image_label));
        
        // Mettre a jour la structure
        strcpy(app_data->current_country.name, new_nom);
        strcpy(app_data->current_country.capital, new_capitale);
        strcpy(app_data->current_country.currency, new_devise);
        strcpy(app_data->current_country.continent, new_continent);
        strcpy(app_data->current_country.description, new_description);
        strcpy(app_data->current_country.image, new_image);
        app_data->current_country.population = atol(new_population);
        app_data->current_country.area = atof(new_surface);
        
        // Utiliser votre fonction backend pour mettre a jour
        if (update_country_index(app_data->current_code, app_data->current_country) == 0) {
            gtk_label_set_text(GTK_LABEL(app_data->status_label), 
                              "Pays modifie avec succes");
            charger_liste_pays(app_data);
        } else {
            gtk_label_set_text(GTK_LABEL(app_data->status_label), 
                              "Erreur lors de la modification");
        }
        
        g_free(new_continent);
        g_free(new_description);
    }
    
    gtk_widget_destroy(dialog);
}

/* ===================================
   CALLBACKS POUR LES BOUTONS
   =================================== */

// Double-clic sur un pays
void on_pays_double_click(GtkTreeView *treeview, GtkTreePath *path,
                         GtkTreeViewColumn *col, gpointer data) {
    AppData *app_data = (AppData*)data;
    
    GtkTreeModel *model;
    GtkTreeIter iter;
    char *code;
    
    model = gtk_tree_view_get_model(treeview);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, 0, &code, -1);
        afficher_details_pays(app_data, code);
        g_free(code);
    }
}

// Actualiser la liste
void on_actualiser(GtkButton *button, gpointer data) {
    AppData *app_data = (AppData*)data;
    charger_liste_pays(app_data);
}

// Ajouter un nouveau pays - VERSION AVEC IMAGE
void on_ajouter_pays(GtkButton *button, gpointer data) {
    AppData *app_data = (AppData*)data;
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Ajouter un pays",
        GTK_WINDOW(app_data->window),
        GTK_DIALOG_MODAL,
        "Ajouter", GTK_RESPONSE_OK,
        "Annuler", GTK_RESPONSE_CANCEL,
        NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 600); // Augmente pour l'image
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    // Utiliser une GtkBox
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    int current_row = 0;
    
    // Code du pays
    GtkWidget *code_label = gtk_label_new("Code (3 lettres):");
    gtk_widget_set_halign(code_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), code_label, 0, current_row, 1, 1);
    
    GtkWidget *code_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(code_entry), 3);
    gtk_grid_attach(GTK_GRID(grid), code_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Nom
    GtkWidget *nom_label = gtk_label_new("Nom:");
    gtk_widget_set_halign(nom_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), nom_label, 0, current_row, 1, 1);
    
    GtkWidget *nom_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), nom_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Capitale
    GtkWidget *cap_label = gtk_label_new("Capitale:");
    gtk_widget_set_halign(cap_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), cap_label, 0, current_row, 1, 1);
    
    GtkWidget *cap_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), cap_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Population
    GtkWidget *pop_label = gtk_label_new("Population:");
    gtk_widget_set_halign(pop_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), pop_label, 0, current_row, 1, 1);
    
    GtkWidget *pop_entry = gtk_entry_new();
    gtk_entry_set_input_purpose(GTK_ENTRY(pop_entry), GTK_INPUT_PURPOSE_NUMBER);
    gtk_grid_attach(GTK_GRID(grid), pop_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Surface
    GtkWidget *area_label = gtk_label_new("Surface (km²):");
    gtk_widget_set_halign(area_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), area_label, 0, current_row, 1, 1);
    
    GtkWidget *area_entry = gtk_entry_new();
    gtk_entry_set_input_purpose(GTK_ENTRY(area_entry), GTK_INPUT_PURPOSE_NUMBER);
    gtk_grid_attach(GTK_GRID(grid), area_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Devise
    GtkWidget *curr_label = gtk_label_new("Devise:");
    gtk_widget_set_halign(curr_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), curr_label, 0, current_row, 1, 1);
    
    GtkWidget *curr_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), curr_entry, 1, current_row, 1, 1);
    current_row++;
    
    // Continent
    GtkWidget *cont_label = gtk_label_new("Continent:");
    gtk_widget_set_halign(cont_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), cont_label, 0, current_row, 1, 1);
    
    GtkWidget *cont_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cont_combo), "Europe");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cont_combo), "Asie");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cont_combo), "Amerique");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cont_combo), "Afrique");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cont_combo), "Oceanie");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cont_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), cont_combo, 1, current_row, 1, 1);
    current_row++;
    
    // Description
    GtkWidget *desc_label = gtk_label_new("Description:");
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    gtk_widget_set_valign(desc_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), desc_label, 0, current_row, 1, 1);
    
    GtkWidget *desc_text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(desc_text), GTK_WRAP_WORD);
    gtk_widget_set_size_request(desc_text, 250, 80);
    gtk_grid_attach(GTK_GRID(grid), desc_text, 1, current_row, 1, 1);
    current_row++;
    
    // 🔹 IMAGE (AJOUTÉ ICI)
    GtkWidget *img_label = gtk_label_new("Image:");
    gtk_widget_set_halign(img_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), img_label, 0, current_row, 1, 1);
    
    GtkWidget *img_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    // Label pour afficher le chemin
    GtkWidget *image_label = gtk_label_new("data/images/default.jpg");
    gtk_label_set_ellipsize(GTK_LABEL(image_label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_max_width_chars(GTK_LABEL(image_label), 30);
    gtk_box_pack_start(GTK_BOX(img_box), image_label, TRUE, TRUE, 0);
    
    // Bouton pour selectionner l'image
    GtkWidget *img_button = gtk_button_new_with_label("Parcourir...");
    gtk_box_pack_start(GTK_BOX(img_box), img_button, FALSE, FALSE, 0);
    
    gtk_grid_attach(GTK_GRID(grid), img_box, 1, current_row, 1, 1);
    
    // Initialiser le chemin d'image
    strcpy(app_data->image_path, "data/images/default.jpg");
    
    // Connecter le bouton
    g_signal_connect(img_button, "clicked", G_CALLBACK(on_select_image_add), image_label);
    
    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        // Recuperer les valeurs
        const gchar *code = gtk_entry_get_text(GTK_ENTRY(code_entry));
        const gchar *nom = gtk_entry_get_text(GTK_ENTRY(nom_entry));
        const gchar *capitale = gtk_entry_get_text(GTK_ENTRY(cap_entry));
        const gchar *population = gtk_entry_get_text(GTK_ENTRY(pop_entry));
        const gchar *surface = gtk_entry_get_text(GTK_ENTRY(area_entry));
        const gchar *devise = gtk_entry_get_text(GTK_ENTRY(curr_entry));
        gchar *continent = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cont_combo));
        
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(desc_text));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        gchar *description = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        // 🔹 RÉCUPÉRER LE CHEMIN DE L'IMAGE
        const gchar *image_path = gtk_label_get_text(GTK_LABEL(image_label));
        
        // Valider
        if (strlen(code) != 3 || strlen(nom) == 0 || strlen(capitale) == 0) {
            GtkWidget *error = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     "Code (3 lettres), nom et capitale sont obligatoires");
            gtk_dialog_run(GTK_DIALOG(error));
            gtk_widget_destroy(error);
        } else {
            // Creer le pays
            Country new_country;
            strcpy(new_country.name, nom);
            strcpy(new_country.capital, capitale);
            strcpy(new_country.currency, devise);
            strcpy(new_country.continent, continent);
            strcpy(new_country.description, description);
            // 🔹 SAUVEGARDER LE CHEMIN DE L'IMAGE
            strcpy(new_country.image, image_path);
            new_country.population = atol(population);
            new_country.area = atof(surface);
            
            // Utiliser votre fonction backend
            if (add_country_index(new_country, code) == 0) {
                gtk_label_set_text(GTK_LABEL(app_data->status_label), 
                                  "Pays ajoute avec succes");
                charger_liste_pays(app_data);
            } else {
                gtk_label_set_text(GTK_LABEL(app_data->status_label), 
                                  "Erreur lors de l'ajout");
            }
        }
        
        g_free(continent);
        g_free(description);
    }
    
    gtk_widget_destroy(dialog);
}

// Modifier un pays
void on_modifier_pays(GtkButton *button, gpointer data) {
    AppData *app_data = (AppData*)data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app_data->treeview));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        char *code;
        gtk_tree_model_get(model, &iter, 0, &code, -1);
        
        // Trouver le pays
        for (int i = 0; i < get_index_count(); i++) {
            const CountryIndex *idx = get_country_index(i);
            if (strcmp(idx->code, code) == 0) {
                if (load_country_from_file(idx->file, &app_data->current_country) == 0) {
                    strcpy(app_data->current_code, code);
                    app_data->edit_mode = 1;
                    ouvrir_fenetre_modification(app_data);
                }
                break;
            }
        }
        g_free(code);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app_data->window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_WARNING,
                                                  GTK_BUTTONS_OK,
                                                  "Veuillez selectionner un pays a modifier");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// Supprimer un pays
void on_supprimer_pays(GtkButton *button, gpointer data) {
    AppData *app_data = (AppData*)data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app_data->treeview));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        char *code;
        char *nom;
        gtk_tree_model_get(model, &iter, 0, &code, 1, &nom, -1);
        
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app_data->window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_QUESTION,
                                                  GTK_BUTTONS_YES_NO,
                                                  "Voulez-vous vraiment supprimer le pays '%s' ?", nom);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            // Utiliser votre fonction backend pour supprimer
            if (remove_country_index(code) == 0) {
                gtk_label_set_text(GTK_LABEL(app_data->status_label), 
                                  "Pays supprime avec succes");
                charger_liste_pays(app_data);
            } else {
                gtk_label_set_text(GTK_LABEL(app_data->status_label), 
                                  "Erreur lors de la suppression");
            }
        }
        
        gtk_widget_destroy(dialog);
        g_free(code);
        g_free(nom);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app_data->window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_WARNING,
                                                  GTK_BUTTONS_OK,
                                                  "Veuillez selectionner un pays a supprimer");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// Recherche
void on_search_changed(GtkWidget *widget, gpointer data) {
    AppData *app_data = (AppData*)data;
    charger_liste_pays(app_data);
}

// Quitter
void on_quitter(GtkButton *button, gpointer data) {
    gtk_main_quit();
}

/* ===================================
   FONCTION PRINCIPALE DE L'INTERFACE
   =================================== */

int gui_main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    AppData app_data;
    memset(&app_data, 0, sizeof(AppData));
    
    // Creer la fenêtre principale
    app_data.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_data.window), "Gestion des Pays");
    gtk_window_set_default_size(GTK_WINDOW(app_data.window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(app_data.window), GTK_WIN_POS_CENTER);
    
    // Conteneur principal
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app_data.window), main_box);
    
    // Titre
    GtkWidget *titre = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(titre), "<b>GESTION DES PAYS</b>");
    gtk_widget_set_margin_top(titre, 10);
    gtk_widget_set_margin_bottom(titre, 10);
    gtk_box_pack_start(GTK_BOX(main_box), titre, FALSE, FALSE, 0);
    
    // Barre de recherche
    GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(search_box, 10);
    gtk_widget_set_margin_end(search_box, 10);
    
    GtkWidget *search_label = gtk_label_new("Rechercher:");
    app_data.search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app_data.search_entry), 
                                  "Nom du pays...");
    g_signal_connect(app_data.search_entry, "changed", 
                    G_CALLBACK(on_search_changed), &app_data);
    
    gtk_box_pack_start(GTK_BOX(search_box), search_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), app_data.search_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), search_box, FALSE, FALSE, 0);
    
    // Liste des pays
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    
    // Modele de donnees
    app_data.list_store = gtk_list_store_new(7, 
                                            G_TYPE_STRING, // Code
                                            G_TYPE_STRING, // Nom
                                            G_TYPE_STRING, // Capitale
                                            G_TYPE_STRING, // Population
                                            G_TYPE_STRING, // Surface
                                            G_TYPE_STRING, // Continent
                                            G_TYPE_STRING); // Devise
    
    // TreeView
    app_data.treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app_data.list_store));
    
    // Double-clic
    g_signal_connect(app_data.treeview, "row-activated", 
                    G_CALLBACK(on_pays_double_click), &app_data);
    
    // Colonnes
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Code", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app_data.treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app_data.treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Capitale", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app_data.treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Population", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app_data.treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Surface", renderer, "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app_data.treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Continent", renderer, "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app_data.treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Devise", renderer, "text", 6, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app_data.treeview), column);
    
    gtk_container_add(GTK_CONTAINER(scrolled), app_data.treeview);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled, TRUE, TRUE, 0);
    
    // Boutons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(button_box, 10);
    gtk_widget_set_margin_bottom(button_box, 10);
    
    GtkWidget *btn_actualiser = gtk_button_new_with_label("Actualiser");
    GtkWidget *btn_ajouter = gtk_button_new_with_label("Ajouter");
    GtkWidget *btn_modifier = gtk_button_new_with_label("Modifier");
    GtkWidget *btn_supprimer = gtk_button_new_with_label("Supprimer");
    GtkWidget *btn_quitter = gtk_button_new_with_label("Quitter");
    
    g_signal_connect(btn_actualiser, "clicked", G_CALLBACK(on_actualiser), &app_data);
    g_signal_connect(btn_ajouter, "clicked", G_CALLBACK(on_ajouter_pays), &app_data);
    g_signal_connect(btn_modifier, "clicked", G_CALLBACK(on_modifier_pays), &app_data);
    g_signal_connect(btn_supprimer, "clicked", G_CALLBACK(on_supprimer_pays), &app_data);
    g_signal_connect(btn_quitter, "clicked", G_CALLBACK(on_quitter), NULL);
    
    gtk_box_pack_start(GTK_BOX(button_box), btn_actualiser, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_ajouter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_modifier, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_supprimer, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_quitter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);
    
    // Barre de statut
    app_data.status_label = gtk_label_new("Prêt - Cliquez sur Actualiser pour charger les pays");
    gtk_box_pack_start(GTK_BOX(main_box), app_data.status_label, FALSE, FALSE, 0);
    
    // Connecter la fermeture
    g_signal_connect(app_data.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Afficher
    gtk_widget_show_all(app_data.window);
    
    // Charger les pays
    charger_liste_pays(&app_data);
    
    // Lancer
    gtk_main();
    
    // Nettoyer
    free_database();
    
    return 0;
}