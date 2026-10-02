// network_channel_gap_441200  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441200, size 243 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441200..0x4412f2: the query socket's GT2 unrecognized-message callback (socket,
//   ip, port, message, length): the message (at most 0x1fff bytes) is copied to 0x006a6148 and terminated; a natneg
//   packet (the 6 byte magic at 0x00657208) goes to NNProcessData with the sender as a sockaddr_in and is handled
//   (1); a query ("\\" or ";" first, or 0xfe 0xfd) counts as handled (1); anything else 0.
// blam-cc: cdecl (a GT2 unrecognized message callback)

#include "tags.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t network_query_receive_buffer[0x2000]; // 0x006a6148
extern const uint8_t natneg_magic[6];                // 0x00657208
extern void NNProcessData(char *data, int32_t len, void *fromaddr); // 0x615240 NNProcessData

int32_t network_channel_gap_441200(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    uint8_t is_natneg = 0;
    uint8_t is_query;

    (void)socket;
    if (length >= 0x1fff) {
        length = 0x1fff;
    }
    memcpy(network_query_receive_buffer, message, length);
    network_query_receive_buffer[length] = 0;
    if ((int32_t)length >= 6 && memcmp(network_query_receive_buffer, natneg_magic, 6) == 0) {
        is_natneg = 1;
    }
    is_query = ((int32_t)length >= 1 && network_query_receive_buffer[0] == 0x5c) || network_query_receive_buffer[0] == 0x3b ||
               ((int32_t)length >= 2 && network_query_receive_buffer[0] == 0xfe && network_query_receive_buffer[1] == 0xfd);
    if (is_natneg) {
        uint8_t address[16];

        memset(address, 0, sizeof(address));
        *(uint16_t *)(address + 0) = 2;
        *(uint16_t *)(address + 2) = (uint16_t)((port >> 8) | (port << 8));
        *(uint32_t *)(address + 4) = ip;
        NNProcessData((char *)network_query_receive_buffer, (int32_t)length, address);
        return 1;
    }
    return is_query ? 1 : 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
