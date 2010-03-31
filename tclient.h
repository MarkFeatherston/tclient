#ifndef _TCLIENT_H_
#define _TCLIENT_H_

#define GLADE_FILE "tclient.glade"

#include <gtk/gtk.h>

gchar *tftp_server;

enum {
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_TYPE,
  COLUMN_AUTOCONNECT,
  COLUMN_BLANK,
  COLUMN_TOTAL
};

typedef struct
{
    gchar *host;
    gchar *alias;
    gchar *type;
    gchar *icon;
    gchar *description;
    gchar *username;
	gboolean retry;
	gboolean hidden;
} connection_c;

typedef struct
{
    gchar *resolution;
    gchar *connection;
} settings_c;

typedef struct
{
    gchar *alias;
    gchar *program;
    gchar *arguments;
    gchar *description;
} type_c;

typedef struct
{
    GtkWidget *window;
    GtkWidget *quit_button;
    GtkWidget *refresh_button;
    GtkWidget *settings_button;
    GtkWidget *about_button;
    GtkWidget *connect_button;
    GtkTreeView *server_treeview;
    GtkListStore *server_liststore;
} main_window_c;

typedef struct
{
    GtkWidget *dialog;
} error_dialog_c;

typedef struct
{
    GtkWidget *dialog;
} about_dialog_c;

typedef struct
{
    GtkWidget *dialog;
    GtkWidget *timeout_progressbar;
    GtkWidget *auto_connect_icon;
    GTimer *timer;
} auto_connect_dialog_c;

typedef struct
{
    GtkWidget *dialog;
    GtkWidget *resolution_box;
    GtkWidget *resolution_combobox;
} settings_dialog_c;

typedef struct
{
    GtkWidget *dialog;
    GtkWidget *shutdown_button;
    GtkWidget *reboot_button;
    GtkWidget *cancel_button;
} quit_dialog_c;

typedef struct
{
    GtkBuilder *xml;

    /* Current settings */
    settings_c settings;

    /* Windows */
    main_window_c main_window;
    quit_dialog_c quit_dialog;
    error_dialog_c error_dialog;
    about_dialog_c about_dialog;
    settings_dialog_c settings_dialog;
    auto_connect_dialog_c auto_connect_dialog;

    /* List of connections */
    GList *connection;

    /* List of connection types */
    GList *connection_type;

    /* List of possible resolutions */
    GList *possible_resolutions;


} tclient_c;

#endif //_TCLIENT_H_
