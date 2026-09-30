// gt2StringToAddress  (GameSpy SDK in halo.exe; no C existed)
// address 0x614940, size 294 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614940..0x614a65: "host", "host:port" or ":port": the host part (copied to a
//   stack buffer before the colon) goes through inet_addr (WS2_32 #11) and, when that fails, gethostbyname (#52, its
//   first address); the port must be all digits and 0..0xffff. An empty or NULL string is address 0 port 0. Returns 0
//   on a bad port or an unknown host, else stores the results and returns 1.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"
#include <ctype.h>

int gt2StringToAddress(const char *string, unsigned int *ip, unsigned short *port)
{
    char host_buffer[0x100];
    const char *host = string;
    const char *colon;
    unsigned int ip_value = 0;
    unsigned short port_value = 0;

    if (string != 0 && string[0] != 0) {
        colon = strchr(string, ':');
        if (colon != 0) {
            const char *digits;
            int value;

            if (colon == string) {
                host = 0;
            } else {
                memcpy(host_buffer, string, colon - string);
                host_buffer[colon - string] = 0;
                host = host_buffer;
            }
            for (digits = colon + 1; *digits != 0; digits++) {
                if (!isdigit(*digits)) {
                    return 0;
                }
            }
            value = atoi(colon + 1);
            if (value < 0 || value > 0xffff) {
                return 0;
            }
            port_value = (unsigned short)value;
        }
        if (host != 0) {
            ip_value = inet_addr(host);
            if (ip_value == INADDR_NONE) {
                struct hostent *entry = gethostbyname(host);

                if (entry == 0) {
                    return 0;
                }
                ip_value = *(unsigned int *)entry->h_addr_list[0];
            }
        }
    }
    if (ip != 0) {
        *ip = ip_value;
    }
    if (port != 0) {
        *port = port_value;
    }
    return 1;
}
