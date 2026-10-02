// vehicle_update_queue_create  (Ghidra: vehicle_update_queue_create, already named)
// address 0x47a250, size 99 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/game.h circular_queue and player::vehicle_updates (player+0x1d0, 30 records
//   of 0x48) -- this constructor's literals (0x1e capacity, 0x48 record size, 0x870 = 30*72
//   storage bytes) match exactly; identical shape to the sibling position_update_queue_create
//   in this batch, just with a larger record size.
// register convention: the queue to initialize in ESI (unaff_ESI); no stack parameters.
//   // blam-cc: ESI -> queue

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


// blam-cc: ESI -> queue
void vehicle_update_queue_create(circular_queue *queue)
{
    uint32_t *storage;
    uint8_t *record_cursor;
    int32_t i;

    storage = (uint32_t *)GlobalAlloc(0, 0x870);
    queue->storage = storage;
    for (i = 0; i < 0x21c; i++) {
        storage[i] = 0;
    }

    record_cursor = (uint8_t *)queue->storage;
    queue->records = (void **)GlobalAlloc(0, 0x78);
    ((uint32_t *)queue->records)[0] = 0;
    queue->capacity = 0x1e;
    queue->record_size = 0x48;
    queue->read_index = 0;
    queue->write_index = 0;

    i = 0;
    do {
        ((uint32_t *)queue->records)[i / 4] = (uint32_t)record_cursor;
        i = i + 4;
        record_cursor = record_cursor + 0x48;
    } while (i < 0x78);
}

#if 0
Original Ghidra decompilation (0x47a250), from tools/pack.py 0x47a250:

void vehicle_update_queue_create(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_ESI;

  puVar1 = GlobalAlloc(0,0x870);
  unaff_ESI[5] = puVar1;
  for (iVar3 = 0x21c; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar1 = 0;
    puVar1 = puVar1 + 1;
  }
  iVar3 = unaff_ESI[5];
  puVar1 = GlobalAlloc(0,0x78);
  unaff_ESI[2] = puVar1;
  *puVar1 = 0;
  *unaff_ESI = 0x1e;
  unaff_ESI[1] = 0x48;
  unaff_ESI[4] = 0;
  unaff_ESI[3] = 0;
  iVar2 = 0;
  do {
    *(int *)(iVar2 + unaff_ESI[2]) = iVar3;
    iVar2 = iVar2 + 4;
    iVar3 = iVar3 + 0x48;
  } while (iVar2 < 0x78);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
