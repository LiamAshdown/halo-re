// equipment_send_creation  (Ghidra: missed_4bbc30, created by hand this pass -- Ghidra never
// recovered it as a function; only reachable through the equipment object_type_definition row)
// address 0x4bbc30, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: the equipment row (0x0069b810) carries this address at +0x64, the same column that
//   holds projectile_send_creation (0x4c0b10, "+0x64 projectile_send_creation") and
//   weapon_send_creation (0x4c59f0, this batch) on their own rows; all three have the identical
//   shape, one conditional flag then a call into the type's own build_creation_message. Callee
//   equipment_build_creation_message (0x4bbc90, src/items). types/objects.h object.flags
//   (_object_at_rest_bit 0x20); types/items.h item_data.flags (_item_at_rest_on_structure_bit
//   0x08).
// register convention: all three stack parameters are forwarded unchanged to
//   equipment_build_creation_message; Ghidra already recovers a plain stack cdecl signature.
// blam-cc: stack -> (item_index, arg2, arg3)
// UNSURE: arg2/arg3 are never read in this function's own body; kept only for call-site
//   compatibility with equipment_build_creation_message, exactly as that file's own header notes
//   for its matching parameters.
// UNSURE: the exact meaning of the object_flags value 0x20 passed to the creation message (it
//   ends up in equipment_creation_message.object_flags) is not chased further than "advertise
//   _object_at_rest_bit only when the item is at rest on an object rather than on a structure".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern void equipment_build_creation_message(uint32_t item_index, uint32_t unused_arg2,
    uint32_t unused_arg3, uint32_t object_flags); // 0x4bbc90, this module

// The equipment row's "send creation" hook (object_type_definition +0x64). Builds and sends the
// equipment creation message, advertising _object_at_rest_bit in the message's object_flags only
// when the item is at rest and that rest is not on a structure surface (i.e. it is resting on
// another object, or its rest has not been resolved to a surface yet).
void equipment_send_creation(uint32_t item_index, uint32_t arg2, uint32_t arg3)
    // blam-cc: stack -> (item_index, arg2, arg3)
{
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((obj->flags & _object_at_rest_bit) != 0 && (id->flags & _item_at_rest_on_structure_bit) == 0) {
        equipment_build_creation_message(item_index, arg2, arg3, _object_at_rest_bit);
        return;
    }
    equipment_build_creation_message(item_index, arg2, arg3, 0);
}

#if 0
Original Ghidra decompilation (0x4bbc30):

void missed_4bbc30(uint param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (((*(byte *)(iVar1 + 0x10) & 0x20) != 0) && ((*(byte *)(iVar1 + 500) & 8) == 0)) {
    equipment_build_creation_message(param_1,param_2,param_3,0x20);
    return;
  }
  equipment_build_creation_message(param_1,param_2,param_3,0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
