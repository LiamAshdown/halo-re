// unit_get_current_grenade_index  (Ghidra: unit_get_current_grenade_index, already named)
// address 0x56e060, size 30 bytes, name confidence 0.55, rewrite confidence 0.8
// functions.md: "Returns the index of the unit's currently selected grenade type."
// evidence: types/units.h unit_data.current_grenade_index (0x31c).
// blam-cc: in_EAX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

int8_t unit_get_current_grenade_index(uint32_t unit_index) // blam-cc: in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    return unit->current_grenade_index;
}

#if 0
Original Ghidra decompilation (0x56e060):

undefined4 unit_get_current_grenade_index(void)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  return CONCAT22((short)((uint)iVar1 >> 0x10),(short)*(char *)(iVar1 + 0x31c));
}
#endif
