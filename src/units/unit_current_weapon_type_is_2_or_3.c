// unit_current_weapon_type_is_2_or_3  (Ghidra: FUN_0056bd60)
// address 0x56bd60, size 90 bytes, name confidence 0.3, rewrite confidence 0.5
// functions.md: "Returns whether the unit's currently equipped weapon has type code 2 or 3."
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), .weapons[4] (0x2f8).
// blam-cc: in_EAX -> unit_index.
// UNSURE: the weapon-object byte at +0x261 is not named by any header this module has access to.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

uint8_t unit_current_weapon_type_is_2_or_3(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (unit->current_weapon_index == -1) {
        return 0;
    }
    datum_index weapon_index = unit->weapons[unit->current_weapon_index];
    if (weapon_index == k_datum_index_none) {
        return 0;
    }
    object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
    int8_t weapon_kind = *(int8_t *)((uint8_t *)weapon_obj + 0x261);
    return (weapon_kind == 2) || (weapon_kind == 3);
}

#if 0
Original Ghidra decompilation (0x56bd60):

uint FUN_0056bd60(void)

{
  char cVar1;
  uint in_EAX;
  uint uVar2;

  uVar2 = *(uint *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((*(short *)(uVar2 + 0x2f2) == -1) ||
     (uVar2 = *(uint *)(uVar2 + 0x2f8 + *(short *)(uVar2 + 0x2f2) * 4), uVar2 == 0xffffffff)) {
    return uVar2 & 0xffffff00;
  }
  cVar1 = *(char *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x261);
  if ((cVar1 != '\x02') && (cVar1 != '\x03')) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
