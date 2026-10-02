// network_channel_gap_441060  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441060, size 77 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441060..0x4410ac: the GT2 socket-error callback network_channels_open gives
//   both sockets: an unset join error (-1) becomes 6, a host handoff is requested, chat closes, every connection on
//   the socket is closed (gt2CloseAllConnections 0x614740, soft) and the socket global that held it -- the game
//   socket when it is that one, otherwise the query socket -- is cleared (GT2 frees the socket after this returns).
// blam-cc: cdecl (a GT2 socket error callback)

#include "tags.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_join_error_code;            // 0x00718fa4
extern uint8_t network_host_handoff_requested;     // 0x0071c2de
extern int32_t network_game_socket;                // 0x006f14c4
extern int32_t network_query_socket;               // 0x006f14c8
extern void chat_close(void);                      // 0x4aa900
extern void gt2CloseAllConnections(void *socket);  // 0x614740

void network_channel_gap_441060(void *socket)
{
    if (network_join_error_code == -1) {
        network_join_error_code = 6;
    }
    network_host_handoff_requested = 1;
    chat_close();
    gt2CloseAllConnections(socket);
    if ((int32_t)socket == network_game_socket) {
        network_game_socket = 0;
    } else {
        network_query_socket = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
