#include "halo/networking/net1_address.hpp"
#include "halo/core/datum.hpp"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

static auto &network_address_string = halo::link::ref<char [0x100]>(halo::networking::vars().network_address_string);

namespace halo::networking {

/**
 * Finds the first ':' in address_string; with none present, succeeds trivially (port_out
 * untouched). Otherwise every character after the ':' must be a digit and atol() of that
 * substring must fall in 1..0xffff; on success (with no ':' or with a valid in-range number)
 * writes the parsed value through port_out when it is non-NULL.
 *
 * @address 0x4dc560
 */
char AddressText::parse_port(char *address_string, int32_t *port_out)
{
    int all_digits;
    char *colon;
    char *cursor;
    int32_t port;
    char result;

    all_digits = 1;
    colon = strchr(address_string, ':');
    if (colon == 0) {
        return 1;
    }
    cursor = colon + 1;
    while (*cursor != '\0') {
        if (!isdigit((uint8_t)*cursor)) {
            all_digits = 0;
        }
        cursor = cursor + 1;
        if (!all_digits) {
            return 0;
        }
    }
    if (!all_digits) {
        return 0;
    }
    port = atol(colon + 1);
    result = (port < 1 || 0xffff < port) ? 0 : 1;
    if (port_out == 0) {
        return result;
    }
    *port_out = port;
    return result;
}

/**
 * VERIFIED against disassembly 0x4dc730..0x4dc78a (2026-09-30): every branch and the return value match.
 *
 * @address 0x4dc730
 */
char AddressText::string_is_valid(char *address_string)
{
    char scratch[28];
    char normalize_result;
    char valid;
    char c;

    normalize_result = 0;
    if (*address_string != '\0') {
        normalize_result = halo::networking::network_address_string_normalize(address_string, scratch, 0);
        if (normalize_result == 0) {
            valid = 1;
            while (1) {
                c = *address_string;
                if (c == '\0') {
                    return valid;
                }
                if (c == '.' || c == '-' || c == ':' || isalnum((uint8_t)c)) {
                    valid = 1;
                } else {
                    valid = 0;
                }
                address_string = address_string + 1;
                if (valid == 0) {
                    break;
                }
            }
            return valid;
        }
    }
    return normalize_result;
}

/**
 * VERIFIED against disassembly 0x4dc5e0..0x4dc726 (2026-09-30): sscanf argument order, the four range checks, the is-any
 *   flag, the parse_port call and both format argument orders match. Fixed: the original formats with the CRT _snprintf
 *   (0x623a2d, no NUL on truncation), not C99 snprintf.
 * Parses the leading "%d.%d.%d.%d" of address_string (tolerating one extra dot-delimited field
 * it never uses), range-checks each byte to 0..255, reports via out_is_any whether all four are
 * zero, then calls network_address_parse_port on the ORIGINAL string to look for a ':port'
 * suffix; formats out_buffer as "a.b.c.d:port" when a port was found, else "a.b.c.d".
 *
 * @address 0x4dc5e0
 */
char AddressText::string_normalize(char *address_string, char *out_buffer, uint8_t *out_is_any)
{
    char result;
    int32_t a, b, c, d, e;
    int32_t matched;
    uint8_t is_any;
    int32_t port;

    result = 0;
    port = -1;
    matched = sscanf(address_string, "%d.%d.%d.%d.%d", &a, &b, &c, &d, &e);
    if (matched == 4 && -1 < a && a < 0x100 && -1 < b && b < 0x100 &&
        -1 < c && c < 0x100 && -1 < d && d < 0x100) {
        is_any = 0;
        if (a == 0 && b == 0 && c == 0 && d == 0) {
            is_any = 1;
        }
        if (out_is_any != 0) {
            *out_is_any = is_any;
        }
        result = halo::networking::network_address_parse_port(address_string, &port);
        if (result == 1) {
            if (port != -1) {
                _snprintf(out_buffer, 0x19, "%d.%d.%d.%d:%d", a, b, c, d, port);
                return 1;
            }
            _snprintf(out_buffer, 0x19, "%d.%d.%d.%d", a, b, c, d);
        }
    }
    return result;
}

/**
 * Formats addr as "a.b.c.d:port" for an IPv4 address (size == 4) or eight hex groups for an
 * IPv6-shaped address (size == 0x10), into the shared static buffer, and returns it. If
 * neither size matches, the buffer is left as the empty string that was just written to it.
 *
 * @address 0x440570
 */
char * AddressView::to_string()
{
    s_network_address *addr = self;
    uint16_t *halfwords;

    network_address_string[0] = 0;
    if (addr->size == k_network_address_size_ipv4) {
        snprintf(network_address_string, 0x100, "%hd.%hd.%hd.%hd:%hu",
                 (uint32_t)*((uint8_t *)addr + 3),
                 (uint32_t)*((uint8_t *)addr + 2),
                 (uint32_t)*((uint8_t *)addr + 1),
                 (uint32_t)*((uint8_t *)addr + 0),
                 (uint32_t)addr->port);
        return network_address_string;
    }
    if (addr->size == k_network_address_size_ipv6) {
        halfwords = (uint16_t *)addr;
        snprintf(network_address_string, 0x100, "%4X.%4X.%4X.%4X.%4X.%4X.%4X.%4X:%hu",
                 (uint32_t)halfwords[0], (uint32_t)halfwords[1], (uint32_t)halfwords[2],
                 (uint32_t)halfwords[3], (uint32_t)halfwords[4], (uint32_t)halfwords[5],
                 (uint32_t)halfwords[6], (uint32_t)halfwords[7], (uint32_t)addr->port);
    }
    return network_address_string;
}

}
