// unit_detach_from_seat  (Ghidra: FUN_0056c640)
// address 0x56c640, size 1024 bytes, name confidence 0.45, rewrite confidence 0.85
// REWRITTEN from objdump 0x56c640..0x56ca3f: the out-of-line form of the seat detach biped_update inlines
//   (instruction sequences compared, register allocation only differs). Stack: unit, suppress_trigger,
//   require_client_flag, fire_trigger_event. A client does nothing unless require_client_flag is 1. A seated
//   unit is taken out of its seat where its body is; unless suppressed, fire_trigger_event 1 raises scripted
//   event 9 (1) for a local object (+0x04 == 0); a client then drops a local player's prediction history.
// blam-cc: stack -> unit_index, suppress_trigger, require_client_flag, fire_trigger_event

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
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

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void actor_notify_weapon_pickup_once(datum_index object_index); // 0x42c370, ECX
extern void weapon_action_notify_for_unit(datum_index unit_index, int32_t action_code); // 0x492730, EAX, stack
extern uint32_t weapon_prevents_melee_attack(datum_index item_index); // 0x4c2ee0, ECX
extern int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index, int16_t category,
    int16_t mode); // 0x4c2f80, EAX, CX, stack
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
extern void unit_recalculate_position(uint32_t object_index); // 0x558eb0, EAX
extern void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index); // 0x55add0, stack, EDI
extern void biped_update_facing(uint32_t object_index, int8_t *out_animation_state); // 0x55b7c0, EAX, stack
extern void biped_integrate_movement_with_collision(uint32_t object_index, int8_t *state); // 0x55cfd0
extern void biped_check_evade_reaction(uint32_t object_index); // 0x55e190
extern void unit_evaluate_flee_reaction(uint32_t object_index); // 0x55e2d0, EDI
extern void unit_check_fell_off_level(uint32_t object_index); // 0x55e4a0, ECX
extern void biped_update_idle_basis(uint32_t object_index, uint8_t *state_out); // 0x55e840, ESI, EDI
extern void biped_apply_idle_fidget(uint32_t object_index, uint8_t *state_out); // 0x55e940, EDI, stack
extern void biped_advance_frame_counter_trigger(uint32_t object_index, char *state_out); // 0x55eb90, EAX, stack
extern void biped_trigger_on_velocity_threshold(uint32_t object_index); // 0x55ec20, EAX
extern uint32_t unit_snap_to_min_ground_height(uint32_t object_index); // 0x55ecf0
extern void unit_update_footstep_and_idle_triggers(uint32_t unit_index); // 0x560410, EAX
extern void unit_update_up_vector(Biped *biped_tag, object *obj); // 0x560800, EAX, ECX
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack, ECX
extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, ECX
extern void unit_start_seat_overlay_animation_a(uint32_t unit_index, int16_t command); // 0x565e00
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX, CX
extern void unit_notify_weapon_removed(int32_t object_index); // 0x56ab10, EAX (sets animation state 0x25)
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); // 0x56ebd0
extern void unit_melee_attack_scan(uint32_t unit_index); // 0x56f550

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

void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag,
    uint8_t fire_trigger_event)
{
    uint8_t *obj;

    if (network_game_mode == 1 && require_client_flag != 1) {
        return;
    }
    obj = OBJECT_DATA(unit_index);
    if (((unit_object *)obj)->base.parent_object != k_datum_index_none && ((unit_object *)obj)->unit.vehicle_seat_index != -1) {
        biped_detach_from_seat(unit_index, ((unit_object *)obj)->base.parent_object);
    }
    if (!suppress_trigger && fire_trigger_event == 1 && ((unit_object *)obj)->base.network_role == 0) {
        unit_dispatch_scripted_event_9(1, (int32_t)unit_index);
    }
    biped_free_local_player_history(obj);
}

#if 0
Original Ghidra decompilation (0x56c640):

void FUN_0056c640(uint param_1,char param_2,char param_3,char param_4)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint *puVar4;
  uint uVar5;
  char cVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  undefined1 local_dc [96];
  float local_7c;
  float local_78;
  float local_74;
  undefined1 local_6c [4];
  uint local_68;
  uint local_64;
  uint local_60;
  uint local_50;
  uint local_4c;
  uint local_48;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  undefined4 local_1c;
  undefined4 local_18;
  float local_14;
  int local_10;
  uint local_c;
  undefined4 local_8;

  if ((DAT_00719720 != 1) || (param_3 == '\x01')) {
    iVar11 = *(int *)(DAT_008603b0 + 0x34);
    iVar9 = (param_1 & 0xffff) * 0xc;
    puVar3 = *(uint **)(iVar9 + 8 + iVar11);
    local_c = puVar3[0x47];
    iVar10 = DAT_0087a480;
    if ((local_c != 0xffffffff) && ((short)puVar3[0xbc] != -1)) {
      local_8 = *(uint **)(iVar11 + 8 + (local_c & 0xffff) * 0xc);
      iVar11 = *(int *)(iVar11 + 8 + iVar9);
      iVar11 = *(short *)(iVar11 + 0x1f2) + iVar11;
      object_get_node_local_transform
                (local_c,*(int *)(*(int *)((*local_8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8
                                 ) + 0x24 + (short)puVar3[0xbc] * 0x11c,local_dc,1);
      local_28 = *(float *)(iVar11 + 0x28) - local_7c;
      local_24 = *(float *)(iVar11 + 0x2c) - local_78;
      iVar10 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                                    ) + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                ) + 0xbc);
      local_10 = iVar10 + 0x68;
      local_20 = *(float *)(iVar11 + 0x30) - local_74;
      local_1c = *(undefined4 *)(iVar10 + 0x28);
      local_18 = *(undefined4 *)(iVar10 + 0x2c);
      local_14 = *(float *)(iVar10 + 0x30);
      if (((local_8[0xc9] == param_1) && (*(char *)((int)local_8 + 0x2a3) != '%')) &&
         (puVar3[0x47] != 0xffffffff)) {
        unit_try_set_animation_state(puVar3[0x47],0x25);
      }
      iVar11 = DAT_006f1d6c;
      puVar3[0xcb] = local_c;
      puVar3[0xcc] = *(uint *)(iVar11 + 0xc);
      if (puVar3[0xc9] == param_1) {
        puVar3[0xc9] = 0xffffffff;
      }
      if (puVar3[0xca] == param_1) {
        puVar3[0xca] = 0xffffffff;
      }
      FUN_004f6610(param_1);
      local_34 = local_28 + (float)puVar3[0x17];
      local_30 = local_24 + (float)puVar3[0x18];
      local_2c = (local_20 + (float)puVar3[0x19]) - local_14;
      object_set_position_and_orientation(param_1,0,0);
      iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
      (*(code *)PTR_matrix4x3_multiply_00696664)
                (*(short *)(iVar11 + 0x1f2) + iVar11,local_10,local_6c);
      puVar3[0x1d] = local_68;
      puVar3[0x1e] = local_64;
      puVar3[0x1f] = local_60;
      puVar3[0x20] = local_50;
      puVar3[0x21] = local_4c;
      iVar11 = DAT_008603b0;
      puVar3[0x22] = local_48;
      puVar4 = *(uint **)(*(int *)(iVar11 + 0x34) + 8 + iVar9);
      local_10 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(int *)(local_10 + 0x34) != -1) {
        if ((puVar4[4] & 1) != 0) {
          object_for_each_light_attachment(0,1);
        }
        if (*(int *)(local_10 + 0x34) != -1) {
          iVar11 = *(int *)(DAT_008603b0 + 0x34);
          puVar4[4] = puVar4[4] & 0xfffffffe;
          pbVar1 = (byte *)(iVar11 + iVar9 + 2);
          *pbVar1 = *pbVar1 | 2;
        }
      }
      *(undefined2 *)(puVar3 + 0xbc) = 0xffff;
      *(undefined1 *)((int)puVar3 + 0x2a7) = 2;
      if (local_8[0xc9] == param_1) {
        local_8[0xc9] = 0xffffffff;
      }
      if (local_8[0xca] == param_1) {
        local_8[0xca] = 0xffffffff;
      }
      FUN_0056ce30();
      FUN_0056d6a0();
      local_8 = (uint *)(uint)CONCAT12(0x14,(undefined2)local_8);
      FUN_00565420(param_1);
      puVar2 = (undefined4 *)(*(short *)((int)puVar3 + 0x1ea) + 0x10 + (int)puVar3);
      *puVar2 = local_1c;
      puVar2[1] = local_18;
      puVar2[2] = local_14;
      if ((short)puVar3[0x2d] == 0) {
        FUN_0055add0(param_1);
      }
      object_recalculate_bounding_radius_recursive(param_1);
      cVar6 = FUN_00566910();
      if ((cVar6 == '\x01') && (iVar11 = object_try_and_get(2), iVar11 != 0)) {
        *(undefined4 *)(iVar11 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
      }
      iVar10 = DAT_0087a480;
      if (((DAT_00719720 == 1) && (iVar11 = datum_get(), iVar11 != 0)) &&
         (*(short *)(iVar11 + 2) == -1)) {
        *(undefined4 *)(iVar11 + 0x180) = 0;
        *(undefined4 *)(iVar11 + 0x17c) = 0;
        *(undefined4 *)(iVar11 + 0x1e0) = 0;
        *(undefined4 *)(iVar11 + 0x1dc) = 0;
      }
    }
    if (((param_2 == '\0') && (param_4 == '\x01')) && (puVar3[1] == 0)) {
      FUN_0056c370(1);
      iVar10 = DAT_0087a480;
    }
    if (((DAT_00719720 == 1) && (uVar5 = puVar3[0x86], uVar5 != 0xffffffff)) &&
       ((sVar8 = (short)uVar5, -1 < sVar8 && (sVar8 < *(short *)(iVar10 + 0x20))))) {
      iVar11 = (int)*(short *)(iVar10 + 0x22) * (int)sVar8;
      sVar8 = *(short *)(iVar11 + *(int *)(iVar10 + 0x34));
      if (((sVar8 != 0) && ((sVar7 = (short)(uVar5 >> 0x10), sVar7 == 0 || (sVar8 == sVar7)))) &&
         ((*(short *)(iVar11 + *(int *)(iVar10 + 0x34) + 2) != -1 && (DAT_0071c2d8 != 0)))) {
        player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
