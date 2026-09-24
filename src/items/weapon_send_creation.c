// weapon_send_creation  (Ghidra: missed_4c59f0, created by hand this pass -- Ghidra never
// recovered it as a function; only reachable through the weapon object_type_definition row)
// address 0x4c59f0, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: the weapon row (0x0069b748) carries this address at +0x64, the same column that
//   holds projectile_send_creation (0x4c0b10, "+0x64 projectile_send_creation") and
//   equipment_send_creation (0x4bbc30, this batch); byte for byte identical body to
//   equipment_send_creation apart from the callee and the item_data offset (weapon_data starts
//   at the same object+0x22c). Callee weapon_build_creation_message (0x4c5a50, src/items).
//   types/objects.h object.flags (_object_at_rest_bit 0x20); types/items.h item_data.flags
//   (_item_at_rest_on_structure_bit 0x08).
// register convention: all three stack parameters are forwarded unchanged to
//   weapon_build_creation_message; Ghidra already recovers a plain stack cdecl signature.
// blam-cc: stack -> (item_index, arg2, arg3)
// UNSURE: arg2/arg3 are never read in this function's own body; kept only for call-site
//   compatibility with weapon_build_creation_message, matching equipment_send_creation's own
//   header note for the identical pair.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0
extern void weapon_build_creation_message(datum_index item_index, uint32_t unused_param_2,
    uint32_t unused_param_3, uint32_t object_flags); // 0x4c5a50, this module

// The weapon row's "send creation" hook (object_type_definition +0x64). Builds and sends the
// weapon creation message, advertising _object_at_rest_bit in the message's object_flags only
// when the item is at rest and that rest is not on a structure surface.
void weapon_send_creation(uint32_t item_index, uint32_t arg2, uint32_t arg3)
    // blam-cc: stack -> (item_index, arg2, arg3)
{
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((obj->flags & _object_at_rest_bit) != 0 && (id->flags & _item_at_rest_on_structure_bit) == 0) {
        weapon_build_creation_message(item_index, arg2, arg3, _object_at_rest_bit);
        return;
    }
    weapon_build_creation_message(item_index, arg2, arg3, 0);
}

#if 0
Original Ghidra decompilation (0x4c59f0):

void missed_4c59f0(uint param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (((*(byte *)(iVar1 + 0x10) & 0x20) != 0) && ((*(byte *)(iVar1 + 500) & 8) == 0)) {
    weapon_build_creation_message(param_1,param_2,param_3,0x20);
    return;
  }
  weapon_build_creation_message(param_1,param_2,param_3,0);
  return;
}
#endif
