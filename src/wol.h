/* wolbox - Wake-on-LAN magic packet sender
 * SPDX-License-Identifier: MIT
 */
#ifndef WOLBOX_WOL_H
#define WOLBOX_WOL_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WOL_MAC_LEN 6

/* Magic packet size: 6x 0xFF + 16x MAC = 102 bytes. */
#define WOL_PACKET_LEN 102

/* Parse "AA:BB:CC:DD:EE:FF" or "AA-BB-CC-DD-EE-FF" (case insensitive).
 * Returns true and fills mac[6] on success. */
bool wol_parse_mac(const char *text, uint8_t mac[WOL_MAC_LEN]);

/* Build the magic packet: 6x 0xFF followed by the MAC repeated 16 times.
 * out must be at least WOL_PACKET_LEN bytes. Returns the packet length. */
size_t wol_build_magic_packet(const uint8_t mac[WOL_MAC_LEN],
                              uint8_t out[WOL_PACKET_LEN]);

/* Send a magic packet for `mac` to broadcast_ip:port over UDP (SO_BROADCAST).
 * Returns true on success. On failure returns false and sets *err to a
 * human readable message (never NULL when false). */
bool wol_send_packet(const char *broadcast_ip, uint16_t port,
                     const uint8_t mac[WOL_MAC_LEN], const char **err);

#endif /* WOLBOX_WOL_H */
