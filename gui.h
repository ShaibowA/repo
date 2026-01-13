#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

// 🔹 Déclaration forward
typedef struct AppData AppData;

// Fonction principale de l'interface
int gui_main(int argc, char *argv[]);

// 🔹 Prototypes corrigés
void afficher_details_pays(AppData *app_data, const char *code);
gboolean on_description_link_clicked(GtkWidget *label, gchar *uri, gpointer data);

#endif