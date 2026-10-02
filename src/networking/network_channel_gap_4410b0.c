// network_channel_gap_4410b0  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x4410b0, size 331 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4410b0..0x4411fa: the game socket's GT2 unrecognized-message callback (socket,
//   ip, port, message, length): like 0x441200 (copy of at most 0x1fff bytes to 0x006a4140; natneg packets to
//   NNProcessData and handled) but a query ("\\" or ";" first, or 0xfe 0xfd) also goes to qr2_parse_queryA for the
//   host record when there is one; queries count as handled, anything else 0.
// blam-cc: cdecl (a GT2 unrecognized message callback)

#include "tags.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t network_game_receive_buffer[0x2000]; // 0x006a4140
extern const uint8_t natneg_magic[6];               // 0x00657208
extern void *network_session_host_object;           // 0x00722a20
extern void NNProcessData(char *data, int32_t len, void *fromaddr); // 0x615240 NNProcessData
extern void qr2_parse_queryA(void *qrec, char *query, int32_t len, void *sender); // 0x616050 qr2_parse_queryA

int32_t network_channel_gap_4410b0(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    uint8_t is_natneg = 0;
    uint8_t is_query;
    uint8_t address[16];

    (void)socket;
    if (length >= 0x1fff) {
        length = 0x1fff;
    }
    memcpy(network_game_receive_buffer, message, length);
    network_game_receive_buffer[length] = 0;
    if ((int32_t)length >= 6 && memcmp(network_game_receive_buffer, natneg_magic, 6) == 0) {
        is_natneg = 1;
    }
    is_query = ((int32_t)length >= 1 && network_game_receive_buffer[0] == 0x5c) || network_game_receive_buffer[0] == 0x3b ||
               ((int32_t)length >= 2 && network_game_receive_buffer[0] == 0xfe && network_game_receive_buffer[1] == 0xfd);
    memset(address, 0, sizeof(address));
    *(uint16_t *)(address + 0) = 2;
    *(uint16_t *)(address + 2) = (uint16_t)((port >> 8) | (port << 8));
    *(uint32_t *)(address + 4) = ip;
    if (is_natneg) {
        NNProcessData((char *)network_game_receive_buffer, (int32_t)length, address);
        return 1;
    }
    if (!is_query) {
        return 0;
    }
    if (network_session_host_object != 0) {
        qr2_parse_queryA(network_session_host_object, (char *)network_game_receive_buffer, (int32_t)length, address);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
