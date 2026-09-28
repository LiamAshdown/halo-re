// network_game_client_handle_map_data  (Ghidra: FUN_004e2790; named per this rewrite)
// address 0x4e2790, size 126 bytes
// name confidence: 0.35   rewrite confidence: 0.25
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, blam-cc: EAX -> entry
    // blam-cc: EAX -> entry; 0x4de9f0, other module. The EAX convention is pinned by
    // network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
    // `lea eax,[esp+0x20]`), both immediately before the call.

// blam-cc: EDX -> buffer, stack -> context, length
uint32_t network_game_client_handle_map_data(uint8_t *context, uint8_t *buffer, int32_t length)
{
    uint32_t decoded_body[8];
    int16_t out_a;
    int32_t out_b_scratch; // UNSURE: Ghidra reuses &param_1 (the context pointer's own stack
                            // slot) as this decode's `out_b`; modeled as a separate local
                            // instead of aliasing the parameter, which is behaviourally
                            // equivalent since `context` is never re-read after this call.

    (void)length; // only ever computed as length-2 and never used further in the decompile

    if (*(int16_t *)context == 1) {
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, (int16_t *)&out_b_scratch, 5) != 0 &&
            context[0x9f8] == 0) {
            // blam-cc: EAX -> decoded_body (0x4e27d8 `lea eax,[esp+0x8]`). The 8-dword copy right
            // below confirms decoded_body is a network_player_entry (0x20 bytes).
            if (network_player_entry_validate((network_player_entry *)decoded_body) != 0) {
                memcpy(context + 0x9d8, decoded_body, sizeof(decoded_body));
                context[0x9f8] = 1;
                return 1;
            }
        }
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
