// position_update_queue_find_and_remove  (Ghidra: FUN_0047a100; named per
// out/phase4/game_functions.md: "Finds and removes the queued position-update record matching a
// given network tick, discarding any stale entries in front of it.")
// address 0x47a100, size 150 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/halo_decompiled.c's caller (`FUN_0047a100((int *)(in_EAX + 0x170), *(undefined4
//   *)(unaff_EBX + 0x4bc), &local_c)`) passes player+0x170, i.e. player::position_updates
//   (types/game.h), confirming this operates on a circular_queue of position_update_record
//   (this batch's position_update_queue_push.c). The peeked record's dword[1] ("sequence") is
//   compared against a mod-0x40 tick distance to decide staleness, and on a match dwords[2..4]
//   (x, y, z) are copied out -- consistent with that record layout.
// register convention: none -- all three are genuine stack parameters (Ghidra's own
//   param_1/param_2/param_3, unchanged across the recursive tail call).
// UNSURE: the exact meaning of the 0x40 wraparound constant and of "sequence" (record dword[1])
//   as a staleness distance; preserved literally rather than reinterpreted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// position_update_record is types/game.h's (0x14, tick / sequence / position).

extern uint8_t circular_queue_pop(circular_queue *queue, void **out_record); // this batch, 0x47a200

// Peeks (without removing) the head of `queue`. If it matches `target_tick`, copies its x/y/z
// into `out` and removes it, returning 1. If it is older than `target_tick` by more than its
// own "sequence" distance (mod 0x40, wrapping), discards it (this branch DOES remove it) and
// retries. Otherwise (or once the queue is empty) returns 0 and leaves the queue untouched.
uint8_t position_update_queue_find_and_remove(circular_queue *queue, int32_t target_tick,
    real_point3d *out)
{
    position_update_record *record;

    if (queue->read_index == queue->write_index) {
        return 0;
    }
    record = (position_update_record *)queue->records[queue->read_index];

    if ((int32_t)record->tick == target_tick) {
        *out = record->position;
        // Mirrors the original's own redundant not-empty re-check before consuming the peeked
        // record (it cannot actually be empty here, since we just peeked a live record).
        if (queue->read_index != queue->write_index) {
            queue->read_index = (queue->read_index + 1) % queue->capacity;
            return 1;
        }
        return 0;
    }

    {
        int32_t distance;
        int32_t tick = (int32_t)record->tick;
        if (tick < target_tick) {
            distance = (tick - target_tick) + 0x40;
        } else if (target_tick < tick) {
            distance = tick - target_tick;
        } else {
            distance = 0;
        }

        if ((int32_t)record->sequence < distance) {
            void *discarded;
            circular_queue_pop(queue, &discarded);
            return position_update_queue_find_and_remove(queue, target_tick, out);
        }
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x47a100), from tools/pack.py 0x47a100:

uint FUN_0047a100(int *param_1,int param_2,int *param_3)

{
  int iVar1;
  uint3 uVar3;
  uint uVar2;
  undefined4 extraout_ECX;
  int *piVar4;

  iVar1 = param_1[4];
  uVar3 = (uint3)((uint)iVar1 >> 8);
  if (iVar1 == param_1[3]) {
    piVar4 = (int *)0x0;
    uVar2 = (uint)uVar3 << 8;
  }
  else {
    piVar4 = *(int **)(param_1[2] + iVar1 * 4);
    uVar2 = CONCAT31(uVar3,1);
  }
  if ((char)uVar2 == '\x01') {
    iVar1 = *piVar4;
    if (iVar1 == param_2) {
      *param_3 = piVar4[2];
      param_3[1] = piVar4[3];
      param_3[2] = piVar4[4];
      uVar2 = param_1[4];
      if (uVar2 != param_1[3]) {
        param_1[4] = (int)(uVar2 + 1) % *param_1;
        return CONCAT31((int3)((uint)((int)(uVar2 + 1) / *param_1) >> 8),1);
      }
      return uVar2 & 0xffffff00;
    }
    if (iVar1 < param_2) {
      uVar2 = (iVar1 - param_2) + 0x40;
    }
    else if (param_2 < iVar1) {
      uVar2 = iVar1 - param_2;
    }
    else {
      uVar2 = 0;
    }
    if (piVar4[1] < (int)uVar2) {
      circular_queue_pop();
      uVar2 = FUN_0047a100(extraout_ECX,param_2,param_3);
      return uVar2;
    }
    uVar2 = uVar2 & 0xffffff00;
  }
  return uVar2;
}
#endif
