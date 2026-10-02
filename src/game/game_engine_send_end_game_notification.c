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
// FIXED (register inputs, objdump): EAX carries a small integer value (read at 0x4671e0, `mov
//   DWORD PTR [esp+0x14],eax`) that becomes the message's only field, previously faked as a
//   literal 0. Call sites pass small constants (EAX=3 at 0x45ff17, EAX=2 at 0x4601bf, EAX=1 at
//   0x4691f0), consistent with an end-of-game reason code; named `reason` pending a better
//   name (UNSURE of its exact meaning).
//   // blam-cc: EAX -> reason

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
    // EDX -> destination size, then the seven stack arguments. Returns the encoded bit
    // length in EAX. `fields` is a pointer TO a pointer to the field block.
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server

// Encodes and broadcasts a network message of type 0x16 (an end-of-game notification, per this
// function's callers) carrying `reason`.
// blam-cc: EAX -> reason
void game_engine_send_end_game_notification(uint32_t reason)
{
    uint32_t payload;
    void *payload_ptr;
    int32_t encoded_size;

    payload = reason;
    payload_ptr = &payload;

    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x16, 0, &payload_ptr, 0, 1, 0);
    if (encoded_size > 0) {
        network_session_broadcast_to_flagged(encoded_size, network_server, 1, network_message_scratch, 1, 0, 0, 3);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
