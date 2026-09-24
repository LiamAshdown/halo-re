// unit_current_weapon_has_flag  (Ghidra: unit_current_weapon_has_flag)
// address 0x565b60, size 126 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.3
// evidence: types/units.h unit_data.zoom_level (0x320), .current_weapon_index (0x2f2),
//   .weapons[4] (0x2f8); types/objects.h object.definition_tag (0x000).
// register convention: unit index in ECX.
//   // blam-cc: in_ECX -> unit_index
// UNSURE: the weapon tag field at +0x308, bit 0x4000, has no named entry in types/tags.h's
//   Weapon coverage for this batch; kept as a raw offset.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

uint8_t unit_current_weapon_has_flag(uint32_t unit_index) // blam-cc: in_ECX -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->zoom_level != -1) {
        int16_t slot = unit->current_weapon_index;
        if (slot != -1 && unit->weapons[slot] != (datum_index)-1) {
            object *weapon = ((object_header *)object_data->data)[unit->weapons[slot] & 0xffff].data;
            void *weapon_tag = tag_instances[weapon->definition_tag & 0xffff].data;
            if ((*(uint32_t *)((uint8_t *)weapon_tag + 0x308) & 0x4000) != 0) { // UNSURE: raw Weapon-tag field
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x565b60):

undefined1 FUN_00565b60(void)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  undefined1 uVar4;
  uint in_ECX;
  int iVar5;

  iVar2 = *(int *)(DAT_008603b0 + 0x34);
  iVar5 = (in_ECX & 0xffff) * 0xc;
  uVar4 = 0;
  if (*(char *)(*(int *)(iVar5 + 8 + iVar2) + 800) != -1) {
    iVar5 = *(int *)(iVar5 + 8 + iVar2);
    sVar1 = *(short *)(iVar5 + 0x2f2);
    if (((sVar1 != -1) && (uVar3 = *(uint *)(iVar5 + 0x2f8 + sVar1 * 4), uVar3 != 0xffffffff)) &&
       ((*(uint *)(*(int *)((**(uint **)(iVar2 + 8 + (uVar3 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14
                           + DAT_0087bc14) + 0x308) & 0x4000) != 0)) {
      uVar4 = 1;
    }
  }
  return uVar4;
}
#endif
