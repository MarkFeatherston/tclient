#include <gtk/gtk.h>

#include "tclient.h"
#include "main_window.h"
#include "settings_window.h"
#include "error_window.h"
#include "quit_dialog.h"

/* Show generic error dialog */
void start_error_dialog(tclient_c *tclient, const gchar *func, gchar *text)
{
	GtkWidget *error_dialog = 0;
    GtkWindow *parent = 0;

    /* Just in case something fucks up before the parent window exists */
    if (tclient->main_window.window != NULL)
	{
        parent = GTK_WINDOW(tclient->main_window.window);
	}

    error_dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error %s: %s", func, text);

    gtk_dialog_run(GTK_DIALOG(error_dialog));

    gtk_widget_destroy(GTK_WIDGET(error_dialog));
}
