// network_game_message_handle_player_entry_update  (Ghidra: FUN_004e2580; named per this rewrite)
// address 0x4e2580, size 88 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
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
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2580: ESI server, EDX record, stack length; state (+4) 0, class 3; the entry goes to
// network_player_entry_update (EAX entry, ECX server +8).

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

extern uint8_t network_player_entry_update(network_player_entry *incoming, network_game_session *session); // 0x4de5f0, EAX, ECX
extern uint32_t network_game_broadcast_player_set_changed(network_server_globals *session); // 0x4e1bf0, stack

uint32_t network_game_message_handle_player_entry_update(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0 &&
        network_player_entry_update((network_player_entry *)body, (network_game_session *)((uint8_t *)server + 8)) != 0) {
        network_game_broadcast_player_set_changed(server);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
