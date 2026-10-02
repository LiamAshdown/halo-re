// circular_queue_push  (Ghidra: circular_queue_push, already named)
// address 0x47a1a0, size 82 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/game.h circular_queue (capacity 0x00, record_size 0x04, records 0x08,
//   write_index 0x0c, read_index 0x10); the already-rewritten update_server_queue_push_history.c
//   (0x473390) inlines this exact "used slots" and modulo-advance arithmetic against a
//   player_update_queue's embedded circular_queue, confirming the field mapping.
// register convention: the queue in EBX (unaff_EBX); the source record pointer is this
//   function's own recognized stack parameter.
//   // blam-cc: EBX -> queue, stack -> source

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EBX -> queue, stack -> source
// Copies record_size bytes from `source` into the queue's next free slot and advances
// write_index, unless the queue is full (used >= capacity - 1). Returns 1 on success, 0 if full.
uint8_t circular_queue_push(circular_queue *queue, void *source)
{
    int32_t write_index = queue->write_index;
    int32_t read_index = queue->read_index;
    int32_t used;
    int32_t i;
    uint8_t *dst;
    uint8_t *src;

    if (read_index < write_index) {
        used = write_index - read_index;
    } else if (write_index < read_index) {
        used = (queue->capacity - read_index) + write_index;
    } else {
        used = 0;
    }

    if (used < queue->capacity - 1) {
        dst = (uint8_t *)queue->records[write_index];
        src = (uint8_t *)source;
        for (i = 0; i < queue->record_size; i++) {
            dst[i] = src[i];
        }
        queue->write_index = (write_index + 1) % queue->capacity;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x47a1a0), from tools/pack.py 0x47a1a0:

uint circular_queue_push(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int *unaff_EBX;
  undefined4 *puVar5;

  iVar1 = unaff_EBX[3];
  iVar2 = unaff_EBX[4];
  if (iVar2 < iVar1) {
    uVar3 = iVar1 - iVar2;
  }
  else if (iVar1 < iVar2) {
    uVar3 = (*unaff_EBX - iVar2) + iVar1;
  }
  else {
    uVar3 = 0;
  }
  if ((int)uVar3 < *unaff_EBX + -1) {
    uVar3 = unaff_EBX[1];
    puVar5 = *(undefined4 **)(unaff_EBX[2] + iVar1 * 4);
    for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *puVar5 = *param_1;
      param_1 = param_1 + 1;
      puVar5 = puVar5 + 1;
    }
    for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(undefined1 *)puVar5 = *(undefined1 *)param_1;
      param_1 = (undefined4 *)((int)param_1 + 1);
      puVar5 = (undefined4 *)((int)puVar5 + 1);
    }
    iVar1 = unaff_EBX[3];
    unaff_EBX[3] = (iVar1 + 1) % *unaff_EBX;
    return CONCAT31((int3)((uint)((iVar1 + 1) / *unaff_EBX) >> 8),1);
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
