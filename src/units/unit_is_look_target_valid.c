// unit_is_look_target_valid  (Ghidra: unit_is_look_target_valid)
// address 0x562570, size 61 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.35
// evidence: types/units.h unit_data.controlling_player (0x218), unit_data.vehicle_seat_index
//   (0x2f0); types/objects.h object.parent_object (0x11c).
// register convention: unit index in ECX.
//   // blam-cc: in_ECX -> unit_index
// UNSURE: exact caller/use site not traced; kept as a literal boolean gate per the Ghidra body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

uint8_t unit_is_look_target_valid(uint32_t unit_index) // blam-cc: in_ECX -> unit_index
{
    if (unit_index == (uint32_t)-1) {
        return 1;
    }

    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->controlling_player != (datum_index)-1 &&
        (obj->parent_object == (datum_index)-1 || unit->vehicle_seat_index == -1)) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x562570):

undefined1 FUN_00562570(void)

{
  int iVar1;
  undefined1 uVar2;
  uint in_ECX;

  uVar2 = 1;
  if (((in_ECX != 0xffffffff) &&
      (iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc),
      *(int *)(iVar1 + 0x218) != -1)) &&
     ((*(int *)(iVar1 + 0x11c) == -1 || (*(short *)(iVar1 + 0x2f0) == -1)))) {
    uVar2 = 0;
  }
  return uVar2;
}
#endif
