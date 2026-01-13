#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "database.h"

/* ==============================
   Fonction utilitaire : recherche insensible à la casse
   ============================== */
int contains_case_insensitive(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0)
            return 1;
    }
    return 0;
}

/* ==============================
   Widgets globaux
   ============================== */
GtkWidget *listbox;
GtkWidget *entry;
GtkWidget *details_label;

/* ==============================
   Affiche les détails d’un pays
   ============================== */
static void show_country_details(const CountryIndex *idx) {
    if (!idx) return;

    Country c;
    if (load_country_from_file(idx->file, &c) != 0) {
        gtk_label_set_text(GTK_LABEL(details_label), "Erreur chargement pays");
        return;
    }

    #define MAX_DESC 1024
    char desc_trunc[MAX_DESC];
    strncpy(desc_trunc, c.description, MAX_DESC - 1);
    desc_trunc[MAX_DESC - 1] = '\0';

    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
             "Nom      : %s\n"
             "Code     : %s\n"
             "Capitale : %s\n"
             "Population : %ld\n"
             "Superficie : %ld km²\n"
             "Devise   : %s\n"
             "Continent: %s\n"
             "Description :\n%s\n",
             c.name, idx->code, c.capital, c.population,
             c.area, c.currency, c.continent, desc_trunc);

    gtk_label_set_text(GTK_LABEL(details_label), buffer);
}

/* ==============================
   Callback selection pays
   ============================== */
static void on_country_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)user_data;

    if (!row) return;
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "index"));
    const CountryIndex *idx = get_country_index(index);
    show_country_details(idx);
}

/* ==============================
   Callback recherche
   ============================== */
static void on_search_changed(GtkEntry *entry_widget, gpointer user_data) {
    (void)user_data;
    const char *text = gtk_entry_get_text(entry_widget);

    GList *rows = gtk_container_get_children(GTK_CONTAINER(listbox));
    GtkListBoxRow *first_visible = NULL;

    for (GList *iter = rows; iter; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "index"));
        const CountryIndex *idx = get_country_index(index);

        if (strlen(text) == 0 ||
            contains_case_insensitive(idx->name, text) ||
            contains_case_insensitive(idx->code, text)) {
            gtk_widget_show(row);
            if (!first_visible)
                first_visible = GTK_LIST_BOX_ROW(row);
        } else {
            gtk_widget_hide(row);
        }
    }
    g_list_free(rows);

    // Affiche automatiquement le premier pays visible
    if (first_visible)
        on_country_selected(GTK_LIST_BOX(listbox), first_visible, NULL);
}

/* ==============================
   Remplir la liste des pays
   ============================== */
static void populate_listbox() {
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);

    for (int i = 0; i < get_index_count(); i++) {
        const CountryIndex *idx = get_country_index(i);
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(idx->name);
        gtk_container_add(GTK_CONTAINER(row), label);
        g_object_set_data(G_OBJECT(row), "index", GINT_TO_POINTER(i));
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    }

    // Connecter le signal de sélection sur le listbox lui-même
    g_signal_connect(listbox, "row-selected", G_CALLBACK(on_country_selected), NULL);

    gtk_widget_show_all(listbox);
}

/* ==============================
   Main GTK
   ============================== */
int main(int argc, char *argv[]) {
    if (load_countries_index("data/index.json") != 0) {
        printf("Erreur chargement index\n");
        return 1;
    }

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Countrypedia GTK");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Rechercher par nom ou code...");
    g_signal_connect(entry, "changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);

    listbox = gtk_list_box_new();
    gtk_box_pack_start(GTK_BOX(vbox), listbox, TRUE, TRUE, 5);

    details_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(details_label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), details_label, FALSE, FALSE, 5);

    populate_listbox();

    gtk_widget_show_all(window);
    gtk_main();

    free_database();
    return 0;
}
