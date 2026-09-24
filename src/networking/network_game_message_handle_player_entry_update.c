// network_game_message_handle_player_entry_update  (Ghidra: FUN_004e2580; named per this rewrite)
// address 0x4e2580, size 88 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Handles message type 0x12, conditionally
// triggering a player-count broadcast after an approval check" -- the approval check is
// network_player_entry_update (FUN_004de5f0, already written), whose own signature is
// (network_player_entry *incoming, network_game_session *session); on success this broadcasts
// the player set change exactly like the type-0x11 handler.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_player_entry_update and network_game_broadcast_player_set_changed are both
// called here with zero visible arguments, matching Ghidra literally rather than their own
// files' fuller signatures.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern uint32_t network_player_entry_update(void); // 0x4de5f0, this module,
    // called here with no visible arguments (UNSURE, see header)
extern uint32_t network_game_broadcast_player_set_changed(void); // 0x4e1bf0, this module,
    // called here with no visible arguments (UNSURE, see header)

// blam-cc: ESI -> server, EDX -> buffer
uint32_t network_game_message_handle_player_entry_update(network_server_globals *server, uint8_t *buffer)
{
    uint8_t decoded_body[32];
    int16_t out_a, out_b;

    if (server->unknown_004 == 0) {
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, &out_b, 3) != 0) {
            if (network_player_entry_update() != 0) {
                network_game_broadcast_player_set_changed();
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2580), from tools/pack.py 0x4e2580:

undefined4 FUN_004e2580(void)

{
  char cVar1;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,3);
    if (cVar1 != '\0') {
      cVar1 = FUN_004de5f0();
      if (cVar1 != '\0') {
        FUN_004e1bf0();
      }
    }
  }
  return 1;
}
#endif
