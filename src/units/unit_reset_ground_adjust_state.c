// unit_reset_ground_adjust_state  (Ghidra: unit_reset_ground_adjust_state, renamed per out/phase4/units_functions.md,
// which already refers to this address by this name)
// address 0x55ad00, size 108 bytes
// name confidence: 0.7   rewrite confidence: 0.6
// evidence: biped_data.ground_adjust_iteration/_limit (0x524/0x525) and its flags bit 0x20 "the
//   ground-adjust dirty bit 0x55ad00 sets and 0x55ad70 clears" (types/units.h); object.flags bit
//   0x800000 mirrors it. Biped tag offset 0x2f4 bit 9 (mask 0x200) is UNSURE, not yet named.
// blam-cc: EAX -> object_index
// FIXED (register inputs, objdump): this file had no blam-cc note at all; EAX carries
// object_index (read at 0x55ad09 `and eax,0xffff`).

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

// When the Biped tag requests ground adjustment (biped_flags bit 0x200, UNSURE) and the object
// is attached to a parent (object.flags bit 0x20) but the ground-adjust dirty bit isn't already
// set and the biped isn't already grounded, resets the iteration counter, seeds the iteration
// limit to 0x14, and marks both the dirty bit and its object.flags mirror.
void unit_reset_ground_adjust_state(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if (((tag->biped_flags >> 9) & 1) != 0 && (obj->flags & 0x20) != 0 && (biped->flags & 0x21) == 0) {
        biped->ground_adjust_iteration = 0;
        biped->ground_adjust_iteration_limit = 0x14;
        obj->flags |= 0x800000;
        biped->flags |= 0x20;
    }
}

#if 0
Original Ghidra decompilation (0x55ad00):

void FUN_0055ad00(void)

{
  uint *puVar1;
  uint in_EAX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) >> 9 & 1) != 0
       ) && ((puVar1[4] & 0x20) != 0)) && ((puVar1[0x133] & 0x21) == 0)) {
    *(undefined1 *)(puVar1 + 0x149) = 0;
    *(undefined1 *)((int)puVar1 + 0x525) = 0x14;
    puVar1[4] = puVar1[4] | 0x800000;
    puVar1[0x133] = puVar1[0x133] | 0x20;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
