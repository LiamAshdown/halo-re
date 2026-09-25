// object_damage_notify_and_impulse
// address 0x4efcf0, size 432 bytes
// name confidence: 0.5 (Ghidra-recovered name)
// rewrite confidence: 0.25
// evidence: types/objects.h object.type (0xb4), object.network_role (0x04); types/tags.h
// Object.origin_offset (0x18, .z read at 0x20), DamageEffect.damage_instantaneous_acceleration
// (0x1f4, .i read as a scalar at +500), DamageEffect.damage_flags (0x1c8, bit 0x20 ==
// detonates_explosives), damage_data (damage_effect_tag 0x00, flags 0x04, responsible_player
// 0x08, responsible_object 0x0c, team_index 0x10).
// UNSURE (function-wide): object+0x204/+0x1f4 are past the common object struct (unit/type
// extension fields with no name here); FUN_004efbf0 is called with a single float argument at
// this call site even though its own decompile (see object_queue_pickup_denied_event.c) shows a
// completely different (EAX/ECX/EDI) parameter set — the two are irreconcilable from this
// module alone, so this call site keeps its own literal one-float form. DAT_0087aa10,
// game_engine_attribute_player_death, game_engine_on_player_death, unit_apply_impulse, unit_apply_impulse_to_seat,
// unit_any_flagged_seat_occupied, object_apply_impulse_and_spin and unit_apply_damage_effects are all outside this module's range.
// register convention: all parameters on the stack.
// blam-cc: stack=(target_index, dd, notify_flags, shield_damage, body_damage, param_6,
//   node_hint, role_is_deletable)
// reconciled: R04 0x006f1d20 uint8_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_engine_definition *current_game_engine;          // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t g_0087aa10;          // 0x0087aa10, UNSURE: not owned by this module

extern real vector3d_normalize_with_length(real_vector3d *v); // math module, 0x401990
extern void item_accelerate(real_vector3d *impulse, int32_t param_2); // 0x4bd080
extern void unit_apply_impulse(void); // UNSURE: zero visible args; out of range, 0x559fa0
extern void unit_apply_impulse_to_seat(void); // UNSURE: zero visible args; out of range, 0x571cb0
extern int8_t unit_any_flagged_seat_occupied(void); // UNSURE: zero visible args; out of range, 0x56cc80
extern void FUN_004efbf0(float param_1); // UNSURE: one float argument at this call site only,
    // irreconcilable with object_queue_pickup_denied_event.c's own (EAX,ECX,EDI) form
extern void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *delta_velocity); // 0x4bef80, this module;
    // blam-cc: EAX object_index, EDX delta_velocity
extern int32_t player_index_from_unit_index(datum_index object_index); // out of range, 0x474db0
extern void game_engine_attribute_player_death(datum_index responsible_player,
    datum_index responsible_object, int16_t team_index, int32_t param_4); // UNSURE: out of range
extern void game_engine_on_player_death(uint32_t target_index, int32_t param_2, int32_t param_3); // UNSURE: out of range
extern void unit_apply_damage_effects(datum_index unit_index, damage_data *damage, uint8_t flags, float body_damage_amount, float shield_damage_amount, void *forward_object, uint8_t apply_effects); // UNSURE: out of range, 0x5674a0

void object_damage_notify_and_impulse(uint32_t target_index, damage_data *dd, uint32_t notify_flags,
    float param_4, float param_5, uint32_t param_6, int32_t node_hint, uint32_t role_is_deletable)
    // param_4/param_5 are undefined4 in the decompile and are only forwarded to 0x5674a0; the
    // one call site (object_apply_damage) supplies the shield and body damage floats.
{
    object_header *headers = (object_header *)object_data->data;
    object *target = headers[target_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[target->definition_tag & 0xffff].data;
    DamageEffect *effect = (DamageEffect *)tag_instances[dd->damage_effect_tag & 0xffff].data;

    if (0.0001f < definition->origin_offset.z) {
        real_vector3d impulse;

        impulse.i = (float)dd->direction.i;
        impulse.j = (float)dd->direction.j;
        impulse.k = (float)dd->direction.k + 0.45f;
        vector3d_normalize_with_length(&impulse);

        {
            int16_t kind = target->type;
            float scale = definition->origin_offset.z * effect->damage_instantaneous_acceleration.i * 0.033333335f;

            impulse.i = impulse.i * scale;
            impulse.j = impulse.j * scale;
            impulse.k = scale * impulse.k;

            switch (kind) {
            case _object_type_biped:
            case _object_type_vehicle:
                if (0.0001f < effect->damage_instantaneous_acceleration.i &&
                    (*(uint32_t *)((uint8_t *)target + 0x204) & 0x800000) == 0) { // UNSURE: unit extension
                    if (kind == _object_type_biped) {
                        unit_apply_impulse();
                    } else if (kind == _object_type_vehicle) {
                        if ((effect->damage_flags & 0x20) != 0) {
                            impulse.i = impulse.i + impulse.i;
                            impulse.j = impulse.j + impulse.j;
                            impulse.k = impulse.k + impulse.k;
                        }
                        if (target->network_role != 1 || unit_any_flagged_seat_occupied() == 1) {
                            unit_apply_impulse_to_seat();
                        }
                    }
                }
                break;
            case _object_type_weapon:
            case _object_type_equipment:
            case _object_type_garbage: {
                float mag_sq = impulse.i * impulse.i + impulse.j * impulse.j + impulse.k * impulse.k;
                int8_t significant = (0.0001f <= mag_sq) ||
                    ((*(uint32_t *)((uint8_t *)target + 0x1f4) & 8) == 0); // UNSURE: unit extension

                if (target->network_role == 0 && significant) {
                    FUN_004efbf0(scale);
                }
                if (target->network_role == 0 || target->network_role == 3 || !significant) {
                    if (dd->random_blend <= 0.5f || (effect->damage_flags & 0x20) == 0) {
                        item_accelerate(&impulse, 0);
                    } else {
                        item_accelerate(&impulse, 1);
                    }
                }
                break;
            }
            case _object_type_projectile:
                object_apply_impulse_and_spin(target_index, &impulse); // 0x4efdcf: EAX = EDI (target), EDX = &impulse
                break;
            default:
                break;
            }
        }
    }

    if (role_is_deletable == 1) {
        if ((current_game_engine == 0 || g_0087aa10 == 0) && (dd->flags & 0x80) == 0) {
            if ((notify_flags & 1) != 0) {
                game_engine_attribute_player_death(dd->responsible_player, dd->responsible_object,
                    dd->team_index, 1);
            }
        } else if (current_game_engine == 0 || g_0087aa10 == 0) {
            int32_t controller = player_index_from_unit_index(target_index);
            game_engine_on_player_death(target_index, controller, 1);
        }
    }

    if ((1 << (target->type & 0x1f) & _object_mask_unit) != 0) {
        unit_apply_damage_effects(target_index, dd, notify_flags, param_4, param_5, node_hint, role_is_deletable);
    }
}

#if 0
Original Ghidra decompilation (0x4efcf0):

void object_damage_notify_and_impulse
               (uint param_1,uint *param_2,uint param_3,undefined4 param_4,undefined4 param_5,
               undefined4 param_6,undefined4 param_7,undefined4 param_8)

{
  short sVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  bool bVar6;
  char cVar7;
  undefined4 uVar8;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(int *)((*param_2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (0.0001 < *(float *)(iVar3 + 0x20)) {
    local_c = (float)param_2[0xd];
    local_8 = (float)param_2[0xe];
    local_4 = (float)param_2[0xf] + 0.45;
    vector3d_normalize_with_length();
    sVar1 = (short)puVar2[0x2d];
    fVar5 = *(float *)(iVar3 + 0x20) * *(float *)(iVar4 + 500) * 0.033333335;
    local_18 = local_c * fVar5;
    local_14 = local_8 * fVar5;
    local_10 = fVar5 * local_4;
    switch(sVar1) {
    case 0:
    case 1:
      if ((0.0001 < *(float *)(iVar4 + 500)) && ((puVar2[0x81] & 0x800000) == 0)) {
        if (sVar1 == 0) {
          FUN_00559fa0();
        }
        else if (sVar1 == 1) {
          if ((*(byte *)(iVar4 + 0x1c8) & 0x20) != 0) {
            local_18 = local_18 + local_18;
            local_14 = local_14 + local_14;
            local_10 = local_10 + local_10;
          }
          if ((puVar2[1] != 1) || (cVar7 = FUN_0056cc80(), cVar7 == '\x01')) {
            FUN_00571cb0();
          }
        }
      }
      break;
    case 2:
    case 3:
    case 4:
      bVar6 = false;
      if ((0.0001 <= local_14 * local_14 + local_18 * local_18 + local_10 * local_10) ||
         ((puVar2[0x7d] & 8) == 0)) {
        bVar6 = true;
      }
      if ((puVar2[1] == 0) && (bVar6)) {
        FUN_004efbf0(fVar5);
      }
      if (((puVar2[1] == 0) || (puVar2[1] == 3)) || (!bVar6)) {
        if (((float)param_2[0x10] <= 0.5) || ((*(byte *)(iVar4 + 0x1c8) & 0x20) == 0)) {
          item_accelerate(&local_18,0);
        }
        else {
          item_accelerate(&local_18,1);
        }
      }
      break;
    case 5:
      FUN_004bef80();
    }
  }
  if ((char)param_8 == '\x01') {
    if (((DAT_006f1d20 == 0) || (DAT_0087aa10 == 0)) && ((param_2[1] & 0x80) == 0)) {
      if ((param_3 & 1) != 0) {
        game_engine_attribute_player_death(param_2[2],param_2[3],(short)param_2[4],1);
      }
    }
    else if ((DAT_006f1d20 == 0) || (DAT_0087aa10 == 0)) {
      uVar8 = FUN_00474db0(param_1);
      game_engine_on_player_death(param_1,uVar8,1);
    }
  }
  if ((1 << ((byte)puVar2[0x2d] & 0x1f) & 3U) != 0) {
    FUN_005674a0(param_1,param_2,param_3,param_4,param_5,param_7,param_8);
  }
  return;
}
#endif
