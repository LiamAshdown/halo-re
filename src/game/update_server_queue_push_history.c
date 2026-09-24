// update_server_queue_push_history  (Ghidra: FUN_00473390; renamed, no established name)
// address 0x473390, size 160 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Pushes an update entry into a specific player's
// server-side history ring buffer if space is available"); types/game.h update_server_queue
// (queue +0x28), player_update_queue -> circular_queue (capacity, record_size, records,
// write_index, read_index, storage); machine_to_player (0x006b1460).
// register convention: a machine index in EAX (Ghidra's `in_EAX`).
//   // blam-cc: EAX -> machine_index, stack -> source, extra
// Assembles an 11-dword record (`extra` followed by 8 dwords read from `source`, matching
// k_player_update_history_record_size == 0x2c) and pushes it into that player's
// update_server_queue's player_update_queue.queue circular buffer if it is not full.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern datum_index machine_to_player[16]; // 0x006b1460
extern data_array *update_server_queues;   // 0x006f1d90

// blam-cc: EAX -> machine_index, stack -> source, extra
uint32_t update_server_queue_push_history(int32_t machine_index, uint32_t *source, uint32_t extra)
{
    datum_index player = machine_to_player[(uint16_t)machine_index];
    uint32_t used = 0;

    if (player != k_datum_index_none) {
        update_server_queue *entry = (update_server_queue *)((uint8_t *)update_server_queues->data +
            (uint32_t)(uint16_t)player * update_server_queues->size);
        circular_queue *q = &entry->queue.queue;
        uint32_t record[11];
        int32_t i;
        int32_t write_index = q->write_index;
        int32_t read_index = q->read_index;

        record[0] = extra;
        for (i = 0; i < 8; i++) {
            record[3 + i] = source[i];
        }

        if (write_index < read_index) {
            used = read_index - write_index;
        } else if (read_index < write_index) {
            used = (q->capacity - write_index) + read_index;
        } else {
            used = 0;
        }

        if ((int32_t)used < q->capacity - 1) {
            uint8_t *dst = (uint8_t *)((uint8_t **)q->records)[write_index];
            uint8_t *src = (uint8_t *)record;
            int32_t record_size = q->record_size;

            for (i = 0; i < record_size; i++) {
                dst[i] = src[i];
            }
            used = (q->write_index + 1) % q->capacity;
            q->write_index = (q->write_index + 1) % q->capacity;
        }
    }
    return used;
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
