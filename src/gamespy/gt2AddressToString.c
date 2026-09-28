// gt2AddressToString  (GameSpy SDK in halo.exe; no C existed)
// address 0x6148b0, size 144 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6148b0..0x61493f: without a buffer it alternates between two static 0x16 byte
//   strings (0x006a2494, index 0x006a24c0, kept here as statics); "%s:%d" / "%s" with inet_ntoa (WS2_32 #12) for a
//   non-zero ip, ":%d" for a port alone, else "".
// blam-cc: cdecl

#include "gamespy.h"

static char address_strings[2][0x16]; // 0x006a2494
static int address_string_index;      // 0x006a24c0

char *gt2AddressToString(unsigned int ip, unsigned short port, char *string)
{
    struct in_addr address;

    if (string == 0) {
        address_string_index ^= 1;
        string = address_strings[address_string_index];
    }
    address.s_addr = ip;
    if (ip != 0) {
        if (port != 0) {
            sprintf(string, "%s:%d", inet_ntoa(address), port);
        } else {
            sprintf(string, "%s", inet_ntoa(address));
        }
    } else if (port != 0) {
        sprintf(string, ":%d", port);
    } else {
        string[0] = 0;
    }
    return string;
}
