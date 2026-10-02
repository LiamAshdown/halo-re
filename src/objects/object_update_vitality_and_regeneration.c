// object_update_vitality_and_regeneration
// address 0x4ed510, size 1152 bytes
// name confidence: 0.6
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4ed510..0x4ed98f (the draft dropped the shield-depleted effect's EAX/ECX, the
//   player of the overcharge HUD calls, and passed unlock 0 where the binary sets BL = 1). Stack: object.
//   With a collision geometry (tag +0x7c): pending kill requests (+0x106 bits 0x20 / 0x40 / 0x2000) apply the
//   globals' (+0x18c, +0x1c) damage effect as an instant kill (flag 4; 0x10 for bit 0x40, 0x80 for 0x2000)
//   unless already dead (bit 4) and are cleared. Bit 0x1000 (shield charging) is cleared each tick. With a
//   shield (+0xdc) on a live object: an overcharge (bit 0x10) grows 1/30 per tick up to 3 (then the bit is
//   dropped); multiplayer bleeds an overcharge above 1 back by 1/1350 a tick, telling the player's HUD
//   (0x4b16e0); a shield below 1 waits out its stun ticks (+0x104, counted down by the authoritative copy) and
//   then recharges by geometry +0x1c0 times the difficulty table (0x46fe70, ECX 3), first clearing the
//   depleted state (bit 8: the geometry's recharge effect +0x1b4 through 0x4efff0, regions unlocked through
//   0x4f03e0 with BL 1). The body (+0x100 ticks, +0xec, +0xf8) and shield (+0xfc, +0xe8, +0xf4) damage timers
//   then decay by 1/60 a tick (the recent value only after 60 ticks), clamp at 0 and stop at -1 once both are 0.
//   Finally an authoritative biped whose stun state (+0x104 > 0) differs from +0x538 is marked for an update
//   (+0x10 bit 0x4000000).
// blam-cc: stack=object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t node_index, int16_t region_index,
    int16_t material_index, uint32_t plane); // 0x4ee5e0
extern void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag); // 0x4ed990, EDX, stack
extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX, AX
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0, stack
extern void hud_unit_meter_apply_predictive_damage(datum_index player_index, float damage); // 0x4b16e0, ECX, stack
extern void object_dispatch_effect_notify(uint32_t forwarded_eax, uint32_t forwarded_ecx); // 0x4efff0, EAX, ECX
extern void object_regions_reset_permutation_lock(uint32_t object_index, int8_t unlock); // 0x4f03e0, EAX, BL

// 0x4ed7cd / 0x4ed88c: one damage timer pair decays by 1/60 a tick once running
static void object_decay_damage_timer(int32_t *ticks, float *current, float *recent)
{
    int32_t t = *ticks;
    float current_value;
    float recent_value;

    if (t == -1) {
        return;
    }
    t++;
    *ticks = t;
    if (t >= 0) {
        *current = *current - 0.016666668f;
    }
    if (t >= 0x3c) {
        *recent = *recent - 0.016666668f;
    }
    current_value = (0.0f > *current) ? 0.0f : *current;
    *current = current_value;
    recent_value = (0.0f > *recent) ? 0.0f : *recent;
    *recent = recent_value;
    if (current_value == 0.0f && recent_value == 0.0f) {
        *ticks = -1;
    }
}

void object_update_vitality_and_regeneration(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint16_t *vitality_flags = (uint16_t *)(obj + 0x106);
    float *shield = (float *)(obj + 0xe4);

    if (*(datum_index *)&((struct Object *)object_tag)->collision_model.tag_id != k_datum_index_none) {
        uint8_t *geometry = (uint8_t *)tag_instances[*(datum_index *)&((struct Object *)object_tag)->collision_model.tag_id & 0xffff].data;

        if (geometry != 0) {
            uint16_t flags = *vitality_flags;
            uint16_t kill_request = (uint16_t)(flags & 0x2000);

            if (kill_request || (flags & 0x60)) {
                datum_index effect = *(datum_index *)((uint8_t *)global_globals->falling_damage.pointer + 0x1c);

                if ((flags & 4) == 0 && effect != k_datum_index_none) {
                    damage_data dd;

                    damage_data_initialize(&dd, effect);
                    dd.flags |= 4;
                    dd.random_blend = 1.0f;
                    if (flags & 0x40) {
                        dd.flags |= 0x10;
                    }
                    if (kill_request) {
                        dd.flags |= 0x80;
                    }
                    object_apply_damage(&dd, object_index, -1, -1, -1, 0);
                }
                *vitality_flags &= 0xdf9f;
            }
            obj[0x107] &= 0xef;
            if (((object *)obj)->maximum_shield_vitality > 0.0f && (*vitality_flags & 4) == 0) {
                uint16_t current = *vitality_flags;

                if (current & 0x10) {
                    float value = *shield + 0.033333335f;

                    *shield = value;
                    if (value < 3.0f) {
                        *vitality_flags = (uint16_t)(current | 0x1000);
                    } else {
                        *shield = 3.0f;
                        *vitality_flags = (uint16_t)(current & 0xffef);
                    }
                } else if (*shield > 1.0f && current_game_engine != 0) {
                    datum_index player_index = player_index_from_unit_index(object_index);
                    float excess = *shield - 1.0f;

                    if (0.00074074074f > excess) {
                        *shield = 1.0f;
                        hud_unit_meter_apply_predictive_damage(player_index, excess);
                    } else {
                        *shield = *shield - 0.00074074074f;
                        hud_unit_meter_apply_predictive_damage(player_index, 0.00074074074f);
                    }
                } else if (*shield < 1.0f) {
                    int16_t stun = ((object *)obj)->shield_stun_ticks;

                    if (stun == 0) {
                        float rate = weapon_get_zoom_fov_resolved(3, ((object *)obj)->owner_team) *
                            *(float *)(geometry + 0x1c0);
                        float value;

                        if (obj[0x106] & 8) {
                            object_dispatch_effect_notify(object_index, *(uint32_t *)(geometry + 0x1b4));
                            obj[0x106] &= 0xf7;
                            object_regions_reset_permutation_lock(object_index, 1);
                        }
                        obj[0x107] |= 0x10;
                        value = rate + *shield;
                        *shield = value;
                        if (value > 1.0f) {
                            *shield = 1.0f;
                            *vitality_flags &= 0xefff;
                        }
                    } else if (((object *)obj)->network_role == 3 || ((object *)obj)->network_role == 0) {
                        ((object *)obj)->shield_stun_ticks = (int16_t)(stun - 1);
                    }
                }
            }
            object_decay_damage_timer((int32_t *)(obj + 0x100), (float *)(obj + 0xec), (float *)(obj + 0xf8));
            object_decay_damage_timer((int32_t *)(obj + 0xfc), (float *)(obj + 0xe8), (float *)(obj + 0xf4));
        }
    }
    if (((object *)obj)->type == 0 && ((object *)obj)->network_role == 0) {
        uint8_t *object = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;

        if ((uint32_t)(((struct object *)object)->shield_stun_ticks > 0) != (uint32_t)object[0x538]) {
            ((struct object *)object)->flags |= 0x4000000;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ed510):

void object_update_vitality_and_regeneration(uint param_1)

{
  float fVar1;
  float fVar2;
  ushort uVar3;
  uint *puVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  float10 fVar9;
  undefined1 local_58 [4];
  uint local_54;
  undefined4 local_18;

  iVar8 = (param_1 & 0xffff) * 0xc;
  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  uVar7 = *(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c);
  if ((uVar7 != 0xffffffff) &&
     (iVar5 = *(int *)((uVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), iVar5 != 0)) {
    uVar3 = *(ushort *)((int)puVar4 + 0x106);
    if (((uVar3 & 0x2000) != 0) || ((uVar3 & 0x60) != 0)) {
      if (((uVar3 & 4) == 0) &&
         (iVar6 = *(int *)(*(int *)(DAT_00746fa0 + 0x18c) + 0x1c), iVar6 != -1)) {
        damage_data_initialize(iVar6);
        local_18 = 0x3f800000;
        uVar7 = local_54 | 4;
        if ((uVar3 & 0x40) != 0) {
          uVar7 = local_54 | 0x14;
        }
        local_54 = uVar7;
        if ((uVar3 & 0x2000) != 0) {
          local_54 = local_54 | 0x80;
        }
        object_apply_damage(local_58,param_1,0xffffffff,0xffffffff,0xffffffff,0);
      }
      *(ushort *)((int)puVar4 + 0x106) = *(ushort *)((int)puVar4 + 0x106) & 0xdf9f;
    }
    *(byte *)((int)puVar4 + 0x107) = *(byte *)((int)puVar4 + 0x107) & 0xef;
    uVar3 = *(ushort *)((int)puVar4 + 0x106);
    if ((0.0 < (float)puVar4[0x37]) && ((uVar3 & 4) == 0)) {
      if ((uVar3 & 0x10) == 0) {
        if (((float)puVar4[0x39] <= 1.0) || (DAT_006f1d20 == 0)) {
          if ((float)puVar4[0x39] < 1.0) {
            if ((short)puVar4[0x41] == 0) {
              fVar1 = *(float *)(iVar5 + 0x1c0);
              fVar9 = (float10)FUN_0046fe70();
              if ((*(byte *)((int)puVar4 + 0x106) & 8) != 0) {
                FUN_004efff0();
                *(byte *)((int)puVar4 + 0x106) = *(byte *)((int)puVar4 + 0x106) & 0xf7;
                FUN_004f03e0();
              }
              *(byte *)((int)puVar4 + 0x107) = *(byte *)((int)puVar4 + 0x107) | 0x10;
              fVar1 = (float)(fVar9 * (float10)fVar1) + (float)puVar4[0x39];
              puVar4[0x39] = (uint)fVar1;
              if (1.0 < fVar1) {
                puVar4[0x39] = 0x3f800000;
                *(ushort *)((int)puVar4 + 0x106) = *(ushort *)((int)puVar4 + 0x106) & 0xefff;
              }
            }
            else if ((puVar4[1] == 3) || (puVar4[1] == 0)) {
              *(short *)(puVar4 + 0x41) = (short)puVar4[0x41] + -1;
            }
          }
        }
        else {
          FUN_00474db0(param_1);
          fVar1 = (float)puVar4[0x39];
          if (0.00074074074 <= fVar1 - 1.0) {
            puVar4[0x39] = (uint)((float)puVar4[0x39] - 0.00074074074);
            FUN_004b16e0(0x3a422e45);
          }
          else {
            puVar4[0x39] = 0x3f800000;
            FUN_004b16e0(fVar1 - 1.0);
          }
        }
      }
      else {
        fVar1 = (float)puVar4[0x39] + 0.033333335;
        puVar4[0x39] = (uint)fVar1;
        if (fVar1 < 3.0) {
          *(ushort *)((int)puVar4 + 0x106) = uVar3 | 0x1000;
        }
        else {
          puVar4[0x39] = 0x40400000;
          *(ushort *)((int)puVar4 + 0x106) = uVar3 & 0xffef;
        }
      }
    }
    if (puVar4[0x40] != 0xffffffff) {
      uVar7 = puVar4[0x40] + 1;
      puVar4[0x40] = uVar7;
      if (-1 < (int)uVar7) {
        puVar4[0x3b] = (uint)((float)puVar4[0x3b] - 0.016666668);
      }
      if (0x3b < (int)uVar7) {
        puVar4[0x3e] = (uint)((float)puVar4[0x3e] - 0.016666668);
      }
      if (0.0 <= (float)puVar4[0x3b]) {
        fVar1 = (float)puVar4[0x3b];
      }
      else {
        fVar1 = 0.0;
      }
      puVar4[0x3b] = (uint)fVar1;
      if (0.0 <= (float)puVar4[0x3e]) {
        fVar2 = (float)puVar4[0x3e];
      }
      else {
        fVar2 = 0.0;
      }
      puVar4[0x3e] = (uint)fVar2;
      if ((fVar1 == 0.0) && (fVar2 == 0.0)) {
        puVar4[0x40] = 0xffffffff;
      }
    }
    if (puVar4[0x3f] != 0xffffffff) {
      uVar7 = puVar4[0x3f] + 1;
      puVar4[0x3f] = uVar7;
      if (-1 < (int)uVar7) {
        puVar4[0x3a] = (uint)((float)puVar4[0x3a] - 0.016666668);
      }
      if (0x3b < (int)uVar7) {
        puVar4[0x3d] = (uint)((float)puVar4[0x3d] - 0.016666668);
      }
      if (0.0 <= (float)puVar4[0x3a]) {
        fVar1 = (float)puVar4[0x3a];
      }
      else {
        fVar1 = 0.0;
      }
      puVar4[0x3a] = (uint)fVar1;
      if (0.0 <= (float)puVar4[0x3d]) {
        fVar2 = (float)puVar4[0x3d];
      }
      else {
        fVar2 = 0.0;
      }
      puVar4[0x3d] = (uint)fVar2;
      if ((fVar1 == 0.0) && (fVar2 == 0.0)) {
        puVar4[0x3f] = 0xffffffff;
      }
    }
  }
  if ((((short)puVar4[0x2d] == 0) && (puVar4[1] == 0)) &&
     (iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8),
     0 < *(short *)(iVar8 + 0x104) != (bool)*(char *)(iVar8 + 0x538))) {
    *(uint *)(iVar8 + 0x10) = *(uint *)(iVar8 + 0x10) | 0x4000000;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
