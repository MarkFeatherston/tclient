#include <gtk/gtk.h>

#include "tclient.h"
#include "main_window.h"
#include "settings_window.h"
#include "error_window.h"
#include "quit_dialog.h"

/* Probably need a more nonspecific way to perform both of these commands.
 * 
 * Currently, shutdown works because sudo on the thin clientuser has 
 * access to the shutdown commands */
void shutdown_system()
{
    system("sudo shutdown -h now");
}

void reboot_system()
{
    system("sudo shutdown -r now");
}
