// object_apply_shield_damage
// address 0x4ef820, size 976 bytes
// name confidence: 0.6 (Ghidra-recovered name)
// rewrite confidence: 0.25
// evidence: types/objects.h object (shield_vitality 0xe4, current_shield_damage 0xe8,
// recent_shield_damage 0xf4, shield_damage_ticks 0xfc, shield_stun_ticks 0x104, vitality_flags
// 0x106, maximum_shield_vitality 0xdc); types/tags.h ModelCollisionGeometry (shield_damaged_threshold
// 0x184, shield_failure_threshold 0xf0, failing_shield_leak_fraction 0xf4, minimum_stun_damage
// 0x108, shield_material_type 0xd2, flags bit2 == always_shields_friendly_damage),
// ModelCollisionGeometryMaterial (shield_leak_percentage 0x28, shield_damage_multiplier 0x2c),
// DamageEffect's per-material-type multiplier table (same as object_apply_body_damage.c, indexed
// here by geometry->shield_material_type instead of the material's own material_type).
// UNSURE: the decompiled entry writes through an `unaff_EBX` pointer that appears nowhere in
// Ghidra's own parameter list (`*(undefined4*)(unaff_EBX+4)` and `*(undefined1*)(unaff_EBX+8)`),
// meaning a 10th argument is passed in EBX by a caller further up the chain than
// object_apply_damage itself (which never shows touching EBX either). It is modeled here as an
// explicit trailing out-parameter, `impulse_result`, and object_apply_damage.c currently passes
// 0 for it with its own UNSURE note, since the true source is not recoverable from this module.
// transition_function_evaluate (0x4ccac0), weapon_get_zoom_fov, weapon_get_zoom_fov_resolved and FUN_006391b4 are opaque externals outside this
// module (FUN_006391b4 is explicitly unresolved even in out/phase4/objects_types_notes.md).
// register convention: all parameters on the stack; the 4th (`effect_offset`) is received here
// as the DamageEffect base pointer under the same -0x1c4 re-basing used throughout this batch.
// blam-cc: stack=(target_index, geometry, material, effect, notify_flags, shield_damage_out,
//   remaining_damage_inout, role_is_deletable, attributable_to_live_player, impulse_result)
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

extern real weapon_get_zoom_fov(int32_t param_1); // UNSURE: out of range, 0x46fe10
extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX table, AX team: difficulty scale
extern real transition_function_evaluate(transition_function_t type, real phase); // math
    // module, 0x4ccac0. The transition type travels in CX and is not visible at this call
    // site; the one value Ghidra shows pushed is the phase. UNSURE: type passed as 0.
extern void object_set_shield_depleted_flag(uint32_t object_index); // this module, 0x4edb10
extern void object_dispatch_effect_notify(void); // this module, 0x4efff0
extern int32_t __ftol(); // 0x006391b4, MSVC 7.1 CRT x87 float-to-int truncation
    // (verified by disassembling 0x006391b4: fld st(0) / fst [esp+0x18] / fistp qword /
    // fild qword ... , the classic _ftol2 body). The value arrives on the x87 stack, so
    // some call sites show a visible float argument and others show none; the empty
    // parameter list asserts no prototype, the same convention this module already uses
    // for FUN_00450870.

void object_apply_shield_damage(uint32_t target_index, ModelCollisionGeometry *geometry,
    ModelCollisionGeometryMaterial *material, DamageEffect *effect, uint32_t *notify_flags,
    float *shield_damage_out, float *remaining_damage, int8_t role_is_deletable,
    int8_t attributable_to_live_player, object_shield_impulse_result *impulse_result)
{
    object_header *headers = (object_header *)object_data->data;
    object *target = headers[target_index & 0xffff].data;
    float total_damage = *remaining_damage;
    float leaked_damage; // local_14
    float body_passthrough; // local_10

    if (impulse_result != 0) {
        impulse_result->shield_damage_dealt = 0.0f;
        impulse_result->depleted_this_call = 0;
    }

    if (target->shield_vitality <= 0.0f) {
        leaked_damage = 0.0f;
        body_passthrough = total_damage;
        if (role_is_deletable != 1) {
            goto done;
        }
        target->shield_vitality = 0.0f;
    } else {
        float max_shield_vitality = target->maximum_shield_vitality;
        float inv_max_shield_vitality;
        uint8_t friendly_shield_immune;

        if (!(current_game_engine == 0 && effect->damage_category == 1 && target->owner_team == 1)) {
            max_shield_vitality = weapon_get_zoom_fov_resolved(2, target->owner_team) * max_shield_vitality; // 0x4ef8ae
        }
        inv_max_shield_vitality = (max_shield_vitality <= 0.0f) ? 0.0f : (1.0f / max_shield_vitality);

        friendly_shield_immune = ((*notify_flags & 0x10) != 0) && ((geometry->flags & 4) != 0);
        if (friendly_shield_immune) {
            leaked_damage = total_damage;
        } else {
            leaked_damage = (1.0f - material->shield_leak_percentage) * total_damage;
            if (target->shield_vitality <= geometry->shield_failure_threshold &&
                0.0f < geometry->shield_failure_threshold) {
                real ratio = transition_function_evaluate(0, target->shield_vitality / geometry->shield_failure_threshold);
                leaked_damage = ((1.0f - geometry->failing_shield_leak_fraction) * ratio +
                    geometry->failing_shield_leak_fraction) * leaked_damage;
            }
        }

        if ((target->vitality_flags & _object_shield_recharging_bit) == 0) {
            float scaled_damage;

            if (leaked_damage < 0.0f) {
                leaked_damage = 0.0f;
            }
            body_passthrough = total_damage - leaked_damage;

            if ((*notify_flags & 0x10) != 0 && (*notify_flags & 0x20) != 0) {
                real scalar = weapon_get_zoom_fov(0);
                if (0.0f < scalar) {
                    leaked_damage = leaked_damage / scalar;
                }
            }

            scaled_damage = leaked_damage * material->shield_damage_multiplier *
                (&effect->dirt)[geometry->shield_material_type];

            if (target->shield_vitality < inv_max_shield_vitality * scaled_damage || effect->damage_side_effect == 3) {
                float overflow = scaled_damage - max_shield_vitality * target->shield_vitality;

                if (0.0f < overflow) {
                    body_passthrough = overflow + body_passthrough;
                }
                if (role_is_deletable == 1) {
                    target->shield_vitality = 0.0f;
                }
                if ((target->vitality_flags & _object_shield_depleted_bit) == 0 && attributable_to_live_player == 1) {
                    object_set_shield_depleted_flag(target_index);
                    *notify_flags |= 8;
                    if (impulse_result != 0) {
                        impulse_result->depleted_this_call = 1;
                    }
                }
            } else {
                if (role_is_deletable == 1 && (target->vitality_flags & _object_hash_flag_bit) == 0) {
                    target->shield_vitality -= inv_max_shield_vitality * scaled_damage;
                }
                if ((target->vitality_flags & _object_shield_below_low_bit) == 0 &&
                    target->shield_vitality < geometry->shield_damaged_threshold) {
                    object_dispatch_effect_notify();
                    target->vitality_flags |= _object_shield_below_low_bit;
                }
            }

            if (0.0001f <= scaled_damage) {
                goto shield_stun_section;
            }
        } else {
            body_passthrough = 0.0f;
            leaked_damage = total_damage;
            goto shield_stun_section;
        }
        goto after_stun_section;

shield_stun_section:
        if (attributable_to_live_player == 1) {
            float dt;

            total_damage = *remaining_damage;
            target->shield_damage_ticks = 0;
            dt = (total_damage - body_passthrough) * inv_max_shield_vitality;
            if ((target->vitality_flags & _object_shield_depleted_bit) == 0) {
                target->current_shield_damage = 1.0f;
            }
            {
                float sum = dt + target->recent_shield_damage;
                target->recent_shield_damage = sum;
                if (1.0f < target->current_shield_damage) {
                    target->current_shield_damage = 1.0f;
                }
                if (1.0f < sum) {
                    target->recent_shield_damage = 1.0f;
                }
            }
            if (impulse_result != 0) {
                impulse_result->shield_damage_dealt = dt;
            }
        }
after_stun_section:
        if (role_is_deletable != 1) {
            goto done;
        }
    }

    if (geometry->minimum_stun_damage <= leaked_damage || target->shield_vitality == 0.0f) {
        target->shield_stun_ticks = (int16_t)__ftol();
    }

done:
    *shield_damage_out = leaked_damage;
    *remaining_damage = body_passthrough;
}

#if 0
Original Ghidra decompilation (0x4ef820):

void object_apply_shield_damage
               (uint param_1,byte *param_2,int param_3,short *param_4,uint *param_5,float *param_6,
               float *param_7,char param_8,char param_9)

{
  int iVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  int iVar5;
  undefined2 uVar6;
  int iVar7;
  int unaff_EBX;
  bool bVar8;
  float10 fVar9;
  float local_14;
  float local_10;
  float local_c;
  float local_8;

  iVar5 = DAT_008603b0;
  iVar7 = (param_1 & 0xffff) * 0xc;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
  fVar2 = *param_7;
  bVar4 = false;
  bVar8 = DAT_006f1d20 == 0;
  *(undefined4 *)(unaff_EBX + 4) = 0;
  *(undefined1 *)(unaff_EBX + 8) = 0;
  if (((bVar8) && (param_4[1] == 1)) && (*(short *)(iVar1 + 0xb8) == 1)) {
    bVar4 = true;
  }
  if (*(float *)(iVar1 + 0xe4) <= 0.0) {
    local_14 = 0.0;
    local_10 = fVar2;
    if (param_8 != '\x01') goto LAB_004efbba;
    *(undefined4 *)(iVar1 + 0xe4) = 0;
  }
  else {
    local_c = *(float *)(*(int *)(*(int *)(iVar5 + 0x34) + 8 + iVar7) + 0xdc);
    if (!bVar4) {
      fVar9 = (float10)FUN_0046fe70();
      local_c = (float)(fVar9 * (float10)local_c);
    }
    if (local_c <= 0.0) {
      local_8 = 0.0;
    }
    else {
      local_8 = 1.0 / local_c;
    }
    if ((((*param_5 & 0x10) == 0) || (local_14 = fVar2, (*param_2 & 4) == 0)) &&
       ((local_14 = (1.0 - *(float *)(param_3 + 0x28)) * fVar2,
        *(float *)(iVar1 + 0xe4) < *(float *)(param_2 + 0xf0) !=
        (*(float *)(iVar1 + 0xe4) == *(float *)(param_2 + 0xf0)) &&
        (0.0 < *(float *)(param_2 + 0xf0))))) {
      fVar9 = (float10)FUN_004ccac0(*(float *)(iVar1 + 0xe4) / *(float *)(param_2 + 0xf0));
      local_14 = (float)((((float10)1.0 - (float10)*(float *)(param_2 + 0xf4)) * fVar9 +
                         (float10)*(float *)(param_2 + 0xf4)) * (float10)local_14);
    }
    if ((*(byte *)(iVar1 + 0x106) & 0x10) == 0) {
      if (local_14 < 0.0) {
        local_14 = 0.0;
      }
      local_10 = fVar2 - local_14;
      if ((((*param_5 & 0x10) != 0) && ((*param_5 & 0x20) != 0)) &&
         (fVar9 = (float10)FUN_0046fe10(0), (float10)0.0 < fVar9)) {
        local_14 = (float)((float10)local_14 / fVar9);
      }
      fVar2 = local_14 * *(float *)(param_3 + 0x2c) *
              *(float *)(param_4 + *(short *)(param_2 + 0xd2) * 2 + 0x1e);
      if ((*(float *)(iVar1 + 0xe4) < local_8 * fVar2) || (*param_4 == 3)) {
        fVar3 = fVar2 - local_c * *(float *)(iVar1 + 0xe4);
        if (0.0 < fVar3) {
          local_10 = fVar3 + local_10;
        }
        if (param_8 == '\x01') {
          *(undefined4 *)(iVar1 + 0xe4) = 0;
        }
        if (((*(byte *)(iVar1 + 0x106) & 8) == 0) && (param_9 == '\x01')) {
          FUN_004edb10();
          *param_5 = *param_5 | 8;
          *(undefined1 *)(unaff_EBX + 8) = 1;
        }
      }
      else {
        if ((param_8 == '\x01') && ((*(ushort *)(iVar1 + 0x106) & 0x800) == 0)) {
          *(float *)(iVar1 + 0xe4) = *(float *)(iVar1 + 0xe4) - local_8 * fVar2;
        }
        if (((*(byte *)(iVar1 + 0x106) & 2) == 0) &&
           (*(float *)(iVar1 + 0xe4) < *(float *)(param_2 + 0x184))) {
          FUN_004efff0();
          *(byte *)(iVar1 + 0x106) = *(byte *)(iVar1 + 0x106) | 2;
        }
      }
      if (0.0001 <= fVar2) goto LAB_004efb04;
    }
    else {
      local_10 = 0.0;
      local_14 = fVar2;
LAB_004efb04:
      if (param_9 == '\x01') {
        fVar2 = *param_7;
        *(undefined4 *)(iVar1 + 0xfc) = 0;
        local_8 = (fVar2 - local_10) * local_8;
        if ((*(byte *)(iVar1 + 0x106) & 8) == 0) {
          *(undefined4 *)(iVar1 + 0xe8) = 0x3f800000;
        }
        fVar2 = local_8 + *(float *)(iVar1 + 0xf4);
        *(float *)(iVar1 + 0xf4) = fVar2;
        if (1.0 < *(float *)(iVar1 + 0xe8)) {
          *(undefined4 *)(iVar1 + 0xe8) = 0x3f800000;
        }
        if (1.0 < fVar2) {
          *(undefined4 *)(iVar1 + 0xf4) = 0x3f800000;
        }
        *(float *)(unaff_EBX + 4) = local_8;
      }
    }
    if (param_8 != '\x01') goto LAB_004efbba;
  }
  if ((*(float *)(param_2 + 0x108) <= local_14) || (*(float *)(iVar1 + 0xe4) == 0.0)) {
    uVar6 = FUN_006391b4();
    *(undefined2 *)(iVar1 + 0x104) = uVar6;
  }
LAB_004efbba:
  *param_6 = local_14;
  *param_7 = local_10;
  return;
}
#endif
