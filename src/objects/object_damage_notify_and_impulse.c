// object_damage_notify_and_impulse
// address 0x4efcf0, size 432 bytes
// name confidence: 0.5 (Ghidra-recovered name)
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4efcf0..0x4effd6 (jump table 0x4effd8 on the object type) and its one caller,
//   object_apply_damage (0x4ef0fd). Stack: (target, dd, notify flags, shield damage, body damage, unused
//   float, region, is_local). When the object tag's +0x20 exceeds 0.0001 the damage direction, lifted by 0.45 and
//   normalized, is scaled by tag+0x20 * damage effect +0x1f4 / 30 into an impulse:
//     biped / vehicle (effect acceleration > 0.0001, object +0x204 bit 0x800000 clear): biped
//       unit_apply_impulse(target, impulse); vehicle doubles it for flag 0x20 and, unless it is a remote
//       (role 1) vehicle without a flagged seat occupied, unit_apply_impulse_to_seat(target, impulse);
//     weapon / equipment / garbage: significant = |impulse|^2 >= 0.0001 or object +0x1f4 bit 8 clear; role 0 and
//       significant queues the network event (0x4efbf0: stack scale, ECX target, EDI direction); roles 0 and 3,
//       or not significant, item_accelerate(target, impulse, blend > 0.5 and flag 0x20);
//     projectile: object_apply_impulse_and_spin(target, impulse).
//   With is_local == 1 and no multiplayer engine or 0x87aa10 clear: damage without flag 0x80 attributes the
//   death (0x46ff00, only for notify flag 1), with flag 0x80 the unit's player gets
//   game_engine_on_player_death(p, target, p, 1). Units finally get
//   unit_apply_damage_effects(target, dd, flags, shield, body, region, is_local).
// blam-cc: stack=(target_index, dd, notify_flags, shield_damage, body_damage, unused_6, region_index,
//   is_local)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include <stdint.h>

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value; // 0x0087aa10, read as a dword

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer); // 0x4bd080, EAX, stack
extern void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse); // 0x559fa0, EAX, EDI
extern void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse); // 0x571cb0, ECX, EAX
extern uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index); // 0x56cc80, EAX
extern void object_queue_pickup_denied_event(void *param_1, int32_t key, uint32_t *source); // 0x4efbf0, stack, ECX, EDI
extern void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *delta_velocity); // 0x4bef80, EAX, EDX
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0, stack
extern void game_engine_attribute_player_death(datum_index victim_unit, datum_index killer,
    datum_index death_object, int32_t killer_team, char credit_kills); // 0x46ff00, EAX, stack
extern void game_engine_on_player_death(datum_index killer, datum_index death_object, datum_index victim,
    char is_suicide); // 0x460200, EDX, stack
extern void unit_apply_damage_effects(datum_index unit_index, damage_data *dd, uint32_t flags, float shield_damage,
    float body_damage, int32_t region_index, uint8_t is_local); // 0x5674a0, stack

void object_damage_notify_and_impulse(uint32_t target_index, damage_data *dd, uint32_t notify_flags,
    float shield_damage, float body_damage, uint32_t unused_6, int32_t region_index, uint32_t is_local)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[target_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *effect = (uint8_t *)tag_instances[dd->damage_effect_tag & 0xffff].data;
    int16_t type;

    if (((struct Object *)object_tag)->acceleration_scale > 0.0001f) {
        real_vector3d direction;
        real_vector3d impulse;
        float scale;

        direction = dd->direction;
        direction.k = direction.k + 0.45f;
        vector3d_normalize_with_length(&direction);
        type = ((object *)obj)->type;
        scale = ((struct Object *)object_tag)->acceleration_scale * *(float *)(effect + 0x1f4) * 0.033333335f;
        impulse.i = direction.i * scale;
        impulse.j = direction.j * scale;
        impulse.k = scale * direction.k;
        switch (type) {
        case 0:
        case 1:
            if (*(float *)(effect + 0x1f4) > 0.0001f && (*(uint32_t *)(obj + 0x204) & 0x800000) == 0) {
                if (type == 0) {
                    unit_apply_impulse(target_index, &impulse);
                } else {
                    if (*(uint32_t *)(effect + 0x1c8) & 0x20) {
                        impulse.i = impulse.i + impulse.i;
                        impulse.j = impulse.j + impulse.j;
                        impulse.k = impulse.k + impulse.k;
                    }
                    if (((object *)obj)->network_role != 1 || unit_any_flagged_seat_occupied(target_index) == 1) {
                        unit_apply_impulse_to_seat(target_index, &impulse);
                    }
                }
            }
            break;
        case 2:
        case 3:
        case 4: {
            uint8_t significant = 0;
            int32_t role;

            if (!(impulse.k * impulse.k + impulse.i * impulse.i + impulse.j * impulse.j < 0.0001f) ||
                (*(uint8_t *)(obj + 0x1f4) & 8) == 0) {
                significant = 1;
            }
            if (((object *)obj)->network_role == 0 && significant == 1) {
                object_queue_pickup_denied_event(*(void **)&scale, (int32_t)target_index, (uint32_t *)&direction);
            }
            role = ((object *)obj)->network_role;
            if (role == 0 || role == 3 || !significant) {
                item_accelerate(target_index, &impulse,
                    (uint8_t)((dd->random_blend > 0.5f && (*(uint8_t *)(effect + 0x1c8) & 0x20)) ? 1 : 0));
            }
            break;
        }
        case 5:
            object_apply_impulse_and_spin(target_index, &impulse);
            break;
        default:
            break;
        }
    }
    if ((uint8_t)is_local == 1 && (current_game_engine == 0 || game_engine_state_value == 0)) {
        if ((dd->flags & 0x80) == 0) {
            if (notify_flags & 1) {
                game_engine_attribute_player_death(target_index, dd->responsible_player, dd->responsible_object,
                    (int32_t)(uint16_t)dd->team_index, 1); // 0x4eff62: zero-extended
            }
        } else {
            datum_index player_index = player_index_from_unit_index(target_index);

            game_engine_on_player_death(player_index, target_index, player_index, 1);
        }
    }
    if ((1u << (obj[0xb4] & 0x1f)) & 3) {
        unit_apply_damage_effects(target_index, dd, notify_flags, shield_damage, body_damage, region_index,
            (uint8_t)is_local);
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
