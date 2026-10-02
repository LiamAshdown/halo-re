// unit_get_weapon_object_index  (Ghidra: unit_get_weapon_object_index, already named)
// address 0x569970, size 44 bytes, name confidence 0.55, rewrite confidence 0.8
// evidence: types/units.h unit_data.weapons[4] (0x2f8).
// blam-cc: in_EAX -> unit_index, in_CX -> slot_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index) // blam-cc: in_EAX, in_CX
{
    if (slot_index == -1) {
        return k_datum_index_none;
    }
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    return unit->weapons[slot_index];
}

#if 0
Original Ghidra decompilation (0x569970):

undefined4 unit_get_weapon_object_index(void)

{
  uint in_EAX;
  short in_CX;

  if (in_CX != -1) {
    return *(undefined4 *)
            (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x2f8 +
            in_CX * 4);
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
