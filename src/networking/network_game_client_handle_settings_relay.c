// network_game_client_handle_settings_relay  (Ghidra: FUN_004e2810; named per this rewrite)
// address 0x4e2810, size 88 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1c, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x1d: FUN_004e2810();` (case 0x1c instead reaches FUN_004e2790, the map-data latch) --
// the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
// summary. Forwards to network_game_settings_broadcast_send, the same forward the server-side
// FUN_004e24d0 handler makes, but gated on role == 1 (client) and decode class 5 instead of 3.
// register convention: ESI = context (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> context, EDX -> buffer
// UNSURE: network_game_settings_broadcast_send is called here with zero visible arguments;
// matching Ghidra literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern uint32_t network_game_settings_broadcast_send(void); // 0x4df0e0, this module,
    // called here with no visible arguments (UNSURE, see header)

// blam-cc: ESI -> context, EDX -> buffer
uint32_t network_game_client_handle_settings_relay(uint8_t *context, uint8_t *buffer)
{
    uint8_t decoded_body[32];
    int16_t out_a, out_b;

    if (*(int16_t *)context == 1) {
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, &out_b, 5) != 0) {
            return network_game_settings_broadcast_send();
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2810), from tools/pack.py 0x4e2810:

undefined4 FUN_004e2810(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 1) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,5);
    if (cVar1 != '\0') {
      uVar2 = FUN_004df0e0();
      return uVar2;
    }
  }
  return 1;
}
#endif
