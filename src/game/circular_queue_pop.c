// circular_queue_pop  (Ghidra: circular_queue_pop, already named)
// address 0x47a200, size 45 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/game.h circular_queue (record_size 0x04, records 0x08, write_index 0x0c,
//   read_index 0x10); sibling circular_queue_push/circular_queue_count in this batch confirm the
//   field mapping. Note this returns the STORAGE POINTER at the read cursor, not a copy of the
//   record's bytes -- callers (e.g. this batch's position_update_queue_find_and_remove) then
//   dereference that pointer themselves.
// register convention: the queue in ECX (in_ECX), an out-parameter for the popped record
//   pointer in EDX (in_EDX).
//   // blam-cc: ECX -> queue, EDX -> out_record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: ECX -> queue, EDX -> out_record
// If the queue is non-empty, writes the storage pointer at read_index into *out_record,
// advances read_index, and returns 1; otherwise writes NULL and returns 0.
uint8_t circular_queue_pop(circular_queue *queue, void **out_record)
{
    int32_t read_index = queue->read_index;
    if (read_index != queue->write_index) {
        *out_record = queue->records[read_index];
        queue->read_index = (read_index + 1) % queue->capacity;
        return 1;
    }
    *out_record = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x47a200), from tools/pack.py 0x47a200:

uint circular_queue_pop(void)

{
  uint uVar1;
  int iVar2;
  int *in_ECX;
  undefined4 *in_EDX;

  uVar1 = in_ECX[4];
  if (uVar1 != in_ECX[3]) {
    *in_EDX = *(undefined4 *)(in_ECX[2] + uVar1 * 4);
    iVar2 = in_ECX[4];
    in_ECX[4] = (iVar2 + 1) % *in_ECX;
    return CONCAT31((int3)((uint)((iVar2 + 1) / *in_ECX) >> 8),1);
  }
  *in_EDX = 0;
  return uVar1 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
