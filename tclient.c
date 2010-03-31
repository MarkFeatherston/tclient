#include <glib/gstdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <glib.h>

#include "tclient.h"
#include "main_window.h"
#include "settings_window.h"
#include "error_window.h"
#include "quit_dialog.h"

gint
main (gint argc, gchar *argv[])
{
    tclient_c tclient = {0};

    /* Initialize Libs */
    gtk_init(&argc, &argv);

    /* Find tftp server */
    if (argc == 2)
    {
        tftp_server = g_strdup(argv[1]);
    }
    else
    {
        g_printerr("Usage: %s [Tftp Host]\n", argv[0]);
        return 1;
    }

    /* Load remote config files */
    if(!load_remote_configs(&tclient))
	{
        return 1;
	}

    /* Set resolution if they have one */
    if (tclient.settings.resolution)
	{
        set_resolution(&tclient, tclient.settings.resolution);
	}

    /* Create main window */
    if(!create_main_window(&tclient))
	{
        return 1;
	}

    /* Connect signals to widgets */
    if(!connect_signals(&tclient))
	{
        return 1;
	}

    /* Enter main loop */
    gtk_main ();

	return 0;
}
