#include <gtk/gtk.h>

#include "tclient.h"
#include "main_window.h"
#include "settings_window.h"
#include "error_window.h"
#include "quit_dialog.h"

/* Populate the settings window */
gboolean populate_settings(tclient_c *tclient)
{
    gint i;

    /* Update the glist */
    update_possible_resolutions(tclient);

    /* Populate combobox */
    tclient->settings_dialog.resolution_combobox = gtk_combo_box_new_text();

    for (i = 0; i < g_list_length(tclient->possible_resolutions); i++)
    {
        GString *resolution;
        resolution = g_list_nth_data(tclient->possible_resolutions, i);

        gtk_combo_box_append_text(GTK_COMBO_BOX(tclient->settings_dialog.resolution_combobox), resolution->str);
    }

    gtk_box_pack_start(GTK_BOX(tclient->settings_dialog.resolution_box), tclient->settings_dialog.resolution_combobox, FALSE, FALSE, 0);
    gtk_widget_show_all(tclient->settings_dialog.resolution_box);

    return TRUE;
}
