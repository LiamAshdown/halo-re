// weapon_trigger_get_average_damage  (Ghidra: FUN_004c12b0; renamed per
// out/phase4/items_types_notes.md "Renames this pass establishes")
// address 0x4c12b0, size 152 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: types/tags.h Weapon.triggers (0x4fc TagReflexive), WeaponTrigger.maximum_rate_of_fire
//   (0x04), WeaponTrigger.projectile (0x94 TagDependency, tag_id at +0xc), Projectile
//   .attached_detonation_damage (tag_id at 0x220) and .impact_damage (tag_id at 0x230),
//   DamageEffect.damage_upper_bound[2] (0x1d4/0x1d8). Confirmed against the binary with an
//   offsetof probe against types/tags.h: Weapon.triggers=0x4fc, WeaponTrigger
//   .maximum_rate_of_fire=0x4, WeaponTrigger.projectile=0x94, TagDependency.tag_id=+0xc,
//   Projectile.attached_detonation_damage.tag_id=0x220, Projectile.impact_damage.tag_id=0x230,
//   DamageEffect.damage_upper_bound=0x1d4 -- all match the raw literals below exactly.
// register convention: weapon tag id in EAX, optional output pointer in ECX.
// blam-cc: EAX -> weapon_tag_id, ECX -> out_max_rate_of_fire (nullable)
// UNSURE: only the FIRST trigger (Weapon.triggers[0]) is ever read; there is no index argument,
// so a two-trigger weapon's secondary trigger damage is not part of this average. Preserved
// exactly as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

// Averages the impact/attached-detonation damage of a weapon's first trigger's projectile, and
// optionally reports that trigger's maximum rate of fire (upper bound of the two-entry range).
real weapon_trigger_get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire)
{
    Weapon *weapon_tag;
    WeaponTrigger *trigger;
    datum_index projectile_tag_id;
    Projectile *projectile_tag;
    datum_index damage_tag_id;
    DamageEffect *damage_tag;
    real total;

    total = 0.0f;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)weapon_tag_id].data;
    trigger = (WeaponTrigger *)weapon_tag->triggers.pointer;

    if (out_max_rate_of_fire != 0) {
        *out_max_rate_of_fire = trigger->maximum_rate_of_fire[1];
    }

    projectile_tag_id = *(datum_index *)&trigger->projectile.tag_id;
    if (projectile_tag_id != (datum_index)0xffffffff) {
        projectile_tag = (Projectile *)tag_instances[(uint16_t)projectile_tag_id].data;

        damage_tag_id = *(datum_index *)&projectile_tag->impact_damage.tag_id;
        if (damage_tag_id != (datum_index)0xffffffff) {
            damage_tag = (DamageEffect *)tag_instances[(uint16_t)damage_tag_id].data;
            total = (damage_tag->damage_upper_bound[1] + damage_tag->damage_upper_bound[0]) * 0.5f;
        }

        damage_tag_id = *(datum_index *)&projectile_tag->attached_detonation_damage.tag_id;
        if (damage_tag_id != (datum_index)0xffffffff) {
            damage_tag = (DamageEffect *)tag_instances[(uint16_t)damage_tag_id].data;
            total = total + (damage_tag->damage_upper_bound[1] + damage_tag->damage_upper_bound[0]) * 0.5f;
        }
    }

    return total;
}

#if 0
Original Ghidra decompilation (0x4c12b0):

float10 FUN_004c12b0(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint in_EAX;
  undefined4 *in_ECX;
  float10 fVar5;

  iVar4 = DAT_0087bc14;
  fVar5 = (float10)0.0;
  iVar1 = *(int *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x500);
  if (in_ECX != (undefined4 *)0x0) {
    *in_ECX = *(undefined4 *)(iVar1 + 8);
  }
  uVar2 = *(uint *)(iVar1 + 0xa0);
  if (uVar2 != 0xffffffff) {
    iVar1 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + iVar4);
    uVar2 = *(uint *)(iVar1 + 0x230);
    if (uVar2 != 0xffffffff) {
      iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + iVar4);
      fVar5 = ((float10)*(float *)(iVar3 + 0x1d8) + (float10)*(float *)(iVar3 + 0x1d4)) *
              (float10)0.5;
    }
    uVar2 = *(uint *)(iVar1 + 0x220);
    if (uVar2 != 0xffffffff) {
      iVar1 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + iVar4);
      fVar5 = ((float10)*(float *)(iVar1 + 0x1d8) + (float10)*(float *)(iVar1 + 0x1d4)) *
              (float10)0.5 + fVar5;
    }
  }
  return fVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
