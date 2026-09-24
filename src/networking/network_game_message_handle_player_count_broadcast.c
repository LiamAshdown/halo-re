// network_game_message_handle_player_count_broadcast  (Ghidra: FUN_004e2530; named per this rewrite)
// address 0x4e2530, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Handles message type 0x11 by decoding it and
// triggering a session-wide player-count broadcast" -- network_game_broadcast_player_set_changed
// (FUN_004e1bf0, this batch). Same decode shape as every sibling handler.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_game_broadcast_player_set_changed is called here with zero visible arguments,
// matching Ghidra literally rather than its own file's (server, param_1) signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern uint32_t network_game_broadcast_player_set_changed(void); // 0x4e1bf0, this module,
    // called here with no visible arguments (UNSURE, see header)

// blam-cc: ESI -> server, EDX -> buffer
uint32_t network_game_message_handle_player_count_broadcast(network_server_globals *server, uint8_t *buffer)
{
    uint8_t decoded_body[4];
    int16_t out_a, out_b;

    if (server->unknown_004 == 0 || server->unknown_004 == 1) {
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, &out_b, 3) != 0) {
            network_game_broadcast_player_set_changed();
        }
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
