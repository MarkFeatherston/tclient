#ifndef _MAIN_WINDOW_H_
#define _MAIN_WINDOW_H_

#include <glib.h>
#include "tclient.h"

gchar *get_host_name();
gchar *get_mac_address();
gchar *get_ip_address();
gboolean disable_all_checked(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, tclient_c *tclient);
gboolean create_main_window (tclient_c *tclient);
void set_resolution(tclient_c *tclient, gchar *resolution);
gboolean populate_treeview(tclient_c *tclient);
void quit_button_clicked (GtkButton *button, tclient_c *tclient);
void server_check_toggled (GtkCellRendererToggle *button, gchar *path, tclient_c *tclient);
void refresh_button_clicked (GtkButton *button, tclient_c *tclient);
type_c *get_type_from_string (tclient_c *tclient, gchar *type_string);
connection_c *get_connection_from_alias (tclient_c *tclient, gchar *alias);
gboolean connect_from_alias(tclient_c *tclient, gchar *active_server);
void about_button_clicked (GtkButton *button, tclient_c *tclient);
gboolean setup_server_treeview (tclient_c *tclient);
void autoconnect_server(tclient_c *tclient);
gchar *get_description_from_alias (tclient_c *tclient, gchar *alias);
static gboolean tooltip_main_window_query(GtkWidget  *widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip *tooltip, tclient_c tclient);

#endif //_MAIN_WINDOW_H_
