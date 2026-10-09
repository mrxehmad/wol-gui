/* wolbox - entry point
 * SPDX-License-Identifier: MIT
 */
#include "ui.h"

int main(int argc, char **argv)
{
    static App app; /* one small struct, owned by main(), freed on destroy */

    gtk_init(&argc, &argv);
    g_set_prgname("wolbox");
    g_set_application_name("wolbox");

    /* Build the window and load hosts once the main loop is running. */
    g_idle_add(ui_startup, &app);

    gtk_main();
    return 0;
}
