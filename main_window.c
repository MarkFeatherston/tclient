#include <gtk/gtk.h>
#include <string.h>
#include <sys/ioctl.h> 
#include <net/if.h>  
#include <stdlib.h>
#include <netdb.h>

#include "tclient.h"
#include "main_window.h"
#include "settings_window.h"
#include "error_window.h"
#include "quit_dialog.h"

/* Disable all checked Autoconnect buttons */
gboolean
disable_all_checked(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, tclient_c *tclient)
{
    gtk_list_store_set (tclient->main_window.server_liststore, iter, COLUMN_AUTOCONNECT, FALSE, -1);
    return FALSE;
}

/* Check on the status of an autoconnect button */
void
server_check_toggled (GtkCellRendererToggle *button, 
	gchar *path, tclient_c *tclient)
{
    GtkTreeIter iter;
    GtkTreePath *treepath = 0;
    gboolean status = FALSE;

    treepath = gtk_tree_path_new_from_string(path);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (tclient->main_window.server_liststore),
		&iter, treepath);

    gtk_tree_path_free (treepath);
    gtk_tree_model_get(GTK_TREE_MODEL(tclient->main_window.server_liststore), 
		&iter, COLUMN_AUTOCONNECT, &status, -1);
    
    /* If the button is not checked already... */
    if (!status)
    {
        gchar *server_alias;

        /* Uncheck all of the buttons, then check the button that was clicked */
        gtk_tree_model_foreach(GTK_TREE_MODEL(tclient->main_window.server_liststore),
			(GtkTreeModelForeachFunc)disable_all_checked, tclient);
        gtk_list_store_set (tclient->main_window.server_liststore, &iter, 
			COLUMN_AUTOCONNECT, TRUE, -1);
        gtk_tree_model_get (GTK_TREE_MODEL(tclient->main_window.server_liststore), 
			&iter, COLUMN_NAME, &server_alias, -1);

        if(!update_config_file(tclient, server_alias, tclient->settings.resolution))
		{
			return;
		}
    }
    else
    {
        gtk_list_store_set (tclient->main_window.server_liststore, &iter,
			COLUMN_AUTOCONNECT, FALSE, -1);
        if(!update_config_file(tclient, NULL, tclient->settings.resolution))
		{
            return;
		}
    }
}

/* Resets the list of servers from the tftp server */
void
refresh_button_clicked (GtkButton *button, tclient_c *tclient)
{
    gtk_list_store_clear(tclient->main_window.server_liststore);
    g_list_foreach(tclient->connection, (GFunc)g_free, NULL);

    if(!read_connections(tclient))
	{
        return;
	}

    populate_treeview(tclient);
}

/* Sets up the treeview with the various servers */
gboolean
populate_treeview(tclient_c *tclient)
{
    GtkTreeIter iter;
    gint i;
    gint z;

    for (i = 0; i < g_list_length(tclient->connection); i++)
    {
        connection_c *connection = 0;
        GdkPixbuf *pixbuf = NULL;

        connection = g_list_nth_data (tclient->connection, i);
		
        g_print("Server: %i\nAlias: %s\nHost: %s\nType: %s\nIcon: %s\nHidden: %i\nRetry: %i\n\n",
			i + 1, connection->alias, connection->host, connection->type, 
			connection->icon, connection->hidden, connection->retry);
		
		if (connection->hidden == TRUE)
		{
			continue;
		}
		
        if (connection->icon)
        {
            gchar *icon_path = 0;

            icon_path = (gchar *)load_remote_icon(tclient, connection->icon);
            pixbuf = gdk_pixbuf_new_from_file_at_scale(icon_path, 40, 40, TRUE, NULL);
            g_free(icon_path);
        }

        gtk_list_store_append(GTK_LIST_STORE(tclient->main_window.server_liststore), &iter);

        gtk_list_store_set(GTK_LIST_STORE(tclient->main_window.server_liststore), &iter,
			COLUMN_ICON, pixbuf, COLUMN_NAME, connection->alias,
			COLUMN_TYPE, connection->type, COLUMN_AUTOCONNECT, FALSE,
			COLUMN_BLANK, "", -1);
		
        if (tclient->settings.connection)
        {
            if (strlen(tclient->settings.connection) > 1)
            {
                if (g_strstr_len(connection->alias, -1, tclient->settings.connection))
                {
                    gtk_list_store_set (tclient->main_window.server_liststore, &iter,
                                    COLUMN_AUTOCONNECT, TRUE,
                                    -1);
                }
            }
        }
    }

    return TRUE;
}

/* Gets the connection type from the string */
type_c *
get_type_from_string (tclient_c *tclient, gchar *type_string)
{
    type_c *type = 0;
    gint i;

    for (i = 0; i < g_list_length (tclient->connection_type); i++)
    {
        type = g_list_nth_data (tclient->connection_type, i);
        if (type != NULL)
		{
            if (type->alias != NULL)
            {
                if (g_str_equal (type->alias, type_string))
				{
                    break;
				}
                else
				{
                    type = 0;
				}
            }
		}
    }

    return type;
}

/* Returns the mac address in a newly allocated string */
gchar 
*get_mac_address()
{
    int s;
    struct ifreq buffer;
    gchar *mac_address;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&buffer, 0x00, sizeof(buffer));
    strcpy(buffer.ifr_name, "eth0");
    ioctl(s, SIOCGIFHWADDR, &buffer);
    close(s);

    mac_address = g_strdup_printf("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
                    (unsigned char)buffer.ifr_hwaddr.sa_data[0],
                    (unsigned char)buffer.ifr_hwaddr.sa_data[1],
                    (unsigned char)buffer.ifr_hwaddr.sa_data[2],
                    (unsigned char)buffer.ifr_hwaddr.sa_data[3],
                    (unsigned char)buffer.ifr_hwaddr.sa_data[4],
                    (unsigned char)buffer.ifr_hwaddr.sa_data[5]);

    return mac_address;
}

/* Returns the ip address in a newly allocated string */
gchar
*get_ip_address()
{
    struct hostent *addr;
    gchar *hostname;
    gint result;

    hostname = get_host_name();

    addr = gethostbyname(hostname);

    return (gchar *)inet_ntoa (*(struct in_addr *)*addr->h_addr_list);
}

/* Returns the hostname in a newly allocated string */
gchar 
*get_host_name()
{
    gchar hostname[HOST_NAME_MAX];

    if (gethostname(&hostname, HOST_NAME_MAX))
	{
        return NULL;
	}

    return g_strdup(hostname);
}

/* Sets up the about window with various diagnostics */
void 
about_button_clicked (GtkButton *button, tclient_c *tclient)
{
    gchar *comments;
    gchar *mac_address;
    gchar *ip_address;
    gchar *hostname;

    mac_address = get_mac_address();
    hostname = get_host_name();

    comments = g_strdup_printf("System Info\nHostname: %s\nMac Address: %s\n",
		hostname, mac_address, ip_address);

    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(tclient->about_dialog.dialog), comments);
    gtk_dialog_run(GTK_DIALOG(tclient->about_dialog.dialog));
    gtk_widget_hide(tclient->about_dialog.dialog);

    g_free(mac_address);
    g_free(comments);
}

/* Gets a description of a connection from it's alias */
gchar *
get_description_from_alias (tclient_c *tclient, gchar *alias)
{
	connection_c * connection = 0;
    gchar *description = 0;
    gint i;

    for (i = 0; i < g_list_length (tclient->connection); i++)
    {
        connection = g_list_nth_data (tclient->connection, i);
        if (connection != NULL)
		{
            if (connection->alias != NULL)
            {
                if (g_str_equal (connection->alias, alias))
				{
                    break;
				}
                else
				{
                    description = 0;
				}
            }
		}
    }

    return description;
}

/* Gets the connection struct from a string alias */
connection_c *
get_connection_from_alias (tclient_c *tclient, gchar *alias)
{
    connection_c *connection = 0;
    gint i;

    for (i = 0; i < g_list_length (tclient->connection); i++)
    {
        connection = g_list_nth_data (tclient->connection, i);
        if (connection != NULL)
		{
            if (connection->alias != NULL)
            {
                if (g_str_equal (connection->alias, alias))
				{
                    break;
				}
                else
				{
                    connection = 0;
				}
            }
		}
    }

    return connection;
}

/* Connects to a server based on it's alias */
gboolean 
connect_from_alias(tclient_c *tclient, gchar *active_server)
{
    connection_c *connection = 0;
    type_c *type = 0;
    gchar *arguments = 0;
    gchar *command = 0;
    gchar *proc_stdout = 0;
    gint i = 0;
    gchar *proc_stderr = 0;
    gchar **err = 0;
    gint result = 0;
    GError *error = 0;
    GString *error_log;

    /* Get connection + connection type */
    connection = get_connection_from_alias(tclient, active_server);
    type = get_type_from_string(tclient, connection->type);

    if (connection == NULL || type == NULL)
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't get the connection type synced");
        return;
    }

    /* Sanitize comamnds */
    type->program = g_strstrip (type->program);
    type->arguments = g_strstrip (type->arguments);

    /* Replace special chars in arguments */
    arguments = g_strjoinv(connection->alias, g_strsplit(type->arguments, "%a", -1));
    arguments = g_strjoinv(connection->host, g_strsplit(arguments, "%h", -1));
    arguments = g_strjoinv(connection->username, g_strsplit(arguments, "%u", -1));

    command = g_strdup_printf("%s %s", type->program, arguments);

    if (!g_spawn_command_line_sync(command, &proc_stdout, &proc_stderr, &result, &error))
    {
        start_error_dialog(tclient, __FUNCTION__, error->message);
    }

    err = g_strsplit(proc_stderr, "\n", -1);
    error_log = g_string_new(NULL);

    /* Allow certain "errors" */
    for (i = 0; i < g_strv_length(err); i++)
    {
		/* The automatic keyboard selection is not important */
        if (g_strstr_len(err[i], -1, "Autoselected keyboard map en-us"))
		{
            continue;
		}

        error_log = g_string_append(error_log, err[i]);
    }

    if (error_log->len > 1)
    {
		GtkWidget *retry_dialog = 0;
		GtkWindow *parent = 0;
		gint response;
		
		retry_dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Failed to connect to \"%s\".\n\"%s\"\nDo you want to retry?", connection->alias, error_log->str);

		response = gtk_dialog_run(GTK_DIALOG(retry_dialog));

		gtk_widget_destroy(GTK_WIDGET(retry_dialog));
				
		if (response == -8)
		{
			connect_from_alias(tclient, active_server);
		}
    }

    g_string_free(error_log, TRUE);
    g_strfreev(err);
    g_free(command);
}

/* Connect the user to a server */
void
connect_button_clicked(GtkButton *button, tclient_c *tclient)
{
    GtkTreeIter iter;
    GtkTreeSelection *selected;
    gchar *active_server;
    GtkTreeModel *model;

    /* Get alias of selected server */
    selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(tclient->main_window.server_treeview));
    model = GTK_TREE_MODEL(tclient->main_window.server_liststore);

    if (!gtk_tree_selection_get_selected (selected, &model, &iter))
	{
        return;
	}

    gtk_tree_model_get(GTK_TREE_MODEL(tclient->main_window.server_liststore), &iter, COLUMN_NAME, &active_server, -1);

    connect_from_alias(tclient, active_server);
}

/* Create the window for the user to configure settings */
void
settings_button_clicked(GtkButton *button, tclient_c *tclient)
{
    GtkTreeModel *model;
    gint result;

    result = gtk_dialog_run(GTK_DIALOG(tclient->settings_dialog.dialog));

    if (result == 1)
    {
        gchar *resolution;
        gchar *server;

        resolution = gtk_combo_box_get_active_text(GTK_COMBO_BOX(tclient->settings_dialog.resolution_combobox));
        if (resolution)
        {
            if (g_str_equal(resolution, "None") || g_str_equal(resolution, "(NULL)"))
            {
                resolution = tclient->settings.resolution;
            }
        }
        else
        {
            resolution = 0;
        }
		
        set_resolution(tclient, resolution);
        update_config_file(tclient, tclient->settings.connection, resolution);
    }

    gtk_widget_hide(tclient->settings_dialog.dialog);
}

/* Set the resolution.  Currently this is done using the command line
 * xrandr, this should be moved to using the library.  This works, but it
 * has some workarounds for the time it takes for the monitor to refresh itself.
 * 
 * TODO: Make sure the monitor can switch to that resolution before blindly using it */
void
set_resolution(tclient_c *tclient, gchar *resolution)
{
    GError *error = 0;
    gint result;
    gint height;
    gint width;
    gchar *command = 0;
    gchar *current_resolution = 0;
    GdkScreen *screen = 0;

    if (!resolution)
	{
        return;
	}

    /* If the resolution string is too small, ignore it */
    if (strlen(resolution) < 7)
	{
        return;
	}

    /* Ignore checks if the main window isn't up yet */
    if (tclient->main_window.window)
    {
        screen = gtk_widget_get_screen(tclient->main_window.window);
        width = gdk_screen_get_width(screen);
        height = gdk_screen_get_height(screen);

        current_resolution = g_strdup_printf("%ix%i", width, height);

        if (g_str_equal(current_resolution, resolution))
		{
            return;
		}

        if (g_str_equal(resolution, "None") || g_str_equal(resolution, "(NULL)"))
		{
            return;
		}
	}

    command = g_strdup_printf("xrandr -s %s", resolution);
    g_print("Running command \"%s\"\n", command);

    g_spawn_command_line_sync(command, NULL, NULL, &result, &error);
    if (error)
    {
        start_error_dialog(tclient, __FUNCTION__, error->message);
        return;
    }

    /* Give the monitor time to switch */
    g_usleep(2 * 1000000);

    if (screen)
    {
        width = gdk_screen_get_width(screen);
        height = gdk_screen_get_height(screen);

        if (current_resolution)
		{
            g_free(current_resolution);
		}

        current_resolution = g_strdup_printf("%ix%i", width, height);

        width = (gint)g_ascii_strtoull(resolution, NULL, 10);
        height = (gint)g_ascii_strtoull(g_strstr_len(resolution, -1, "x") + 1, NULL, 10);
        /* TODO - SET DEFAULT IN SETTINGS WINDOW TO CURRENT */
    }

    if(tclient->main_window.window)
    {
        gtk_window_reshow_with_initial_size(GTK_WINDOW(tclient->main_window.window));
        gtk_window_set_position(GTK_WINDOW(tclient->main_window.window), GTK_WIN_POS_CENTER_ALWAYS);
    }

    g_free(command);
    g_free(current_resolution);
}

/* Update the listing of possible resolutions. */
gboolean update_possible_resolutions(tclient_c *tclient)
{
    gchar *proc_stdout = 0;
    gchar **out = 0;
    gchar *proc_stderr = 0;
    GIOChannel *io_stdout = 0;
    gchar **argv = 0;
    GError *error = 0;
    gint status;
    gint i;

    g_spawn_command_line_sync("xrandr", &proc_stdout, &proc_stderr, &status, &error);
    out = g_strsplit(proc_stdout, "\n", -1);

    if (error)
    {
        g_printerr("%s\n", error->message);
        return 1;
    }

    if (status)
    {
        g_printerr("%s\n", proc_stderr);
        return 1;
    }

    for (i = 0; i < g_strv_length(out); i++)
    {
        GString *line = 0;
        gint itr = 0;

        line = g_string_new(out[i]);

        if (line->str[0] == ' ' && line->str[1] == ' ' && line->str[2] == ' ')
        {
            line = g_string_erase(line, 0, 3);
            for (itr = 0; itr < strlen(line->str); itr++)
            {
                if (line->str[itr] == ' ')
                {
                    line = g_string_truncate(line, itr);
                    tclient->possible_resolutions = g_list_append(tclient->possible_resolutions, line);
                }

            }

        }
    }
}

/* Shows a countdown for when the automatic connection starts */
gboolean autoconnect_countdown (tclient_c *tclient)
{
    gchar *label = 0;
    gdouble time_elapsed;
    gint timeout;

    if (!tclient->auto_connect_dialog.timer)
	{
        return FALSE;
	}

    time_elapsed = g_timer_elapsed(tclient->auto_connect_dialog.timer, NULL);

    timeout = 5 - (gint)time_elapsed;
    if (timeout <= 0)
    {
        gtk_dialog_response(GTK_DIALOG(tclient->auto_connect_dialog.dialog), 1);
        return FALSE;
    }

    gtk_window_set_position(GTK_WINDOW(tclient->auto_connect_dialog.dialog), GTK_WIN_POS_CENTER_ALWAYS);


    label = g_strdup_printf ("Connecting automatically in %i seconds", timeout);

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(tclient->auto_connect_dialog.timeout_progressbar), label);
    g_free(label);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tclient->auto_connect_dialog.timeout_progressbar), (double)timeout / (double)5);

    return TRUE;
}

/* Connect various signals */
gboolean
connect_signals (tclient_c *tclient)
{
    g_signal_connect(tclient->main_window.window, "destroy", gtk_main_quit, tclient);
    g_signal_connect(tclient->main_window.quit_button, "clicked", G_CALLBACK(quit_button_clicked), tclient);
    g_signal_connect(tclient->main_window.refresh_button, "clicked", G_CALLBACK(refresh_button_clicked), tclient);
    g_signal_connect(tclient->main_window.about_button, "clicked", G_CALLBACK(about_button_clicked), tclient);
    g_signal_connect(tclient->main_window.settings_button, "clicked", G_CALLBACK(settings_button_clicked), tclient);
    g_signal_connect(tclient->main_window.connect_button, "clicked", G_CALLBACK(connect_button_clicked), tclient);
	g_signal_connect(tclient->main_window.server_treeview, "query-tooltip", G_CALLBACK (tooltip_main_window_query), tclient);

    return TRUE;
}

/* Show tooltip for main window */
static gboolean
tooltip_main_window_query(GtkWidget  *widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip *tooltip, tclient_c tclient)
{
	GtkTreeIter iter;
	GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
	GtkTreePath *path = NULL;
	gchar *tmp;
	gchar *pathstring;

	char buffer[512];

	if (!gtk_tree_view_get_tooltip_context (tree_view, &x, &y, keyboard_tip, &model, &path, &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter, 0, &tmp, -1);
	pathstring = gtk_tree_path_to_string (path);

	g_snprintf (buffer, 511, "<b>Path %s:</b> %s", pathstring, tmp);
	gtk_tooltip_set_markup (tooltip, buffer);

	gtk_tree_view_set_tooltip_row (tree_view, tooltip, path);

	gtk_tree_path_free (path);
	g_free (pathstring);
	g_free (tmp);

	return TRUE;
}

/* Create main window */
gboolean create_main_window (tclient_c *tclient)
{
	if(!load_xml_widgets(tclient))
    {
        g_printerr("couldn't load xml\n");
        return FALSE;
    }

    /* Autoconnect if set */
    if (tclient->settings.connection)
	{
        if (strlen(tclient->settings.connection) > 1);
		{
            autoconnect_server(tclient);
		}
	}

    if(!setup_server_treeview(tclient))
        return FALSE;

    if(!populate_treeview(tclient))
        return FALSE;

    if(!populate_settings(tclient))
        return FALSE;


    /* Draw widgets */
    gtk_widget_show_all(tclient->main_window.window);
    gtk_window_set_position(GTK_WINDOW(tclient->main_window.window), GTK_WIN_POS_CENTER_ALWAYS);

    return TRUE;
}

/* Prepare the treeview columns */
gboolean 
setup_server_treeview (tclient_c *tclient)
{
    GtkCellRenderer *renderer;

    /* Column 1 */
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tclient->main_window.server_treeview),
                                    -1,
                                    "",
                                    renderer,
                                    "pixbuf", COLUMN_ICON,
                                    NULL);
    /* Column 2 */
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tclient->main_window.server_treeview),
                                    -1,
                                    "Name",
                                    renderer,
                                    "text", COLUMN_NAME,
                                    NULL);
    /* Column 3 */
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tclient->main_window.server_treeview),
                                    -1,
                                    "Type",
                                    renderer,
                                    "text", COLUMN_TYPE,
                                    NULL);
    /* Column 4 */
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set ((GObject *) renderer, "activatable", TRUE, NULL);
    g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (server_check_toggled), tclient);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tclient->main_window.server_treeview),
                                    -1,
                                    "Auto connect",
                                    renderer,
                                    "active", COLUMN_AUTOCONNECT,
                                    NULL);
    /* Blank Column */
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tclient->main_window.server_treeview),
                                    -1,
                                    "",
                                    renderer,
                                    "text", COLUMN_BLANK,
                                    NULL);
}

/* Load the widgets from the gtkbuilder file */
gboolean
load_xml_widgets(tclient_c *tclient)
{
    GError *error = NULL;

    tclient->xml = gtk_builder_new();

    /* Load builder xml */
    gtk_builder_add_from_file(tclient->xml, GLADE_FILE, &error);
    if(error)
    {
        start_error_dialog(tclient, __FUNCTION__, error->message);
        return FALSE;
    }

    if(!(tclient->main_window.window = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "main_window"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find main_window");
        return FALSE;
    }
    if(!(tclient->main_window.quit_button = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "quit_button"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find quit_button");
        return FALSE;
    }
    if(!(tclient->main_window.connect_button = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "connect_button"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find connect_button");
        return FALSE;
    }
    if(!(tclient->main_window.about_button = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "about_button"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find about_button");
        return FALSE;
    }
    if(!(tclient->main_window.settings_button = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "settings_button"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find settings_button");
        return FALSE;
    }
    if(!(tclient->settings_dialog.resolution_box = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "resolution_box"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find resolution_box");
        return FALSE;
    }
    if(!(tclient->settings_dialog.dialog = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "settings_dialog"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find settings_dialog");
        return FALSE;
    }
    if(!(tclient->quit_dialog.dialog = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "quit_dialog"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find quit_dialog");
        return FALSE;
    }
    if(!(tclient->about_dialog.dialog = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "about_dialog"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find quit_dialog");
        return FALSE;
    }
    if(!(tclient->auto_connect_dialog.dialog = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "auto_connect_dialog"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find auto_connect_dialog");
        return FALSE;
    }
    if(!(tclient->auto_connect_dialog.timeout_progressbar = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "timeout_progressbar"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find timeout_progressbar");
        return FALSE;
    }
    if(!(tclient->auto_connect_dialog.auto_connect_icon = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "auto_connect_icon"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find auto_connect_icon");
        return FALSE;
    }
    if(!(tclient->main_window.refresh_button = GTK_WIDGET(gtk_builder_get_object(tclient->xml, "refresh_button"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find refresh_button");
        return FALSE;
    }
    if(!(tclient->main_window.server_treeview = GTK_TREE_VIEW(gtk_builder_get_object(tclient->xml, "server_treeview"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find server_treeview");
        return FALSE;
    }
    if(!(tclient->main_window.server_liststore = GTK_LIST_STORE(gtk_builder_get_object(tclient->xml, "server_liststore"))))
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't find server_liststore");
        return FALSE;
    }

    return TRUE;
}

void quit_button_clicked (GtkButton *button, tclient_c *tclient)
{
    gint response;

    response = gtk_dialog_run(GTK_DIALOG(tclient->quit_dialog.dialog));
    gtk_widget_hide(GTK_WIDGET(tclient->quit_dialog.dialog));

    if (response == 1)
    {
        shutdown_system();
    }
    else if (response == 2)
    {
        reboot_system();
    }
}

/* Connect to this client's default server */
void
autoconnect_server(tclient_c *tclient)
{
    connection_c *connection = 0;
    GdkPixbuf *pixbuf = 0;
    gchar *icon_filename = 0;
    gchar *title = 0;
    gint response = 0;

    if (!tclient->settings.connection)
        return;

    if (strlen(tclient->settings.connection) < 1)
        return;

    connection = get_connection_from_alias(tclient, tclient->settings.connection);
	
	if (connection == NULL)
	{
		GtkWidget *info_dialog = 0;
		GtkWindow *parent = 0;

		parent = GTK_WINDOW(tclient->main_window.window);
		
		info_dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "The server \"%s\" that you had set to automatically connect is no longer available.  Please pick a new server.", tclient->settings.connection);
		
		gtk_dialog_run(GTK_DIALOG(info_dialog));

		gtk_widget_destroy(GTK_WIDGET(info_dialog));
		
		/* Clear the old server setting */
		update_config_file (tclient, NULL, tclient->settings.resolution);
		
		return;
	}
	
    /* Set Title */
    title = g_strdup_printf("Connecting to \"%s\"", connection->alias);
    gtk_window_set_title(GTK_WINDOW(tclient->auto_connect_dialog.dialog), title);
    g_free(title);

    /* Set Icon */
    icon_filename = (gchar *)load_remote_icon(tclient, connection->icon);
    pixbuf = gdk_pixbuf_new_from_file_at_scale(icon_filename, 150, 150, TRUE, NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(tclient->auto_connect_dialog.auto_connect_icon), pixbuf);

    /* 6 second countdown */
    tclient->auto_connect_dialog.timer = g_timer_new();
    g_timeout_add_seconds (1, (GSourceFunc)autoconnect_countdown, tclient);

    /* Initialize window */
    autoconnect_countdown(tclient);

    gtk_window_set_position(GTK_WINDOW(tclient->auto_connect_dialog.dialog), GTK_WIN_POS_CENTER_ALWAYS);
    response = gtk_dialog_run(GTK_DIALOG(tclient->auto_connect_dialog.dialog));

    g_timer_destroy(tclient->auto_connect_dialog.timer);
    gtk_widget_hide(tclient->auto_connect_dialog.dialog);
    tclient->auto_connect_dialog.timer = 0;

    if (response == 1)
    {
        /* Connect */
        connect_from_alias(tclient, connection->alias);
    }
    else if (response == 2)
    {
        /* Canceled */
    }
}
