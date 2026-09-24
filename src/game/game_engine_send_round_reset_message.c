// game_engine_send_round_reset_message  (Ghidra: FUN_004682c0; named per this rewrite)
// address 0x4682c0, size 96 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Sends a small one-byte-payload network message
//   tagged with type 0x17"); message_delta_encode_message and its shared_hud_text_draw_state /
//   network_session_broadcast_to_flagged broadcast pair already established in
//   src/game/game_engine_dispatch_item_pickup_event.c (same call shapes, this batch just uses
//   message type 0x17 with a one-byte payload fixed at 1).
// register convention: no parameters.
// UNSURE: the exact roles of message_delta_encode_message's unknown_0/unknown_2 and
//   network_session_broadcast_to_flagged's parameters are inherited unresolved from the sibling function.

#include "tags.h"

extern uint8_t shared_hud_text_draw_state; // 0x00871de0, UNSURE identity (see game_types_notes.md)

extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940
extern void network_session_broadcast_to_flagged(uint32_t unknown_0, void *unknown_1, uint32_t unknown_2, uint32_t unknown_3,
    uint32_t unknown_4, uint32_t unknown_5); // 0x4e1a80, not in this batch

// Encodes a one-byte (value 1) network message of type 0x17 and, if the encoder reports it was
// actually written (return > 0), broadcasts it via network_session_broadcast_to_flagged.
void game_engine_send_round_reset_message(void)
{
    uint8_t payload_value = 1;
    uint8_t *payload = &payload_value;
    int32_t encoded_bits;

    encoded_bits = message_delta_encode_message(0, 0x17, 0, (void **)&payload, 0, 1, 0);
    if (encoded_bits > 0) {
        network_session_broadcast_to_flagged(1, &shared_hud_text_draw_state, 1, 0, 0, 3);
    }
}

#if 0
Original Ghidra decompilation (0x4682c0), from tools/pack.py 0x4682c0:

void FUN_004682c0(void)

{
  int iVar1;
  undefined1 local_9;
  undefined1 *local_8;
  undefined4 local_4;

  local_8 = &local_9;
  local_9 = 1;
  local_4 = 0;
  iVar1 = message_delta_encode_message(0,0x17,0,&local_8,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  }
  return;
}
#endif
