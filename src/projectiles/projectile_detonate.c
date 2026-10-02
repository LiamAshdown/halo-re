// projectile_detonate  (Ghidra: item_detonate; renamed per
// out/phase4/projectiles_types_notes.md: "matches the CEA hint for the 'gravity' string;
// super-combining sibling sweep, attached_detonation_damage, material detonation_effect")
// address 0x4c0670, size 1087 bytes
// name confidence: 0.7   rewrite confidence: 0.6 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: types/tags.h Projectile.projectile_flags (has_super_combining_explosion, tag 0x17c
//   bit 0x08), .effect/.super_detonation/.attached_detonation_damage (tag_id-only
//   TagDependency reads), .projectile_material_response (count/pointer at 0x240/0x244);
//   types/projectiles.h k_projectile_super_combine_detonate_threshold (6),
//   projectile_flags._projectile_super_detonation_counted_bit (0x40),
//   projectile_default_material_response (0x00695e20 fallback record); types/objects.h
//   object.parent_object (0x11c), .first_child_object (0x118), .next_object (0x114),
//   .creator_object (0xc4, the creating object), .type (0x0b4); types/units.h
//   unit_data.controlling_player (0x218), guarded by object.type == _object_type_biped;
//   types/objects.h damage_data (0x54 bytes, the `rep stos` of 0x15 dwords this function
//   builds); the "gravity" string plus the CEA hint via out/phase4/projectiles_types_notes.md.
// register convention: object index in EBX (unaff_EBX); the two remaining arguments are
//   Ghidra-recognized stack parameters.
// blam-cc: EBX -> object_index, stack -> (first_collision, remaining_tick_fraction)
// This pack is unusually hard: Ghidra elides almost every argument to object_get_position,
// object_get_orientation and effect_new_with_color (all called with zero or one visible argument), and
// object_snap_to_parent_marker_and_detach/object_reposition_to_spawn_location/ai_accumulate_repeated_event/contrail_advance are opaque foreign-module calls with no
// established signature anywhere else in this codebase. Every UNSURE note below documents one
// such elided or inferred piece; the field offsets, branch conditions and store order (which
// drive the actual gameplay behaviour) are reproduced exactly from the decompile.
// UNSURE: object_get_position/object_get_orientation's destinations are inferred from the
// stack-frame layout of the values read immediately afterward (the same technique
// src/objects/damage_effect_new_at_location.c and src/hs/hs_damage_apply_with_sound.c already
// use for this exact kind of elided call), not confirmed independently.
// UNSURE: effect_new_with_color's `position_block`/`direction_block` arguments here are inferred as
// two-entry arrays -- position_block both entries the same point, direction_block {up, the
// shared "down" constant} -- paired positionally with the two-entry {"", "gravity"} name array,
// on the theory that Ghidra's local_1c (the "up" output of object_get_orientation) and the
// immediately-following local_10/c/8 (a literal copy of the down constant) are one contiguous
// 24-byte block rather than two unrelated locals, by analogy with the duplicate-per-name-slot
// idiom in src/objects/damage_effect_new_at_location.c and the fully-resolved 12-argument call
// in src/hs/hs_effect_spawn_at_location.c. The exact block layout effect_new_with_color expects for
// kind == 2 is not independently confirmed.
// UNSURE: object_get_orientation(0) partway through the object_apply_damage setup discards its
// result entirely as far as this pack shows -- reproduced as a call whose result is unused.
// UNSURE: `local_74` (team_index) is set to 0xffff and then immediately overwritten with
// obj->owner_team before object_apply_damage is called; reproduced literally even though a
// "team index" being fed from a scenario name index looks unintentional.
// UNSURE: object_reposition_to_spawn_location is called here with zero visible arguments, unlike its other call site in
// src/items/trigger_create_projectiles.c (two datum_index arguments); declared separately here
// rather than reusing that signature.
// TYPES-GAP: object flag bit tests below use raw hex (no named object_flags bits exist yet for
// this path).
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)
// reconciled: R04 0x006f1d20 int32_t game_is_server -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern char k_empty_string[1];      // 0x0065512c
extern game_engine_definition *current_game_engine;      // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern ProjectileMaterialResponse projectile_default_material_response; // 0x00695e20
extern real_vector3d *global_down3d_pointer; // 0x0069672c, PTR_DAT_0069672c

extern real random_real(void); // 0x4019f0, math module
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up); // 0x4f6970
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack (location may be 0)
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index,
                                 int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610, objects module, UNSURE signature
extern uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position,
    uint32_t ignore_object_index); // 0x4f7b70, stack, ECX
    // see file header
extern void contrail_advance(datum_index contrail_handle, uint8_t detach, real delta_time); // 0x44ca60, EDI, stack
    // blam-cc: EDI -> the contrail attachment handle
    // obj->attachment_handles[proj->contrail_attachment_index], reloaded at 0x4c089b immediately
    // before the call. Same declaration as src/projectiles/projectile_update.c, whose 0x4bea4b
    // call is the identical idiom; src/objects/object_delete_attachments.c declares the same
    // address as (int, int) from its own call site's literals, which is the same stack shape.
extern void ai_accumulate_repeated_event(datum_index object_index, real_point3d *origin, int32_t kind,
    ObjectNoise_t noise, int32_t param_5); // 0x42c610, foreign module: plays an ObjectNoise at a
    // world point. Ghidra shows zero arguments; 0x4c0a98..0x4c0aa3 pushes
    // 1 / tag->detonation_noise (the word at Projectile 0x1f0) / 2 / &position_block[0] (the
    // same `lea` the preceding effect_new_with_color call used for its position block) / object_index.
    // Same declaration as src/projectiles/projectile_update.c, which passes kind = 1 and
    // tag->impact_noise.
extern void effect_new_with_color(uint32_t effect, uint32_t target_or_index, void *velocity, int32_t kind,
    char **labels, void *position_block, void *direction_block, real fade_in, real fade_out,
    int32_t color, int32_t tint_source, int32_t force_create); // 0x450980, established 12-argument
    // form, see src/hs/hs_effect_spawn_at_location.c; identical to the declaration in
    // src/projectiles/projectile_response.c

// Runs the ProjectileResponse "detonate" side effect: an optional super-combining explosion
// sweep across sibling projectiles attached to the same (biped) parent, a contrail/attachment
// bookkeeping step on the first collision of the tick, the primary detonation effect spawn,
// attached_detonation_damage applied to the parent object, and a second effect spawn keyed by
// the last-hit material's ProjectileMaterialResponse.
void projectile_detonate(uint32_t object_index, char first_collision, real remaining_tick_fraction)
    // blam-cc: EBX -> object_index, stack -> (first_collision, remaining_tick_fraction)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Projectile *tag = (Projectile *)tag_instances[(uint16_t)obj->definition_tag].data;
    char *effect_names[2];
    datum_index effect_tag_id;
    real_point3d position_block[2];   // UNSURE shape, see file header; both entries the same point
    real_point3d relink_position;      // Ghidra's local_90/8c/88, the super-detonation relink point
    real_vector3d direction_block[2]; // [0] = "up" from object_get_orientation, [1] = the shared
                                       // "down" constant (0x0069672c) -- paired positionally with
                                       // effect_names[2] = {"", "gravity"}, see file header

    effect_names[0] = k_empty_string;
    effect_names[1] = (char *)"gravity";
    effect_tag_id = *(datum_index *)&tag->effect.tag_id;

    if ((tag->projectile_flags & _projectile_definition_has_super_combining_explosion_bit) != 0 &&
        (((projectile_data *)((uint8_t *)obj + k_projectile_data_offset))->flags &
         _projectile_super_detonation_counted_bit) == 0 &&
        obj->parent_object != (datum_index)k_datum_index_none) {

        object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;
        datum_index first_child = parent->first_child_object;
        datum_index cursor;
        int16_t sibling_count = 0;

        cursor = first_child;
        while (cursor != (datum_index)k_datum_index_none) {
            object *sibling = ((object_header *)object_data->data)[cursor & 0xffff].data;
            projectile_data *sibling_proj = (projectile_data *)((uint8_t *)sibling + k_projectile_data_offset);
            if (sibling->definition_tag == obj->definition_tag &&
                (sibling_proj->flags & _projectile_super_detonation_counted_bit) == 0) {
                sibling_count = sibling_count + 1;
            }
            cursor = sibling->next_object;
        }

        if (parent->type == _object_type_biped &&
            (((unit_data *)((uint8_t *)parent + k_unit_data_offset))->controlling_player == (datum_index)k_datum_index_none ||
             current_game_engine != 0) &&
            sibling_count > k_projectile_super_combine_detonate_threshold) {

            cursor = first_child;
            while (cursor != (datum_index)k_datum_index_none) {
                object *sibling = ((object_header *)object_data->data)[cursor & 0xffff].data;
                projectile_data *sibling_proj = (projectile_data *)((uint8_t *)sibling + k_projectile_data_offset);
                if (sibling->definition_tag == obj->definition_tag &&
                    (sibling_proj->flags & _projectile_super_detonation_counted_bit) == 0) {
                    if (sibling_count < k_projectile_super_combine_detonate_threshold + 1) {
                        sibling_proj->flags |= _projectile_super_detonation_counted_bit;
                        sibling_proj->detonation_timer = random_real() * sibling_proj->detonation_timer;
                        sibling_proj->arming_timer = random_real() * sibling_proj->arming_timer;
                    } else {
                        sibling_proj->detonation_timer = 0.0f;
                        sibling_proj->arming_timer = 0.0f;
                    }
                    sibling_count = sibling_count - 1;
                }
                cursor = sibling->next_object;
            }

            effect_tag_id = *(datum_index *)&tag->super_detonation.tag_id;

            // The relink uses its OWN point buffer (Ghidra's local_90/8c/88), not the effect
            // position block at local_a8 that object_get_position refills further down.
            // 0x4c0803: hold the parent's position, detach, then relink inside the parent and sweep back
            // out to where the projectile was stuck
            real_point3d parent_position;   // [esp+0xa4]

            object_get_position(&parent_position, obj->parent_object);
            object_snap_to_parent_marker_and_detach(object_index);
            relink_position = ((object_header *)object_data->data)[object_index & 0xffff].data->position;
            object_set_position_and_relink(&parent_position, object_index, 0);
            object_reposition_to_spawn_location(object_index, &relink_position, k_datum_index_none);
            object_recalculate_bounding_radius_recursive(object_index);
        }
    }

    {
        projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
        if (first_collision != 0 && proj->contrail_attachment_index != -1 &&
            obj->attachment_handles[proj->contrail_attachment_index] != (datum_index)k_datum_index_none) {
            object_recalculate_bounding_radius(object_index);
            // FIXED (0x4c0895..0x4c08ad): EDI = the contrail attachment handle
            contrail_advance(obj->attachment_handles[proj->contrail_attachment_index], 0,
                (1.0f - remaining_tick_fraction) * 0.033333335f);
        }
    }

    {
        real_vector3d forward_scratch; // UNSURE: never read again, see file header

        object_get_position(&position_block[0], object_index); // UNSURE elided destination
        object_get_orientation(&forward_scratch, object_index, &direction_block[0]);
        position_block[1] = position_block[0]; // duplicate point into the second slot
        direction_block[1] = *global_down3d_pointer;

        effect_new_with_color(effect_tag_id, obj->creator_object, 0, 2, effect_names, position_block,
                     direction_block, 0, 0, 0, 0, 1);
    }

    if (obj->parent_object != (datum_index)k_datum_index_none &&
        *(int32_t *)&tag->attached_detonation_damage.tag_id != -1) {
        damage_data dd;
        uint8_t *zero = (uint8_t *)&dd;
        int32_t i;

        for (i = 0; i < (int32_t)sizeof(dd); i++) {
            zero[i] = 0;
        }
        dd.flags |= 0x08;
        dd.responsible_player = (datum_index)k_datum_index_none;
        dd.responsible_object = (datum_index)k_datum_index_none;
        dd.team_index = -1;
        dd.location_cluster_index = -1;
        dd.material_type = -1; // Ghidra's `local_38 = 0xffff`, damage_data + 0x4c
        dd.random_blend = 1.0f;
        dd.multiplier = 1.0f;
        dd.damage_effect_tag = *(datum_index *)&tag->attached_detonation_damage.tag_id;

        object_get_orientation(0, object_index, 0); // UNSURE: result discarded, see file header
        object_get_position(&dd.epicentre, object_index); // UNSURE elided destination
        dd.origin = dd.epicentre;
        dd.responsible_object = obj->creator_object;   // creating object (0xc4); overwrites the -1 above
        dd.responsible_player = obj->owner_linkage; // owner_linkage (0xc0); overwrites the -1 above
        dd.team_index = (int16_t)obj->owner_team; // UNSURE, see file header -- overwrites the -1 above

        object_apply_damage(&dd, obj->parent_object, -1, -1, -1, 0);
    }

    {
        projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
        int16_t index = proj->material_response_index;

        if (index != -1) {
            ProjectileMaterialResponse *response;

            if (index < 0 || tag->projectile_material_response.count <= (uint32_t)index) {
                response = &projectile_default_material_response;
            } else {
                response = (ProjectileMaterialResponse *)tag->projectile_material_response.pointer + index;
            }
            effect_new_with_color(*(uint32_t *)&response->detonation_effect.tag_id, obj->creator_object, 0, 2,
                         effect_names, position_block, direction_block, 0, 0, 0, 0, 1);
        }
    }

    ai_accumulate_repeated_event(object_index, &position_block[0], 2, tag->detonation_noise, 1);
}

#if 0
Original Ghidra decompilation (0x4c0670):

void item_detonate(char param_1,float param_2)

{
  short sVar1;
  int iVar2;
  uint *puVar3;
  int iVar4;
  uint *puVar5;
  uint uVar6;
  undefined *puVar7;
  int iVar8;
  uint unaff_EBX;
  int *piVar9;
  float fVar10;
  undefined4 local_b8;
  undefined1 *local_b4;
  char *local_b0;
  uint local_ac;
  undefined4 local_a8;
  undefined4 local_a4;
  undefined4 local_a0;
  undefined4 local_9c;
  undefined4 local_98;
  undefined4 local_94;
  uint local_90;
  uint local_8c;
  uint local_88;
  int local_84 [4];
  undefined2 local_74;
  undefined2 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_44;
  undefined4 local_40;
  undefined2 local_38;
  undefined1 local_28 [12];
  undefined1 local_1c [12];
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;

  iVar2 = *(int *)(DAT_008603b0 + 0x34);
  puVar3 = *(uint **)(iVar2 + 8 + (unaff_EBX & 0xffff) * 0xc);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_b4 = &DAT_0065512c;
  local_b0 = "gravity";
  local_b8 = *(undefined4 *)(iVar4 + 0x1b8);
  if ((((*(byte *)(iVar4 + 0x17c) & 8) != 0) && ((puVar3[0x8b] & 0x40) == 0)) &&
     (puVar3[0x47] != 0xffffffff)) {
    iVar8 = *(int *)(iVar2 + 8 + (puVar3[0x47] & 0xffff) * 0xc);
    sVar1 = 0;
    local_ac = *(uint *)(iVar8 + 0x118);
    uVar6 = local_ac;
    while (uVar6 != 0xffffffff) {
      puVar5 = *(uint **)(iVar2 + 8 + (uVar6 & 0xffff) * 0xc);
      if ((*puVar5 == *puVar3) && ((puVar5[0x8b] & 0x40) == 0)) {
        sVar1 = sVar1 + 1;
      }
      uVar6 = puVar5[0x45];
    }
    if (((*(short *)(iVar8 + 0xb4) == 0) && ((*(int *)(iVar8 + 0x218) == -1 || (DAT_006f1d20 != 0)))
        ) && (uVar6 = local_ac, 6 < sVar1)) {
      while (uVar6 != 0xffffffff) {
        puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
        if ((*puVar5 == *puVar3) && ((puVar5[0x8b] & 0x40) == 0)) {
          if (sVar1 < 7) {
            puVar5[0x8b] = puVar5[0x8b] | 0x40;
            fVar10 = random_real();
            puVar5[0x90] = (uint)(fVar10 * (float)puVar5[0x90]);
            fVar10 = random_real();
            puVar5[0x92] = (uint)(fVar10 * (float)puVar5[0x92]);
          }
          else {
            puVar5[0x90] = 0;
            puVar5[0x92] = 0;
          }
          sVar1 = sVar1 + -1;
        }
        uVar6 = puVar5[0x45];
      }
      local_b8 = *(undefined4 *)(iVar4 + 0x198);
      object_get_position();
      FUN_004f6610();
      local_90 = puVar3[0x17];
      local_8c = puVar3[0x18];
      local_88 = puVar3[0x19];
      object_set_position_and_relink(0);
      FUN_004f7b70();
      object_recalculate_bounding_radius_recursive();
    }
  }
  if (((param_1 != '\0') && (puVar3[0x8f] != 0xffffffff)) &&
     (puVar3[puVar3[0x8f] + 0x53] != 0xffffffff)) {
    object_recalculate_bounding_radius();
    FUN_0044ca60(0,(1.0 - param_2) * 0.033333335);
  }
  object_get_position();
  object_get_orientation(local_28);
  local_9c = local_a8;
  local_94 = local_a0;
  local_98 = local_a4;
  local_10 = *(undefined4 *)PTR_DAT_0069672c;
  local_c = *(undefined4 *)(PTR_DAT_0069672c + 4);
  local_8 = *(undefined4 *)(PTR_DAT_0069672c + 8);
  FUN_00450980(local_b8,puVar3[0x31],0,2,&local_b4,&local_a8,local_1c,0,0,0,0,1);
  if ((puVar3[0x47] != 0xffffffff) && (iVar2 = *(int *)(iVar4 + 0x220), iVar2 != -1)) {
    piVar9 = local_84;
    for (iVar8 = 0x15; iVar8 != 0; iVar8 = iVar8 + -1) {
      *piVar9 = 0;
      piVar9 = piVar9 + 1;
    }
    local_84[1] = local_84[1] | 8;
    local_38 = 0xffff;
    local_84[2] = 0xffffffff;
    local_84[3] = 0xffffffff;
    local_74 = 0xffff;
    local_6c = 0xffff;
    local_44 = 0x3f800000;
    local_40 = 0x3f800000;
    local_84[0] = iVar2;
    object_get_orientation(0);
    object_get_position();
    local_5c = local_68;
    local_84[3] = puVar3[0x31];
    local_58 = local_64;
    local_84[2] = puVar3[0x30];
    local_54 = local_60;
    local_74 = (undefined2)puVar3[0x2e];
    object_apply_damage(local_84,puVar3[0x47],0xffffffff,0xffffffff,0xffffffff,0);
  }
  sVar1 = *(short *)((int)puVar3 + 0x232);
  if (sVar1 != -1) {
    if ((sVar1 < 0) || (*(int *)(iVar4 + 0x240) <= (int)sVar1)) {
      puVar7 = &DAT_00695e20;
    }
    else {
      puVar7 = (undefined *)(sVar1 * 0xa0 + *(int *)(iVar4 + 0x244));
    }
    FUN_00450980(*(undefined4 *)(puVar7 + 0x74),puVar3[0x31],0,2,&local_b4,&local_a8,local_1c,0,0,0,
                 0,1);
  }
  FUN_0042c610();
  return;
}
#endif
