/* wol-gui - GTK3 user interface
 * SPDX-License-Identifier: MIT
 */
#include "ui.h"
#include "version.h"
#include "wol.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Columns of the GtkListStore (all G_TYPE_STRING). */
enum { COL_NAME, COL_MAC, COL_BCAST, COL_PORT, N_COLS };

#define DEFAULT_BCAST "255.255.255.255"
#define DEFAULT_PORT  9

static void status_msg(App *app, const char *fmt, ...)
    G_GNUC_PRINTF(2, 3);

static void status_msg(App *app, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    guint id = gtk_statusbar_get_context_id(app->status, "wol-gui");
    gtk_statusbar_pop(app->status, id);
    gtk_statusbar_push(app->status, id, buf);
}

static void error_dialog(App *app, const char *msg)
{
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                          GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_MESSAGE_ERROR,
                                          GTK_BUTTONS_OK,
                                          "%s", msg ? msg : "Unknown error");
    gtk_window_set_title(GTK_WINDOW(d), APP_NAME);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

/* About dialog: version, config location, license + attribution. */
static void on_about_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    App *app = user_data;

    char *cfg = hosts_config_path();
    GtkWidget *d = gtk_message_dialog_new(
        GTK_WINDOW(app->window), GTK_DIALOG_MODAL |
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_OTHER, GTK_BUTTONS_CLOSE,
        "%s %s\nSimple Wake-on-LAN client (GTK3)\n\n"
        "Config file: %s\n\n"
        "Inspired by gwakeonlan (https://github.com/benbotello/gwakeonlan).\n"
        "Icon: \"totalcmd-lan-windows-shares\" via svgrepo.com.\n"
        "Licensed under the MIT License.",
        APP_NAME, APP_VERSION, cfg);
    gtk_window_set_title(GTK_WINDOW(d), "About " APP_NAME);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    g_free(cfg);
}

/* Persist the model; report failures in the status bar. */
static void save_hosts(App *app)
{
    char *err = NULL;
    if (!hosts_save(&app->hosts, &err)) {
        status_msg(app, "Save failed: %s", err ? err : "unknown error");
        error_dialog(app, err ? err : "Failed to save host list.");
    }
    g_free(err);
}

static void refresh_tree(App *app)
{
    gtk_list_store_clear(app->store);
    for (size_t i = 0; i < app->hosts.len; i++) {
        Host *h = &app->hosts.items[i];
        GtkTreeIter it;
        gtk_list_store_append(app->store, &it);
        char port[8];
        snprintf(port, sizeof(port), "%u", (unsigned)h->port);
        gtk_list_store_set(app->store, &it,
                           COL_NAME, h->name,
                           COL_MAC, h->mac,
                           COL_BCAST, h->broadcast_ip,
                           COL_PORT, port,
                           -1);
    }
}

/* Return selected row index, or -1 if none. */
static gint selected_row(App *app)
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(app->tree);
    GtkTreeModel *model;
    GtkTreeIter it;
    if (!gtk_tree_selection_get_selected(sel, &model, &it))
        return -1;
    GtkTreePath *path = gtk_tree_model_get_path(model, &it);
    gint row = gtk_tree_path_get_indices(path)[0];
    gtk_tree_path_free(path);
    return row;
}

/* ---- Wake ----------------------------------------------------------- */

static bool wake_host(App *app, size_t idx)
{
    if (idx >= app->hosts.len)
        return false;
    Host *h = &app->hosts.items[idx];
    uint8_t mac[WOL_MAC_LEN];
    if (!wol_parse_mac(h->mac, mac)) {
        status_msg(app, "\"%s\": stored MAC is invalid", h->name);
        return false;
    }
    const char *err = NULL;
    if (wol_send_packet(h->broadcast_ip, h->port, mac, &err)) {
        status_msg(app, "Wake-on-LAN packet sent to %s (%s via %s:%u)",
                   h->name, h->mac, h->broadcast_ip, (unsigned)h->port);
        return true;
    }
    status_msg(app, "Failed to wake %s: %s", h->name, err ? err : "send error");
    return false;
}

static void on_wake_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    App *app = data;
    gint row = selected_row(app);
    if (row < 0) {
        status_msg(app, "Select a host to wake first.");
        return;
    }
    wake_host(app, (size_t)row);
}

static void on_wake_all_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    App *app = data;
    if (app->hosts.len == 0) {
        status_msg(app, "No hosts configured.");
        return;
    }
    size_t ok = 0;
    for (size_t i = 0; i < app->hosts.len; i++)
        ok += wake_host(app, i) ? 1 : 0;
    status_msg(app, "Wake All: sent %zu of %zu packet(s).",
               ok, app->hosts.len);
}

static void on_row_double_click(GtkTreeView *tree, GtkTreePath *path,
                                GtkTreeViewColumn *col, gpointer data)
{
    (void)tree; (void)col;
    App *app = data;
    wake_host(app, (size_t)gtk_tree_path_get_indices(path)[0]);
}

/* ---- Add / Edit dialog ---------------------------------------------- */

typedef struct {
    GtkWidget *dialog;
    GtkWidget *name;
    GtkWidget *mac;
    GtkWidget *bcast;
    GtkWidget *port;
} HostForm;

static void form_fill(HostForm *f, const Host *h)
{
    gtk_entry_set_text(GTK_ENTRY(f->name), h ? h->name : "");
    gtk_entry_set_text(GTK_ENTRY(f->mac), h ? h->mac : "");
    gtk_entry_set_text(GTK_ENTRY(f->bcast),
                       h ? h->broadcast_ip : DEFAULT_BCAST);
    char port[8];
    snprintf(port, sizeof(port), "%u",
             (unsigned)(h ? h->port : DEFAULT_PORT));
    gtk_entry_set_text(GTK_ENTRY(f->port), port);
}

/* Build the dialog contents; returns the form widgets. */
static void form_setup(HostForm *f, GtkWidget *grid)
{
    f->name = gtk_entry_new();
    f->mac = gtk_entry_new();
    f->bcast = gtk_entry_new();
    f->port = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(f->mac), 20);
    gtk_entry_set_placeholder_text(GTK_ENTRY(f->mac), "AA:BB:CC:DD:EE:FF");
    gtk_entry_set_placeholder_text(GTK_ENTRY(f->name), "e.g. desktop");

    const char *labels[] = { "Name:", "MAC address:",
                             "Broadcast IP:", "UDP port:" };
    GtkWidget *entries[] = { f->name, f->mac, f->bcast, f->port };
    for (int i = 0; i < 4; i++) {
        GtkWidget *l = gtk_label_new(labels[i]);
        gtk_widget_set_halign(l, GTK_ALIGN_END);
        gtk_grid_attach(GTK_GRID(grid), l, 0, i, 1, 1);
        gtk_widget_set_hexpand(entries[i], TRUE);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }
}

/* Validate the form into `out`. Returns false and sets *msg (a static
 * string literal) on error. On success `out` owns newly allocated strings. */
static bool form_validate(HostForm *f, Host *out, const char **msg)
{
    const char *name = gtk_entry_get_text(GTK_ENTRY(f->name));
    const char *macs = gtk_entry_get_text(GTK_ENTRY(f->mac));
    const char *ip = gtk_entry_get_text(GTK_ENTRY(f->bcast));
    const char *ports = gtk_entry_get_text(GTK_ENTRY(f->port));

    uint8_t mac[WOL_MAC_LEN];
    char *endp = NULL;
    long port = strtol(ports, &endp, 10);

    GInetAddress *ia = ip && *ip ? g_inet_address_new_from_string(ip) : NULL;
    bool ip_ok = ia && g_inet_address_get_family(ia) == G_SOCKET_FAMILY_IPV4;
    g_clear_object(&ia);

    if (name == NULL || *name == '\0') {
        *msg = "Name must not be empty.";
        return false;
    }
    if (!wol_parse_mac(macs, mac)) {
        *msg = "Invalid MAC address. Use AA:BB:CC:DD:EE:FF or "
               "AA-BB-CC-DD-EE-FF.";
        return false;
    }
    if (!ip_ok) {
        *msg = "Invalid IPv4 broadcast address (e.g. 255.255.255.255 "
               "or 192.168.1.255).";
        return false;
    }
    if (port < 1 || port > 65535 || (endp && *endp != '\0')) {
        *msg = "Port must be a number between 1 and 65535.";
        return false;
    }

    out->name = g_strdup(name);
    /* Store the MAC normalised to upper-case colon form. */
    char normmac[18];
    snprintf(normmac, sizeof(normmac), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    out->mac = g_strdup(normmac);
    out->broadcast_ip = g_strdup(ip);
    out->port = (uint16_t)port;
    return true;
}

static gboolean edit_host(App *app, const Host *existing, Host *out)
{
    const char *title = existing ? "Edit host" : "Add host";
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        title, GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_OK", GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 8);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    HostForm f = { .dialog = dialog };
    form_setup(&f, grid);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);
    form_fill(&f, existing);
    gtk_widget_show_all(dialog);

    gboolean accepted = FALSE;
    for (;;) {
        gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp != GTK_RESPONSE_OK)
            break;
        const char *msg = NULL;
        if (form_validate(&f, out, &msg)) {
            accepted = TRUE;
            break;
        }
        error_dialog(app, msg);
    }
    gtk_widget_destroy(dialog);
    return accepted;
}

static void on_add_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    App *app = data;
    Host h = {0};
    if (!edit_host(app, NULL, &h))
        return;
    if (!host_list_append(&app->hosts, &h)) {
        status_msg(app, "Out of memory while adding host.");
        host_clear(&h);
        return;
    }
    host_clear(&h);
    refresh_tree(app);
    save_hosts(app);
    status_msg(app, "Added \"%s\".", app->hosts.items[app->hosts.len - 1].name);
}

static void on_edit_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    App *app = data;
    gint row = selected_row(app);
    if (row < 0) {
        status_msg(app, "Select a host to edit first.");
        return;
    }
    Host h = {0};
    if (!edit_host(app, &app->hosts.items[row], &h))
        return;
    char *name = g_strdup(h.name);
    if (!host_list_replace(&app->hosts, (size_t)row, &h)) {
        status_msg(app, "Failed to update host.");
        host_clear(&h);
        g_free(name);
        return;
    }
    host_clear(&h);
    refresh_tree(app);
    save_hosts(app);
    status_msg(app, "Updated \"%s\".", name ? name : "host");
    g_free(name);
}

static void on_delete_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    App *app = data;
    gint row = selected_row(app);
    if (row < 0) {
        status_msg(app, "Select a host to delete first.");
        return;
    }
    char *name = g_strdup(app->hosts.items[row].name);
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                          GTK_DIALOG_MODAL,
                                          GTK_MESSAGE_QUESTION,
                                          GTK_BUTTONS_YES_NO,
                                          "Delete host \"%s\"?", name);
    gint resp = gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    if (resp == GTK_RESPONSE_YES) {
        host_list_remove(&app->hosts, (size_t)row);
        refresh_tree(app);
        save_hosts(app);
        status_msg(app, "Deleted \"%s\".", name);
    } else {
        status_msg(app, "Delete cancelled.");
    }
    g_free(name);
}

static void on_destroy(GtkWidget *w, gpointer data)
{
    (void)w;
    App *app = data;
    host_list_free(&app->hosts);
    gtk_main_quit();
}

/* ---- Window construction -------------------------------------------- */

static GtkWidget *make_button(const char *label, GCallback cb, App *app)
{
    GtkWidget *b = gtk_button_new_with_label(label);
    g_signal_connect(b, "clicked", cb, app);
    return b;
}

gboolean ui_startup(gpointer user_data)
{
    App *app = user_data;
    host_list_init(&app->hosts);

    char *warning = NULL;
    hosts_load(&app->hosts, &warning);

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "wol-gui");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 720, 420);
    gtk_window_set_icon_name(GTK_WINDOW(app->window), "wol-gui");
    g_signal_connect(app->window, "destroy", G_CALLBACK(on_destroy), app);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);

    /* Tree view inside a scrolled window. */
    app->store = gtk_list_store_new(N_COLS,
                                    G_TYPE_STRING, G_TYPE_STRING,
                                    G_TYPE_STRING, G_TYPE_STRING);
    app->tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(
                                  GTK_TREE_MODEL(app->store)));
    g_signal_connect(app->tree, "row-activated",
                     G_CALLBACK(on_row_double_click), app);

    const char *titles[] = { "Name", "MAC", "Broadcast IP", "Port" };
    for (int i = 0; i < N_COLS; i++) {
        GtkCellRenderer *r = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c = gtk_tree_view_column_new_with_attributes(
            titles[i], r, "text", i, NULL);
        gtk_tree_view_column_set_resizable(c, TRUE);
        gtk_tree_view_column_set_min_width(c, i == COL_NAME ? 140 : 90);
        gtk_tree_view_append_column(app->tree, c);
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(app->tree));
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    /* Button bar. */
    GtkWidget *hb = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(hb), 6);
    gtk_box_pack_start(GTK_BOX(hb), make_button("Add",
                         G_CALLBACK(on_add_clicked), app), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb), make_button("Edit",
                         G_CALLBACK(on_edit_clicked), app), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb), make_button("Delete",
                         G_CALLBACK(on_delete_clicked), app), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb), make_button("Wake",
                         G_CALLBACK(on_wake_clicked), app), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hb), make_button("Wake All",
                         G_CALLBACK(on_wake_all_clicked), app), TRUE, TRUE, 0);

    GtkWidget *about = make_button("About",
                                   G_CALLBACK(on_about_clicked), app);
    gtk_widget_set_halign(about, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(hb), about, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hb, FALSE, FALSE, 0);

    /* Status bar. */
    app->status = GTK_STATUSBAR(gtk_statusbar_new());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(app->status), FALSE, FALSE, 0);

    gtk_widget_show_all(app->window);
    refresh_tree(app);

    if (warning) {
        status_msg(app, "%s", warning);
        g_free(warning);
    } else {
        char *path = hosts_config_path();
        status_msg(app, "Loaded %zu host(s) from %s",
                   app->hosts.len, path);
        g_free(path);
    }
    return FALSE;
}
