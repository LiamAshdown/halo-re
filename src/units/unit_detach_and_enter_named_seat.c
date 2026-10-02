// unit_detach_and_enter_named_seat  (Ghidra: FUN_00569d40)
// address 0x569d40, size 1340 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x569d40..0x56a283 -- the engine side of the unit_enter_vehicle script command (a10's
//   tutorial_setup seats player0 in the cryotube with it; the draft never managed to, so the player stood
//   beside the tube). Stack: (unit, vehicle, seat label). Nothing happens for a missing unit / vehicle, an empty
//   label or a dead unit (+0x106 bit 4). A unit already seated (never on a client) first leaves its seat exactly
//   as biped_update does (0x569df1..0x56a129, helpers copied from biped_update.c), an authoritative one (role
//   0) raises scripted event 9 with 1, and a client drops the local player's prediction history. A unit still
//   attached to anything stops there. Otherwise the vehicle tag's seats (+0x2e4 / +0x2e8, 0x11c each) are
//   searched for the label (seat +0x04, _stricmp); the first matching free seat (0x56cc10: EDI vehicle, ESI
//   seat) is entered (0x566970: EAX unit, stack vehicle, seat) -- directly for a vehicle unit, else once the
//   seat's animation label is set on the unit (0x5651e0: EAX unit, stack label, 0, 0).
// blam-cc: stack -> (unit_index, target_parent_index, seat_marker_name)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *player_data;      // 0x0087a480
extern int16_t network_game_mode; // 0x00719720: 1 = client
extern game_time_globals *game_time; // 0x006f1d6c
extern network_client_globals *network_client;

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0 (via 0x696664)
extern void player_update_history_free_all(void *history); // 0x4e6f20
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up,
    real_point3d *position); // 0x4f51c0, stack, EDI position
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, EAX, stack
extern void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index); // 0x55add0, stack, EDI
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
    uint8_t test_only); // 0x5651e0, EAX, stack
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); // 0x566970, EAX unit, stack
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index); // 0x56cc10, EDI, ESI
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define OBJECT_HEADER(h) (((object_header *)object_data->data)[(h) & 0xffff])
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// 0x5596d3.. / 0x5591a9..: take the unit out of its vehicle seat, keep it where its body was.
static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = OBJECT_DATA(object_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *nodes = self + ((unit_object *)self)->base.nodes.offset;
    uint8_t *seat = *(uint8_t **)(TAG_DATA(*(datum_index *)vehicle) + 0x2e8) + ((unit_object *)self)->unit.vehicle_seat_index * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(TAG_DATA(*(datum_index *)(TAG_DATA(*(datum_index *)self) + 0x34)) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (((unit_object *)vehicle)->unit.driver_unit_index == object_index && vehicle[0x2a3] != 0x25 &&
        ((unit_object *)self)->base.parent_object != k_datum_index_none) {
        unit_try_set_animation_state(((unit_object *)self)->base.parent_object, 0x25);
    }
    ((unit_object *)self)->unit.last_parent_object_index = vehicle_index;
    ((unit_object *)self)->unit.last_seat_change_tick = game_time->game_time;
    if (((unit_object *)self)->unit.driver_unit_index == object_index) {
        ((unit_object *)self)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((unit_object *)self)->unit.gunner_unit_index == object_index) {
        ((unit_object *)self)->unit.gunner_unit_index = k_datum_index_none;
    }
    object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + ((unit_object *)self)->base.position.x;
    position.y = offset.y + ((unit_object *)self)->base.position.y;
    position.z = offset.z + ((unit_object *)self)->base.position.z - default_translation.z;
    object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = OBJECT_DATA(object_index);

        matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((unit_object *)self)->base.forward.i = basis.forward;
    *(real_vector3d *)&((unit_object *)self)->base.up.i = basis.up;
    {
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag = TAG_DATA(*(datum_index *)object);

        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && (object[0x10] & 1) != 0) {
            object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
            *(uint32_t *)(object + 0x10) &= ~1u;
            OBJECT_HEADER(object_index).flags |= 2;
        }
    }
    ((unit_object *)self)->unit.vehicle_seat_index = -1;
    self[0x2a7] = 2;
    if (((unit_object *)vehicle)->unit.driver_unit_index == object_index) {
        ((unit_object *)vehicle)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((unit_object *)vehicle)->unit.gunner_unit_index == object_index) {
        ((unit_object *)vehicle)->unit.gunner_unit_index = k_datum_index_none;
    }
    unit_recompute_seat_occupants(vehicle_index);
    unit_pick_and_ready_next_weapon(object_index);
    {
        int8_t request[2] = { 0x14, 0 };

        unit_update_animation_state_machine(object_index, request);
    }
    *(real_point3d *)(self + ((unit_object *)self)->base.node_function_values.offset + 0x10) = default_translation;
    if (((unit_object *)self)->base.type == 0) {
        unit_reset_orientation_and_find_position(object_index, vehicle_index); // EDI = the seat parent
    }
    object_recalculate_bounding_radius_recursive(object_index);
    if (unit_all_seats_unoccupied(vehicle_index) == 1) {
        uint8_t *empty = (uint8_t *)object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = game_time->game_time;
        }
    }
    if (network_game_mode == 1) {
        uint8_t *player = (uint8_t *)datum_get(((unit_object *)self)->unit.controlling_player, player_data);

        if (player != 0 && ((struct player *)player)->local_player_index == -1) {
            ((struct player *)player)->position_updates.read_index = 0;
            ((struct player *)player)->position_updates.write_index = 0;
            ((struct player *)player)->vehicle_updates.read_index = 0;
            ((struct player *)player)->vehicle_updates.write_index = 0;
        }
    }
}

// 0x559505 / 0x559a59: a client drops the prediction history of a local player's unit.
static void biped_free_local_player_history(uint8_t *self)
{
    datum_index player_index = ((unit_object *)self)->unit.controlling_player;
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    uint8_t *player;

    if (network_game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= *(int16_t *)((uint8_t *)player_data + 0x20)) {
        return;
    }
    player = (uint8_t *)player_data->data + *(int16_t *)((uint8_t *)player_data + 0x22) * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || ((struct player *)player)->local_player_index == -1) {
        return;
    }
    if (network_client != 0) {
        player_update_history_free_all(*(void **)&network_client->update_history);
    }
}

void unit_detach_and_enter_named_seat(uint32_t unit_index, uint32_t target_parent_index, char *seat_marker_name)
{
    uint8_t *obj;
    uint8_t *vehicle_tag;
    int16_t i;

    if (unit_index == 0xffffffff || target_parent_index == 0xffffffff || seat_marker_name[0] == 0) {
        return;
    }
    obj = OBJECT_DATA(unit_index);
    if (obj[0x106] & 4) {
        return;
    }
    if (((unit_object *)obj)->base.parent_object != k_datum_index_none && ((unit_object *)obj)->unit.vehicle_seat_index != -1 &&
        network_game_mode != 1) {
        if (((unit_object *)obj)->base.parent_object != k_datum_index_none && ((unit_object *)obj)->unit.vehicle_seat_index != -1) {
            biped_detach_from_seat(unit_index, ((unit_object *)obj)->base.parent_object);
        }
        if (((unit_object *)obj)->base.network_role == 0) {
            unit_dispatch_scripted_event_9(1, (int32_t)unit_index);
        }
        biped_free_local_player_history(obj);
    }
    if (((unit_object *)obj)->base.parent_object != k_datum_index_none) {
        return;
    }
    vehicle_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(target_parent_index));
    for (i = 0; i < *(int32_t *)(vehicle_tag + 0x2e4); i++) {
        char *seat_label = (char *)(*(uint8_t **)(vehicle_tag + 0x2e8) + i * 0x11c + 0x4);

        if (_stricmp(seat_marker_name, seat_label) != 0) {
            continue;
        }
        if (unit_is_seat_occupied((int32_t)target_parent_index, i)) {
            continue;
        }
        if (((unit_object *)obj)->base.type == 1 || unit_set_or_test_seat_and_weapon_label(unit_index, seat_label, 0, 0)) {
            unit_enter_vehicle_seat(target_parent_index, i, unit_index);
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x569d40):

void FUN_00569d40(uint param_1,uint param_2,char *param_3)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  uint *puVar9;
  char cVar10;
  char *pcVar11;
  short *psVar12;
  int iVar13;
  short sVar14;
  short sVar15;
  int iVar16;
  int iVar17;
  undefined1 local_b0 [4];
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_94;
  uint uStack_90;
  uint uStack_8c;
  undefined1 local_78 [116];

  if ((param_1 != 0xffffffff) && (param_2 != 0xffffffff)) {
    pcVar11 = param_3;
    do {
      cVar10 = *pcVar11;
      pcVar11 = pcVar11 + 1;
    } while (cVar10 != '\0');
    if (pcVar11 != param_3 + 1) {
      iVar16 = (param_1 & 0xffff) * 0xc;
      puVar3 = *(uint **)(iVar16 + 8 + *(int *)(DAT_008603b0 + 0x34));
      if ((*(byte *)((int)puVar3 + 0x106) & 4) == 0) {
        uVar4 = puVar3[0x47];
        if (((uVar4 != 0xffffffff) && ((short)puVar3[0xbc] != -1)) && (DAT_00719720 != 1)) {
          if ((uVar4 != 0xffffffff) && ((short)puVar3[0xbc] != -1)) {
            puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
            object_get_node_local_transform
                      (uVar4,*(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                     0x2e8) + 0x24 + (short)puVar3[0xbc] * 0x11c,local_78,1);
            iVar17 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 +
                                                          DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                       0x14 + DAT_0087bc14) + 0xbc);
            uVar6 = *(undefined4 *)(iVar17 + 0x28);
            uVar7 = *(undefined4 *)(iVar17 + 0x2c);
            uVar8 = *(undefined4 *)(iVar17 + 0x30);
            if ((puVar5[0xc9] == param_1) &&
               ((*(char *)((int)puVar5 + 0x2a3) != '%' && (puVar3[0x47] != 0xffffffff)))) {
              unit_try_set_animation_state(puVar3[0x47],0x25);
            }
            iVar13 = DAT_006f1d6c;
            puVar3[0xcb] = uVar4;
            puVar3[0xcc] = *(uint *)(iVar13 + 0xc);
            if (puVar3[0xc9] == param_1) {
              puVar3[0xc9] = 0xffffffff;
            }
            if (puVar3[0xca] == param_1) {
              puVar3[0xca] = 0xffffffff;
            }
            FUN_004f6610(param_1);
            object_set_position_and_orientation(param_1,0,0);
            iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
            (*(code *)PTR_matrix4x3_multiply_00696664)
                      (*(short *)(iVar13 + 0x1f2) + iVar13,iVar17 + 0x68,local_b0);
            puVar3[0x1d] = uStack_ac;
            puVar3[0x1e] = uStack_a8;
            puVar3[0x1f] = uStack_a4;
            puVar3[0x20] = uStack_94;
            puVar3[0x21] = uStack_90;
            iVar17 = DAT_008603b0;
            puVar3[0x22] = uStack_8c;
            puVar9 = *(uint **)(*(int *)(iVar17 + 0x34) + 8 + iVar16);
            iVar17 = *(int *)((*puVar9 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (*(int *)(iVar17 + 0x34) != -1) {
              if ((puVar9[4] & 1) != 0) {
                object_for_each_light_attachment(0,1);
              }
              if (*(int *)(iVar17 + 0x34) != -1) {
                iVar17 = *(int *)(DAT_008603b0 + 0x34);
                puVar9[4] = puVar9[4] & 0xfffffffe;
                pbVar1 = (byte *)(iVar17 + iVar16 + 2);
                *pbVar1 = *pbVar1 | 2;
              }
            }
            *(undefined2 *)(puVar3 + 0xbc) = 0xffff;
            *(undefined1 *)((int)puVar3 + 0x2a7) = 2;
            if (puVar5[0xc9] == param_1) {
              puVar5[0xc9] = 0xffffffff;
            }
            if (puVar5[0xca] == param_1) {
              puVar5[0xca] = 0xffffffff;
            }
            FUN_0056ce30();
            FUN_0056d6a0();
            FUN_00565420(param_1);
            puVar2 = (undefined4 *)(*(short *)((int)puVar3 + 0x1ea) + 0x10 + (int)puVar3);
            *puVar2 = uVar6;
            puVar2[1] = uVar7;
            puVar2[2] = uVar8;
            if ((short)puVar3[0x2d] == 0) {
              FUN_0055add0(param_1);
            }
            object_recalculate_bounding_radius_recursive(param_1);
            cVar10 = FUN_00566910();
            if ((cVar10 == '\x01') && (iVar16 = object_try_and_get(2), iVar16 != 0)) {
              *(undefined4 *)(iVar16 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
            }
            if (((DAT_00719720 == 1) && (iVar16 = datum_get(), iVar16 != 0)) &&
               (*(short *)(iVar16 + 2) == -1)) {
              *(undefined4 *)(iVar16 + 0x180) = 0;
              *(undefined4 *)(iVar16 + 0x17c) = 0;
              *(undefined4 *)(iVar16 + 0x1e0) = 0;
              *(undefined4 *)(iVar16 + 0x1dc) = 0;
            }
          }
          if (puVar3[1] == 0) {
            FUN_0056c370(1);
          }
          if (((DAT_00719720 == 1) && (uVar4 = puVar3[0x86], uVar4 != 0xffffffff)) &&
             ((sVar14 = (short)uVar4, -1 < sVar14 && (sVar14 < *(short *)(DAT_0087a480 + 0x20))))) {
            psVar12 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar14 +
                               *(int *)(DAT_0087a480 + 0x34));
            sVar14 = *psVar12;
            if (((sVar14 != 0) &&
                ((sVar15 = (short)(uVar4 >> 0x10), sVar15 == 0 || (sVar14 == sVar15)))) &&
               ((psVar12[1] != -1 && (DAT_0071c2d8 != 0)))) {
              player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
            }
          }
        }
        if (puVar3[0x47] == 0xffffffff) {
          iVar16 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (param_2 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                           DAT_0087bc14);
          iVar17 = 0;
          if (0 < *(int *)(iVar16 + 0x2e4)) {
            iVar13 = 0;
            while( true ) {
              pcVar11 = (char *)(iVar13 * 0x11c + *(int *)(iVar16 + 0x2e8) + 4);
              iVar13 = __stricmp(param_3,pcVar11);
              if (((iVar13 == 0) && (cVar10 = FUN_0056cc10(), cVar10 == '\0')) &&
                 (((short)puVar3[0x2d] == 1 ||
                  (cVar10 = unit_set_or_test_seat_and_weapon_label(pcVar11,0,0), cVar10 != '\0'))))
              break;
              iVar17 = iVar17 + 1;
              iVar13 = (int)(short)iVar17;
              if (*(int *)(iVar16 + 0x2e4) <= iVar13) {
                return;
              }
            }
            unit_enter_vehicle_seat(param_2,iVar17);
          }
        }
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
