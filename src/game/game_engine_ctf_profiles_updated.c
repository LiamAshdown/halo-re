// game_engine_ctf_profiles_updated  (not a Ghidra function; the ctf game engine definition's +0x90 slot (profiles_updated); no C existed, so that
//   stored pointer trapped as unlisted_469bf0)
// address 0x469bf0, size 287 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469bf0..0x469d0e: mode 0 encodes a type 0x11 request for the replicated touch
//   counts (0x87a9e0) with the flag auto-return ticks as the second field; otherwise encodes the live touch counts
//   and active team against that replicated copy, then stores them into it. A positive bit count is broadcast when
//   the machine is -1, else sent to that machine. The binary leaves the upper three bytes of the active team dword as
//   stack garbage; they are zero here (the decoder reads the low byte).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, blam-cc: EAX buffer, EDX size
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit,
    void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused); // 0x4e1a80, blam-cc: EAX bits, ECX server
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *server, uint32_t status_bit, void *data,
    uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, blam-cc: EAX machine, ESI server
extern int32_t ctf_touch_counts_network[3]; // 0x0087a9e0 (team 0 / team 1 touch counts, active team)
extern uint8_t ctf_active_team; // 0x006b0eb8
extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0

void game_engine_ctf_profiles_updated(int32_t mode, int32_t machine_index)
{
    int32_t ticks = ctf_flag_auto_return_ticks;
    void *extra[1];
    void *items[2];
    int32_t bits;

    extra[0] = &ticks;
    if (mode == 0) {
        items[0] = ctf_touch_counts_network;
        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x11, (int32_t)extra, items, 0, 1, 0);
    } else {
        int32_t live[3];
        void *baseline[1];

        live[0] = ctf_team_flag_touch_count[0];
        live[1] = ctf_team_flag_touch_count[1];
        live[2] = ctf_active_team;
        items[0] = live;
        items[1] = (void *)ticks;
        baseline[0] = ctf_touch_counts_network;
        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x11, (int32_t)extra, items,
            (int32_t)baseline, 1, 0);
        ctf_touch_counts_network[0] = live[0];
        ctf_touch_counts_network[1] = live[1];
        ctf_touch_counts_network[2] = live[2];
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
