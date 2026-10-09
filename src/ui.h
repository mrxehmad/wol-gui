/* wolbox - GTK3 user interface
 * SPDX-License-Identifier: MIT
 */
#ifndef WOLBOX_UI_H
#define WOLBOX_UI_H

#include <gtk/gtk.h>

#include "hosts.h"

typedef struct {
    HostList hosts;
    GtkWidget *window;
    GtkListStore *store;
    GtkTreeView *tree;
    GtkStatusbar *status;
} App;

/* Build the main window and all widgets. Returns false if setup failed. */
gboolean ui_startup(gpointer user_data);

#endif /* WOLBOX_UI_H */
