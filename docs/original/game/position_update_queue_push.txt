// position_update_queue_push  (Ghidra: FUN_0047a0c0; named per out/phase4/game_functions.md:
// "Reorders and pushes a position-update record (tick key plus payload) onto a position-update
// queue.")
// address 0x47a0c0, size 56 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/halo_decompiled.c's one caller (the "Received pos update [%d]..." network
//   handler around line 151101) calls this as
//   `FUN_0047a0c0(param_3,param_4,param_5,param_1,iVar4)` where the outer function's param_1 is
//   logged as the update tick and param_3/4/5 as a position; matched against the fixed 5-dword
//   reordering this function performs, that gives the on-the-wire record layout
//   {tick, sequence, x, y, z}. types/game.h circular_queue::record_size == 0x14 (this batch's
//   position_update_queue_create) confirms the record is exactly 5 dwords.
// register convention: none recovered by Ghidra as a register argument, but the destination
//   queue is never loaded in this function's own body -- it relies on EBX already holding it,
//   exactly like circular_queue_push (this batch), to which it tail-forwards.
//   // blam-cc: EBX -> queue, stack -> tick, sequence, x, y, z
// UNSURE: field 2 ("sequence") is the outer caller's own `FUN_004e6aa0()` result, not otherwise
//   identified; x/y/z are presumed position floats carried as raw 32-bit patterns (this
//   function never interprets them, only reorders and copies them).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// position_update_record is types/game.h's (0x14, tick / sequence / position).

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t circular_queue_push(circular_queue *queue, void *source); // this batch, 0x47a1a0

// blam-cc: EBX -> queue, stack -> tick, sequence, x, y, z
uint8_t position_update_queue_push(circular_queue *queue, real x, real y, real z,
    int32_t tick, int32_t sequence)
{
    position_update_record record;
    record.tick = tick;
    record.sequence = sequence;
    record.position.x = x;
    record.position.y = y;
    record.position.z = z;
    return circular_queue_push(queue, &record);
}

#if 0
Original Ghidra decompilation (0x47a0c0), from tools/pack.py 0x47a0c0:

void FUN_0047a0c0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5)

{
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = param_3;
  local_c = param_1;
  local_8 = param_2;
  local_10 = param_5;
  local_14 = param_4;
  circular_queue_push(&local_14);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
