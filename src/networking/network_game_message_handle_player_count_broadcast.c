// network_game_message_handle_player_count_broadcast  (Ghidra: FUN_004e2530; named per this rewrite)
// address 0x4e2530, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Handles message type 0x11 by decoding it and
// triggering a session-wide player-count broadcast" -- network_game_broadcast_player_set_changed
// (FUN_004e1bf0, this batch). Same decode shape as every sibling handler.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_game_broadcast_player_set_changed is called here with zero visible arguments,
// matching Ghidra literally rather than its own file's (server, param_1) signature.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2530: ESI server, EDX record, stack length; state (+4) 0 or 1, class 3.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6

extern uint32_t network_game_broadcast_player_set_changed(network_server_globals *session); // 0x4e1bf0, stack

uint32_t network_game_message_handle_player_count_broadcast(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);

    if ((state == 0 || state == 1) && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        network_game_broadcast_player_set_changed(server);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2530), from tools/pack.py 0x4e2530:

undefined4 FUN_004e2530(void)

{
  char cVar1;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if ((*(short *)(unaff_ESI + 4) == 0) || (*(short *)(unaff_ESI + 4) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,in_EDX + 2,local_8,
                       local_c,3);
    if (cVar1 != '\0') {
      FUN_004e1bf0();
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
