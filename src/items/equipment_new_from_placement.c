// equipment_new_from_placement  (Ghidra: missed_4bbae0, created by hand this pass -- Ghidra
// never recovered it as a function; only reachable through the equipment object_type_definition
// row)
// address 0x4bbae0, size 104 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: types/objects.h object_type_definition.notify_two_args_2c (+0x2c), already
//   implemented generically by src/objects/object_type_definitions_notify_two_args_0x2c.c; the
//   equipment row (0x0069b810) carries this address at +0x2c and the weapon row carries its own
//   sibling weapon_new_from_placement (0x4c1350) at the identical +0x2c column, byte-for-byte
//   the same body apart from the missing magazine-count init (equipment has no magazines).
//   types/tags.h ScenarioEquipment (misc_flags/ScenarioItemFlags at +0x22, matching
//   object_get_or_build_render_permutation's already-established {tag_index,name_index,flags}
//   placement-record shape); ScenarioItemFlags bit 0x01 initially_at_rest, bit 0x04
//   does_accelerate. types/objects.h object.flags (_object_at_rest_bit 0x20,
//   _object_unknown_20000_bit 0x20000, _object_definition_flag0_bit 0x40000), object.position
//   (0x05c). types/items.h item_data.flags (_item_does_not_accelerate_bit 0x20, set by
//   weapon_new_from_placement "when the scenario record does NOT have does_accelerate").
// register convention: both arguments are Ghidra-recognized stack parameters, exactly like
//   weapon_new_from_placement's own pair.
// blam-cc: stack -> (equipment_object_index, scenario_equipment_placement)
// UNSURE: the position.z nudge loads the shared 0.05f constant at 0x00672be8 (documented in
//   types/scenario.h as "the z nudge per retry in 0x53e870") rather than an immediate the way
//   weapon_new_from_placement's own +0.05f reads; reproduced here as the same literal.
// UNSURE: unlike weapon_new_from_placement (which only sets _object_unknown_20000_bit), this
//   function also sets _object_definition_flag0_bit in the same store -- confirmed byte for
//   byte against objdump (`or edi,0x60000`), not a transcription slip.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0

// The equipment row's notify_two_args_2c hook (object_type_definition +0x2c). Applies a
// ScenarioEquipment placement record's at-rest and does-accelerate flags to a freshly created
// equipment object, and nudges it slightly off the placement point when it is not going to
// start out resting.
void equipment_new_from_placement(uint32_t equipment_object_index, ScenarioEquipment *placement)
    // blam-cc: stack -> (equipment_object_index, placement)
{
    object *obj = ((object_header *)object_data->data)[equipment_object_index & 0xffff].data;
    item_data *id = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((placement->misc_flags & 1) == 0) { // initially_at_rest
        obj->flags = obj->flags & ~(uint32_t)_object_at_rest_bit;
    } else {
        obj->flags = obj->flags | _object_at_rest_bit;
    }
    obj->flags = obj->flags | 0x60000; // _object_unknown_20000_bit | _object_definition_flag0_bit

    if ((placement->misc_flags & 4) == 0) { // does_accelerate
        id->flags = id->flags | _item_does_not_accelerate_bit;
    } else {
        id->flags = id->flags & ~(uint32_t)_item_does_not_accelerate_bit;
    }

    if ((placement->misc_flags & 1) == 0) { // initially_at_rest
        obj->position.z = obj->position.z + 0.05f; // shared constant at 0x00672be8
    }
}

#if 0
Original Ghidra decompilation (0x4bbae0):

void missed_4bbae0(uint param_1,int param_2)

{
  int iVar1;
  uint uVar2;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if ((*(byte *)(param_2 + 0x22) & 1) == 0) {
    uVar2 = *(uint *)(iVar1 + 0x10) & 0xffffffdf;
  }
  else {
    uVar2 = *(uint *)(iVar1 + 0x10) | 0x20;
  }
  *(uint *)(iVar1 + 0x10) = uVar2;
  *(uint *)(iVar1 + 0x10) = uVar2 | 0x60000;
  if ((*(byte *)(param_2 + 0x22) & 4) == 0) {
    *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) | 0x20;
  }
  else {
    *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) & 0xffffffdf;
  }
  if ((*(byte *)(param_2 + 0x22) & 1) == 0) {
    *(float *)(iVar1 + 100) = *(float *)(iVar1 + 100) + 0.05;
  }
  return;
}
#endif
