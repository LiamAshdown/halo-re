// network_game_client_handle_map_data  (Ghidra: FUN_004e2790; named per this rewrite)
// address 0x4e2790, size 126 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.25)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1b, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x1c: FUN_004e2790(param_1);` -- type 0x1b is instead handled inline inside the
// dispatcher itself (decode + a direct call to network_game_client_apply_position_update). The
// dispatcher's literal call table is trusted here instead of the low-confidence (0.3) summary.
// This function latches an incoming 32-byte game/map data block into the client's pending-state
// fields.
// register convention: stack = context (param_1, checked against state 1 and read/written at
// +0x9d8/+0x9f8), EDX = buffer (implicit passthrough), stack = length (param_2, only ever used
// as `length - 2`, its result unused beyond that -- dead by the time of the actual decode,
// which reads through the implicit EDX buffer instead).
//   // blam-cc: EDX -> buffer, stack -> context, length
// UNSURE (major): the summary calls this "client-side", but +0x9d8/+0x9f8 fall inside both
// types/networking.h's network_server_globals::unknown_9bc[0x3c] span AND
// network_client_globals::unknown_002[0xab2] span (both still-unresolved regions) -- nothing in
// this function's own body disambiguates which struct `context` really is, so it is kept as a
// raw byte pointer with explicit offset casts rather than asserting either type.
// UNSURE: network_player_entry_is_valid (FUN_004de9f0) is called here with zero visible
// arguments, against its own presumed EAX-based convention documented elsewhere in this batch.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2790: stack (server, length), EDX record, ESI machine; state (+4) 1, class 5. A decoded 0x20-byte player
// entry that validates (network_player_entry_validate, EAX) is stored at server +0x9d8 once (+0x9f8 set).

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

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, EAX

uint32_t network_game_client_handle_map_data(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;
    uint8_t *s = (uint8_t *)server;

    if (*(int16_t *)(s + 4) == 1 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) != 0 && s[0x9f8] == 0 &&
        network_player_entry_validate((network_player_entry *)body) != 0) {
        memcpy(s + 0x9d8, body, sizeof(body));
        s[0x9f8] = 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2790), from tools/pack.py 0x4e2790:

undefined4 FUN_004e2790(int param_1,int param_2)

{
  int iVar1;
  char cVar2;
  int iVar3;
  int in_EDX;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined1 local_24 [4];
  undefined4 local_20 [8];

  iVar1 = param_1;
  if (*(short *)(param_1 + 4) == 1) {
    param_2 = param_2 + -2;
    cVar2 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       &param_1,5);
    if ((cVar2 != '\0') && (*(char *)(iVar1 + 0x9f8) == '\0')) {
      cVar2 = FUN_004de9f0();
      if (cVar2 != '\0') {
        puVar4 = local_20;
        puVar5 = (undefined4 *)(iVar1 + 0x9d8);
        for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar5 = *puVar4;
          puVar4 = puVar4 + 1;
          puVar5 = puVar5 + 1;
        }
        *(undefined1 *)(iVar1 + 0x9f8) = 1;
        return 1;
      }
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
