/* wol-gui - entry point
 * SPDX-License-Identifier: MIT
 */
#include "ui.h"
#include "version.h"

int main(int argc, char **argv)
{
    static App app; /* one small struct, owned by main(), freed on destroy */

    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "--version") == 0) {
            g_print("%s %s\n", APP_NAME, APP_VERSION);
            return 0;
        }
        if (g_strcmp0(argv[i], "--help") == 0) {
            g_print("Usage: %s [--version | --help]\n", APP_NAME);
            return 0;
        }
    }

    /* XDG default: if the user left XDG_CONFIG_HOME unset but has a
     * ~/.local/config directory, keep dotfiles tidy by using it. */
    if (g_getenv("XDG_CONFIG_HOME") == NULL) {
        char *local_cfg = g_build_filename(g_get_home_dir(),
                                           ".local", "config", NULL);
        if (g_file_test(local_cfg, G_FILE_TEST_IS_DIR))
            g_setenv("XDG_CONFIG_HOME", local_cfg, TRUE);
        g_free(local_cfg);
    }

    gtk_init(&argc, &argv);
    g_set_prgname(APP_NAME);
    g_set_application_name(APP_NAME);

    /* Build the window and load hosts once the main loop is running. */
    g_idle_add(ui_startup, &app);

    gtk_main();
    return 0;
}
