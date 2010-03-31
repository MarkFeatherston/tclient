#ifndef _ERROR_WINDOW_H_
#define _ERROR_WINDOW_H_

#include <glib.h>

#include "tclient.h"

void start_error_dialog(tclient_c *tclient, const gchar *func, gchar *text);

#endif //_ERROR_WINDOW_H_
