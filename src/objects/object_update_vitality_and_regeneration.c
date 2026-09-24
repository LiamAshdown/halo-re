// object_update_vitality_and_regeneration
// address 0x4ed510, size 1152 bytes
// name confidence: 0.5 (Ghidra-recovered name)
// rewrite confidence: 0.35
// evidence: types/objects.h object (maximum_shield_vitality 0xdc, shield_vitality 0xe4,
// current_shield_damage 0xe8, current_body_damage 0xec, recent_shield_damage 0xf4,
// recent_body_damage 0xf8, shield_damage_ticks 0xfc, body_damage_ticks 0x100,
// shield_stun_ticks 0x104, vitality_flags 0x106, network_role 0x04, type 0xb4);
// types/tags.h ModelCollisionGeometry.shield_recharge_rate (0x1c0).
// UNSURE: the crush/out-of-bounds gate tests vitality_flags bits 0x20 and 0x40, which are not
// in object_vitality_flags (a documented gap in types/objects.h between 0x10 and 0x80); the
// global chain `*(int*)(DAT_00746fa0+0x18c)+0x1c` walks into the player/local-player globals
// this module does not own; weapon_get_zoom_fov_resolved/players_iterate_and_discard/hud_unit_meter_apply_predictive_damage are opaque externals; and the
// trailing `object+0x538` test is a biped-specific extension field outside the common object
// struct (gated on object.type == biped), so it is kept as a raw offset.
// register convention: datum_index object_index on the stack (param_1).
// blam-cc: stack=object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern void *g_00746fa0;            // 0x00746fa0, UNSURE: player/local-player globals, not owned here
extern uint8_t network_predicted_state_flag;          // 0x006f1d20, "the network/predicted-state flag every damage
                                    // and creation path branches on" (objects.h globals note)

extern void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag); // 0x4ed990
extern void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t node_index,
    int16_t param_4, int16_t material_index, uint32_t param_6); // this module, 0x4ee5e0 // 0x4ee5e0
extern void object_dispatch_effect_notify(void); // this module, 0x4efff0
extern void object_regions_reset_permutation_lock(uint32_t object_index, int8_t unlock); // 0x4f03e0

extern real weapon_get_zoom_fov_resolved(void); // UNSURE: zero visible args; objects module (0x46fe70, out of range)
extern int32_t players_iterate_and_discard(datum_index object_index); // UNSURE: out of range, 0x474db0
extern void hud_unit_meter_apply_predictive_damage(float delta); // UNSURE: out of range, 0x4b16e0. Ghidra prints one call
    // as hud_unit_meter_apply_predictive_damage(0x3a422e45); that integer literal IS the IEEE-754 encoding of
    // 0.00074074074f, so both call sites pass a float delta and the value below is exact.

void object_update_vitality_and_regeneration(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    TagID collision_model = definition->collision_model.tag_id;

    if (collision_model.index != 0xffff) {
        ModelCollisionGeometry *geometry = (ModelCollisionGeometry *)tag_instances[collision_model.index].data;

        if (geometry != 0) {
            uint16_t vitality = obj->vitality_flags;

            if ((vitality & 0x2000) != 0 || (vitality & 0x60) != 0) { // shield_stationary, or
                                                                       // UNSURE 0x20/0x40 crush bits
                if ((vitality & _object_health_frozen_bit) == 0) {
                    int32_t *chain = *(int32_t **)((uint8_t *)g_00746fa0 + 0x18c); // UNSURE
                    int32_t damage_effect_tag = chain[7]; // UNSURE: +0x1c

                    if (damage_effect_tag != -1) {
                        damage_data dd;
                        uint32_t flags;

                        damage_data_initialize(&dd, (datum_index)damage_effect_tag);
                        dd.random_blend = 1.0f;
                        flags = dd.flags | 4;
                        if ((vitality & 0x40) != 0) {
                            flags = dd.flags | 0x14;
                        }
                        dd.flags = flags;
                        if ((vitality & 0x2000) != 0) {
                            dd.flags |= 0x80;
                        }
                        object_apply_damage(&dd, object_index, 0xffffffff, 0xffffffff, 0xffffffff, 0);
                    }
                }
                obj->vitality_flags &= 0xdf9f;
            }

            *((uint8_t *)obj + 0x107) &= 0xef; // clears _object_stunned_bit (0x1000)
            vitality = obj->vitality_flags;

            if (obj->maximum_shield_vitality > 0.0f && (vitality & _object_health_frozen_bit) == 0) {
                if ((vitality & _object_shield_recharging_bit) == 0) {
                    if (obj->shield_vitality <= 1.0f || network_predicted_state_flag == 0) {
                        if (obj->shield_vitality < 1.0f) {
                            if (obj->shield_stun_ticks == 0) {
                                real rate = geometry->shield_recharge_rate;
                                real scalar = weapon_get_zoom_fov_resolved();

                                if ((obj->vitality_flags & _object_shield_depleted_bit) != 0) {
                                    object_dispatch_effect_notify();
                                    *((uint8_t *)obj + 0x106) &= 0xf7;
                                    object_regions_reset_permutation_lock(object_index, 0); // UNSURE
                                }
                                *((uint8_t *)obj + 0x107) |= 0x10; // sets _object_stunned_bit

                                obj->shield_vitality = scalar * rate + obj->shield_vitality;
                                if (1.0f < obj->shield_vitality) {
                                    obj->shield_vitality = 1.0f;
                                    obj->vitality_flags &= 0xefff; // clears _object_stunned_bit
                                }
                            } else if (obj->network_role == 3 || obj->network_role == 0) {
                                obj->shield_stun_ticks -= 1;
                            }
                        }
                    } else {
                        float shield = obj->shield_vitality;

                        players_iterate_and_discard(object_index);
                        if (0.00074074074f <= shield - 1.0f) {
                            obj->shield_vitality = obj->shield_vitality - 0.00074074074f;
                            hud_unit_meter_apply_predictive_damage(0.00074074074f);
                        } else {
                            obj->shield_vitality = 1.0f;
                            hud_unit_meter_apply_predictive_damage(shield - 1.0f);
                        }
                    }
                } else {
                    float ramp = obj->shield_vitality + 0.033333335f;

                    obj->shield_vitality = ramp;
                    if (ramp < 3.0f) {
                        obj->vitality_flags = vitality | 0x1000;
                    } else {
                        obj->shield_vitality = 3.0f;
                        obj->vitality_flags = vitality & 0xffef; // clears _object_shield_recharging_bit
                    }
                }
            }

            if (obj->body_damage_ticks != -1) {
                int32_t ticks = obj->body_damage_ticks + 1;

                obj->body_damage_ticks = ticks;
                if (ticks >= 0) {
                    obj->current_body_damage -= 0.016666668f;
                }
                if (ticks > 0x3b) {
                    obj->recent_body_damage -= 0.016666668f;
                }
                obj->current_body_damage = (obj->current_body_damage >= 0.0f) ? obj->current_body_damage : 0.0f;
                obj->recent_body_damage = (obj->recent_body_damage >= 0.0f) ? obj->recent_body_damage : 0.0f;
                if (obj->current_body_damage == 0.0f && obj->recent_body_damage == 0.0f) {
                    obj->body_damage_ticks = -1;
                }
            }

            if (obj->shield_damage_ticks != -1) {
                int32_t ticks = obj->shield_damage_ticks + 1;

                obj->shield_damage_ticks = ticks;
                if (ticks >= 0) {
                    obj->current_shield_damage -= 0.016666668f;
                }
                if (ticks > 0x3b) {
                    obj->recent_shield_damage -= 0.016666668f;
                }
                obj->current_shield_damage = (obj->current_shield_damage >= 0.0f) ? obj->current_shield_damage : 0.0f;
                obj->recent_shield_damage = (obj->recent_shield_damage >= 0.0f) ? obj->recent_shield_damage : 0.0f;
                if (obj->current_shield_damage == 0.0f && obj->recent_shield_damage == 0.0f) {
                    obj->shield_damage_ticks = -1;
                }
            }
        }
    }

    if (obj->type == _object_type_biped && obj->network_role == 0) {
        // The original re-reads the object pointer out of the data array here rather than reusing
        // the one it loaded at the top, so the reload is kept: any of the calls above can move the
        // object pool.
        object *current = ((object_header *)object_data->data)[object_index & 0xffff].data;

        // UNSURE: object+0x538 is a biped-extension field outside the common object struct.
        // The original compares two booleans, so the byte is normalized with != 0 rather than
        // widened (Ghidra prints the same thing as a (bool) cast of a char).
        if ((0 < current->shield_stun_ticks) != (*((uint8_t *)current + 0x538) != 0)) {
            current->flags |= _object_changed_bit;
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
