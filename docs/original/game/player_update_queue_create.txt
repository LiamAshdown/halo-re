// player_update_queue_create  (Ghidra: FUN_00479f40; named per out/phase4/game_functions.md:
// "Allocates and initializes a 120-entry, 44-byte-record circular event queue.")
// address 0x479f40, size 108 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// evidence: types/game.h player_update_queue (a circular_queue of 120 records of 0x2c, plus a
//   has_current byte at +0x18 this constructor clears) and player::update_history (player+0x120).
//   Identical shape to the sibling position_update_queue_create/vehicle_update_queue_create in
//   this batch, at 120 x 0x2c = 0x1e0 records-table bytes / 120 x 0x2c = 0x14a0 storage bytes,
//   plus the extra has_current byte this larger form owns.
// register convention: the queue to initialize in ESI (unaff_ESI); no stack parameters.
//   // blam-cc: ESI -> queue

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"


// blam-cc: ESI -> queue
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void player_update_queue_create(player_update_queue *queue)
{
    uint32_t *storage;
    uint8_t *record_cursor;
    int32_t i;

    storage = (uint32_t *)GlobalAlloc(0, 0x14a0);
    queue->queue.storage = storage;
    for (i = 0; i < 0x528; i++) {
        storage[i] = 0;
    }

    record_cursor = (uint8_t *)queue->queue.storage;
    queue->queue.records = (void **)GlobalAlloc(0, 0x1e0);
    ((uint32_t *)queue->queue.records)[0] = 0;
    queue->queue.capacity = 0x78;
    queue->queue.record_size = 0x2c;
    queue->queue.read_index = 0;
    queue->queue.write_index = 0;

    i = 0;
    do {
        ((uint32_t *)queue->queue.records)[i / 4] = (uint32_t)record_cursor;
        i = i + 4;
        record_cursor = record_cursor + 0x2c;
    } while (i < 0x1e0);

    queue->has_current = 0;
}

#if 0
Original Ghidra decompilation (0x479f40), from tools/pack.py 0x479f40:

void FUN_00479f40(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_ESI;

  puVar1 = GlobalAlloc(0,0x14a0);
  unaff_ESI[5] = puVar1;
  for (iVar3 = 0x528; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar1 = 0;
    puVar1 = puVar1 + 1;
  }
  iVar3 = unaff_ESI[5];
  puVar1 = GlobalAlloc(0,0x1e0);
  unaff_ESI[2] = puVar1;
  *puVar1 = 0;
  *unaff_ESI = 0x78;
  unaff_ESI[1] = 0x2c;
  unaff_ESI[4] = 0;
  unaff_ESI[3] = 0;
  iVar2 = 0;
  do {
    *(int *)(iVar2 + unaff_ESI[2]) = iVar3;
    iVar2 = iVar2 + 4;
    iVar3 = iVar3 + 0x2c;
  } while (iVar2 < 0x1e0);
  *(undefined1 *)(unaff_ESI + 6) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
