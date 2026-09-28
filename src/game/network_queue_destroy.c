// network_queue_destroy  (Ghidra: network_queue_destroy, already named)
// address 0x47a090, size 35 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/game.h circular_queue (records 0x08, storage 0x14); mirrors the allocation
//   pattern of the sibling position_update_queue_create / vehicle_update_queue_create /
//   player_update_queue_create constructors in this batch, freeing the same two GlobalAlloc
//   blocks they create.
// register convention: the queue to tear down in ESI (unaff_ESI); no stack parameters.
//   // blam-cc: ESI -> queue

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern void *__stdcall GlobalFree(void *block); // Win32

// blam-cc: ESI -> queue
void network_queue_destroy(circular_queue *queue)
{
    GlobalFree(queue->records);
    queue->records = 0;
    GlobalFree(queue->storage);
    queue->storage = 0;
}

#if 0
Original Ghidra decompilation (0x47a090), from tools/pack.py 0x47a090:

void network_queue_destroy(void)

{
  int unaff_ESI;

  GlobalFree(*(HGLOBAL *)(unaff_ESI + 8));
  *(undefined4 *)(unaff_ESI + 8) = 0;
  GlobalFree(*(HGLOBAL *)(unaff_ESI + 0x14));
  *(undefined4 *)(unaff_ESI + 0x14) = 0;
  return;
}
#endif
