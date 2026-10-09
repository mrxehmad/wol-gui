/* wol-gui - host list model + GKeyFile persistence
 * SPDX-License-Identifier: MIT
 */
#define _POSIX_C_SOURCE 200809L

#include "hosts.h"
#include "version.h"
#include "wol.h"

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_rename, g_unlink */
#include <stdio.h>
#include <string.h>

#define CFG_GROUP "host"

void host_list_init(HostList *list)
{
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

void host_list_free(HostList *list)
{
    for (size_t i = 0; i < list->len; i++)
        host_clear(&list->items[i]);
    g_free(list->items);
    list->items = NULL;
    list->len = list->cap = 0;
}

bool host_copy(Host *dst, const Host *src)
{
    dst->name = g_strdup(src->name);
    dst->mac = g_strdup(src->mac);
    dst->broadcast_ip = g_strdup(src->broadcast_ip);
    dst->port = src->port;
    return dst->name && dst->mac && dst->broadcast_ip;
}

void host_clear(Host *h)
{
    g_free(h->name);
    g_free(h->mac);
    g_free(h->broadcast_ip);
    h->name = h->mac = h->broadcast_ip = NULL;
    h->port = 0;
}

static bool host_list_reserve(HostList *list, size_t need)
{
    if (need <= list->cap)
        return true;
    size_t cap = list->cap ? list->cap : 8;
    while (cap < need)
        cap *= 2;
    Host *tmp = g_try_realloc(list->items, cap * sizeof(*tmp));
    if (!tmp)
        return false;
    list->items = tmp;
    list->cap = cap;
    return true;
}

bool host_list_append(HostList *list, const Host *h)
{
    if (!host_list_reserve(list, list->len + 1))
        return false;
    return host_copy(&list->items[list->len++], h);
}

void host_list_remove(HostList *list, size_t i)
{
    if (i >= list->len)
        return;
    host_clear(&list->items[i]);
    memmove(&list->items[i], &list->items[i + 1],
            (list->len - i - 1) * sizeof(Host));
    list->len--;
}

bool host_list_replace(HostList *list, size_t i, const Host *h)
{
    if (i >= list->len)
        return false;
    Host copy;
    if (!host_copy(&copy, h))
        return false;
    host_clear(&list->items[i]);
    list->items[i] = copy;
    return true;
}

/* Config lives under $XDG_CONFIG_HOME (default: ~/.config), i.e.
 * ~/.config/wol-gui/hosts.ini unless the user overrides XDG_CONFIG_HOME
 * (e.g. XDG_CONFIG_HOME=~/.local/config). */
char *hosts_config_path(void)
{
    return g_build_filename(g_get_user_config_dir(), APP_NAME, "hosts.ini",
                            NULL);
}

bool hosts_load(HostList *list, char **warning)
{
    if (warning)
        *warning = NULL;

    char *path = hosts_config_path();
    GKeyFile *kf = g_key_file_new();
    GError *err = NULL;

    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, &err)) {
        if (err && !g_error_matches(err, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            /* Corrupt / unreadable file: start empty but tell the user. */
            if (warning)
                *warning = g_strdup_printf("Could not load %s (%s). "
                                           "Starting with an empty list.",
                                           path, err->message);
        }
        /* Missing file is perfectly fine. */
        g_clear_error(&err);
        g_key_file_free(kf);
        g_free(path);
        return true;
    }

    gsize ngroups = 0;
    char **groups = g_key_file_get_groups(kf, &ngroups);
    for (gsize g = 0; g < ngroups; g++) {
        GError *e2 = NULL;
        char *name = g_key_file_get_string(kf, groups[g], "name", &e2);
        char *macstr = g_key_file_get_string(kf, groups[g], "mac", &e2);
        char *bcast = g_key_file_get_string(kf, groups[g], "broadcast", &e2);
        gint port = g_key_file_get_integer(kf, groups[g], "port", &e2);
        g_clear_error(&e2); /* missing/invalid keys are caught by `ok` below */

        uint8_t mac[WOL_MAC_LEN];
        bool ok = name && *name && macstr && bcast &&
                  wol_parse_mac(macstr, mac) &&
                  port > 0 && port <= 65535;
        if (ok) {
            Host h = { .name = name, .mac = macstr,
                       .broadcast_ip = bcast, .port = (uint16_t)port };
            if (!host_list_append(list, &h) && warning)
                *warning = g_strdup("Out of memory while loading hosts.");
        } else if (warning && !*warning) {
            *warning = g_strdup_printf("Skipped invalid entry \"%s\" in %s.",
                                       groups[g], path);
        }
        g_free(name);
        g_free(macstr);
        g_free(bcast);
    }
    g_strfreev(groups);
    g_key_file_free(kf);
    g_free(path);
    return true;
}

bool hosts_save(const HostList *list, char **err)
{
    if (err)
        *err = NULL;

    char *dir = g_build_filename(g_get_user_config_dir(), APP_NAME, NULL);
    char *path = hosts_config_path();
    char *tmp = g_strconcat(path, ".tmp", NULL);
    GKeyFile *kf = g_key_file_new();
    bool ret = false;
    GError *e = NULL;

    if (!g_mkdir_with_parents(dir, 0755)) {
        for (size_t i = 0; i < list->len; i++) {
            char group[64];
            snprintf(group, sizeof(group), CFG_GROUP "%zu", i);
            g_key_file_set_string(kf, group, "name", list->items[i].name);
            g_key_file_set_string(kf, group, "mac", list->items[i].mac);
            g_key_file_set_string(kf, group, "broadcast",
                                  list->items[i].broadcast_ip);
            g_key_file_set_integer(kf, group, "port", list->items[i].port);
        }
        if (g_key_file_save_to_file(kf, tmp, &e)) {
            if (g_rename(tmp, path) == 0)
                ret = true;
            else
                g_set_error(&e, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                            "rename to %s: %s", path, strerror(errno));
        }
    } else {
        g_set_error(&e, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                    "cannot create %s: %s", dir, strerror(errno));
    }

    if (!ret && err && e)
        *err = g_strdup(e->message);
    g_unlink(tmp);
    g_clear_error(&e);
    g_key_file_free(kf);
    g_free(tmp);
    g_free(path);
    g_free(dir);
    return ret;
}
