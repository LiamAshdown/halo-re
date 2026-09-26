// object_apply_body_damage
// address 0x4ef2a0, size 1403 bytes
// name confidence: 0.6 (Ghidra-recovered name)
// rewrite confidence: 0.25
// evidence: types/objects.h object (type 0xb4, maximum_body_vitality 0xd8, body_vitality 0xe0,
// current_body_damage 0xec, recent_body_damage 0xf8, body_damage_ticks 0x100,
// destroyed_region_flags 0x174, region_vitality[8] 0x178, vitality_flags 0x106/0x107,
// first_child_object 0x118, next_object 0x114), damage_data (flags 0x04); types/tags.h
// ModelCollisionGeometry (flags, friendly_damage_resistance 0x44, regions TagReflexive 0x240),
// ModelCollisionGeometryRegion (flags 0x20, damage_threshold 0x28), ModelCollisionGeometryMaterial
// (flags 0x20, body_damage_multiplier 0x3c), DamageEffect (damage_category, damage_flags,
// the per-material-type multiplier array starting at damage_instantaneous_acceleration+0xc,
// i.e. the `dirt` field, indexed by MaterialType_t).
// UNSURE (function-wide, same caveats as object_apply_damage.c): unit-extension fields at
// object+0x218/+0x324/+0xb8 are not part of the common object struct; FUN_006391b4 (the
// producer of the region-vitality byte) is explicitly unresolved even in
// out/phase4/objects_types_notes.md; weapon_get_zoom_fov_resolved/weapon_get_zoom_fov/effect_new_on_object/FUN_004eda20/
// FUN_004edc80 are called with only their visible arguments preserved.
// register convention: all parameters on the stack; the 7th (`effect_offset`) is passed by the
// caller as a pointer already offset to DamageEffect+0x1c4, but here it is received as the
// DamageEffect base pointer instead (a consistent re-basing shared with object_apply_damage.c
// and object_apply_shield_damage.c, so every offset below is translated by -0x1c4 relative to
// the original decompile).
// blam-cc: stack=(target_index, region_index, node_index, param_4_masked, geometry, material,
//   effect, dd, notify_flags, body_damage_out, param11_out, remaining_damage, role_is_deletable)
// reconciled: R29 raw object +0xb8 int16 read -> target->owner_team
// reconciled: R04 0x006f1d20 uint8_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern game_engine_definition *current_game_engine;      // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t g_0087abc0;      // 0x0087abc0, UNSURE: not owned by this module

extern void effect_new_on_object(); // effects module, 0x4507a0
    // The convention of this foreign callee is not established: different call sites in this
    // module pass different numbers of visible arguments, and it also takes values in EAX
    // and ECX that the decompiler never models. Declared with an empty parameter list so
    // every site in the module agrees on ONE declaration without fabricating arguments. // UNSURE: effects module, 0x4507a0
extern real weapon_get_zoom_fov(int32_t param_1); // UNSURE: out of range, 0x46fe10
extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX table, AX team: difficulty scale
extern void object_set_health_frozen_flag(void); // UNSURE: zero visible args; this module (object_set_health_frozen_flag)
extern void object_delete_teardown(void); // UNSURE: zero visible args; this module (object_delete_teardown)
extern void damage_effect_new_at_location(datum_index effect_tag, int16_t node_index,
    real_vector3d *normal, real_vector3d *incident, real_point3d *impact_position,
    uint32_t object_index); // this module, 0x4f0010. Only the first two arguments are visible
    // at this call site; the remaining four travel in registers. UNSURE: passed as NULL/0.
extern void object_destroy_region(uint32_t object_index, int32_t region_index); // this module,
    // 0x4f02d0. The object index travels in EAX and is not visible at these call sites; only
    // the region index is pushed.
extern int32_t __ftol(); // 0x006391b4, MSVC 7.1 CRT x87 float-to-int truncation
    // (verified by disassembling 0x006391b4: fld st(0) / fst [esp+0x18] / fistp qword /
    // fild qword ... , the classic _ftol2 body). The value arrives on the x87 stack, so
    // some call sites show a visible float argument and others show none; the empty
    // parameter list asserts no prototype, the same convention this module already uses
    // for FUN_00450870.

void object_apply_body_damage(uint32_t target_index, int32_t region_index, int32_t node_index,
    uint32_t param_4, ModelCollisionGeometry *geometry, ModelCollisionGeometryMaterial *material,
    DamageEffect *effect, damage_data *dd, uint32_t *notify_flags, float *body_damage_out,
    uint32_t *param11_out, float remaining_damage, int8_t role_is_deletable)
{
    object_header *headers = (object_header *)object_data->data;
    object *target = headers[target_index & 0xffff].data;
    float raw_damage = remaining_damage * material->body_damage_multiplier;
    int8_t friendly_fire_exempt = 0;
    float max_body_vitality;
    float inv_max_body_vitality;
    float fVar10;
    float normalized_damage;
    uint32_t flags;

    if ((geometry->flags & 0x40) != 0 && target->type == _object_type_vehicle &&
        *(int32_t *)((uint8_t *)target + 0x324) == -1) { // UNSURE: unit extension field
        raw_damage = 0.0f;
    }

    if (current_game_engine == 0 && effect->damage_category == 1 &&
        target->owner_team == 1) { // UNSURE: raw object field
        friendly_fire_exempt = 1;
    }

    max_body_vitality = target->maximum_body_vitality;
    if (!friendly_fire_exempt) {
        max_body_vitality = weapon_get_zoom_fov_resolved(1, target->owner_team) * max_body_vitality; // 0x4ef32b, ECX still 1
    }
    inv_max_body_vitality = (max_body_vitality <= 0.0f) ? 0.0f : (1.0f / max_body_vitality);

    fVar10 = raw_damage;
    flags = *notify_flags;
    if ((flags & 0x10) != 0) {
        fVar10 = (1.0f - geometry->friendly_damage_resistance) * raw_damage;
        if ((flags & 0x20) != 0) {
            real scalar = weapon_get_zoom_fov(0);
            if (scalar <= 0.0f) {
                fVar10 = fVar10;
            } else {
                fVar10 = fVar10 / scalar;
            }
        }
    }

    normalized_damage = fVar10 * inv_max_body_vitality *
        (&effect->dirt)[material->material_type]; // per-material-type multiplier table

    if ((target->vitality_flags & _object_hash_flag_bit) == 0) {
        if (0.0f < raw_damage && (material->flags & 1) != 0) {
            if ((effect->damage_flags & 2) == 0) {
                if ((effect->damage_flags & 0x800) != 0 && current_game_engine != 0) {
                    normalized_damage = normalized_damage + normalized_damage;
                    if (target->body_vitality < normalized_damage) {
                        *notify_flags = flags | 0x80;
                    }
                }
            } else if (current_game_engine != 0 || target->type != _object_type_biped ||
                       *(int32_t *)((uint8_t *)target + 0x218) == -1) { // UNSURE: unit extension field
                if (role_is_deletable == 1) {
                    target->body_vitality = 0.0f;
                }
                flags = *notify_flags;
                *notify_flags = flags | 0x40;
                if (current_game_engine != 0) {
                    *notify_flags = flags | 0xc0;
                }
            }
        }
        if (role_is_deletable != 1) {
            goto after_vitality;
        }
        target->body_vitality -= normalized_damage;
    } else if (role_is_deletable != 1) {
        goto after_vitality;
    }

    if (region_index != -1 &&
        (1 << (region_index & 0x1f) & (uint32_t)target->destroyed_region_flags) == 0) {
        ModelCollisionGeometryRegion *region = &((ModelCollisionGeometryRegion *)geometry->regions.pointer)[region_index];
        uint8_t region_byte = __ftol();

        target->region_vitality[region_index] = region_byte;
        if (0.0f < region->damage_threshold && region->damage_threshold < (float)region_byte * 0.003921569f) {
            object_destroy_region(target_index, region_index);
            *notify_flags |= 2;
        }
    }

after_vitality:
    target->body_damage_ticks = 0;
    {
        float sum1 = normalized_damage + target->current_body_damage;
        float sum2;

        target->current_body_damage = sum1;
        sum2 = normalized_damage + target->recent_body_damage;
        target->recent_body_damage = sum2;
        if (1.0f < sum1) {
            target->current_body_damage = 1.0f;
        }
        if (1.0f < sum2) {
            target->recent_body_damage = 1.0f;
        }
    }

    if (g_0087abc0 != 0 && target->body_vitality < 0.0f &&
        (1 << (target->type & 0x1f) & _object_mask_unit) != 0) {
        if (*(int32_t *)((uint8_t *)target + 0x218) == -1) { // UNSURE: unit extension field
            if (target->type == _object_type_vehicle) {
                uint32_t walker = target->first_child_object;
                int8_t found = 0;

                while (walker != (datum_index)0xffffffff) {
                    object *child = headers[walker & 0xffff].data;
                    if ((1 << (child->type & 0x1f) & _object_mask_unit) != 0 &&
                        *(int32_t *)((uint8_t *)child + 0x218) != -1) { // UNSURE
                        found = 1;
                        break;
                    }
                    walker = child->next_object;
                }
                if (found) {
                    target->body_vitality = 0.0f;
                }
            }
        } else {
            target->body_vitality = 0.0f;
        }
    }

    if (role_is_deletable == 1) {
        object *self = headers[target_index & 0xffff].data;
        float max_v = self->maximum_body_vitality;
        float cur_v = self->body_vitality;
        real scalar = weapon_get_zoom_fov_resolved(1, self->owner_team); // 0x4ef689
        real threshold = scalar * max_v * cur_v;

        if (0.0f <= geometry->body_destroyed_threshold || (real)geometry->body_destroyed_threshold <= threshold) {
            if (0.0f <= threshold) {
                if (threshold < geometry->body_damaged_threshold &&
                    (target->vitality_flags & 1) == 0) {
                    effect_new_on_object(target_index, 0xffffffff, 0, 0, 0, 0);
                    target->vitality_flags |= 1;
                }
            } else if ((target->vitality_flags & _object_health_frozen_bit) == 0) {
                int32_t region_count = geometry->regions.count;
                int32_t i;

                for (i = 0; i < region_count; i++) {
                    ModelCollisionGeometryRegion *region = &((ModelCollisionGeometryRegion *)geometry->regions.pointer)[i];
                    if ((region->flags & 4) != 0) {
                        object_destroy_region(target_index, i);
                    }
                }
                object_set_health_frozen_flag();
                *notify_flags |= 1;
            }
        } else {
            object_delete_teardown();
            *notify_flags |= 5;
        }

        if ((effect->damage_flags & 2) != 0 && geometry->localized_damage_effect.tag_id.index != 0xffff) {
            damage_effect_new_at_location(geometry->localized_damage_effect.tag_id.index,
                                          (int16_t)node_index, 0, 0, 0, 0);
        }

        if ((effect->damage_flags & 1) != 0 && geometry->area_damage_effect_threshold < raw_damage &&
            geometry->area_damage_effect.tag_id.index != 0xffff && effect->damage_category != 7) {
            effect_new_on_object(target_index, 0xffffffff, 0, 0, 0, 0);
        }
    }

    *body_damage_out = raw_damage;
    *param11_out = *(uint32_t *)&material->body_damage_multiplier;
}

#if 0
Original Ghidra decompilation (0x4ef2a0):

void object_apply_body_damage
               (uint param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,byte *param_5,
               int param_6,int param_7,int param_8,uint *param_9,float *param_10,
               undefined4 *param_11,float param_12,char param_13)

{
  uint uVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  byte bVar5;
  short sVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  float10 fVar11;
  float local_8;
  float local_4;

  local_8 = param_12 * *(float *)(param_6 + 0x3c);
  iVar9 = (param_1 & 0xffff) * 0xc;
  iVar8 = *(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34));
  bVar4 = false;
  if ((((*param_5 & 0x40) != 0) && (*(short *)(iVar8 + 0xb4) == 1)) &&
     (*(int *)(iVar8 + 0x324) == -1)) {
    local_8 = 0.0;
  }
  if (((DAT_006f1d20 == 0) && (*(short *)(param_7 + 2) == 1)) && (*(short *)(iVar8 + 0xb8) == 1)) {
    bVar4 = true;
  }
  param_12 = *(float *)(iVar8 + 0xd8);
  if (!bVar4) {
    fVar10 = (float10)FUN_0046fe70();
    param_12 = (float)(fVar10 * (float10)param_12);
  }
  if (param_12 <= 0.0) {
    local_4 = 0.0;
  }
  else {
    local_4 = 1.0 / param_12;
  }
  fVar10 = (float10)local_8;
  uVar1 = *param_9;
  if ((uVar1 & 0x10) != 0) {
    fVar10 = ((float10)1.0 - (float10)*(float *)(param_5 + 0x44)) * (float10)local_8;
    if ((uVar1 & 0x20) != 0) {
      fVar11 = (float10)FUN_0046fe10(0);
      if (fVar11 <= (float10)0.0) {
        fVar10 = (float10)(float)fVar10;
      }
      else {
        fVar10 = (float10)(float)fVar10 / fVar11;
      }
    }
  }
  iVar7 = DAT_006f1d20;
  param_12 = (float)(fVar10 * (float10)local_4 *
                    (float10)*(float *)(param_7 + 0x3c + *(short *)(param_6 + 0x24) * 4));
  if ((*(byte *)(iVar8 + 0x107) & 8) == 0) {
    if ((0.0 < local_8) && ((*(byte *)(param_6 + 0x20) & 1) != 0)) {
      if ((*(uint *)(param_7 + 4) & 2) == 0) {
        if ((((*(uint *)(param_7 + 4) & 0x800) != 0) && (DAT_006f1d20 != 0)) &&
           (param_12 = param_12 + param_12, *(float *)(iVar8 + 0xe0) < param_12)) {
          *param_9 = uVar1 | 0x80;
        }
      }
      else if (((DAT_006f1d20 != 0) || (*(short *)(iVar8 + 0xb4) != 0)) ||
              (*(int *)(*(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x218) == -1)) {
        if (param_13 == '\x01') {
          *(undefined4 *)(iVar8 + 0xe0) = 0;
        }
        uVar1 = *param_9;
        *param_9 = uVar1 | 0x40;
        if (iVar7 != 0) {
          *param_9 = uVar1 | 0xc0;
        }
      }
    }
    if (param_13 != '\x01') goto LAB_004ef55f;
    *(float *)(iVar8 + 0xe0) = *(float *)(iVar8 + 0xe0) - param_12;
  }
  else if (param_13 != '\x01') goto LAB_004ef55f;
  sVar6 = (short)param_2;
  if ((sVar6 != -1) && ((1 << ((byte)param_2 & 0x1f) & (uint)*(ushort *)(iVar8 + 0x174)) == 0)) {
    iVar7 = sVar6 * 0x54 + *(int *)(param_5 + 0x244);
    bVar5 = FUN_006391b4();
    *(byte *)(iVar8 + 0x178 + (int)sVar6) = bVar5;
    if ((0.0 < *(float *)(iVar7 + 0x28)) && (*(float *)(iVar7 + 0x28) < (float)bVar5 * 0.003921569))
    {
      object_destroy_region(param_2);
      *param_9 = *param_9 | 2;
    }
  }
LAB_004ef55f:
  *(undefined4 *)(iVar8 + 0x100) = 0;
  fVar2 = param_12 + *(float *)(iVar8 + 0xec);
  *(float *)(iVar8 + 0xec) = fVar2;
  param_12 = param_12 + *(float *)(iVar8 + 0xf8);
  *(float *)(iVar8 + 0xf8) = param_12;
  if (1.0 < fVar2) {
    *(undefined4 *)(iVar8 + 0xec) = 0x3f800000;
  }
  if (1.0 < param_12) {
    *(undefined4 *)(iVar8 + 0xf8) = 0x3f800000;
  }
  if (((DAT_0087abc0 != '\0') && (*(float *)(iVar8 + 0xe0) < 0.0)) &&
     ((1 << ((byte)*(short *)(iVar8 + 0xb4) & 0x1f) & 3U) != 0)) {
    if (*(int *)(*(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x218) == -1) {
      if (*(short *)(iVar8 + 0xb4) == 1) {
        uVar1 = *(uint *)(iVar8 + 0x118);
        while (uVar1 != 0xffffffff) {
          iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
          if (((1 << (*(byte *)(iVar7 + 0xb4) & 0x1f) & 3U) != 0) && (*(int *)(iVar7 + 0x218) != -1)
             ) goto LAB_004ef654;
          uVar1 = *(uint *)(iVar7 + 0x114);
        }
      }
    }
    else {
LAB_004ef654:
      *(undefined4 *)(iVar8 + 0xe0) = 0;
    }
  }
  if (param_13 == '\x01') {
    iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
    fVar2 = *(float *)(iVar9 + 0xd8);
    fVar3 = *(float *)(iVar9 + 0xe0);
    fVar10 = (float10)FUN_0046fe70();
    fVar10 = fVar10 * (float10)fVar2 * (float10)fVar3;
    if ((0.0 <= *(float *)(param_5 + 0xb8)) || ((float10)*(float *)(param_5 + 0xb8) <= fVar10)) {
      if ((float10)0.0 <= fVar10) {
        if ((fVar10 < (float10)*(float *)(param_5 + 0x94)) && ((*(byte *)(iVar8 + 0x106) & 1) == 0))
        {
          FUN_004507a0(param_1,0xffffffff,0,0,0,0);
          *(byte *)(iVar8 + 0x106) = *(byte *)(iVar8 + 0x106) | 1;
        }
      }
      else if ((*(byte *)(iVar8 + 0x106) & 4) == 0) {
        iVar8 = 0;
        if (0 < *(int *)(param_5 + 0x240)) {
          iVar9 = 0;
          do {
            if ((*(byte *)(iVar9 * 0x54 + 0x20 + *(int *)(param_5 + 0x244)) & 4) != 0) {
              object_destroy_region(iVar8);
            }
            iVar8 = iVar8 + 1;
            iVar9 = (int)(short)iVar8;
          } while (iVar9 < *(int *)(param_5 + 0x240));
        }
        FUN_004eda20();
        *param_9 = *param_9 | 1;
      }
    }
    else {
      FUN_004edc80();
      *param_9 = *param_9 | 5;
    }
    if (((*(byte *)(param_8 + 4) & 2) != 0) && (*(int *)(param_5 + 0x7c) != -1)) {
      damage_effect_new_at_location(*(int *)(param_5 + 0x7c),param_3);
    }
    if (((((*(byte *)(param_8 + 4) & 1) != 0) && (*(float *)(param_5 + 0x80) < local_8)) &&
        (*(int *)(param_5 + 0x90) != -1)) && (*(short *)(param_7 + 2) != 7)) {
      FUN_004507a0(param_1,0xffffffff,0,0,0,0);
    }
  }
  *param_10 = local_8;
  *param_11 = *(undefined4 *)(param_6 + 0x3c);
  return;
}
#endif
