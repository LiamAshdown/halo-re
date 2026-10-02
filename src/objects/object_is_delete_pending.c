// object_is_delete_pending
// address 0x4f5c10, size 27 bytes
// name confidence: 0.7 (still FUN_004f5c10 in Ghidra; types/objects.h's
//   _object_header_delete_pending_bit comment names this function directly: "object_is_delete_
//   pending reads it")
// rewrite confidence: 0.85
// evidence: types/objects.h object_header (flags at 0x02); the CONCAT31/shift/mask dance in the
//   original is Ghidra's rendering of a plain bit test with the result placed in AL; the mask
//   and shift used (>>3, &1) is exactly _object_header_delete_pending_bit (0x08).
// register convention: object index in EAX (in_EAX).

// RETURN TYPE (phase-4 review pass): Ghidra returns this as CONCAT31(garbage, AL) / bool,
//   i.e. only the low byte is defined -- the top three bytes are whatever happened to be in
//   the register. The return type is uint8_t so no caller can depend on the garbage, which
//   is the same correction already recorded for object_type_definitions_query_0x44 0x4f41d0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

uint8_t object_is_delete_pending(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    return (header->flags & _object_header_delete_pending_bit) != 0;
}

#if 0
Original Ghidra decompilation (0x4f5c10):

uint FUN_004f5c10(void)

{
  uint in_EAX;

  return CONCAT31((int3)((in_EAX & 0xffff) * 3 >> 8),
                  *(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + (in_EAX & 0xffff) * 0xc) >> 3) &
         0xffffff01;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
