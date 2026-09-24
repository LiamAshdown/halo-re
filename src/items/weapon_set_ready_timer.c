// weapon_set_ready_timer  (Ghidra: FUN_004c2b20; renamed per items_types_notes.md:
// "single store to weapon 0x248")
// address 0x4c2b20, size 32 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/items.h weapon_data.ready_timer (0x248, UNSURE field meaning; only writer).
// register convention: item index in EAX; new value is a Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, stack -> value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0

// Writes a weapon's ready_timer directly. UNSURE: called by unit_update when the holder Object
// tag has flag 0x800000 (integrated_light_cntrls_weapon); see items_types_notes.md.
void weapon_set_ready_timer(datum_index item_index, real value)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->ready_timer = value;
}

#if 0
Original Ghidra decompilation (0x4c2b20):

void FUN_004c2b20(undefined4 param_1)

{
  uint in_EAX;

  *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x248) =
       param_1;
  return;
}
#endif
