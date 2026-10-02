// object_apply_body_damage
// address 0x4ef2a0, size 1403 bytes
// name confidence: 0.6 (Ghidra-recovered name)
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4ef2a0..0x4ef81a and its one caller, object_apply_damage (0x4eefe4). Stack:
//   (target, region, node, hit plane or 0, collision geometry tag, material, damage effect block (tag +0x1c4),
//   damage_data, &notify flags, &body damage out, &material multiplier out, damage, is_local).
//   body = damage * material +0x3c (0 for a driverless vehicle when geometry flag 0x40 is set). The maximum
//   body (+0xd8) is scaled by the difficulty table (0x46fe70, ECX 1, AX team) unless a single-player category 1
//   hit lands on team 1. Friendly damage (notify 0x10) keeps 1 - geometry +0x44 of it, divided by the
//   difficulty multiplier for notify 0x20. The vitality fraction taken is that over the maximum times the
//   effect block's material table (+0x3c[material +0x24]). Unless the object ignores body damage (+0x106 bit
//   0x800): effect flag 2 on geometry flag 1 kills outright (not single-player player bipeds; local zeroes the
//   body, notify 0x40, 0x80 in multiplayer) and effect flag 0x800 doubles it in multiplayer (notify 0x80 when
//   lethal); locally the body (+0xe0) is reduced. Locally a live region (+0x174 bit clear) accumulates
//   damage * 255 in its byte (+0x178) and is destroyed past the region threshold (geometry +0x244 entries of
//   0x54, +0x28; notify 2). The recent body damage (+0xec, +0xf8, ticks +0x100) always updates; the deathless
//   cheat (0x87abc0) keeps player units -- and vehicles carrying one -- at 0. Locally the absolute vitality is
//   tested against the destroyed threshold (+0xb8 when negative: teardown, notify 5), zero (regions flagged 4
//   destroyed, object killed through 0x4eda20, notify 1) or the damaged threshold (+0x94: the damaged effect
//   +0xa4 once, +0x106 bit 1). Damage flag 2 spawns the geometry's hit effect (+0x7c) at the hit
//   (0x4f0010), damage flag 1 above +0x80 the body damage effect (+0x90) unless category 7.
// VERIFIED against disassembly 0x4ef2a0..0x4ef81a (2026-09-30): the one __ftol (0x4ef506) converts
//   taken * 255.0 + (int)region byte, as written below; every branch, callee register/stack argument and store was
//   compared.
// blam-cc: stack=(target_index, region_index, node_index, plane, geometry, material, effect_block, dd,
//   notify_flags, body_damage_out, material_multiplier_out, damage, is_local)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t g_0087abc0;      // 0x0087abc0, the deathless-player cheat
extern game_main_globals *main_game_globals; // 0x006b0b80

extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source); // 0x4507a0, EAX, ECX, stack
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification); // 0x46fe10, stack, CX
extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX, AX
extern void object_set_health_frozen_flag(uint32_t object_index); // 0x4eda20, EAX
extern void object_delete_teardown(uint32_t object_index); // 0x4edc80, EAX
extern void damage_effect_new_at_location(datum_index effect_tag, int16_t node_index, real_vector3d *normal,
    real_vector3d *incident, real_point3d *impact_position, uint32_t object_index); // 0x4f0010, stack, EAX, ECX, EBX, EDI
extern void object_destroy_region(uint32_t object_index, int32_t region_index); // 0x4f02d0, EAX, stack

void object_apply_body_damage(uint32_t target_index, int32_t region_index, int32_t node_index, void *plane,
    uint8_t *geometry, uint8_t *material, uint8_t *effect_block, damage_data *dd, uint32_t *notify_flags,
    float *body_damage_out, float *material_multiplier_out, float damage, uint8_t is_local)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[target_index & 0xffff].data;
    uint8_t *vitality_flags = obj + 0x106;
    float *vitality = (float *)(obj + 0xe0);
    float body = damage * *(float *)(material + 0x3c);
    float maximum;
    float inverse_maximum;
    float value;
    float taken;
    uint8_t unscaled = 0;

    if ((*geometry & 0x40) && ((object *)obj)->type == 1 && *(datum_index *)(obj + 0x324) == k_datum_index_none) {
        body = 0.0f;
    }
    if (current_game_engine == 0 && *(int16_t *)(effect_block + 0x2) == 1 && ((object *)obj)->owner_team == 1) {
        unscaled = 1;
    }
    maximum = ((object *)obj)->maximum_body_vitality;
    if (!unscaled) {
        maximum = weapon_get_zoom_fov_resolved(1, ((object *)obj)->owner_team) * maximum;
    }
    inverse_maximum = (maximum > 0.0f) ? 1.0f / maximum : 0.0f;
    value = body;
    if (*notify_flags & 0x10) {
        value = (1.0f - *(float *)(geometry + 0x44)) * body;
        if (*notify_flags & 0x20) {
            real multiplier = weapon_get_zoom_fov(0, main_game_globals->difficulty);

            if (multiplier > 0.0f) {
                value = value / multiplier;
            }
        }
    }
    taken = value * inverse_maximum * *(float *)(effect_block + 0x3c + *(int16_t *)(material + 0x24) * 4);
    if (*(uint16_t *)vitality_flags & 0x800) {
        if (is_local != 1) {
            goto bookkeeping;
        }
    } else {
        if (body > 0.0f && (*(uint8_t *)(geometry + 0x20) & 1)) {
            uint32_t effect_flags = *(uint32_t *)(effect_block + 0x4);

            if (effect_flags & 2) {
                if (!(current_game_engine == 0 && ((object *)obj)->type == 0 &&
                      *(datum_index *)(obj + 0x218) != k_datum_index_none)) {
                    if (is_local == 1) {
                        *vitality = 0.0f;
                    }
                    *notify_flags |= 0x40;
                    if (current_game_engine != 0) {
                        *notify_flags |= 0x80;
                    }
                }
            } else if ((effect_flags & 0x800) && current_game_engine != 0) {
                taken = taken + taken;
                if (taken > *vitality) {
                    *notify_flags |= 0x80;
                }
            }
        }
        if (is_local != 1) {
            goto bookkeeping;
        }
        *vitality = *vitality - taken;
    }
    if ((int16_t)region_index != -1) {
        int32_t region = (int16_t)region_index;

        if ((((object *)obj)->destroyed_region_flags & (1u << (region & 0x1f))) == 0) {
            uint8_t *region_block = *(uint8_t **)(geometry + 0x244) + region * 0x54;
            uint8_t region_damage = (uint8_t)(int32_t)(taken * 255.0f + (int32_t)obj[0x178 + region]);

            obj[0x178 + region] = region_damage;
            if (*(float *)(region_block + 0x28) > 0.0f &&
                (float)region_damage * 0.0039215689f > *(float *)(region_block + 0x28)) {
                object_destroy_region(target_index, region_index);
                *notify_flags |= 2;
            }
        }
    }
bookkeeping:
    {
        float current = taken + ((object *)obj)->current_body_damage;
        float recent;

        ((object *)obj)->body_damage_ticks = 0;
        ((object *)obj)->current_body_damage = current;
        recent = taken + ((object *)obj)->recent_body_damage;
        ((object *)obj)->recent_body_damage = recent;
        if (current > 1.0f) {
            ((object *)obj)->current_body_damage = 1.0f;
        }
        if (recent > 1.0f) {
            ((object *)obj)->recent_body_damage = 1.0f;
        }
    }
    if (g_0087abc0 && *vitality < 0.0f && ((1u << (obj[0xb4] & 0x1f)) & 3)) {
        if (*(datum_index *)(obj + 0x218) != k_datum_index_none) {
            *vitality = 0.0f;
        } else if (((object *)obj)->type == 1) {
            datum_index child = ((object *)obj)->first_child_object;

            while (child != k_datum_index_none) {
                uint8_t *child_obj = (uint8_t *)((object_header *)object_data->data)[child & 0xffff].data;

                if (((1u << (child_obj[0xb4] & 0x1f)) & 3) &&
                    *(datum_index *)(child_obj + 0x218) != k_datum_index_none) {
                    *vitality = 0.0f;
                    break;
                }
                child = ((object *)child_obj)->next_object;
            }
        }
    }
    if (is_local == 1) {
        float absolute = weapon_get_zoom_fov_resolved(1, ((object *)obj)->owner_team) * ((object *)obj)->maximum_body_vitality *
            *vitality;
        float destroyed = *(float *)(geometry + 0xb8);

        if (destroyed < 0.0f && absolute < destroyed) {
            object_delete_teardown(target_index);
            *notify_flags |= 5;
        } else if (absolute < 0.0f) {
            if ((*vitality_flags & 4) == 0) {
                int16_t i;

                for (i = 0; i < *(int32_t *)(geometry + 0x240); i++) {
                    if (*(*(uint8_t **)(geometry + 0x244) + i * 0x54 + 0x20) & 4) {
                        object_destroy_region(target_index, i);
                    }
                }
                object_set_health_frozen_flag(target_index);
                *notify_flags |= 1;
            }
        } else if (absolute < *(float *)(geometry + 0x94) && (*vitality_flags & 1) == 0) {
            effect_new_on_object(target_index, *(datum_index *)(geometry + 0xa4), target_index, -1, 0.0f, 0.0f, 0, 0);
            *vitality_flags |= 1;
        }
    }
    if ((dd->flags & 2) && *(datum_index *)(geometry + 0x7c) != k_datum_index_none) {
        damage_effect_new_at_location(*(datum_index *)(geometry + 0x7c), (int16_t)node_index, &dd->direction,
            (real_vector3d *)plane, &dd->origin, target_index);
    }
    if ((dd->flags & 1) && body > *(float *)(geometry + 0x80) &&
        *(datum_index *)(geometry + 0x90) != k_datum_index_none && *(int16_t *)(effect_block + 0x2) != 7) {
        effect_new_on_object(target_index, *(datum_index *)(geometry + 0x90), target_index, -1, 0.0f, 0.0f, 0, 0);
    }
    *body_damage_out = body;
    *material_multiplier_out = *(float *)(material + 0x3c);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
