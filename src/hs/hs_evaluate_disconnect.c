// hs_evaluate_disconnect  (not a Ghidra function; the evaluate handler of hs function 378 "disconnect" ( -> void))
// address 0x482770, size 40 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482770 trapped.
// WRITTEN 2026-09-28 from objdump 0x482770..0x482797: as a client in network_game_mode 1:
//   network_client_rejoin_check(0) (0x4de390); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern network_client_globals *network_client;
extern int16_t network_game_mode; // 0x00719720
extern void network_client_rejoin_check(int8_t machine_player_index); // 0x4de390

void hs_evaluate_disconnect(int16_t function_index, uint32_t thread_index, char first)
{
    if (network_client != 0 && network_game_mode == 1) {
        network_client_rejoin_check(0);
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
