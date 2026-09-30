// weapon_trigger_projectile_time_fraction  (Ghidra: FUN_004c2be0; named from
// out/phase4/items_functions.md, "Computes a normalized time fraction for one of the item's
// attachments given an elapsed value and its tag-defined duration")
// address 0x4c2be0, size 140 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: types/tags.h Weapon.triggers (0x4fc), WeaponTrigger.projectile (tag_id 0xa0);
//   out/phase4/items_types_notes.md projectile_data notes name tag+0x1e4/0x1e8 as
//   initial_velocity/final_velocity.
// register convention: item index in EAX; trigger index in CX; elapsed value is a
//   Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, CX -> trigger_index, stack -> elapsed
// UNSURE: types/projectiles.h does not exist yet, so the Projectile field at 0x1e4 is read by
// raw offset rather than a named field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Divides `elapsed` by trigger_index's projectile initial_velocity (offset 0x1e4 in the
// Projectile tag), returning 0 when the trigger index is out of range or the projectile has no
// positive initial velocity.
real weapon_trigger_projectile_time_fraction(datum_index item_index, int16_t trigger_index, real elapsed)
{
    object *item_obj;
    Weapon *weapon_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (trigger_index >= 0 && trigger_index < weapon_tag->triggers.count) {
        WeaponTrigger *trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        uint8_t *projectile_tag = (uint8_t *)tag_instances[(uint16_t)(*(datum_index *)&trigger->projectile.tag_id)].data;
        real initial_velocity = ((Projectile *)projectile_tag)->initial_velocity;

        if (initial_velocity > 0.0f) {
            return elapsed / initial_velocity;
        }
    }
    return 0.0f;
}

#if 0
Original Ghidra decompilation (0x4c2be0):

float10 FUN_004c2be0(float param_1)

{
  int iVar1;
  uint in_EAX;
  short in_CX;
  float10 fVar2;

  fVar2 = (float10)0.0;
  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((-1 < in_CX) && ((int)in_CX < *(int *)(iVar1 + 0x4fc))) {
    fVar2 = (float10)0.0;
    iVar1 = *(int *)((*(uint *)(in_CX * 0x114 + 0xa0 + *(int *)(iVar1 + 0x500)) & 0xffff) * 0x20 +
                     0x14 + DAT_0087bc14);
    if (0.0 < *(float *)(iVar1 + 0x1e4)) {
      fVar2 = (float10)param_1 / (float10)*(float *)(iVar1 + 0x1e4);
    }
  }
  return fVar2;
}
#endif
