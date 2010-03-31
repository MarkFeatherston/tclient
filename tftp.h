#ifndef _TFTP_H_
#define _TFTP_H_

#include <glib.h>
#include "tclient.h"

gboolean update_config_file (tclient_c *tclient, gchar *server, gchar *resolution);
gboolean set_tftp_file (tclient_c *tclient, gchar *filename, gpointer *data);
gpointer *load_remote_file (tclient_c *tclient, const gchar *filename);
gchar *load_remote_icon (tclient_c *tclient, const gchar *filename);
gboolean read_connections (tclient_c *tclient);
gboolean read_settings (tclient_c *tclient);
gboolean read_types (tclient_c *tclient);
gchar *get_tftp_file (gchar *filename);
gboolean load_remote_configs(tclient_c *tclient);

#endif //_TFTP_H_
