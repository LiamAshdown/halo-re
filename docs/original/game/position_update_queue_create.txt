// position_update_queue_create  (Ghidra: position_update_queue_create, already named)
// address 0x47a020, size 99 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/game.h circular_queue (capacity 0x00, record_size 0x04, records 0x08,
//   write_index 0x0c, read_index 0x10, storage 0x14) and player::position_updates (player+0x170,
//   30 records of 0x14) -- this constructor's own literals (0x1e capacity, 0x14 record size,
//   600 = 30*20 storage bytes, 0x78 = 30*4 records-pointer-table bytes) match that field
//   exactly. GlobalAlloc/GlobalFree are the Win32 CRT wrappers already used by the sibling
//   vehicle_update_queue_create / network_queue_destroy in this batch.
// register convention: the queue to initialize in ESI (unaff_ESI); no stack parameters.
//   // blam-cc: ESI -> queue

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"


// blam-cc: ESI -> queue
// Allocates and zeroes 30 records of 0x14 bytes each (storage), allocates a 30-entry pointer
// table (records) and points each entry at its own record slot in storage, and resets the
// read/write cursors.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void position_update_queue_create(circular_queue *queue)
{
    uint32_t *storage;
    uint32_t *records;
    int32_t i;
    uint8_t *record_cursor;

    storage = (uint32_t *)GlobalAlloc(0, 600);
    queue->storage = storage;
    for (i = 0; i < 0x96; i++) {
        storage[i] = 0;
    }

    record_cursor = (uint8_t *)queue->storage;
    records = (uint32_t *)GlobalAlloc(0, 0x78);
    queue->records = (void **)records;
    records[0] = 0;
    queue->capacity = 0x1e;
    queue->record_size = 0x14;
    queue->read_index = 0;
    queue->write_index = 0;

    i = 0;
    do {
        ((uint32_t *)queue->records)[i / 4] = (uint32_t)record_cursor;
        i = i + 4;
        record_cursor = record_cursor + 0x14;
    } while (i < 0x78);
}

#if 0
Original Ghidra decompilation (0x47a020), from tools/pack.py 0x47a020:

void position_update_queue_create(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_ESI;

  puVar1 = GlobalAlloc(0,600);
  unaff_ESI[5] = puVar1;
  for (iVar3 = 0x96; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar1 = 0;
    puVar1 = puVar1 + 1;
  }
  iVar3 = unaff_ESI[5];
  puVar1 = GlobalAlloc(0,0x78);
  unaff_ESI[2] = puVar1;
  *puVar1 = 0;
  *unaff_ESI = 0x1e;
  unaff_ESI[1] = 0x14;
  unaff_ESI[4] = 0;
  unaff_ESI[3] = 0;
  iVar2 = 0;
  do {
    *(int *)(iVar2 + unaff_ESI[2]) = iVar3;
    iVar2 = iVar2 + 4;
    iVar3 = iVar3 + 0x14;
  } while (iVar2 < 0x78);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
