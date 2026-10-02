// circular_queue_count  (Ghidra: circular_queue_count, already named)
// address 0x47a230, size 23 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/game.h circular_queue (capacity 0x00, write_index 0x0c, read_index 0x10);
//   matches the "used slots" arithmetic already established in circular_queue_push (this batch)
//   and update_server_queue_push_history.c.
// register convention: the queue in EDX (in_EDX); no stack parameters.
//   // blam-cc: EDX -> queue

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EDX -> queue
int32_t circular_queue_count(circular_queue *queue)
{
    int32_t write_index = queue->write_index;
    int32_t read_index = queue->read_index;
    if (read_index < write_index) {
        return write_index - read_index;
    }
    if (write_index < read_index) {
        return (write_index - read_index) + queue->capacity;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x47a230), from tools/pack.py 0x47a230:

int circular_queue_count(void)

{
  int iVar1;
  int iVar2;
  int *in_EDX;

  iVar1 = in_EDX[3];
  iVar2 = in_EDX[4];
  if (iVar2 < iVar1) {
    return iVar1 - iVar2;
  }
  if (iVar1 < iVar2) {
    return (iVar1 - iVar2) + *in_EDX;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
