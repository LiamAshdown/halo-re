// unit_detach_child_at_named_seat  (Ghidra: FUN_0056ab50)
// address 0x56ab50, size 1848 bytes, name confidence 0.4, rewrite confidence 0.85
// REWRITTEN from objdump 0x56ab50..0x56b287. Stack: unit, seat name (NULL or empty = any seat). Walks every
//   unit (object iterator, types 3) seated in this unit whose seat label (tag seat +0x04, lowercased) contains
//   the name, and makes it leave: the same code as unit_try_exit_controlled_seat (0x56b5f0) inlined (sections
//   compared). Returns how many started their exit animation (a vehicle unit detached is not counted).
// blam-cc: stack -> unit_index, seat_marker_name

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
#include <string.h>
#include "networking.h"
#include "fn_ai.h"
#include "fn_units.h"
#include "fn_math.h"
#include "fn_items.h"
#include "fn_interface.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480
extern int16_t network_game_mode; // 0x00719720: 1 = client
extern game_time_globals *game_time; // 0x006f1d6c
extern network_client_globals *network_client;
extern uint8_t biped_detach_from_flipped_vehicle; // 0x006893cc
extern uint8_t unit_updates_suppressed; // 0x0071c419
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_point3d *global_origin3d_pointer;   // 0x00696714


extern void weapon_reset_triggers(datum_index item_index); // 0x4c4b50, stack
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


extern void biped_update_facing(uint32_t object_index, int8_t *out_animation_state); // 0x55b7c0, EAX, stack
extern void biped_integrate_movement_with_collision(uint32_t object_index, int8_t *state); // 0x55cfd0


extern void unit_update_footstep_and_idle_triggers(uint32_t unit_index); // 0x560410, EAX
extern void unit_update_up_vector(Biped *biped_tag, object *obj); // 0x560800, EAX, ECX

extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, ECX
extern void unit_start_seat_overlay_animation_a(uint32_t unit_index, int16_t command); // 0x565e00
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX, CX
extern void unit_notify_weapon_removed(int32_t object_index); // 0x56ab10, EAX (sets animation state 0x25)
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX, DX, stack


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

extern object *object_iterator_next(void *iterator); // 0x4f6f20, stack

// the 0x10-byte iterator this function builds on its stack (0x56abc8)
typedef struct unit_seat_iterator {
    uint32_t type_mask;
    uint8_t flags_mask;
    uint8_t unknown_05;
    int16_t index;
    datum_index handle;
    uint32_t signature;
} unit_seat_iterator;

int16_t unit_detach_child_at_named_seat(uint32_t unit_index, char *seat_marker_name)
{
    int16_t count = 0;
    uint8_t *unit_tag;
    uint8_t any_seat;
    unit_seat_iterator iterator;
    uint8_t *child;

    if (unit_index == 0xffffffff) {
        return 0;
    }
    unit_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(unit_index));
    any_seat = (uint8_t)(seat_marker_name == 0 || seat_marker_name[0] == 0);
    iterator.signature = 0x86868686;
    iterator.type_mask = 3;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;
    for (child = (uint8_t *)object_iterator_next(&iterator); child != 0;
         child = (uint8_t *)object_iterator_next(&iterator)) {
        datum_index child_index;
        uint8_t *self;
        char label[0x100];
        char *c;

        if (*(datum_index *)(child + 0x11c) != unit_index) {
            continue;
        }
        strcpy(label, (char *)(*(uint8_t **)(unit_tag + 0x2e8) + *(int16_t *)(child + 0x2f0) * 0x11c + 4));
        for (c = label; *c != 0; c++) {
            *c = (char)tolower((uint8_t)*c);
        }
        if (!any_seat && strstr(label, seat_marker_name) == 0) {
            continue;
        }
        child_index = iterator.handle;
        self = (uint8_t *)object_try_and_get(child_index, 3);
        if (self == 0 || network_game_mode == 1 || ((unit_object *)self)->base.parent_object == k_datum_index_none ||
            ((unit_object *)self)->unit.vehicle_seat_index == -1) {
            continue;
        }
        if (((unit_object *)self)->base.type == 1) {
            // 0x56ad05: a vehicle unit leaves its seat where it is (not counted)
            uint8_t *obj = OBJECT_DATA(child_index);
            datum_index vehicle_index = ((unit_object *)obj)->base.parent_object;

            if (vehicle_index != k_datum_index_none && ((unit_object *)obj)->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(child_index, vehicle_index);
            }
            biped_free_local_player_history(OBJECT_DATA(child_index));
            continue;
        }
        // 0x56b125: the seat's exit animation (slot 8), counted
        if (!unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
            uint8_t *self_tag = TAG_DATA(*(datum_index *)self);
            datum_index graph = *(datum_index *)(self_tag + 0x44);
            uint8_t *seat_block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)self[0x2a0] * 0x64;

            if (*(int32_t *)(seat_block + 0x40) > 8 && (*(int16_t **)(seat_block + 0x44))[8] != -1) {
                int16_t exit_animation = (*(int16_t **)(seat_block + 0x44))[8];
                datum_index vehicle_index = ((unit_object *)self)->base.parent_object;
                uint8_t *object;
                uint8_t *object_tag;

                if (*(datum_index *)(OBJECT_DATA(vehicle_index) + 0x324) == child_index) {
                    unit_notify_weapon_removed((int32_t)vehicle_index);
                }
                unit_set_custom_animation(child_index, *(datum_index *)(self_tag + 0x44),
                    animation_choose_random_permutation(graph, exit_animation, 1));
                object = OBJECT_DATA(child_index);
                object_tag = TAG_DATA(*(datum_index *)object);
                if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                    if ((object[0x10] & 1) != 0) {
                        object_for_each_light_attachment(child_index, 0, 1);
                    }
                    if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                        *(uint32_t *)(object + 0x10) &= ~1u;
                        OBJECT_HEADER(child_index).flags |= 2;
                    }
                }
                self[0x2a3] = 0x1b;
                actor_notify_weapon_pickup_once(child_index);
                if (((unit_object *)self)->base.network_role == 0) {
                    unit_dispatch_scripted_event_9(0, (int32_t)child_index);
                }
                count++;
            }
        }
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x56ab50):

short FUN_0056ab50(uint param_1,char *param_2)

{
  byte *pbVar1;
  undefined4 *puVar2;
  byte bVar3;
  uint uVar4;
  uint *puVar5;
  undefined4 uVar6;
  float fVar7;
  uint *puVar8;
  uint uVar9;
  bool bVar10;
  short sVar11;
  char cVar12;
  char *pcVar13;
  int iVar14;
  uint *puVar15;
  int iVar16;
  undefined4 uVar17;
  short sVar18;
  short sVar19;
  byte *pbVar20;
  int iVar21;
  undefined4 local_1dc;
  undefined1 local_1d8;
  undefined2 local_1d6;
  uint local_1d4;
  undefined4 local_1d0;
  int local_1cc;
  float local_1c8;
  float local_1c4;
  float local_1c0;
  float local_1bc;
  float local_1b8;
  float local_1b4;
  undefined1 local_1b0 [4];
  uint uStack_1ac;
  uint uStack_1a8;
  uint uStack_1a4;
  uint uStack_194;
  uint uStack_190;
  uint uStack_18c;
  undefined1 local_178 [96];
  float local_118;
  float local_114;
  float local_110;
  char acStack_10c [4];
  byte local_108 [260];

  sVar11 = 0;
  if (param_1 == 0xffffffff) {
    return 0;
  }
  local_1cc = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) &
                       0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_2 != (char *)0x0) {
    pcVar13 = param_2;
    do {
      cVar12 = *pcVar13;
      pcVar13 = pcVar13 + 1;
    } while (cVar12 != '\0');
    bVar10 = false;
    if (pcVar13 != param_2 + 1) goto LAB_0056abc8;
  }
  bVar10 = true;
LAB_0056abc8:
  local_1d0 = 0x86868686;
  local_1dc = 3;
  local_1d8 = 0;
  local_1d6 = 0;
  local_1d4 = 0xffffffff;
  iVar14 = object_iterator_next(&local_1dc);
  if (iVar14 == 0) {
    return 0;
  }
  do {
    if (*(uint *)(iVar14 + 0x11c) == param_1) {
      sVar19 = *(short *)(iVar14 + 0x2f0);
      iVar14 = *(int *)(local_1cc + 0x2e8);
      pcVar13 = (char *)(sVar19 * 0x11c + 4 + iVar14);
      do {
        cVar12 = *pcVar13;
        pcVar13[(int)(acStack_10c + (sVar19 * -0x11c - iVar14))] = cVar12;
        pcVar13 = pcVar13 + 1;
      } while (cVar12 != '\0');
      pbVar20 = local_108;
      bVar3 = local_108[0];
      while (bVar3 != 0) {
        iVar14 = _tolower((uint)*pbVar20);
        *pbVar20 = (byte)iVar14;
        pbVar1 = pbVar20 + 1;
        pbVar20 = pbVar20 + 1;
        bVar3 = *pbVar1;
      }
      if ((((bVar10) || (iVar14 = FUN_00625430(local_108,param_2), iVar14 != 0)) &&
          (uVar9 = local_1d4, puVar15 = (uint *)object_try_and_get(3), puVar15 != (uint *)0x0)) &&
         (((DAT_00719720 != 1 && (uVar4 = puVar15[0x47], uVar4 != 0xffffffff)) &&
          ((short)puVar15[0xbc] != -1)))) {
        if ((short)puVar15[0x2d] == 1) {
          iVar21 = (uVar9 & 0xffff) * 0xc;
          puVar15 = *(uint **)(iVar21 + 8 + *(int *)(DAT_008603b0 + 0x34));
          uVar4 = puVar15[0x47];
          iVar14 = DAT_0087a480;
          if ((uVar4 == 0xffffffff) || ((short)puVar15[0xbc] == -1)) {
LAB_0056b09b:
            if (DAT_00719720 == 1) {
LAB_0056b0a8:
              uVar9 = puVar15[0x86];
              if (((uVar9 != 0xffffffff) && (sVar19 = (short)uVar9, -1 < sVar19)) &&
                 (sVar19 < *(short *)(iVar14 + 0x20))) {
                iVar21 = (int)*(short *)(iVar14 + 0x22) * (int)sVar19;
                sVar19 = *(short *)(iVar21 + *(int *)(iVar14 + 0x34));
                if ((((sVar19 != 0) &&
                     ((sVar18 = (short)(uVar9 >> 0x10), sVar18 == 0 || (sVar19 == sVar18)))) &&
                    (*(short *)(iVar21 + *(int *)(iVar14 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)
                   ) {
                  player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
                }
              }
            }
          }
          else {
            puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
            iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar21);
            iVar14 = *(short *)(iVar14 + 0x1f2) + iVar14;
            object_get_node_local_transform
                      (uVar4,*(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                     0x2e8) + 0x24 + (short)puVar15[0xbc] * 0x11c,local_178,1);
            local_1c8 = *(float *)(iVar14 + 0x28) - local_118;
            local_1c4 = *(float *)(iVar14 + 0x2c) - local_114;
            iVar16 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar15 & 0xffff) * 0x20 + 0x14 +
                                                          DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                       0x14 + DAT_0087bc14) + 0xbc);
            uVar17 = *(undefined4 *)(iVar16 + 0x28);
            local_1c0 = *(float *)(iVar14 + 0x30) - local_110;
            uVar6 = *(undefined4 *)(iVar16 + 0x2c);
            fVar7 = *(float *)(iVar16 + 0x30);
            if ((puVar5[0xc9] == uVar9) &&
               ((*(char *)((int)puVar5 + 0x2a3) != '%' && (puVar15[0x47] != 0xffffffff)))) {
              unit_try_set_animation_state(puVar15[0x47],0x25);
            }
            iVar14 = DAT_006f1d6c;
            puVar15[0xcb] = uVar4;
            puVar15[0xcc] = *(uint *)(iVar14 + 0xc);
            if (puVar15[0xc9] == uVar9) {
              puVar15[0xc9] = 0xffffffff;
            }
            if (puVar15[0xca] == uVar9) {
              puVar15[0xca] = 0xffffffff;
            }
            FUN_004f6610(uVar9);
            local_1bc = local_1c8 + (float)puVar15[0x17];
            local_1b8 = local_1c4 + (float)puVar15[0x18];
            local_1b4 = (local_1c0 + (float)puVar15[0x19]) - fVar7;
            object_set_position_and_orientation(uVar9,0,0);
            iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar21);
            (*(code *)PTR_matrix4x3_multiply_00696664)
                      (*(short *)(iVar14 + 0x1f2) + iVar14,iVar16 + 0x68,local_1b0);
            puVar15[0x1d] = uStack_1ac;
            puVar15[0x1e] = uStack_1a8;
            puVar15[0x1f] = uStack_1a4;
            puVar15[0x20] = uStack_194;
            puVar15[0x21] = uStack_190;
            iVar14 = DAT_008603b0;
            puVar15[0x22] = uStack_18c;
            puVar8 = *(uint **)(*(int *)(iVar14 + 0x34) + 8 + iVar21);
            iVar14 = *(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (*(int *)(iVar14 + 0x34) != -1) {
              if ((puVar8[4] & 1) != 0) {
                object_for_each_light_attachment(0,1);
              }
              if (*(int *)(iVar14 + 0x34) != -1) {
                iVar14 = *(int *)(DAT_008603b0 + 0x34);
                puVar8[4] = puVar8[4] & 0xfffffffe;
                pbVar20 = (byte *)(iVar14 + iVar21 + 2);
                *pbVar20 = *pbVar20 | 2;
              }
            }
            *(undefined2 *)(puVar15 + 0xbc) = 0xffff;
            *(undefined1 *)((int)puVar15 + 0x2a7) = 2;
            if (puVar5[0xc9] == uVar9) {
              puVar5[0xc9] = 0xffffffff;
            }
            if (puVar5[0xca] == uVar9) {
              puVar5[0xca] = 0xffffffff;
            }
            FUN_0056ce30();
            FUN_0056d6a0();
            FUN_00565420(uVar9);
            puVar2 = (undefined4 *)(*(short *)((int)puVar15 + 0x1ea) + 0x10 + (int)puVar15);
            *puVar2 = uVar17;
            puVar2[1] = uVar6;
            puVar2[2] = fVar7;
            if ((short)puVar15[0x2d] == 0) {
              FUN_0055add0(uVar9);
            }
            object_recalculate_bounding_radius_recursive(uVar9);
            cVar12 = FUN_00566910();
            if ((cVar12 == '\x01') && (iVar14 = object_try_and_get(2), iVar14 != 0)) {
              *(undefined4 *)(iVar14 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
            }
            iVar14 = DAT_0087a480;
            if (DAT_00719720 == 1) {
              iVar21 = datum_get();
              if ((iVar21 != 0) && (*(short *)(iVar21 + 2) == -1)) {
                *(undefined4 *)(iVar21 + 0x180) = 0;
                *(undefined4 *)(iVar21 + 0x17c) = 0;
                *(undefined4 *)(iVar21 + 0x1e0) = 0;
                *(undefined4 *)(iVar21 + 0x1dc) = 0;
                goto LAB_0056b09b;
              }
              goto LAB_0056b0a8;
            }
          }
        }
        else {
          cVar12 = FUN_00565c60();
          if (cVar12 == '\0') {
            iVar14 = *(int *)((*puVar15 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            iVar21 = *(int *)(*(int *)((*(uint *)(iVar14 + 0x44) & 0xffff) * 0x20 + 0x14 +
                                      DAT_0087bc14) + 0x10);
            iVar16 = (char)puVar15[0xa8] * 100;
            if ((8 < *(int *)(iVar16 + 0x40 + iVar21)) &&
               (*(short *)(*(int *)(iVar16 + iVar21 + 0x44) + 0x10) != -1)) {
              if (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) +
                           0x324) == uVar9) {
                FUN_0056ab10();
              }
              uVar17 = FUN_004d6280(1);
              unit_set_custom_animation(*(undefined4 *)(iVar14 + 0x44),uVar17);
              iVar21 = (uVar9 & 0xffff) * 0xc;
              puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar21);
              iVar14 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
              if (*(int *)(iVar14 + 0x34) != -1) {
                if ((puVar5[4] & 1) != 0) {
                  object_for_each_light_attachment(0,1);
                }
                if (*(int *)(iVar14 + 0x34) != -1) {
                  iVar14 = *(int *)(DAT_008603b0 + 0x34);
                  puVar5[4] = puVar5[4] & 0xfffffffe;
                  pbVar20 = (byte *)(iVar14 + iVar21 + 2);
                  *pbVar20 = *pbVar20 | 2;
                }
              }
              *(undefined1 *)((int)puVar15 + 0x2a3) = 0x1b;
              FUN_0042c370();
              if (puVar15[1] == 0) {
                FUN_0056c370(0);
              }
              sVar11 = sVar11 + 1;
            }
          }
        }
      }
    }
    iVar14 = object_iterator_next(&local_1dc);
    if (iVar14 == 0) {
      return sVar11;
    }
  } while( true );
}
#endif
