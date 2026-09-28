// network_session_host_natneg_completed  (not a Ghidra function; no C existed)
// address 0x578120, size 49 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x578120..0x578150: the NAT negotiation completion callback (result, socket,
//   remote sockaddr_in, user data): on success it formats the remote address into a stack buffer with
//   gt2AddressToString and does nothing with it (a leftover).
// blam-cc: cdecl (a NAT negotiation callback)

#include "tags.h"

extern uint16_t gt2NetworkToHostShort(uint16_t value); // 0x6148a0 gt2NetworkToHostShort
extern char *gt2AddressToString(uint32_t ip, uint16_t port, char *string); // 0x6148b0 gt2AddressToString

void network_session_host_natneg_completed(int32_t result, uint32_t socket, const uint8_t *remote_address, void *user_data)
{
    char text[0x16];

    (void)socket;
    (void)user_data;
    if (result == 0) {
        gt2AddressToString(*(const uint32_t *)(remote_address + 4), gt2NetworkToHostShort(*(const uint16_t *)(remote_address + 2)), text);
    }
}
