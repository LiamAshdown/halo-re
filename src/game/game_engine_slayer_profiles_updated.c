// game_engine_slayer_profiles_updated  (not a Ghidra function; the slayer game engine definition's +0x90 slot (profiles_updated); no C existed, so that
//   stored pointer trapped as unlisted_46fa40)
// address 0x46fa40, size 214 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46fa40..0x46fb15: mode 0 encodes a type 0x10 request for the replicated scores
//   at 0x87a4a0; otherwise encodes type 0x10 from the live team scores 0x6b13d8 against 0x87a4a0 and then copies the
//   0x20 live dwords (team and player scores) over the replicated copy. A positive bit count is broadcast when the
//   machine is -1, else sent to that machine.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>
#include "objects.h"
#include "units.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, blam-cc: EAX buffer, EDX size
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit,
    void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused); // 0x4e1a80, blam-cc: EAX bits, ECX server
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *server, uint32_t status_bit, void *data,
    uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, blam-cc: EAX machine, ESI server

void game_engine_slayer_profiles_updated(int32_t mode, int32_t machine_index)
{
    void *items[2];
    void *network_fields[1];
    int32_t bits;

    if (mode == 0) {
        network_fields[0] = slayer_unknown_0087a4a0;
        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x10, 0, network_fields, 0, 1, 0);
    } else {
        items[0] = slayer_team_score;
        items[1] = 0;
        network_fields[0] = slayer_unknown_0087a4a0;
        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x10, 0, items, (int32_t)network_fields, 1, 0);
        memcpy(slayer_unknown_0087a4a0, slayer_team_score, 0x20 * 4);
    }
    if (bits <= 0) {
        return;
    }
    if (machine_index == -1) {
        network_session_broadcast_to_flagged(bits, network_server, 1, network_message_scratch, 1, 0, 0, 3);
    } else {
        network_session_send_to_machine(machine_index, network_server, 1, network_message_scratch, bits, 1, 0, 0, 3);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
