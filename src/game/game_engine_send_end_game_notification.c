// game_engine_send_end_game_notification  (Ghidra: FUN_004671d0; named per
// out/phase4/game_functions.md: "Sends a small fixed-payload network message tagged with type
// 0x16.")
// address 0x4671d0, size 95 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: already referenced (as FUN_004671d0) by the committed game_engine_begin_end_game_
//   sequence.c, which calls it right after queuing the end-of-game announcer sound and closing
//   widgets -- i.e. this is the network notification counterpart of
//   game_engine_end_game_sequence_stage1. Same message_delta_encode_message /
//   network_session_broadcast_to_flagged / network_message_scratch (0x00871de0) trio as the sibling network-notify
//   functions in this batch.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0

extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940; blam-cc: EAX -> destination buffer,
    // EDX -> destination size, then the seven stack arguments. Returns the encoded bit
    // length in EAX. `fields` is a pointer TO a pointer to the field block.
extern void network_session_broadcast_to_flagged(uint32_t unknown_0, void *unknown_1, uint32_t unknown_2, uint32_t unknown_3,
    uint32_t unknown_4, uint32_t unknown_5); // 0x4e1a80, not in this batch (matches other callers)

// Encodes and broadcasts a fixed, empty-payload network message of type 0x16 (an end-of-game
// notification, per this function's callers).
void game_engine_send_end_game_notification(void)
{
    uint32_t empty_payload;
    void *payload_ptr;
    int32_t encoded_size;

    empty_payload = 0;
    payload_ptr = &empty_payload;

    encoded_size = message_delta_encode_message(0, 0x16, 0, &payload_ptr, 0, 1, 0);
    if (encoded_size > 0) {
        network_session_broadcast_to_flagged(1, network_message_scratch, 1, 0, 0, 3);
    }
}

#if 0
Original Ghidra decompilation (0x4671d0), from tools/pack.py 0x4671d0:

void FUN_004671d0(void)

{
  int iVar1;
  undefined1 local_c [4];
  undefined1 *local_8;
  undefined4 local_4;

  local_8 = local_c;
  local_4 = 0;
  iVar1 = message_delta_encode_message(0,0x16,0,&local_8,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  }
  return;
}
#endif
