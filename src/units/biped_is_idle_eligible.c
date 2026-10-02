// biped_is_idle_eligible  (Ghidra: biped_is_idle_eligible, renamed)
// address 0x55e8e0, size 82 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: biped_data.unknown_501 (types/units.h), object.vitality_flags bit 4 (objects.h),
//   Biped.biped_flags bit 4 (types/tags.h).
// register convention: object index in EAX (in_EAX).
//   // blam-cc: EAX -> object_index
// FIXED (register inputs, objdump): EAX carries object_index (read at 0x55e8e9, and eax,0xffff);
// this file had no "// blam-cc:" note at all, so the checker saw no register mapping.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Returns whether a biped has been in its current grounded state long enough (more than 3
// ticks), and is either unattached to a parent or the Biped tag's bit 4 is clear, to be eligible
// for idle behaviors.
uint32_t biped_is_idle_eligible(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    return (int8_t)biped->airborne_ticks > 3 &&
           ((tag->biped_flags & 4) == 0 || (obj->vitality_flags & 4) != 0);
}

#if 0
Original Ghidra decompilation (0x55e8e0):

undefined4 FUN_0055e8e0(void)

{
  uint *puVar1;
  uint in_EAX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (('\x03' < *(char *)((int)puVar1 + 0x501)) &&
     (((*(byte *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 4) == 0 ||
      ((*(byte *)((int)puVar1 + 0x106) & 4) != 0)))) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
