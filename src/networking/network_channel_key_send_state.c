// network_channel_key_send_state  (Ghidra: FUN_004de950; named per this rewrite)
// address 0x4de950, size 148 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Builds and sends a large state packet (via
// FUN_004ec590/network_game_settings_packet_receive) when the connection is mid-game as host or client, otherwise
// delegates to FUN_004ec670." client->state (state 2 or 3) matches this cluster's established
// field.
// UNSURE: `entry` (in_EDX, a pointer-to-pointer) and the ~940-byte zeroed scratch buffer passed
// to FUN_004ec590/network_game_settings_packet_receive are not independently identified; declared generically.
// register convention: client in ESI (unaff_ESI), entry in EDX (in_EDX). blam-cc: EDX -> entry,
// ESI -> client

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern int16_t network_game_mode; // 0x00719720

extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670
extern char network_game_settings_packet_receive(void *scratch); // 0x4d9800, outside this batch, elided args

// blam-cc: EDX -> entry, ESI -> client
int32_t network_channel_key_send_state(network_client_globals *client, int32_t **entry)
{
    uint8_t scratch[0x3ba];

    if (network_game_mode == 2 || (client->state != 2 && client->state != 3)) {
        message_delta_decode_compound_field_staged(entry); // blam-cc: EAX -> entry (0x4de9c5 `mov eax,edx`)
        return 0;
    }
    if (**entry == 0) {
        memset(scratch, 0, sizeof(scratch));
        if (message_delta_decode_compound_field(entry, scratch) != 0) { // blam-cc: EAX -> entry, ECX -> scratch
            if (network_game_settings_packet_receive(scratch) != 0) {
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de950):

undefined4 FUN_004de950(void)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *in_EDX;
  int unaff_ESI;
  undefined4 *puVar4;
  undefined2 local_3b8;
  undefined4 local_3b6 [236];

  if ((DAT_00719720 == 2) ||
     ((*(short *)(unaff_ESI + 0xeda) != 2 && (*(short *)(unaff_ESI + 0xeda) != 3)))) {
    uVar2 = FUN_004ec670();
    return uVar2;
  }
  if (*(int *)*in_EDX == 0) {
    local_3b8 = 0;
    puVar4 = local_3b6;
    for (iVar3 = 0xeb; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *(undefined2 *)puVar4 = 0;
    cVar1 = FUN_004ec590();
    if (cVar1 != '\0') {
      cVar1 = FUN_004d9800(&local_3b8);
      if (cVar1 != '\0') {
        return 1;
      }
      return 0;
    }
  }
  return 0;
}
#endif
