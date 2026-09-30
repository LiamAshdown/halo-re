// damage_apply_area_effect
// address 0x4edd30, size 119 bytes
// name confidence: 0.5 (Ghidra-recovered name, corroborated by out/phase4/objects_types_notes.md's
// damage_data section: "damage_apply_area_effect 0x4edd30 passes &dd[5] (0x14) as the search
// location and &dd[7] (0x1c) as the sphere centre")
// rewrite confidence: 0.55
// evidence: types/objects.h damage_data.location_leaf_index (0x14) / .epicentre (0x1c) /
// .damage_effect_tag (0x00); types/objects.h k_maximum_damage_candidates (0x40); types/tags.h
// DamageEffect.radius[2] (0x00, index 1 used here as the search radius).
// register convention: damage_data *dd on the stack (param_1).
// UNSURE: object_find_in_sphere's first two literal 0 arguments and the exact meaning of its
// parameter list are not established in this module; they are preserved literally from the call.
// blam-cc: stack=dd

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "fn_objects.h"

extern tag_instance *tag_instances; // 0x0087bc14

extern uint16_t object_find_in_sphere(int32_t unknown_0, int32_t unknown_1, int32_t *location_leaf_index,
    real_point3d *center, float radius, datum_index *out_buffer, int32_t max_count); // 0x4f6fe0 (out of range)

extern void breakable_surface_damage_in_blast_radius(damage_data *dd); // UNSURE: objects module, 0x4fff20 (out of range)

void damage_apply_area_effect(damage_data *dd)
{
    datum_index candidates[k_maximum_damage_candidates + 1]; // local_104: 65 dwords in Ghidra
    DamageEffect *effect = (DamageEffect *)tag_instances[dd->damage_effect_tag & 0xffff].data;
    int16_t found = (int16_t)object_find_in_sphere(0, 0, &dd->location_leaf_index, &dd->epicentre,
        effect->radius[1], candidates, k_maximum_damage_candidates);

    if (found > 0) {
        int32_t i;
        for (i = 0; i < found; i++) {
            object_damage_apply_line_of_sight(dd, candidates[i], 0);
        }
    }

    breakable_surface_damage_in_blast_radius(dd);
}

#if 0
Original Ghidra decompilation (0x4edd30):

void damage_apply_area_effect(uint *param_1)

{
  ushort uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 local_104 [65];

  uVar1 = object_find_in_sphere
                    (0,0,param_1 + 5,param_1 + 7,
                     *(undefined4 *)(*(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4)
                     ,local_104,0x40);
  if (0 < (short)uVar1) {
    puVar3 = local_104;
    uVar2 = (uint)uVar1;
    do {
      object_damage_apply_line_of_sight(param_1,*puVar3,0);
      puVar3 = puVar3 + 1;
      uVar2 = uVar2 - 1;
    } while (uVar2 != 0);
  }
  FUN_004fff20(param_1);
  return;
}
#endif
