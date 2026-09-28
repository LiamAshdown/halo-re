// unit_release_transient_state  (Ghidra: FUN_00568610)
// address 0x568610, size 1696 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// REWRITTEN from objdump 0x568610..0x568caf. Stack: (unit, knocked_down). A kill (knocked_down 0) clears the
//   knock-down timer (+0x420), leaves the object list (0x4f7450, ECX unit, stack 1), detaches the player
//   (+0x218, 0x474e10) and releases the actor (+0x1f4, 0x428ab0) and swarm actor (+0x1f8, 0x428e50), keeping
//   their +0x34 / +0x3a words in +0x334 / +0x336. A knock-down instead rolls the tag's +0x248 chance into flag
//   0x2000 (+0x204, one LCG step). Both then stamp the tick (+0x41c), clear +0x204 bits 0x11 and +0x208, reset
//   the current weapon's +0x230 / +0x234 (transition function 4 at 0), clear +0x204 bit 0x2000000 and, for a
//   unit in a seat (never on a client), leave it exactly as biped_update does (helpers copied from
//   biped_update.c), or with a parent but no seat reposition it (0x56ca40). Finally the queued speech
//   (+0x3b8) is cleared, the inventory, the held object (+0x318) and the grenades dropped, the current weapon
//   dropped too unless +0x28c, the seat overlays (+0x2aa, +0x2ae) reset, the melee state (+0x289) and +0x28d
//   cleared.
// blam-cc: stack -> (unit_index, knocked_down)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>
#include "networking.h"
#include "ai.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480
extern data_array *actor_data;      // 0x00880360
extern int16_t network_game_mode; // 0x00719720: 1 = client
extern game_time_globals *game_time; // 0x006f1d6c
extern network_client_globals *network_client;
extern uint32_t random_seed_global;  // 0x00719cd0

extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, ECX, stack
extern void player_reset_after_unit_change(uint32_t player_index); // 0x474e10, stack
extern void actor_attempt_grenade_throw(datum_index actor_index); // 0x428ab0, stack (it releases the actor)
extern void actor_release_from_cluster_or_delete(datum_index actor_index, datum_index unit_index); // 0x428e50, EAX, stack
extern real transition_function_evaluate(transition_function_t type, real phase); // 0x4ccac0, CX, stack
extern void unit_detach_reposition_and_nudge(uint32_t unit_index); // 0x56ca40, EDI
extern void unit_drop_inventory_weapons(uint32_t unit_index); // 0x56f060
extern void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index); // 0x56ed00
extern void unit_drop_grenades(uint32_t unit_index); // 0x56ef60
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0

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

void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset)
{
    uint8_t *obj = OBJECT_DATA(unit_index);

    if (is_light_reset == 0) {
        ((struct unit_object *)obj)->unit.unknown_420 = 0;
        object_list_membership_set(unit_index, 1);
        if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
            player_reset_after_unit_change(((unit_object *)obj)->unit.controlling_player);
            ((unit_object *)obj)->unit.controlling_player = k_datum_index_none;
        }
        if (((unit_object *)obj)->unit.actor_index != k_datum_index_none) {
            datum_index actor_index = ((unit_object *)obj)->unit.actor_index;
            uint8_t *actor_record = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

            ((struct unit_object *)obj)->unit.unknown_334 = *(int16_t *)&((actor *)actor_record)->encounter_index;
            ((struct unit_object *)obj)->unit.unknown_336 = ((actor *)actor_record)->squad_index;
            actor_attempt_grenade_throw(actor_index);
            ((unit_object *)obj)->unit.actor_index = k_datum_index_none;
        }
        if (((unit_object *)obj)->unit.swarm_actor_index != k_datum_index_none) {
            datum_index swarm_index = ((unit_object *)obj)->unit.swarm_actor_index;
            uint8_t *actor_record = (uint8_t *)actor_data->data + (swarm_index & 0xffff) * 0x724;

            ((struct unit_object *)obj)->unit.unknown_334 = *(int16_t *)&((actor *)actor_record)->encounter_index;
            ((struct unit_object *)obj)->unit.unknown_336 = ((actor *)actor_record)->squad_index;
            actor_release_from_cluster_or_delete(swarm_index, unit_index);
            ((unit_object *)obj)->unit.swarm_actor_index = k_datum_index_none;
        }
    } else {
        uint8_t *unit_tag = TAG_DATA(*(datum_index *)obj);

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        if ((float)(int32_t)(random_seed_global >> 16) * 1.5259022e-05f < *(float *)(unit_tag + 0x248)) {
            ((unit_object *)obj)->unit.flags |= 0x2000;
        } else {
            ((unit_object *)obj)->unit.flags &= 0xffffdfff;
        }
    }
    ((struct unit_object *)obj)->unit.unknown_41c = game_time->game_time;
    ((unit_object *)obj)->unit.flags &= 0xffffffee;
    ((unit_object *)obj)->unit.control_flags = 0;
    if (((unit_object *)obj)->unit.current_weapon_index != -1) {
        uint8_t *unit = OBJECT_DATA(unit_index);
        int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;
        datum_index weapon_index = (slot != -1) ? *(datum_index *)(unit + 0x2f8 + slot * 4) : k_datum_index_none;
        uint8_t *weapon = OBJECT_DATA(weapon_index);

        *(int16_t *)&((struct weapon_object *)weapon)->weapon.control_flags = 0;
        ((struct weapon_object *)weapon)->weapon.primary_trigger = transition_function_evaluate((transition_function_t)4, 0.0f);
    }
    *(uint32_t *)(OBJECT_DATA(unit_index) + 0x204) &= 0xfdffffff;
    if (((unit_object *)obj)->base.parent_object != k_datum_index_none) {
        if (((unit_object *)obj)->unit.vehicle_seat_index == -1) {
            unit_detach_reposition_and_nudge(unit_index);
        } else if (network_game_mode != 1) {
            uint8_t *me = OBJECT_DATA(unit_index);

            if (((struct object *)me)->parent_object != k_datum_index_none && *(int16_t *)(me + 0x2f0) != -1) {
                biped_detach_from_seat(unit_index, ((struct object *)me)->parent_object);
            }
            biped_free_local_player_history(me);
        }
    }
    ((unit_object *)obj)->unit.pending_speech.priority = 0;
    unit_drop_inventory_weapons(unit_index);
    {
        uint8_t *holder = OBJECT_DATA(unit_index);

        if (*(datum_index *)(holder + 0x318) != k_datum_index_none) {
            unit_drop_object_from_hand(unit_index, *(datum_index *)(holder + 0x318));
            *(datum_index *)(holder + 0x318) = k_datum_index_none;
        }
    }
    unit_drop_grenades(unit_index);
    if (obj[0x28c] == 0) {
        unit_drop_current_weapon(unit_index, 1);
    }
    *(int16_t *)(obj + 0x2ae) = -1;
    *(int16_t *)(obj + 0x2aa) = -1;
    obj[0x289] = 0;
    if (obj[0x28d] == 1) {
        obj[0x28d] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x568610):

void FUN_00568610(uint param_1,char param_2)

{
  byte *pbVar1;
  uint *puVar2;
  undefined4 *puVar3;
  uint *puVar4;
  char cVar5;
  int iVar6;
  uint uVar7;
  short sVar8;
  short sVar9;
  int iVar10;
  uint *puVar11;
  int iVar12;
  float10 fVar13;
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
  uint *local_10;
  uint local_c;
  int local_8;

  iVar12 = DAT_008603b0;
  iVar10 = (param_1 & 0xffff) * 0xc;
  puVar11 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar10);
  local_10 = puVar11;
  local_8 = iVar10;
  if (param_2 == '\0') {
    *(undefined2 *)(puVar11 + 0x108) = 0;
    FUN_004f7450(1);
    if (puVar11[0x86] != 0xffffffff) {
      FUN_00474e10(puVar11[0x86]);
      iVar12 = DAT_008603b0;
      puVar11[0x86] = 0xffffffff;
    }
    uVar7 = puVar11[0x7d];
    if (uVar7 != 0xffffffff) {
      iVar12 = *(int *)(DAT_00880360 + 0x34);
      iVar6 = (uVar7 & 0xffff) * 0x724;
      *(undefined2 *)(puVar11 + 0xcd) = *(undefined2 *)(iVar6 + 0x34 + iVar12);
      *(undefined2 *)((int)puVar11 + 0x336) = *(undefined2 *)(iVar6 + iVar12 + 0x3a);
      actor_attempt_grenade_throw(uVar7);
      iVar12 = DAT_008603b0;
      puVar11[0x7d] = 0xffffffff;
    }
    if (puVar11[0x7e] != 0xffffffff) {
      iVar12 = *(int *)(DAT_00880360 + 0x34);
      iVar6 = (puVar11[0x7e] & 0xffff) * 0x724;
      *(undefined2 *)(puVar11 + 0xcd) = *(undefined2 *)(iVar6 + 0x34 + iVar12);
      *(undefined2 *)((int)puVar11 + 0x336) = *(undefined2 *)(iVar6 + iVar12 + 0x3a);
      FUN_00428e50(param_1);
      iVar12 = DAT_008603b0;
      puVar11[0x7e] = 0xffffffff;
    }
    puVar11[0x107] = *(uint *)(DAT_006f1d6c + 0xc);
  }
  else {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    if (*(float *)(*(int *)((*puVar11 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x248) <=
        (float)(random_seed_global >> 0x10) * 1.5259022e-05) {
      puVar11[0x81] = puVar11[0x81] & 0xffffdfff;
    }
    else {
      puVar11[0x81] = puVar11[0x81] | 0x2000;
    }
  }
  uVar7 = 0xffffffff;
  puVar11[0x81] = puVar11[0x81] & 0xffffffee;
  puVar11[0x82] = 0;
  if (*(short *)((int)puVar11 + 0x2f2) != -1) {
    iVar10 = *(int *)(iVar10 + 8 + *(int *)(iVar12 + 0x34));
    sVar9 = *(short *)(iVar10 + 0x2f2);
    if (sVar9 != -1) {
      uVar7 = *(uint *)(iVar10 + 0x2f8 + sVar9 * 4);
    }
    iVar10 = *(int *)(*(int *)(iVar12 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
    *(undefined2 *)(iVar10 + 0x230) = 0;
    fVar13 = (float10)transition_function_evaluate(0);
    *(float *)(iVar10 + 0x234) = (float)fVar13;
    iVar10 = local_8;
  }
  puVar2 = (uint *)(*(int *)(iVar10 + 8 + *(int *)(iVar12 + 0x34)) + 0x204);
  *puVar2 = *puVar2 & 0xfdffffff;
  if (puVar11[0x47] == 0xffffffff) goto LAB_00568c1f;
  if ((short)puVar11[0xbc] == -1) {
    FUN_0056ca40();
    goto LAB_00568c1f;
  }
  if (DAT_00719720 == 1) goto LAB_00568c1f;
  iVar12 = *(int *)(iVar12 + 0x34);
  puVar2 = *(uint **)(iVar10 + 8 + iVar12);
  local_c = puVar2[0x47];
  iVar6 = DAT_0087a480;
  if ((local_c == 0xffffffff) || ((short)puVar2[0xbc] == -1)) {
LAB_00568ba2:
    iVar10 = local_8;
    if (DAT_00719720 != 1) goto LAB_00568c1f;
  }
  else {
    puVar11 = *(uint **)(iVar12 + 8 + (local_c & 0xffff) * 0xc);
    iVar12 = *(int *)(iVar12 + 8 + local_8);
    iVar12 = *(short *)(iVar12 + 0x1f2) + iVar12;
    object_get_node_local_transform
              (local_c,*(int *)(*(int *)((*puVar11 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)
                       + 0x24 + (short)puVar2[0xbc] * 0x11c,local_dc,1);
    local_28 = *(float *)(iVar12 + 0x28) - local_7c;
    local_24 = *(float *)(iVar12 + 0x2c) - local_78;
    iVar10 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                                         + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0xbc);
    local_20 = *(float *)(iVar12 + 0x30) - local_74;
    local_1c = *(undefined4 *)(iVar10 + 0x28);
    local_18 = *(undefined4 *)(iVar10 + 0x2c);
    local_14 = *(float *)(iVar10 + 0x30);
    if ((puVar11[0xc9] == param_1) &&
       ((*(char *)((int)puVar11 + 0x2a3) != '%' && (puVar2[0x47] != 0xffffffff)))) {
      unit_try_set_animation_state(puVar2[0x47],0x25);
    }
    iVar12 = DAT_006f1d6c;
    puVar2[0xcb] = local_c;
    puVar2[0xcc] = *(uint *)(iVar12 + 0xc);
    if (puVar2[0xc9] == param_1) {
      puVar2[0xc9] = 0xffffffff;
    }
    if (puVar2[0xca] == param_1) {
      puVar2[0xca] = 0xffffffff;
    }
    FUN_004f6610(param_1);
    local_34 = local_28 + (float)puVar2[0x17];
    local_30 = local_24 + (float)puVar2[0x18];
    local_2c = (local_20 + (float)puVar2[0x19]) - local_14;
    object_set_position_and_orientation(param_1,0,0);
    iVar6 = local_8;
    iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_8);
    (*(code *)PTR_matrix4x3_multiply_00696664)
              (*(short *)(iVar12 + 0x1f2) + iVar12,iVar10 + 0x68,local_6c);
    puVar2[0x1d] = local_68;
    puVar2[0x1e] = local_64;
    puVar2[0x1f] = local_60;
    puVar2[0x20] = local_50;
    puVar2[0x21] = local_4c;
    iVar12 = DAT_008603b0;
    puVar2[0x22] = local_48;
    puVar4 = *(uint **)(*(int *)(iVar12 + 0x34) + 8 + iVar6);
    iVar12 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar12 + 0x34) != -1) {
      if ((puVar4[4] & 1) != 0) {
        object_for_each_light_attachment(0,1);
      }
      if (*(int *)(iVar12 + 0x34) != -1) {
        iVar12 = *(int *)(DAT_008603b0 + 0x34);
        puVar4[4] = puVar4[4] & 0xfffffffe;
        pbVar1 = (byte *)(iVar12 + local_8 + 2);
        *pbVar1 = *pbVar1 | 2;
      }
    }
    *(undefined2 *)(puVar2 + 0xbc) = 0xffff;
    *(undefined1 *)((int)puVar2 + 0x2a7) = 2;
    if (puVar11[0xc9] == param_1) {
      puVar11[0xc9] = 0xffffffff;
    }
    if (puVar11[0xca] == param_1) {
      puVar11[0xca] = 0xffffffff;
    }
    FUN_0056ce30();
    FUN_0056d6a0();
    FUN_00565420(param_1);
    puVar3 = (undefined4 *)(*(short *)((int)puVar2 + 0x1ea) + 0x10 + (int)puVar2);
    *puVar3 = local_1c;
    puVar3[1] = local_18;
    puVar3[2] = local_14;
    if ((short)puVar2[0x2d] == 0) {
      FUN_0055add0(param_1);
    }
    object_recalculate_bounding_radius_recursive(param_1);
    cVar5 = FUN_00566910();
    if ((cVar5 == '\x01') && (iVar12 = object_try_and_get(2), iVar12 != 0)) {
      *(undefined4 *)(iVar12 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
    }
    iVar6 = DAT_0087a480;
    iVar10 = local_8;
    puVar11 = local_10;
    if (DAT_00719720 != 1) goto LAB_00568c1f;
    iVar12 = datum_get();
    puVar11 = local_10;
    if ((iVar12 != 0) && (*(short *)(iVar12 + 2) == -1)) {
      *(undefined4 *)(iVar12 + 0x180) = 0;
      *(undefined4 *)(iVar12 + 0x17c) = 0;
      *(undefined4 *)(iVar12 + 0x1e0) = 0;
      *(undefined4 *)(iVar12 + 0x1dc) = 0;
      goto LAB_00568ba2;
    }
  }
  uVar7 = puVar2[0x86];
  iVar10 = local_8;
  if (((uVar7 != 0xffffffff) && (sVar9 = (short)uVar7, -1 < sVar9)) &&
     (sVar9 < *(short *)(iVar6 + 0x20))) {
    iVar12 = (int)*(short *)(iVar6 + 0x22) * (int)sVar9;
    sVar9 = *(short *)(iVar12 + *(int *)(iVar6 + 0x34));
    if ((((sVar9 != 0) && ((sVar8 = (short)(uVar7 >> 0x10), sVar8 == 0 || (sVar9 == sVar8)))) &&
        (*(short *)(iVar12 + *(int *)(iVar6 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)) {
      player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      iVar10 = local_8;
    }
  }
LAB_00568c1f:
  *(undefined2 *)(puVar11 + 0xee) = 0;
  unit_drop_inventory_weapons(param_1);
  iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar10);
  iVar10 = *(int *)(iVar12 + 0x318);
  if (iVar10 != -1) {
    unit_drop_object_from_hand(param_1,iVar10);
    *(undefined4 *)(iVar12 + 0x318) = 0xffffffff;
  }
  unit_drop_grenades(param_1);
  if ((char)puVar11[0xa3] == '\0') {
    unit_drop_current_weapon(param_1,1);
  }
  *(undefined2 *)((int)puVar11 + 0x2ae) = 0xffff;
  *(undefined2 *)((int)puVar11 + 0x2aa) = 0xffff;
  *(undefined1 *)((int)puVar11 + 0x289) = 0;
  if (*(char *)((int)puVar11 + 0x28d) == '\x01') {
    *(undefined1 *)((int)puVar11 + 0x28d) = 0;
  }
  return;
}
#endif
