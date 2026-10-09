/* wolbox - host list model + GKeyFile persistence
 * SPDX-License-Identifier: MIT
 */
#ifndef WOLBOX_HOSTS_H
#define WOLBOX_HOSTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *name;
    char *mac;            /* "AA:BB:CC:DD:EE:FF" as entered/stored */
    char *broadcast_ip;   /* e.g. "255.255.255.255" */
    uint16_t port;        /* UDP port, usually 9 */
} Host;

typedef struct {
    Host *items;
    size_t len;
    size_t cap;
} HostList;

void host_list_init(HostList *list);
void host_list_free(HostList *list);

/* Duplicate `h` and append it. Returns false on allocation failure. */
bool host_list_append(HostList *list, const Host *h);

/* Remove index `i`, freeing its strings. */
void host_list_remove(HostList *list, size_t i);

/* Replace element at `i` with a copy of `h`. */
bool host_list_replace(HostList *list, size_t i, const Host *h);

/* Deep-copy helper. Caller frees with host_clear(). */
bool host_copy(Host *dst, const Host *src);
void host_clear(Host *h);

/* Load from $XDG_CONFIG_HOME/wolbox/hosts.ini (fallback ~/.config/wolbox/).
 * Missing or corrupt files are handled gracefully: the list stays empty and
 * a warning message (if any) is returned through *warning (caller g_frees). */
bool hosts_load(HostList *list, char **warning);

/* Atomically save to the same path, creating directories as needed.
 * Returns false and sets *err (g_free by caller) on failure. */
bool hosts_save(const HostList *list, char **err);

/* Full path of the config file (newly allocated string). */
char *hosts_config_path(void);

#endif /* WOLBOX_HOSTS_H */
