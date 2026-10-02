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
#include "math.h"
#include "memory.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t shared_hud_text_draw_state; // 0x00871de0, UNSURE identity (see game_types_notes.md)

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server

// Encodes a one-byte (value 1) network message of type 0x17 and, if the encoder reports it was
// actually written (return > 0), broadcasts it via network_session_broadcast_to_flagged.
void game_engine_send_round_reset_message(void)
{
    uint8_t payload_value = 1;
    uint8_t *payload = &payload_value;
    int32_t encoded_bits;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x17, 0, (void **)&payload, 0, 1, 0);
    if (encoded_bits > 0) {
        network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
