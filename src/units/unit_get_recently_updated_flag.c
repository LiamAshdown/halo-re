// unit_get_recently_updated_flag  (Ghidra: FUN_00570c80; renamed from the phase2 proposal)
// address 0x570c80, size 34 bytes
// name confidence: 0.25 (phase2 proposal at 0.25, matches functions.md summary)
// rewrite confidence: 0.5
// evidence: types/objects.h object.flags (0x010, bit 0x20 = extension_of_parent).
// register convention: unit object index in EAX (in_EAX).
//   // blam-cc: EAX -> object_index
// UNSURE: the return value is byte-only (AL), with the upper 24 bits left over from an
//   unrelated register per Ghidra's CONCAT31; declared here as a clean uint8_t.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

// Tests whether object.flags bit 0x20 (extension_of_parent) is set on the unit; used by
// vehicle_update alongside a distance check to decide whether to force a position resync.
uint8_t unit_get_recently_updated_flag(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    return (obj->flags & 0x20) == 0x20;
}

#if 0
Original Ghidra decompilation (0x570c80):

undefined4 FUN_00570c80(void)

{
  uint in_EAX;

  return CONCAT31((int3)((in_EAX & 0xffff) * 3 >> 8),
                  (*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) +
                            0x10) & 0x20) == 0x20);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
