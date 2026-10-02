// unit_find_weapon_index_by_flag  (Ghidra: FUN_00570520; renamed from the phase2 proposal)
// address 0x570520, size 116 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary)
// rewrite confidence: 0.5
// evidence: types/units.h unit_data.weapons[4] (0x2f8); types/tags.h Weapon.weapon_flags
//   (absolute tag offset 0x308); parameterized counterpart of
//   unit_find_weapon_index_with_fixed_flag (0x570460, this batch).
// register convention: unit object index in EAX (in_EAX); the flag bit index in a stack byte
//   parameter.
//   // blam-cc: EAX -> unit_index, stack -> flag_bit
// UNSURE: Ghidra's own decompile returns a bare `CONCAT31(garbage, 1)` on the success path
//   (i.e. AL=1 with EAX's upper 3 bytes left over from reusing that register as a tag-data
//   pointer inside the loop condition), not the slot index -- a classic "byte-only return, rest
//   of EAX not zero-extended" artifact identical to the one documented in
//   src/math/vector3d_rotate_toward.c. The parallel structure to
//   unit_find_weapon_index_with_fixed_flag.c (which *does* cleanly return the index) is taken
//   as evidence this is the same underlying convention miscompiled by the decompiler, and the
//   index is returned here rather than a bare boolean; this could not be confirmed against the
//   disassembly.

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

// Finds the index of the first carried weapon whose tag flags have the given bit set, or 0xffff
// if none.
uint16_t unit_find_weapon_index_by_flag(uint32_t unit_index, uint8_t flag_bit)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int32_t slot = 0;

    for (;;) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
            Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
            if ((weapon_tag->weapon_flags & (1u << (flag_bit & 0x1f))) != 0) {
                return (uint16_t)slot;
            }
        }
        slot++;
        if (slot > 3) {
            return 0xffff;
        }
    }
}

#if 0
Original Ghidra decompilation (0x570520):

uint FUN_00570520(byte param_1)

{
  uint in_EAX;
  uint uVar1;
  int iVar2;
  uint *puVar3;

  uVar1 = in_EAX & 0xffff;
  iVar2 = 0;
  puVar3 = (uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + uVar1 * 0xc) + 0x2f8);
  while ((*puVar3 == 0xffffffff ||
         (uVar1 = *(uint *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (*puVar3 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                           DAT_0087bc14), (*(uint *)(uVar1 + 0x308) & 1 << (param_1 & 0x1f)) == 0)))
  {
    iVar2 = iVar2 + 1;
    puVar3 = puVar3 + 1;
    if (3 < iVar2) {
      return uVar1 & 0xffffff00;
    }
  }
  return CONCAT31((int3)(uVar1 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
