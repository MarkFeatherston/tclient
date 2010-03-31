#include <curl/curl.h>
#include <glib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <curl/curl.h>

#include "tclient.h"
#include "main_window.h"
#include "settings_window.h"
#include "error_window.h"
#include "quit_dialog.h"

/* This function connects using curl to the tftp server.  It grabs needed
 * files from there */
void *
get_tftp_file (tclient_c *tclient, gchar *filename)
{
    CURL *curl = 0;
    FILE *config_file = 0;
    GError *error = 0;
    gchar *local_filename = 0;
    gchar *uri = 0;
    gchar *dirs = 0;
    gchar *itr = 0;

    /* Make the directory structure for the file being fetched */
    itr = g_strrstr(filename, "/");
    if (itr != NULL)
    {
        dirs = g_strndup(filename, (strlen(filename) - strlen(itr)));
        itr = g_strdup_printf("/tmp/%s", dirs);
        g_mkdir_with_parents(itr, 0777);
        g_free(dirs);
        g_free(itr);
    }

    local_filename = g_strdup_printf("/tmp/%s", filename);
    uri = g_strdup_printf("tftp://%s/tclient/%s", tftp_server, filename);

    curl = curl_easy_init();
    config_file = fopen (local_filename, "w+b");
    if (config_file != NULL)
    {
        CURLcode result;

        curl_easy_setopt (curl, CURLOPT_WRITEDATA, config_file);
        curl_easy_setopt (curl, CURLOPT_URL, uri);
        curl_easy_setopt (curl, CURLOPT_TIMEOUT, 5);
        result = curl_easy_perform(curl);

        curl_easy_cleanup(curl);

		/* If there is an error code, and it isn't 
		 * CURLE_TFTP_NOTFOUND
		 * CURLE_TFTP_ILLEGAL
		 * Both of these can originate on missing or null files which
		 * are not to be considered errors */
        if (result != 0 && result != CURLE_TFTP_NOTFOUND && result != CURLE_TFTP_ILLEGAL)
        {
            start_error_dialog(tclient, __FUNCTION__, (gchar *)curl_easy_strerror(result));

            return NULL;
        }
    }
    else
    {
        if (error != NULL)
        {
            start_error_dialog(tclient, __FUNCTION__, error->message);
        }
    }

    /* Cleanup */
    g_free(uri);

    if (config_file != NULL)
    {
        fclose(config_file);
        return local_filename;
    }

    return NULL;
}

/* Loads a file from the tftp server */
gpointer *
load_remote_file (tclient_c *tclient, gchar *filename)
{
    gchar *buf = 0;
    GFile *file = 0;
    gchar *file_path = 0;
    GError *error = 0;

    /* Get the path for the downloaded conf file */
    file_path = get_tftp_file(tclient, filename);

    if (file_path == NULL)
        return NULL;

    file = g_file_new_for_path(file_path);

    if (!file_path)
        return NULL;

    /* Load the config_file into memory */
    g_file_load_contents(file, NULL, &buf, NULL, NULL, &error);
    if (error)
        start_error_dialog(tclient, __FUNCTION__, error->message);

    return (gpointer *)buf;
}

/* Loads a file from the tftp server */
gchar *
load_remote_icon (tclient_c *tclient, const gchar *filename)
{
    GString *tftp_path = 0;
    gchar *file_path = 0;
    /* Get the path for the downloaded conf file */

    tftp_path = g_string_new(NULL);
    g_string_printf(tftp_path, "icons/%s", filename);

    file_path = get_tftp_file(tclient, tftp_path->str);

    g_string_free(tftp_path, TRUE);

    if (file_path == NULL)
        return NULL;

    return file_path;
}

/* Writes a file to the tftp server */
gboolean
set_tftp_file (tclient_c *tclient, gchar *filename, gpointer *data)
{
    CURL *curl = 0;
    GError *error = 0;
    FILE *config_file;
    gchar *local_filename = 0;
    gchar *uri = 0;

    local_filename = g_strdup_printf("/tmp/%s", filename);
    uri = g_strdup_printf("tftp://%s/tclient/clients/%s", tftp_server, filename);

    curl = curl_easy_init();

    /* Write config file to the disk */
    config_file = fopen (local_filename, "w+b");
    fwrite(data, strlen((gchar *)data), 1, config_file);
    fclose(config_file);

    /* Open the config file with read */
    config_file = fopen (local_filename, "r");

    if (config_file != NULL)
    {
        CURLcode curl_result;
        gchar *error = 0;

        curl_easy_setopt (curl, CURLOPT_UPLOAD, 1);
        curl_easy_setopt (curl, CURLOPT_READDATA, config_file);
        curl_easy_setopt (curl, CURLOPT_URL, uri);
        curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, error);

        curl_result = curl_easy_perform(curl);

        if (curl_result != 0)
        {
            if (curl_result == CURLE_TFTP_ILLEGAL)
            {
                start_error_dialog(tclient, __FUNCTION__, "Could not write settings file.  Check permissions on tftp server\n");
                return FALSE;
            }

            start_error_dialog(tclient, __FUNCTION__, error);
            return FALSE;
        }

        curl_easy_cleanup(curl);
    }
    else
    {
        if (error != NULL)
        {
            start_error_dialog(tclient, __FUNCTION__, error->message);
            return FALSE;
        }
    }

    /* Cleanup */
    g_free(uri);
    if (config_file != NULL)
    {
        fclose(config_file);
    }

    return TRUE;
}

/* This function updates the connection_c contexts.  */
gboolean
read_connections (tclient_c *tclient)
{
    GKeyFile *keys = 0;
    gint i = 0;
    gchar *buf = 0;
    gint buf_size = 0;

    tclient->connection = 0;

    buf = (gchar *)load_remote_file(tclient, "connections.conf");
    if (!buf)
    {
        start_error_dialog(tclient, __FUNCTION__, "Couldn't read connections\n");
        return FALSE;
    }
    buf_size = strlen(buf);

    /* Parse the keys */
    keys = g_key_file_new();
    if(g_key_file_load_from_data(keys, buf, buf_size, G_KEY_FILE_NONE, NULL))
    {
        gchar **group = 0;
        gsize group_size = 0;

        group = g_key_file_get_groups(keys, &group_size);

        /* Go through the groups and create connections for them */
        for (i = 0; i < group_size; i++)
        {
            connection_c *connection;
            connection = g_malloc0 (sizeof (connection_c));

			/* conf file reads like this
			 * [connection alias]
			 * host=127.0.0.1 #any resolvable IP/DNS
			 * icon=windows.png #Loaded from %server%/tclient/icons/
			 * type=RDP # Type of conneciton, must be defined in connections
			 * description=Test Station # This will show as a tooltip
			 * username=root #This is the default username to try
			 * retry=false #retry the connection or not
			 * hidden=true #Whether or not it shows up in the list */

            connection->alias = g_strdup(group[i]);
			
			/* Get the hostname/ip.  Verify that there is text there.  Fail
			 * if this is not there */
			if(!(connection->host = g_key_file_get_value(keys, group[i], "host", NULL)))
			{
				gchar *error_str = 0;
					
				error_str = g_strdup_printf("Malformed connection.conf: Error in group \"%s\".  You must provide a valid host to connect to.", group[i]);
				start_error_dialog(tclient, __FUNCTION__, error_str);
				
				g_free(error_str);
				g_free(connection);
				continue;
			}
			if (strlen(connection->host) <= 0)
			{
				gchar *error_str = 0;
				
				error_str = g_strdup_printf("Malformed connection.conf: Error in group \"%s\".  You must provide a valid host to connect to.", group[i]);
				start_error_dialog(tclient, __FUNCTION__, error_str);
				
				g_free(error_str);
				g_free(connection);
				continue;
			}
			
			/* Default username.  This can be empty */
			connection->username = g_key_file_get_value(keys, group[i], "username", NULL);
			
			/* Type to use.  This must exist in the types.conf file.  Fail if this
			 * entry is blank */
			if (!(connection->type = g_key_file_get_value(keys, group[i], "type", NULL)))
			{
				gchar *error_str = 0;
				
				error_str = g_strdup_printf("Malformed connection.conf: Error in group \"%s\".  \"%s\" must be set in types.conf", group[i], connection->type);
				start_error_dialog(tclient, __FUNCTION__, error_str);
				
				g_free(error_str);
				g_free(connection);
				continue;
			}
			if (connection->type == NULL)
			{
				gchar *error_str = 0;
				
				error_str = g_strdup_printf("Malformed connection.conf: Error in group \"%s\".  Type must be set", group[i]);
				start_error_dialog(tclient, __FUNCTION__, error_str);
				
				g_free(error_str);
				g_free(connection);
				continue;
			}
			
			/* Set retry setting.  If nothing else, say true */
			if (g_ascii_strncasecmp(g_key_file_get_value(keys, group[i], "retry", NULL), "true", strlen("true")) == 0)
			{
				connection->retry = TRUE;
			} 
			else if (g_ascii_strncasecmp(g_key_file_get_value(keys, group[i], "retry", NULL), "false", strlen("false")) == 0)
			{
				connection->retry = FALSE;
			} 
			else
			{
				connection->retry = TRUE;
			}
			
			/* Get this hidden setting.  Set to false by default */
			if(g_key_file_get_value(keys, group[i], "hidden", NULL) != NULL)
			{
				if (g_ascii_strncasecmp(g_key_file_get_value(keys, group[i], "hidden", NULL), "true", strlen("true")) == 0)
				{
					connection->hidden = TRUE;
				} 
				else if (g_ascii_strncasecmp(g_key_file_get_value(keys, group[i], "hidden", NULL), "false", strlen("false")) == 0)
				{
					connection->hidden = FALSE;
				}
			}
			else
			{
				connection->hidden = FALSE;
			}
			
			/* Get the icon name, this is optional */
            connection->icon = g_key_file_get_value(keys, group[i], "icon", NULL);
			
			/* Get the description, this is optional */
            connection->description = g_key_file_get_value(keys, group[i], "description", NULL);
			
            tclient->connection = g_list_append(tclient->connection, connection);
        }

        g_strfreev(group);
    }

    g_key_file_free(keys);

    return TRUE;
}

gboolean
read_settings (tclient_c *tclient)
{
    GKeyFile *keys = 0;
    gchar *buf;
    gint buf_size = 0;
    gchar hostname[_POSIX_HOST_NAME_MAX + 1] = {0};
    gchar *filename = 0;

    gethostname(hostname, _POSIX_HOST_NAME_MAX);

    filename = g_strdup_printf("clients/%s.conf", &hostname[0]);

    buf = (gchar *)load_remote_file(tclient, filename);

    if (!buf || strlen(buf) <= 1)
    {
        /* Client settings can be blank, just return true */
        return TRUE;
    }

    buf_size = strlen(buf);
    if (buf_size < 1)
        return TRUE;

    /* Parse the keys */
    keys = g_key_file_new();
    if(g_key_file_load_from_data(keys, buf, buf_size, G_KEY_FILE_NONE, NULL))
    {
        gchar **group = 0;
        gsize group_size = 0;

        group = g_key_file_get_groups(keys, &group_size);

        tclient->settings.connection = g_key_file_get_value(keys, group[0], "server", NULL);
        tclient->settings.resolution = g_key_file_get_value(keys, group[0], "resolution", NULL);

        g_strfreev(group);
    }

    g_free(filename);
    g_key_file_free(keys);

    return TRUE;
}

/* This function updates the config file contexts.  */
gboolean
update_config_file (tclient_c *tclient, gchar *server, gchar *resolution)
{
    gchar *config = 0;
    gchar *filename = 0;
    gchar hostname[_POSIX_HOST_NAME_MAX + 1] = {0};

    gethostname(hostname, _POSIX_HOST_NAME_MAX);

    if (resolution == NULL && server == NULL)
    {
        config = g_strdup_printf("[%s]\nserver=\nresolution=\n", hostname);
    }
    else if (server == NULL)
    {
        config = g_strdup_printf("[%s]\nserver=\nresolution=%s\n", hostname, resolution);
    }
    else if (resolution == NULL)
    {
        config = g_strdup_printf("[%s]\nserver=%s\nresolution=\n", hostname, server);
    }
    else
    {
        config = g_strdup_printf("[%s]\nserver=%s\nresolution=%s\n", hostname, server, resolution);
    }

    filename = g_strdup_printf("%s.conf", hostname);
    if (!set_tftp_file (tclient, filename, (gpointer)config))
        return FALSE;

    return TRUE;
}

/* This function reads the type_c contexts.  */
gboolean 
read_types (tclient_c *tclient)
{
    GKeyFile *keys = 0;
    gint i = 0;
    gchar *buf = 0;
    gint buf_size = 0;

    buf = (gchar *)load_remote_file(tclient, "types.conf");
    if (!buf)
    {
        start_error_dialog(tclient, __FUNCTION__, "Could not load the types file");
        return FALSE;
    }

    buf_size = strlen(buf);

    /* Parse the keys */
    keys = g_key_file_new();
    if(g_key_file_load_from_data(keys, buf, buf_size, G_KEY_FILE_NONE, NULL))
    {
        gchar **group = 0;
        gsize group_size = 0;

        group = g_key_file_get_groups(keys, &group_size);

        /* Go through the groups and create connections for them */
        for (i = 0; i < group_size; i++)
        {
            type_c *type;

            type = g_malloc0 (sizeof (type_c));

            type->alias = g_strdup(group[i]);
            type->program = g_key_file_get_value(keys, group[i], "program", NULL);
            type->arguments = g_key_file_get_value(keys, group[i], "arguments", NULL);
            type->description = g_key_file_get_value(keys, group[i], "description", NULL);

            tclient->connection_type = g_list_append(tclient->connection_type, type);
        }

        g_strfreev(group);
    }

    g_key_file_free(keys);

    return TRUE;
}

/* Load all of the config filse */
gboolean
load_remote_configs(tclient_c *tclient)
{
	if(!read_types(tclient))
        return FALSE;
    
	if(!read_connections(tclient))
        return FALSE;

    if(!read_settings(tclient))
        return FALSE;

    return TRUE;
}
