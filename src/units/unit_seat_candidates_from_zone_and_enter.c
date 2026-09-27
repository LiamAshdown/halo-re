// unit_seat_candidates_from_zone_and_enter  (Ghidra: FUN_0056a4c0; the vehicle_load_magic script command's worker)
// address 0x56a4c0, size 1593 bytes, name confidence 0.3, rewrite confidence 0.9
// REWRITTEN from objdump 0x56a4c0..0x56aaf8. EAX: an object list, stack: (vehicle, seat name). The draft relinked a
//   NULL position and multiplied NULL matrices when a candidate was already seated elsewhere. For the vehicle's
//   seats matching the name (0x56a310, up to 16) and each live biped / vehicle in the list: a biped must be able
//   to use the seat (0x5651e0); a unit seated elsewhere is first taken out of that seat exactly as biped_update
//   does (helpers copied from biped_update.c, then scripted event 9 for a local unit and the client prediction
//   drop); then it enters the seat (0x566970), which is used up. Returns the number seated.
// blam-cc: stack -> vehicle_index, seat_name, EAX -> object_list

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *player_data;      // 0x0087a480
extern int16_t game_connection_role; // 0x00719720: 1 = client
extern game_time_globals *game_time; // 0x006f1d6c
extern uint8_t *network_client;      // 0x0071c2d8, +0xf48 the prediction history

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
extern void unit_reset_orientation_and_find_position(uint32_t object_index); // 0x55add0
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

extern int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector,
    int16_t *out_indices, int16_t max_indices); // 0x56a310
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
    uint8_t apply); // 0x5651e0, EAX, stack
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); // 0x566970, stack, EAX

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
        unit_reset_orientation_and_find_position(object_index);
    }
    object_recalculate_bounding_radius_recursive(object_index);
    if (unit_all_seats_unoccupied(vehicle_index) == 1) {
        uint8_t *empty = (uint8_t *)object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = game_time->game_time;
        }
    }
    if (game_connection_role == 1) {
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

    if (game_connection_role != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= *(int16_t *)((uint8_t *)player_data + 0x20)) {
        return;
    }
    player = (uint8_t *)player_data->data + *(int16_t *)((uint8_t *)player_data + 0x22) * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || *(int16_t *)(player + 2) == -1) {
        return;
    }
    if (network_client != 0) {
        player_update_history_free_all(*(void **)(network_client + 0xf48));
    }
}

// the next object of an object list walk: (reference) -> object, advancing *reference
static datum_index object_list_next(datum_index *reference)
{
    uint8_t *link;

    if (*reference == k_datum_index_none) {
        return k_datum_index_none;
    }
    link = (uint8_t *)object_list_reference_data->data + (*reference & 0xffff) * 0xc;
    *reference = *(datum_index *)(link + 0x8);
    return *(datum_index *)(link + 0x4);
}

int16_t unit_seat_candidates_from_zone_and_enter(datum_index vehicle_index, char *seat_name, datum_index object_list)
{
    int16_t seated = 0;             // [esp+0x24]
    uint8_t *vehicle;
    uint8_t *vehicle_tag;
    int16_t seats[16];              // [esp+0xa0]
    int16_t seat_count;             // [esp+0x34]
    datum_index reference = k_datum_index_none; // [esp+0x28]
    datum_index candidate_index;

    if (vehicle_index == k_datum_index_none) {
        return 0;
    }
    vehicle = OBJECT_DATA(vehicle_index);
    vehicle_tag = TAG_DATA(*(datum_index *)vehicle);
    seat_count = unit_find_seats_matching_name_and_flags(vehicle_index, seat_name, 0xffff, seats, 0x10);
    if (object_list != k_datum_index_none) {
        reference = *(datum_index *)((uint8_t *)object_list_header_data->data + (object_list & 0xffff) * 0xc + 0x8);
    }
    for (candidate_index = object_list_next(&reference); candidate_index != k_datum_index_none;
         candidate_index = object_list_next(&reference)) {
        uint8_t *candidate = OBJECT_DATA(candidate_index);
        int16_t i;

        if (!((1u << (candidate[0xb4] & 0x1f)) & 3) || (vehicle[0x106] & 4)) {
            continue;
        }
        for (i = 0; i < seat_count; i++) {
            int16_t seat = seats[i];

            if (seat == -1) {
                continue;
            }
            if (*(int16_t *)(candidate + 0xb4) != 1 &&
                !unit_set_or_test_seat_and_weapon_label(candidate_index,
                    (char *)(*(uint8_t **)(vehicle_tag + 0x2e8) + seat * 0x11c + 0x4), 0, 0)) {
                continue;
            }
            if (*(datum_index *)(candidate + 0x11c) != k_datum_index_none) {
                // 0x56a651: seated elsewhere; a server takes it out first
                if (*(int16_t *)(candidate + 0x2f0) != -1 && game_connection_role != 1) {
                    uint8_t *self = OBJECT_DATA(candidate_index);

                    if (*(datum_index *)(self + 0x11c) != k_datum_index_none && *(int16_t *)(self + 0x2f0) != -1) {
                        biped_detach_from_seat(candidate_index, *(datum_index *)(self + 0x11c));
                    }
                    if (*(int32_t *)(self + 0x4) == 0) {
                        unit_dispatch_scripted_event_9(1, (int32_t)candidate_index);
                    }
                    biped_free_local_player_history(self);
                }
                if (*(datum_index *)(candidate + 0x11c) != k_datum_index_none) {
                    continue;
                }
            }
            if (unit_enter_vehicle_seat(vehicle_index, seat, candidate_index) & 0xff) {
                seats[i] = -1;
                seated++;
                break;
            }
        }
    }
    return seated;
}

#if 0
Original Ghidra decompilation (0x56a4c0):

short FUN_0056a4c0(uint param_1,undefined4 param_2)

{
  byte *pbVar1;
  int iVar2;
  undefined4 *puVar3;
  short sVar4;
  uint *puVar5;
  int iVar6;
  uint uVar7;
  uint *puVar8;
  uint *puVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  int iVar13;
  uint *puVar14;
  short sVar15;
  char cVar16;
  short sVar17;
  short sVar18;
  uint in_EAX;
  int iVar19;
  short sVar20;
  short sVar21;
  int iVar22;
  uint local_128;
  uint local_110;
  undefined1 local_d0 [4];
  uint uStack_cc;
  uint uStack_c8;
  uint uStack_c4;
  uint uStack_b4;
  uint uStack_b0;
  uint uStack_ac;
  short local_98 [16];
  undefined1 local_78 [116];

  sVar15 = 0;
  if (param_1 != 0xffffffff) {
    puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    iVar6 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar17 = FUN_0056a310(param_1,param_2,0xffffffff,local_98,0x10);
    local_128 = 0xffffffff;
    if (in_EAX != 0xffffffff) {
      uVar7 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
      if (uVar7 == 0xffffffff) {
        local_128 = 0xffffffff;
        local_110 = 0xffffffff;
      }
      else {
        iVar2 = *(int *)(DAT_0087a468 + 0x34) + (uVar7 & 0xffff) * 0xc;
        local_110 = *(uint *)(iVar2 + 8);
        local_128 = *(uint *)(iVar2 + 4);
      }
    }
    if (local_128 != 0xffffffff) {
      do {
        iVar22 = (local_128 & 0xffff) * 0xc;
        iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar22);
        if ((((1 << (*(byte *)(iVar2 + 0xb4) & 0x1f) & 3U) != 0) &&
            ((*(byte *)((int)puVar5 + 0x106) & 4) == 0)) && (sVar18 = 0, 0 < sVar17)) {
          do {
            sVar4 = local_98[sVar18];
            if ((sVar4 != -1) &&
               ((*(short *)(iVar2 + 0xb4) == 1 ||
                (cVar16 = unit_set_or_test_seat_and_weapon_label
                                    (sVar4 * 0x11c + *(int *)(iVar6 + 0x2e8) + 4,0,0),
                cVar16 != '\0')))) {
              if (*(int *)(iVar2 + 0x11c) != -1) {
                if ((*(short *)(iVar2 + 0x2f0) != -1) && (DAT_00719720 != 1)) {
                  puVar8 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar22);
                  uVar7 = puVar8[0x47];
                  if ((uVar7 != 0xffffffff) && ((short)puVar8[0xbc] != -1)) {
                    puVar9 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
                    object_get_node_local_transform
                              (uVar7,*(int *)(*(int *)((*puVar9 & 0xffff) * 0x20 + 0x14 +
                                                      DAT_0087bc14) + 0x2e8) + 0x24 +
                                     (short)puVar8[0xbc] * 0x11c,local_78,1);
                    iVar19 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar8 & 0xffff) * 0x20 + 0x14
                                                                  + DAT_0087bc14) + 0x34) & 0xffff)
                                               * 0x20 + 0x14 + DAT_0087bc14) + 0xbc);
                    uVar10 = *(undefined4 *)(iVar19 + 0x28);
                    uVar11 = *(undefined4 *)(iVar19 + 0x2c);
                    uVar12 = *(undefined4 *)(iVar19 + 0x30);
                    if (((puVar9[0xc9] == local_128) && (*(char *)((int)puVar9 + 0x2a3) != '%')) &&
                       (puVar8[0x47] != 0xffffffff)) {
                      unit_try_set_animation_state(puVar8[0x47],0x25);
                    }
                    iVar13 = DAT_006f1d6c;
                    puVar8[0xcb] = uVar7;
                    puVar8[0xcc] = *(uint *)(iVar13 + 0xc);
                    if (puVar8[0xc9] == local_128) {
                      puVar8[0xc9] = 0xffffffff;
                    }
                    if (puVar8[0xca] == local_128) {
                      puVar8[0xca] = 0xffffffff;
                    }
                    FUN_004f6610(local_128);
                    object_set_position_and_orientation(local_128,0,0);
                    iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar22);
                    (*(code *)PTR_matrix4x3_multiply_00696664)
                              (*(short *)(iVar13 + 0x1f2) + iVar13,iVar19 + 0x68,local_d0);
                    puVar8[0x1d] = uStack_cc;
                    puVar8[0x1e] = uStack_c8;
                    puVar8[0x1f] = uStack_c4;
                    puVar8[0x20] = uStack_b4;
                    puVar8[0x21] = uStack_b0;
                    iVar19 = DAT_008603b0;
                    puVar8[0x22] = uStack_ac;
                    puVar14 = *(uint **)(*(int *)(iVar19 + 0x34) + 8 + iVar22);
                    iVar19 = *(int *)((*puVar14 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
                    if (*(int *)(iVar19 + 0x34) != -1) {
                      if ((puVar14[4] & 1) != 0) {
                        object_for_each_light_attachment(0,1);
                      }
                      if (*(int *)(iVar19 + 0x34) != -1) {
                        iVar19 = *(int *)(DAT_008603b0 + 0x34);
                        puVar14[4] = puVar14[4] & 0xfffffffe;
                        pbVar1 = (byte *)(iVar19 + iVar22 + 2);
                        *pbVar1 = *pbVar1 | 2;
                      }
                    }
                    *(undefined2 *)(puVar8 + 0xbc) = 0xffff;
                    *(undefined1 *)((int)puVar8 + 0x2a7) = 2;
                    if (puVar9[0xc9] == local_128) {
                      puVar9[0xc9] = 0xffffffff;
                    }
                    if (puVar9[0xca] == local_128) {
                      puVar9[0xca] = 0xffffffff;
                    }
                    FUN_0056ce30();
                    FUN_0056d6a0();
                    FUN_00565420(local_128);
                    puVar3 = (undefined4 *)(*(short *)((int)puVar8 + 0x1ea) + 0x10 + (int)puVar8);
                    *puVar3 = uVar10;
                    puVar3[1] = uVar11;
                    puVar3[2] = uVar12;
                    if ((short)puVar8[0x2d] == 0) {
                      FUN_0055add0(local_128);
                    }
                    object_recalculate_bounding_radius_recursive(local_128);
                    cVar16 = FUN_00566910();
                    if ((cVar16 == '\x01') && (iVar19 = object_try_and_get(2), iVar19 != 0)) {
                      *(undefined4 *)(iVar19 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
                    }
                    if (((DAT_00719720 == 1) && (iVar19 = datum_get(), iVar19 != 0)) &&
                       (*(short *)(iVar19 + 2) == -1)) {
                      *(undefined4 *)(iVar19 + 0x180) = 0;
                      *(undefined4 *)(iVar19 + 0x17c) = 0;
                      *(undefined4 *)(iVar19 + 0x1e0) = 0;
                      *(undefined4 *)(iVar19 + 0x1dc) = 0;
                    }
                  }
                  if (puVar8[1] == 0) {
                    FUN_0056c370(1);
                  }
                  if ((((DAT_00719720 == 1) && (uVar7 = puVar8[0x86], uVar7 != 0xffffffff)) &&
                      (sVar21 = (short)uVar7, -1 < sVar21)) &&
                     (sVar21 < *(short *)(DAT_0087a480 + 0x20))) {
                    iVar19 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar21;
                    sVar21 = *(short *)(iVar19 + *(int *)(DAT_0087a480 + 0x34));
                    if ((((sVar21 != 0) &&
                         ((sVar20 = (short)(uVar7 >> 0x10), sVar20 == 0 || (sVar21 == sVar20)))) &&
                        (*(short *)(iVar19 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1)) &&
                       (DAT_0071c2d8 != 0)) {
                      player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
                    }
                  }
                }
                if (*(int *)(iVar2 + 0x11c) != -1) goto LAB_0056aa90;
              }
              cVar16 = unit_enter_vehicle_seat(param_1,sVar4);
              if (cVar16 != '\0') {
                local_98[sVar18] = -1;
                sVar15 = sVar15 + 1;
                break;
              }
            }
LAB_0056aa90:
            sVar18 = sVar18 + 1;
          } while (sVar18 < sVar17);
        }
        if (local_110 == 0xffffffff) {
          local_128 = 0xffffffff;
        }
        else {
          iVar2 = *(int *)(DAT_0087a468 + 0x34) + (local_110 & 0xffff) * 0xc;
          local_110 = *(uint *)(iVar2 + 8);
          local_128 = *(uint *)(iVar2 + 4);
        }
        if (local_128 == 0xffffffff) {
          return sVar15;
        }
      } while( true );
    }
  }
  return 0;
}
#endif
