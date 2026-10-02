// unit_find_weapon_index_with_fixed_flag  (Ghidra: FUN_00570460; renamed from the phase2
//   proposal)
// address 0x570460, size 106 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.45
// evidence: types/units.h unit_data.weapons[4] (0x2f8); types/tags.h Weapon.weapon_flags
//   (absolute tag offset 0x308, bit 3 = must_be_readied).
// register convention: unit object index in ECX (in_ECX).
//   // blam-cc: ECX -> unit_index

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

// Finds the index of the first carried weapon whose tag flags have bit 3 (must_be_readied) set,
// or 0xffff if none.
uint16_t unit_find_weapon_index_with_fixed_flag(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot = 0;

    for (;;) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
            Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
            if ((weapon_tag->weapon_flags >> 3 & 1) != 0) {
                return slot;
            }
        }
        slot++;
        if (slot > 3) {
            return 0xffff;
        }
    }
}

#if 0
Original Ghidra decompilation (0x570460):

int FUN_00570460(void)

{
  uint uVar1;
  int iVar2;
  uint in_ECX;

  iVar2 = 0;
  while ((uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) +
                            0x2f8 + (short)iVar2 * 4), uVar1 == 0xffffffff ||
         ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                          (uVar1 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                             DAT_0087bc14) + 0x308) >> 3 & 1) == 0))) {
    iVar2 = iVar2 + 1;
    if (3 < (short)iVar2) {
      return CONCAT22((short)((uint)iVar2 >> 0x10),0xffff);
    }
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
