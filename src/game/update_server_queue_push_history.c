// update_server_queue_push_history  (Ghidra: FUN_00473390; renamed, no established name)
// address 0x473390, size 160 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Pushes an update entry into a specific player's
// server-side history ring buffer if space is available"); types/game.h update_server_queue
// (queue +0x28), player_update_queue -> circular_queue (capacity, record_size, records,
// write_index, read_index, storage); machine_to_player (0x006b1460).
// register convention: machine index in AX, tick count in EDX, source and extra on the stack.
//   // blam-cc: EAX -> machine_index, EDX -> tick_count, stack -> source, extra
// FIXED (verified against 0x473390..0x47342f): the record is extra (+0), the tick count twice (+4, +8,
//   from EDX, which the draft dropped) and the 8 source dwords (+0xc). The fullness test was inverted:
//   the used count is write - read when write > read, capacity - read + write when write < read, else
//   0, and the record is pushed while used < capacity - 1. EAX's final value (not a result) is unused
//   by both callers (0x473353, 0x4e0028), so this returns nothing.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index machine_to_player[16]; // 0x006b1460
extern data_array *update_server_queues;   // 0x006f1d90

void update_server_queue_push_history(int16_t machine_index, int32_t tick_count, uint32_t *source, uint32_t extra)
{
    datum_index player = machine_to_player[(uint16_t)machine_index];
    update_server_queue *entry;
    circular_queue *q;
    uint32_t record[11];
    int32_t used;
    int32_t i;

    if (player == k_datum_index_none) {
        return;
    }
    entry = (update_server_queue *)((uint8_t *)update_server_queues->data + (player & 0xffff) * 0x64);
    q = &entry->queue.queue;
    for (i = 0; i < 8; i++) {
        record[3 + i] = source[i];
    }
    record[0] = extra;
    record[1] = (uint32_t)tick_count;
    record[2] = (uint32_t)tick_count;
    if (q->write_index > q->read_index) {
        used = q->write_index - q->read_index;
    } else if (q->write_index < q->read_index) {
        used = q->capacity - q->read_index + q->write_index;
    } else {
        used = 0;
    }
    if (used < q->capacity - 1) {
        uint8_t *destination = (uint8_t *)q->records[q->write_index];
        uint8_t *from = (uint8_t *)record;

        for (i = 0; i < q->record_size; i++) {
            destination[i] = from[i];
        }
        q->write_index = (q->write_index + 1) % q->capacity;
    }
}

#if 0
Original Ghidra decompilation (0x473390), from tools/pack.py 0x473390:

uint FUN_00473390(undefined4 *param_1,undefined4 param_2)

{
  int *piVar1;
  int iVar2;
  uint in_EAX;
  uint uVar3;
  int iVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 local_2c [11];

  uVar3 = (&DAT_006b1460)[in_EAX & 0xffff];
  if (uVar3 != 0xffffffff) {
    piVar1 = (int *)((uVar3 & 0xffff) * 100 + 0x28 + *(int *)(DAT_006f1d90 + 0x34));
    puVar6 = local_2c + 3;
    for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *param_1;
      param_1 = param_1 + 1;
      puVar6 = puVar6 + 1;
    }
    iVar4 = piVar1[4];
    iVar2 = piVar1[3];
    local_2c[0] = param_2;
    if (iVar4 < iVar2) {
      uVar3 = iVar2 - iVar4;
    }
    else if (iVar2 < iVar4) {
      uVar3 = (*piVar1 - iVar4) + iVar2;
    }
    else {
      uVar3 = 0;
    }
    if ((int)uVar3 < *piVar1 + -1) {
      uVar3 = piVar1[1];
      puVar6 = local_2c;
      puVar7 = *(undefined4 **)(piVar1[2] + iVar2 * 4);
      for (uVar5 = uVar3 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar7 = *puVar6;
        puVar6 = puVar6 + 1;
        puVar7 = puVar7 + 1;
      }
      for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
        *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
        puVar6 = (undefined4 *)((int)puVar6 + 1);
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      uVar3 = (piVar1[3] + 1) / *piVar1;
      piVar1[3] = (piVar1[3] + 1) % *piVar1;
    }
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
