// unit_get_active_weapon_scale  (Ghidra: unit_get_active_weapon_scale)
// address 0x565ab0, size 67 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.4
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), .weapons[4] (0x2f8).
// register convention: unit index in EAX.
//   // blam-cc: in_EAX -> unit_index
// UNSURE: weapon_get_zoom_magnification is a leaf helper outside this batch (no strings/callees); its argument
//   is not shown by Ghidra at this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern float weapon_get_zoom_magnification(void); // 0x4c2d70, UNSURE: no traced args

float unit_get_active_weapon_scale(uint32_t unit_index) // blam-cc: in_EAX -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot = unit->current_weapon_index;

    if (slot != -1 && unit->weapons[slot] != (datum_index)-1) {
        return weapon_get_zoom_magnification();
    }
    return 1.0f;
}

#if 0
Original Ghidra decompilation (0x565ab0):

float10 FUN_00565ab0(void)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  float10 fVar3;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar2 + 0x2f2);
  if ((sVar1 != -1) && (*(int *)(iVar2 + 0x2f8 + sVar1 * 4) != -1)) {
    fVar3 = (float10)FUN_004c2d70();
    return fVar3;
  }
  return (float10)1.0;
}
#endif
