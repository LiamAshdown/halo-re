// breakable_surface_apply_damage  (Ghidra: FUN_004ffde0, still unnamed there. Phase-2 proposed
// physics_point_update_random_extension, but that guess predates the module's type recovery;
// the operand is breakable_surface_globals.health (a vitality/hit-point value, not a spring
// "extension") and every other field it touches is damage_data / DamageEffect, so this rewrite
// uses the more accurate name established from out/phase4/physics_types_notes.md instead.)
// address 0x4ffde0, size 305 bytes
// VERIFIED against disassembly 0x4ffde0..0x4fff11 (2026-09-30). the material argument of globals_material_get is the
//   value just loaded into ECX (0x4ffe42) and the shatter call's stack arguments are (surface_index,
//   collision_surface_index) (0x4fff02)
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: breakable_surface_globals.health/active and
//   damage_data.random_blend/damage_effect_tag/unknown_4c confirmed field-by-field in
//   out/phase4/physics_types_notes.md sections 2 and 4; DamageEffect.maximum_vitality and the
//   per-MaterialType damage-modifier block (dirt..hunter_shield, ordinal-matched to MaterialType
//   in types/tags.h) confirmed against types/tags.h directly.
// register convention: EBX -> damage (damage_data *; never reloaded from the stack, so Ghidra
//   could not attach it to a numbered parameter), stack -> surface_index (a breakable-surface
//   index within the current BSP, passed on unchanged to physics_point_spawn_contact_effect).
//   // blam-cc: EBX -> damage, stack -> surface_index
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"

extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78
                                    // UNSURE name: an earlier objects-module pass named this
                                    // global object_zone_light_table_pointer; the field-by-field
                                    // match here (out/phase4/physics_types_notes.md section 2)
                                    // supersedes that and is owed back as a reconciliation.
extern int16_t global_structure_bsp_index; // 0x0069e8d8
extern tag_instance *tag_instances;        // 0x0087bc14
extern real random_real_range(real min, real max); // 0x401050
extern GlobalsMaterial *globals_material_get(int16_t material_type); // 0x53e7c0, scenario/globals
                                    // module, not physics; resolves Globals.materials[type], or
                                    // a fallback record when the type is out of range.
extern void breakable_surface_shatter(uint16_t surface_index, damage_data *damage, int32_t collision_surface_index); // 0x500090
// FIXED (objdump 0x4fff02): the second stack argument ([esp+0x14]) is passed through as the third
//   argument of 0x500090; projectile_response and unit_melee_attack_scan both push it.
void breakable_surface_apply_damage(damage_data *damage, int32_t surface_index, int32_t collision_surface_index)
{
    int16_t index;
    float *extension;
    GlobalsMaterial *material;

    index = (int16_t)surface_index;
    if ((breakable_surface_state->initialized != 0) && (index != -1) &&
        (damage->damage_effect_tag != k_datum_index_none) && (damage->material_type != -1)) {
        extension = &breakable_surface_state->health[global_structure_bsp_index][index];
        if (0.0f < *extension) {
            // VERIFIED (0x4ffe42): the material argument is the damage's material_type, still live in ECX.
            material = globals_material_get(damage->material_type);
            if ((material != 0) && (0.0f < material->maximum_vitality)) {
                DamageEffect *effect =
                    (DamageEffect *)tag_instances[(uint16_t)damage->damage_effect_tag].data;
                // dirt is the first entry of a per-MaterialType float block that runs through
                // hunter_shield, ordinal-matched to MaterialType in types/tags.h.
                float *material_damage_modifiers = &effect->dirt;
                float random_amount = random_real_range(effect->damage_upper_bound[0],
                                                          effect->damage_upper_bound[1]);
                float blended_amount = (random_amount - effect->damage_lower_bound) *
                                           damage->random_blend +
                                       effect->damage_lower_bound;
                float new_extension = *extension -
                    (blended_amount * material_damage_modifiers[damage->material_type]) /
                        material->maximum_vitality;
                *extension = new_extension;
                // Equivalent to (new_extension <= 0.0f): Ghidra's
                // "(x < 0.0) != (x == 0.0)" reduces to exactly that, including for NaN.
                if (new_extension <= 0.0f) {
                    breakable_surface_state->active[global_structure_bsp_index][index >> 5] &=
                        ~(1u << (index & 0x1f));
                    // VERIFIED (0x4fff02): stack arguments are (surface_index, collision_surface_index); `damage` is EBX.
                    breakable_surface_shatter((uint16_t)index, damage, collision_surface_index);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ffde0):

void FUN_004ffde0(undefined4 param_1)

{
  float *pfVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  uint *unaff_EBX;
  float fVar5;

  if ((((*DAT_006b8d78 != '\0') && (sVar4 = (short)param_1, sVar4 != -1)) &&
      (*unaff_EBX != 0xffffffff)) && ((short)unaff_EBX[0x13] != -1)) {
    pfVar1 = (float *)(DAT_006b8d78 + (DAT_0069e8d8 * 0x100 + (int)sVar4) * 4 + 0x204);
    if (((0.0 < *pfVar1) && (iVar3 = FUN_0053e7c0(), iVar3 != 0)) &&
       (0.0 < *(float *)(iVar3 + 0x2d4))) {
      iVar2 = *(int *)((*unaff_EBX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      fVar5 = random_real_range(*(float *)(iVar2 + 0x1d4),*(float *)(iVar2 + 0x1d8));
      fVar5 = *pfVar1 - (((fVar5 - *(float *)(iVar2 + 0x1d0)) * (float)unaff_EBX[0x10] +
                         *(float *)(iVar2 + 0x1d0)) *
                        *(float *)(iVar2 + 0x200 + (short)unaff_EBX[0x13] * 4)) /
                        *(float *)(iVar3 + 0x2d4);
      *pfVar1 = fVar5;
      if (fVar5 < 0.0 != (fVar5 == 0.0)) {
        iVar3 = ((int)sVar4 >> 5) + DAT_0069e8d8 * 8;
        *(uint *)(DAT_006b8d78 + iVar3 * 4 + 1) =
             *(uint *)(DAT_006b8d78 + iVar3 * 4 + 1) & ~(1 << ((byte)param_1 & 0x1f));
        FUN_00500090(param_1);
      }
    }
  }
  return;
}
#endif
