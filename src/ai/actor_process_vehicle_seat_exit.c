// actor_process_vehicle_seat_exit  (Ghidra: FUN_0040b080; an actor gets out of the vehicle it rides)
// address 0x40b080, size 1767 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x40b080..0x40b766 (the draft called most helpers without their arguments). EAX:
//   actor. An actor driving something (+0x158) gets out when a friendly unit prop (kinds 2..3, a unit) is
//   riding the same vehicle (+0x110), when asked to (+0x2ed), or when committed (+0x160) and in danger (a
//   weapon threat +0x1b0, or danger kind 2 with +0x28a). A seated vehicle-type rider (a turret) is detached
//   exactly as biped_update does (helpers copied from biped_update.c); a biped rider starts its seat's exit
//   animation (slot 8 of the seat's units block, a random permutation, 0x56ebd0), the driver seat first telling
//   the vehicle (0x56ab10), then announces it (0x42c370, scripted event 9 when local) and remembers the vehicle
//   for 180 ticks (+0x390 / +0x394). Returns 1 when an exit animation started. +0x38c holds the "forced" flag
//   during the call; +0x2ed is cleared.
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "game.h"
#include "ai.h"
#include "networking.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
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
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, ECX = unit data
extern void unit_notify_weapon_removed(int32_t object_index); // 0x56ab10, EAX
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
                                                   int32_t stream); // 0x4d6280, EAX, DX, stack
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); // 0x56ebd0, EAX, stack
extern void actor_notify_weapon_pickup_once(datum_index object_index); // 0x42c370, ECX
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define OBJECT_HEADER(h) (((object_header *)object_data->data)[(h) & 0xffff])
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// 0x5596d3.. / 0x5591a9..: take the unit out of its vehicle seat, keep it where its body was.
static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = OBJECT_DATA(object_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *nodes = self + *(int16_t *)(self + 0x1f2);
    uint8_t *seat = *(uint8_t **)(TAG_DATA(*(datum_index *)vehicle) + 0x2e8) + *(int16_t *)(self + 0x2f0) * 0x11c;
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
    if (*(datum_index *)(vehicle + 0x324) == object_index && vehicle[0x2a3] != 0x25 &&
        *(datum_index *)(self + 0x11c) != k_datum_index_none) {
        unit_try_set_animation_state(*(datum_index *)(self + 0x11c), 0x25);
    }
    *(datum_index *)(self + 0x32c) = vehicle_index;
    *(int32_t *)(self + 0x330) = game_time->game_time;
    if (*(datum_index *)(self + 0x324) == object_index) {
        *(datum_index *)(self + 0x324) = k_datum_index_none;
    }
    if (*(datum_index *)(self + 0x328) == object_index) {
        *(datum_index *)(self + 0x328) = k_datum_index_none;
    }
    object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + *(float *)(self + 0x5c);
    position.y = offset.y + *(float *)(self + 0x60);
    position.z = offset.z + *(float *)(self + 0x64) - default_translation.z;
    object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = OBJECT_DATA(object_index);

        matrix4x3_multiply((real_matrix4x3 *)(reloaded + *(int16_t *)(reloaded + 0x1f2)),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)(self + 0x74) = basis.forward;
    *(real_vector3d *)(self + 0x80) = basis.up;
    {
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag = TAG_DATA(*(datum_index *)object);

        if (*(int32_t *)(object_tag + 0x34) != -1 && (object[0x10] & 1) != 0) {
            object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)(object_tag + 0x34) != -1) {
            *(uint32_t *)(object + 0x10) &= ~1u;
            OBJECT_HEADER(object_index).flags |= 2;
        }
    }
    *(int16_t *)(self + 0x2f0) = -1;
    self[0x2a7] = 2;
    if (*(datum_index *)(vehicle + 0x324) == object_index) {
        *(datum_index *)(vehicle + 0x324) = k_datum_index_none;
    }
    if (*(datum_index *)(vehicle + 0x328) == object_index) {
        *(datum_index *)(vehicle + 0x328) = k_datum_index_none;
    }
    unit_recompute_seat_occupants(vehicle_index);
    unit_pick_and_ready_next_weapon(object_index);
    {
        int8_t request[2] = { 0x14, 0 };

        unit_update_animation_state_machine(object_index, request);
    }
    *(real_point3d *)(self + *(int16_t *)(self + 0x1ea) + 0x10) = default_translation;
    if (*(int16_t *)(self + 0xb4) == 0) {
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
        uint8_t *player = (uint8_t *)datum_get(*(datum_index *)(self + 0x218), player_data);

        if (player != 0 && *(int16_t *)(player + 2) == -1) {
            *(int32_t *)(player + 0x180) = 0;
            *(int32_t *)(player + 0x17c) = 0;
            *(int32_t *)(player + 0x1e0) = 0;
            *(int32_t *)(player + 0x1dc) = 0;
        }
    }
}

// 0x559505 / 0x559a59: a client drops the prediction history of a local player's unit.
static void biped_free_local_player_history(uint8_t *self)
{
    datum_index player_index = *(datum_index *)(self + 0x218);
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    uint8_t *player;

    if (network_game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= *(int16_t *)((uint8_t *)player_data + 0x20)) {
        return;
    }
    player = (uint8_t *)player_data->data + *(int16_t *)((uint8_t *)player_data + 0x22) * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || *(int16_t *)(player + 2) == -1) {
        return;
    }
    if (network_client != 0) {
        player_update_history_free_all(*(void **)&network_client->update_history);
    }
}

uint8_t actor_process_vehicle_seat_exit(datum_index actor_index)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    datum_index driving = *(datum_index *)(act + 0x158);
    uint8_t wanted = 0;
    uint8_t forced = 0;
    uint8_t result = 0;
    datum_index rider_index;
    uint8_t *rider;

    if (driving == k_datum_index_none) {
        act[0x2ed] = 0;
        return 0;
    }
    {
        datum_index prop_index = *(datum_index *)(act + 0x50);

        while (prop_index != k_datum_index_none) {
            uint8_t *p = (uint8_t *)prop_data->data + (prop_index & 0xffff) * 0x138;
            int16_t kind = *(int16_t *)(p + 0x24);

            prop_index = *(datum_index *)(p + 0x8);
            if (kind >= 2 && kind <= 3 && p[0x12e] && p[0x60] && *(datum_index *)(p + 0x110) == driving) {
                wanted = 1;
                forced = 1;
                break;
            }
        }
    }
    if (act[0x2ed]) {
        wanted = 1;
    }
    if (act[0x160] && (*(datum_index *)(act + 0x1b0) != k_datum_index_none ||
                       (*(int16_t *)(act + 0x280) == 2 && act[0x28a]))) {
        forced = 1;
    } else if (!wanted) {
        act[0x2ed] = 0;
        return 0;
    }
    act[0x38c] = forced;
    rider_index = *(datum_index *)(act + 0x18);
    rider = (uint8_t *)object_try_and_get(rider_index, 3);
    if (rider != 0 && network_game_mode != 1 && *(datum_index *)(rider + 0x11c) != k_datum_index_none &&
        *(int16_t *)(rider + 0x2f0) != -1) {
        datum_index vehicle_index = *(datum_index *)(rider + 0x11c);

        if (*(int16_t *)(rider + 0xb4) == 1) {
            // 0x40b1ce: a vehicle-type rider (a turret) is taken off
            uint8_t *self = OBJECT_DATA(rider_index);

            if (*(datum_index *)(self + 0x11c) != k_datum_index_none && *(int16_t *)(self + 0x2f0) != -1) {
                biped_detach_from_seat(rider_index, *(datum_index *)(self + 0x11c));
            }
            biped_free_local_player_history(self);
        } else if (!unit_state_is_scripted_animation((unit_data *)(rider + 0x1f4))) {
            // 0x40b5f7: a biped rider plays its seat's exit animation
            uint8_t *rider_tag = TAG_DATA(*(datum_index *)rider);
            datum_index graph = *(datum_index *)(rider_tag + 0x44);
            uint8_t *block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)rider[0x2a0] * 0x64;

            if (*(int32_t *)(block + 0x40) > 8) {
                int16_t exit_animation = (*(int16_t **)(block + 0x44))[8];

                if (exit_animation != -1) {
                    uint8_t *object;
                    uint8_t *object_tag;

                    if (*(datum_index *)(OBJECT_DATA(vehicle_index) + 0x324) == rider_index) {
                        unit_notify_weapon_removed(vehicle_index);
                    }
                    unit_set_custom_animation(rider_index, graph,
                                              animation_choose_random_permutation(graph, exit_animation, 1));
                    object = OBJECT_DATA(rider_index);
                    object_tag = TAG_DATA(*(datum_index *)object);
                    if (*(int32_t *)(object_tag + 0x34) != -1) {
                        if (object[0x10] & 1) {
                            object_for_each_light_attachment(rider_index, 0, 1);
                        }
                        if (*(int32_t *)(object_tag + 0x34) != -1) {
                            *(uint32_t *)(object + 0x10) &= ~1u;
                            OBJECT_HEADER(rider_index).flags |= 2;
                        }
                    }
                    rider[0x2a3] = 0x1b;
                    actor_notify_weapon_pickup_once(rider_index);
                    if (*(int32_t *)(rider + 0x4) == 0) {
                        unit_dispatch_scripted_event_9(0, (int32_t)rider_index);
                    }
                    *(datum_index *)(act + 0x390) = *(datum_index *)(act + 0x158);
                    *(int32_t *)(act + 0x394) = game_time->game_time + 180;
                    result = 1;
                }
            }
        }
    }
    act[0x38c] = 0;
    act[0x2ed] = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x40b080) -- full listing via `python tools/pack.py 0x40b080`:

undefined1 FUN_0040b080(void)

{
  byte *pbVar1;
  int iVar2;
  undefined4 *puVar3;
  uint uVar4;
  uint *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  uint *puVar8;
  bool bVar9;
  char cVar10;
  undefined1 uVar11;
  uint in_EAX;
  int iVar12;
  uint *puVar13;
  int iVar14;
  undefined4 uVar15;
  short sVar16;
  uint uVar17;
  int iVar18;
  short sVar19;
  undefined1 local_ea;
  undefined1 local_e8;
  undefined1 local_b0 [4];
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_94;
  uint uStack_90;
  uint uStack_8c;
  undefined1 local_78 [116];

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_ea = 0;
  uVar11 = 0;
  if (*(int *)(iVar2 + 0x158) == -1) goto LAB_0040b759;
  uVar17 = *(uint *)(iVar2 + 0x50);
  bVar9 = false;
  local_e8 = 0;
  do {
    if (uVar17 == 0xffffffff) goto LAB_0040b11e;
    iVar12 = (uVar17 & 0xffff) * 0x138;
    uVar17 = *(uint *)(iVar12 + 8 + *(int *)(DAT_008802c0 + 0x34));
    iVar12 = iVar12 + *(int *)(DAT_008802c0 + 0x34);
  } while ((((*(short *)(iVar12 + 0x24) < 2) || (3 < *(short *)(iVar12 + 0x24))) ||
           (*(char *)(iVar12 + 0x12e) == '\0')) ||
          ((*(char *)(iVar12 + 0x60) == '\0' ||
           (*(int *)(iVar12 + 0x110) != *(int *)(iVar2 + 0x158)))));
  bVar9 = true;
  local_e8 = 1;
LAB_0040b11e:
  if (*(char *)(iVar2 + 0x2ed) != '\0') {
    bVar9 = true;
  }
  if ((*(char *)(iVar2 + 0x160) == '\0') ||
     ((*(int *)(iVar2 + 0x1b0) == -1 &&
      ((*(short *)(iVar2 + 0x280) != 2 || (*(char *)(iVar2 + 0x28a) == '\0')))))) {
    uVar11 = local_ea;
    if (!bVar9) goto LAB_0040b759;
  }
  else {
    local_e8 = 1;
  }
  uVar17 = *(uint *)(iVar2 + 0x18);
  *(undefined1 *)(iVar2 + 0x38c) = local_e8;
  puVar13 = (uint *)object_try_and_get(3);
  if ((((puVar13 != (uint *)0x0) && (DAT_00719720 != 1)) &&
      (uVar4 = puVar13[0x47], uVar4 != 0xffffffff)) && ((short)puVar13[0xbc] != -1)) {
    if ((short)puVar13[0x2d] == 1) {
      iVar18 = (uVar17 & 0xffff) * 0xc;
      puVar13 = *(uint **)(iVar18 + 8 + *(int *)(DAT_008603b0 + 0x34));
      uVar4 = puVar13[0x47];
      iVar12 = DAT_0087a480;
      if ((uVar4 == 0xffffffff) || ((short)puVar13[0xbc] == -1)) {
LAB_0040b559:
        if (DAT_00719720 == 1) {
LAB_0040b567:
          uVar17 = puVar13[0x86];
          if (((uVar17 != 0xffffffff) && (sVar19 = (short)uVar17, -1 < sVar19)) &&
             (sVar19 < *(short *)(iVar12 + 0x20))) {
            iVar18 = (int)*(short *)(iVar12 + 0x22) * (int)sVar19;
            sVar19 = *(short *)(iVar18 + *(int *)(iVar12 + 0x34));
            if ((((sVar19 != 0) &&
                 ((sVar16 = (short)(uVar17 >> 0x10), sVar16 == 0 || (sVar19 == sVar16)))) &&
                (*(short *)(iVar18 + *(int *)(iVar12 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)) {
              player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
            }
          }
        }
      }
      else {
        puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
        object_get_node_local_transform
                  (uVar4,*(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)
                         + 0x24 + (short)puVar13[0xbc] * 0x11c,local_78,1);
        iVar12 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar13 & 0xffff) * 0x20 + 0x14 +
                                                      DAT_0087bc14) + 0x34) & 0xffff) * 0x20 + 0x14
                                  + DAT_0087bc14) + 0xbc);
        uVar15 = *(undefined4 *)(iVar12 + 0x28);
        uVar6 = *(undefined4 *)(iVar12 + 0x2c);
        uVar7 = *(undefined4 *)(iVar12 + 0x30);
        if ((puVar5[0xc9] == uVar17) &&
           ((*(char *)((int)puVar5 + 0x2a3) != '%' && (puVar13[0x47] != 0xffffffff)))) {
          unit_try_set_animation_state(puVar13[0x47],0x25);
        }
        iVar14 = DAT_006f1d6c;
        puVar13[0xcb] = uVar4;
        puVar13[0xcc] = *(uint *)(iVar14 + 0xc);
        if (puVar13[0xc9] == uVar17) {
          puVar13[0xc9] = 0xffffffff;
        }
        if (puVar13[0xca] == uVar17) {
          puVar13[0xca] = 0xffffffff;
        }
        FUN_004f6610(uVar17);
        object_set_position_and_orientation(uVar17,0,0);
        iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar18);
        (*(code *)PTR_matrix4x3_multiply_00696664)
                  (*(short *)(iVar14 + 0x1f2) + iVar14,iVar12 + 0x68,local_b0);
        puVar13[0x1d] = uStack_ac;
        puVar13[0x1e] = uStack_a8;
        puVar13[0x1f] = uStack_a4;
        puVar13[0x20] = uStack_94;
        puVar13[0x21] = uStack_90;
        iVar12 = DAT_008603b0;
        puVar13[0x22] = uStack_8c;
        puVar8 = *(uint **)(*(int *)(iVar12 + 0x34) + 8 + iVar18);
        iVar12 = *(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (*(int *)(iVar12 + 0x34) != -1) {
          if ((puVar8[4] & 1) != 0) {
            object_for_each_light_attachment(0,1);
          }
          if (*(int *)(iVar12 + 0x34) != -1) {
            iVar12 = *(int *)(DAT_008603b0 + 0x34);
            puVar8[4] = puVar8[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar12 + iVar18 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
        }
        *(undefined2 *)(puVar13 + 0xbc) = 0xffff;
        *(undefined1 *)((int)puVar13 + 0x2a7) = 2;
        if (puVar5[0xc9] == uVar17) {
          puVar5[0xc9] = 0xffffffff;
        }
        if (puVar5[0xca] == uVar17) {
          puVar5[0xca] = 0xffffffff;
        }
        FUN_0056ce30();
        FUN_0056d6a0();
        FUN_00565420(uVar17);
        puVar3 = (undefined4 *)(*(short *)((int)puVar13 + 0x1ea) + 0x10 + (int)puVar13);
        *puVar3 = uVar15;
        puVar3[1] = uVar6;
        puVar3[2] = uVar7;
        if ((short)puVar13[0x2d] == 0) {
          FUN_0055add0(uVar17);
        }
        object_recalculate_bounding_radius_recursive(uVar17);
        cVar10 = FUN_00566910();
        if ((cVar10 == '\x01') && (iVar12 = object_try_and_get(2), iVar12 != 0)) {
          *(undefined4 *)(iVar12 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
        }
        iVar12 = DAT_0087a480;
        if (DAT_00719720 == 1) {
          iVar18 = datum_get();
          if ((iVar18 != 0) && (*(short *)(iVar18 + 2) == -1)) {
            *(undefined4 *)(iVar18 + 0x180) = 0;
            *(undefined4 *)(iVar18 + 0x17c) = 0;
            *(undefined4 *)(iVar18 + 0x1e0) = 0;
            *(undefined4 *)(iVar18 + 0x1dc) = 0;
            goto LAB_0040b559;
          }
          goto LAB_0040b567;
        }
      }
    }
    else {
      cVar10 = FUN_00565c60();
      if (cVar10 == '\0') {
        iVar12 = *(int *)((*puVar13 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        iVar18 = *(int *)(*(int *)((*(uint *)(iVar12 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                         + 0x10);
        iVar14 = (char)puVar13[0xa8] * 100;
        if ((8 < *(int *)(iVar14 + 0x40 + iVar18)) &&
           (*(short *)(*(int *)(iVar14 + iVar18 + 0x44) + 0x10) != -1)) {
          if (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) + 0x324
                       ) == uVar17) {
            FUN_0056ab10();
          }
          uVar15 = FUN_004d6280(1);
          unit_set_custom_animation(*(undefined4 *)(iVar12 + 0x44),uVar15);
          iVar18 = (uVar17 & 0xffff) * 0xc;
          puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar18);
          iVar12 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if (*(int *)(iVar12 + 0x34) != -1) {
            if ((puVar5[4] & 1) != 0) {
              object_for_each_light_attachment(0,1);
            }
            if (*(int *)(iVar12 + 0x34) != -1) {
              iVar12 = *(int *)(DAT_008603b0 + 0x34);
              puVar5[4] = puVar5[4] & 0xfffffffe;
              pbVar1 = (byte *)(iVar12 + iVar18 + 2);
              *pbVar1 = *pbVar1 | 2;
            }
          }
          *(undefined1 *)((int)puVar13 + 0x2a3) = 0x1b;
          FUN_0042c370();
          if (puVar13[1] == 0) {
            FUN_0056c370(0);
          }
          iVar12 = DAT_006f1d6c;
          *(undefined4 *)(iVar2 + 0x390) = *(undefined4 *)(iVar2 + 0x158);
          *(int *)(iVar2 + 0x394) = *(int *)(iVar12 + 0xc) + 0xb4;
          local_ea = 1;
        }
      }
    }
  }
  *(undefined1 *)(iVar2 + 0x38c) = 0;
  uVar11 = local_ea;
LAB_0040b759:
  *(undefined1 *)(iVar2 + 0x2ed) = 0;
  return uVar11;
}
#endif
