#!/bin/bash
echo "í´§ Correction de gui.c..."

# 1. Supprime la deuxiÃ¨me dÃ©claration de current_details_dialog
sed -i '352d' gui.c

# 2. Ajoute la dÃ©claration anticipÃ©e aprÃ¨s la structure AppData
sed -i '/} AppData;/a\
// ============ DÃ‰CLARATIONS ANTICIPÃ‰ES ============\
void afficher_details_pays(AppData *app_data, const char *code);\
// =================================================' gui.c

# 3. Corrige gtk_button_set_child en gtk_container_add
sed -i 's/gtk_button_set_child(GTK_BUTTON(btn), vbox);/gtk_container_add(GTK_CONTAINER(btn), vbox);/' gui.c

# 4. Corrige gtk_flow_box_append en gtk_container_add
sed -i 's/gtk_flow_box_append(GTK_FLOW_BOX(flowbox), btn);/gtk_container_add(GTK_CONTAINER(flowbox), btn);/' gui.c

# 5. Ajoute les nouveaux widgets dans afficher_details_pays
sed -i '/gtk_grid_attach(GTK_GRID(grid), desc_text, 1, 7, 1, 1);/a\
                \
                // ============ NOUVEAUX WIDGETS AJOUTÃ‰S ICI ============\
                \
                // í´¹ PAYS LIMITROPHES (ligne 8)\
                GtkWidget *borders_widget = create_borders_widget(app_data, \&app_data->current_country);\
                gtk_grid_attach(GTK_GRID(grid), borders_widget, 0, 8, 2, 1);\
                \
                // í´¹ GOUVERNEMENT \& LANGUE (ligne 9)\
                GtkWidget *gov_widget = create_government_widget(\&app_data->current_country);\
                gtk_grid_attach(GTK_GRID(grid), gov_widget, 0, 9, 2, 1);\
                \
                // ======================================================' gui.c

echo "âœ… Corrections appliquÃ©es !"
