// circular_buffer_new  (Ghidra: circular_buffer_new, already named)
// address 0x4d0170, size 65 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/memory_types_notes.md "circular_buffer (0x18 + capacity)"; every field of
// types/memory.h circular_buffer is written here (name, 'circ' signature, capacity, data), and
// the GlobalAlloc size (requested_size + 0x19) matches "0x18 header + capacity bytes" exactly.
// register convention: name pointer as the recognized parameter (param_1), requested size in EAX
// (in_EAX).
// UNSURE: Ghidra types this function void, with no explicit return of the allocated pointer,
// unlike its sibling constructor data_new (0x4d0370) which does return char *. Since GlobalAlloc's
// result naturally sits in EAX right up to the final stores, callers may still be relying on the
// EAX-return convention despite the void prototype; preserved as void here for fidelity to the
// decompile.

#include "win32.h"
#include "tags.h"
#include "memory.h"


// blam-cc: name as the recognized parameter, requested size in EAX
// Allocates and initializes a new circular_buffer with capacity requested_size + 1 (one slot is
// always kept empty so the read/write cursors can be told apart). `name` is stored as-is, never
// copied. Does nothing observable if the allocation fails.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void circular_buffer_new(char *name, int32_t requested_size)
{
    circular_buffer *buf;

    buf = (circular_buffer *)GlobalAlloc(0, requested_size + 0x19);
    if (buf != 0) {
        buf->name = 0;
        buf->signature = 0;
        buf->read_cursor = 0;
        buf->write_cursor = 0;
        buf->capacity = 0;
        buf->data = 0;
        buf->name = name;
        buf->signature = k_circular_buffer_signature;
        buf->capacity = requested_size + 1;
        buf->data = (uint8_t *)buf + 0x18;
    }
}

#if 0
Original Ghidra decompilation (0x4d0170):

void circular_buffer_new(undefined4 param_1)

{
  int in_EAX;
  undefined4 *puVar1;

  puVar1 = GlobalAlloc(0,in_EAX + 0x19);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = 0;
    puVar1[1] = 0;
    puVar1[2] = 0;
    puVar1[3] = 0;
    puVar1[4] = 0;
    puVar1[5] = 0;
    *puVar1 = param_1;
    puVar1[1] = 0x63697263;
    puVar1[4] = in_EAX + 1;
    puVar1[5] = puVar1 + 6;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
