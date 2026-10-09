/* wolbox - Wake-on-LAN magic packet sender
 * SPDX-License-Identifier: MIT
 */
#define _POSIX_C_SOURCE 200809L

#include "wol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

bool wol_parse_mac(const char *text, uint8_t mac[WOL_MAC_LEN])
{
    if (!text)
        return false;

    /* Work on a copy so we never touch the caller's buffer. */
    char buf[64];
    size_t len = strlen(text);
    if (len == 0 || len >= sizeof(buf))
        return false;
    memcpy(buf, text, len + 1);

    /* Normalise separators: ':' or '-' allowed, but consistently. */
    char sep = '\0';
    for (size_t i = 0; i < len; i++) {
        char c = buf[i];
        if (c == ':' || c == '-') {
            if (sep == '\0')
                sep = c;
            else if (c != sep)
                return false; /* mixed separators */
            buf[i] = ' ';
        } else if (!isxdigit((unsigned char)c)) {
            return false;
        }
    }

    unsigned int v[WOL_MAC_LEN];
    int n = sscanf(buf, "%2x %2x %2x %2x %2x %2x",
                   &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
    if (n != WOL_MAC_LEN)
        return false;

    /* Make sure nothing trails the six octets. */
    const char *p = buf;
    for (int i = 0; i < WOL_MAC_LEN; i++) {
        while (*p == ' ')
            p++;
        for (int k = 0; k < 2; k++) {
            if (!isxdigit((unsigned char)*p))
                return false;
            p++;
        }
    }
    while (*p == ' ')
        p++;
    if (*p != '\0')
        return false;

    for (int i = 0; i < WOL_MAC_LEN; i++)
        mac[i] = (uint8_t)v[i];
    return true;
}

size_t wol_build_magic_packet(const uint8_t mac[WOL_MAC_LEN],
                              uint8_t out[WOL_PACKET_LEN])
{
    memset(out, 0xFF, 6);
    for (int i = 0; i < 16; i++)
        memcpy(out + 6 + (size_t)i * WOL_MAC_LEN, mac, WOL_MAC_LEN);
    return WOL_PACKET_LEN;
}

bool wol_send_packet(const char *broadcast_ip, uint16_t port,
                     const uint8_t mac[WOL_MAC_LEN], const char **err)
{
    static char errbuf[256];
    struct sockaddr_in addr;

    if (err)
        *err = NULL;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, broadcast_ip, &addr.sin_addr) != 1) {
        snprintf(errbuf, sizeof(errbuf), "invalid broadcast address \"%s\"",
                 broadcast_ip ? broadcast_ip : "");
        if (err)
            *err = errbuf;
        return false;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        snprintf(errbuf, sizeof(errbuf), "socket: %s", strerror(errno));
        if (err)
            *err = errbuf;
        return false;
    }

    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
        snprintf(errbuf, sizeof(errbuf), "setsockopt(SO_BROADCAST): %s",
                 strerror(errno));
        close(fd);
        if (err)
            *err = errbuf;
        return false;
    }

    uint8_t packet[WOL_PACKET_LEN];
    size_t plen = wol_build_magic_packet(mac, packet);

    ssize_t sent = sendto(fd, packet, plen, 0,
                          (struct sockaddr *)&addr, sizeof(addr));
    if (sent < 0) {
        snprintf(errbuf, sizeof(errbuf), "sendto %s:%u: %s",
                 broadcast_ip, (unsigned)port, strerror(errno));
        close(fd);
        if (err)
            *err = errbuf;
        return false;
    }
    close(fd);
    return true;
}
