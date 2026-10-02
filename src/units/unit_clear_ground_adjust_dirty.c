// unit_clear_ground_adjust_dirty  (Ghidra: unit_clear_ground_adjust_dirty, renamed; paired with
// unit_reset_ground_adjust_state per out/phase4/units_functions.md's own description)
// address 0x55ad70, size 86 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: mirrors unit_reset_ground_adjust_state (0x55ad00) exactly, clearing the same
//   object.flags 0x800000 / biped_data.flags 0x20 pair it sets.
// FIXED (register inputs, objdump): EAX (read at 0x55ad79, `and eax,0xffff`) was already a C
//   parameter (object_index) but had no machine-checked "blam-cc" line at all; added.
//   // blam-cc: EAX -> object_index

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

// Once the Biped tag's "requires ground adjust" flag (biped_flags bit 0x200, UNSURE) is clear
// and the ground-adjust dirty bit is still set, clears it along with its object.flags mirror.
// blam-cc: EAX -> object_index
void unit_clear_ground_adjust_dirty(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((tag->biped_flags & 0x200) != 0 && (biped->flags & 0x20) != 0) {
        obj->flags &= ~0x800000u;
        biped->flags &= ~0x20u;
    }
}

#if 0
Original Ghidra decompilation (0x55ad70):

void FUN_0055ad70(void)

{
  uint *puVar1;
  uint in_EAX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 0x200) != 0)
     && ((puVar1[0x133] & 0x20) != 0)) {
    puVar1[4] = puVar1[4] & 0xff7fffff;
    puVar1[0x133] = puVar1[0x133] & 0xffffffdf;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
