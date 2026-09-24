// weapon_trigger_projectile_collision_test  (Ghidra: FUN_004c2b40; named from
// out/phase4/items_functions.md, "Runs the item's collision test against one of its tag-defined
// attachments, if the index is valid")
// address 0x4c2b40, size 153 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: types/tags.h Weapon.triggers (0x4fc), WeaponTrigger.projectile (tag_id at 0x94+0xc
//   =0xa0, confirmed by offsetof probe). FUN_004beec0 is a projectile-module function
//   (out/phase4/items_types_notes.md misattribution table) outside this address range, kept
//   opaque.
// register convention: item index in EAX; trigger index in CX; the remaining seven parameters
//   are Ghidra-recognized stack values threaded through to FUN_004beec0 almost unchanged.
// blam-cc: EAX -> item_index, CX -> trigger_index, stack -> (a1..a7)
// UNSURE: FUN_004beec0's parameter meanings are not established here; preserved literally,
// including which of the seven caller arguments are dropped (a2) versus zeroed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t FUN_004beec0(void *projectile_tag, uint32_t a1, uint32_t a2, uint32_t a3,
    uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8, uint32_t a9, uint32_t a10); // 0x4beec0

// Looks up trigger_index's projectile tag and, if the trigger index is in range, runs the
// projectile-module collision test against it.
int32_t weapon_trigger_projectile_collision_test(datum_index item_index, int16_t trigger_index,
    uint32_t a1, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7)
{
    object *item_obj;
    Weapon *weapon_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (trigger_index >= 0 && trigger_index < weapon_tag->triggers.count) {
        WeaponTrigger *trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        void *projectile_tag = tag_instances[(uint16_t)(*(datum_index *)&trigger->projectile.tag_id)].data;

        FUN_004beec0(projectile_tag, a1, 0, 0, 0, a3, a4, 0, a5, a6, a7);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4c2b40):

undefined4
FUN_004c2b40(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  int iVar1;
  uint in_EAX;
  undefined4 uVar2;
  short in_CX;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar2 = 0;
  if (-1 < in_CX) {
    if ((int)in_CX < *(int *)(iVar1 + 0x4fc)) {
      FUN_004beec0(*(undefined4 *)
                    ((*(uint *)(in_CX * 0x114 + 0xa0 + *(int *)(iVar1 + 0x500)) & 0xffff) * 0x20 +
                     0x14 + DAT_0087bc14),param_1,0,0,0,param_3,param_4,0,param_5,param_6,param_7);
      uVar2 = 1;
    }
  }
  return uVar2;
}
#endif
