// vehicle_update_queue_find_and_remove  (Ghidra: FUN_0047a2c0; named per
// out/phase4/game_functions.md: "Finds and removes the queued vehicle-update record matching a
// given network tick, discarding stale entries ahead of it.")
// address 0x47a2c0, size 145 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/halo_decompiled.c's one caller (`FUN_0047a2c0((int *)(in_EAX + 0x1d0), *(...)
//   (unaff_EBX + 0x4bc))`) passes player+0x1d0, i.e. player::vehicle_updates (types/game.h);
//   the 0x12-dword (18 dwords == 0x48 bytes) whole-record copy on a match matches
//   vehicle_update_queue_create's own record_size exactly (this batch). Structurally identical
//   to the sibling position_update_queue_find_and_remove.c, differing only in record size and
//   in copying the WHOLE record (including its own tick/sequence header) rather than skipping
//   past it.
// register convention: none -- all three are genuine stack parameters (Ghidra's own
//   param_1/param_2/param_3, unchanged across the recursive tail call).
// UNSURE: the record body past dword[1] is entirely unidentified (raw vehicle-state payload).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// TYPES-GAP: no header names this record; declared locally, body left opaque.
// vehicle_update_record is types/game.h's (0x48, tick / sequence / vehicle_update_body).

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t circular_queue_pop(circular_queue *queue, void **out_record); // this batch, 0x47a200

// Peeks (without removing) the head of `queue`. If it matches `target_tick`, copies the whole
// 18-dword record into `out` and removes it, returning 1. If it is older than `target_tick` by
// more than its own "sequence" distance (mod 0x40, wrapping), discards it and retries.
// Otherwise (or once the queue is empty) returns 0 and leaves the queue untouched.
uint8_t vehicle_update_queue_find_and_remove(circular_queue *queue, int32_t target_tick,
    vehicle_update_record *out)
{
    vehicle_update_record *record;

    if (queue->read_index == queue->write_index) {
        return 0;
    }
    record = (vehicle_update_record *)queue->records[queue->read_index];

    if ((int32_t)record->tick == target_tick) {
        uint32_t *src = (uint32_t *)record;
        uint32_t *dst = (uint32_t *)out;
        int32_t i;
        for (i = 0; i < 0x12; i++) {      // the whole 0x48 record, header dwords included
            dst[i] = src[i];
        }
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
            return vehicle_update_queue_find_and_remove(queue, target_tick, out);
        }
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x47a2c0), from tools/pack.py 0x47a2c0:

uint FUN_0047a2c0(int *param_1,int param_2,int *param_3)

{
  uint3 uVar2;
  uint uVar1;
  int iVar3;
  int *piVar4;

  iVar3 = param_1[4];
  uVar2 = (uint3)((uint)iVar3 >> 8);
  if (iVar3 == param_1[3]) {
    piVar4 = (int *)0x0;
    uVar1 = (uint)uVar2 << 8;
  }
  else {
    piVar4 = *(int **)(param_1[2] + iVar3 * 4);
    uVar1 = CONCAT31(uVar2,1);
  }
  if ((char)uVar1 == '\x01') {
    iVar3 = *piVar4;
    if (iVar3 == param_2) {
      for (iVar3 = 0x12; iVar3 != 0; iVar3 = iVar3 + -1) {
        *param_3 = *piVar4;
        piVar4 = piVar4 + 1;
        param_3 = param_3 + 1;
      }
      uVar1 = param_1[4];
      if (uVar1 != param_1[3]) {
        param_1[4] = (int)(uVar1 + 1) % *param_1;
        return CONCAT31((int3)((uint)((int)(uVar1 + 1) / *param_1) >> 8),1);
      }
      return uVar1 & 0xffffff00;
    }
    if (iVar3 < param_2) {
      uVar1 = (iVar3 - param_2) + 0x40;
    }
    else if (param_2 < iVar3) {
      uVar1 = iVar3 - param_2;
    }
    else {
      uVar1 = 0;
    }
    if (piVar4[1] < (int)uVar1) {
      circular_queue_pop();
      uVar1 = FUN_0047a2c0(param_1,param_2,param_3);
      return uVar1;
    }
    uVar1 = uVar1 & 0xffffff00;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
