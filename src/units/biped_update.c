// biped_update  (Ghidra: unit_update, renamed)
// address 0x5590a0, size 3460 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// REWRITTEN from objdump 0x5590a0..0x559e23 (the draft called about a dozen callees without arguments and read the
//   unit's parent vehicle as a stale tag pointer). The biped row's +0x34 update of object_type_definitions.
//   - a networked object (+0x04 == 1) with a fresh network position (+0x18) and no parent is repositioned;
//   - riding a biped: the request is (parent +0x106 bit 2) | 0x20;
//   - in a vehicle seat: unit_evaluate_flee_reaction; a unit flagged to leave (+0x208 bit 6) on a non-client plays
//     its seat's exit animation (slot 8; the driver's leaving sets the vehicle's state 0x25), and when the
//     vehicle is upside down (up.k < 0, +0x10 bit 1, global 0x6893cc) the unit is detached from the seat where
//     its body is (biped_detach_from_seat);
//   - on foot: the up vector, the planar aim (+0x224), the stance byte (+0x4d2 from the animation state), the
//     airborne / slipping counters (+0x501/+0x502), facing, movement with collision, the idle / fidget / frame
//     trigger branches, melee (a player's melee input starts the overlay 7 swing and sets the +0x505 countdown
//     and the +0x506 hit frame from the weapon's first-person melee animation 0xd; the scan runs at the hit
//     frame), footsteps, evasion and falling off the level (all skipped while unit updates are suppressed);
//   then the animation state machine runs on the request, a result of 1 snaps the unit to the ground, and the
//   +0xbc counter counts ticks while +0x106 bit 2 and +0x10 bit 5 are set. Always returns 1.
// blam-cc: stack -> object_index

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

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
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX, DX, stack
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

uint8_t biped_update(uint32_t object_index)
{
    uint8_t *obj = OBJECT_DATA(object_index);
    uint8_t *tag = TAG_DATA(*(datum_index *)obj);
    int8_t state[2];

    if (((unit_object *)obj)->base.network_role == 1 && obj[0x18] == 1 && ((unit_object *)obj)->base.parent_object == k_datum_index_none) {
        unit_recalculate_position(object_index);
    }
    state[0] = 0;
    state[1] = 0;

    if (((unit_object *)obj)->base.parent_object != k_datum_index_none) {
        uint8_t *parent = OBJECT_DATA(((unit_object *)obj)->base.parent_object);

        if (((struct object *)parent)->type != 1) {
            // 0x559adc: riding another biped
            if (((struct object *)parent)->type == 0) {
                state[0] = (int8_t)((parent[0x106] & 4) | 0x20);
            }
            goto tail;
        }
        unit_evaluate_flee_reaction(object_index);
        if ((obj[0x208] & 0x40) != 0 && network_game_mode != 1) {
            uint8_t *self = (uint8_t *)object_try_and_get(object_index, 3);
            datum_index vehicle_index;

            if (self != 0 && (vehicle_index = ((unit_object *)self)->base.parent_object) != k_datum_index_none &&
                ((unit_object *)self)->unit.vehicle_seat_index != -1) {
                if (((unit_object *)self)->base.type == 1) {
                    // 0x5591a9: never taken for a biped (type 0)
                    biped_detach_from_seat(object_index, vehicle_index);
                    biped_free_local_player_history(OBJECT_DATA(object_index));
                } else if (!unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
                    // 0x55957f: start the seat's exit animation (slot 8)
                    uint8_t *self_tag = TAG_DATA(*(datum_index *)self);
                    datum_index graph = *(datum_index *)(self_tag + 0x44);
                    uint8_t *seat_block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)self[0x2a0] * 0x64;

                    if (*(int32_t *)(seat_block + 0x40) > 8 && (*(int16_t **)(seat_block + 0x44))[8] != -1) {
                        int16_t exit_animation = (*(int16_t **)(seat_block + 0x44))[8];
                        uint8_t *object;
                        uint8_t *object_tag;

                        if (*(datum_index *)(OBJECT_DATA(vehicle_index) + 0x324) == object_index) {
                            unit_notify_weapon_removed((int32_t)vehicle_index);
                        }
                        unit_set_custom_animation(object_index, *(datum_index *)(self_tag + 0x44),
                            animation_choose_random_permutation(graph, exit_animation, 1));
                        object = OBJECT_DATA(object_index);
                        object_tag = TAG_DATA(*(datum_index *)object);
                        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                            if ((object[0x10] & 1) != 0) {
                                object_for_each_light_attachment(object_index, 0, 1);
                            }
                            if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                                *(uint32_t *)(object + 0x10) &= ~1u;
                                OBJECT_HEADER(object_index).flags |= 2;
                            }
                        }
                        self[0x2a3] = 0x1b;
                        actor_notify_weapon_pickup_once(object_index);
                        if (((unit_object *)self)->base.network_role == 0) {
                            unit_dispatch_scripted_event_9(0, (int32_t)object_index);
                        }
                    }
                }
            }
        }
        // 0x5596ac: falling out of an upside-down vehicle
        if (biped_detach_from_flipped_vehicle && ((struct object *)parent)->up.k < 0.0f && (parent[0x10] & 2) != 0 &&
            network_game_mode != 1) {
            uint8_t *self = OBJECT_DATA(object_index);
            datum_index vehicle_index = ((unit_object *)self)->base.parent_object;

            if (vehicle_index != k_datum_index_none && ((unit_object *)self)->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(object_index, vehicle_index);
            }
            if (((unit_object *)self)->base.network_role == 0) {
                unit_dispatch_scripted_event_9(1, (int32_t)object_index);
            }
            biped_free_local_player_history(self);
        }
        goto tail;
    }

    // 0x559af9: on foot
    unit_update_up_vector((Biped *)tag, (object *)obj);
    if ((obj[0x106] & 4) != 0 || (*(uint32_t *)(tag + 0x2f4) & 0x44) == 0) {
        ((unit_object *)obj)->unit.desired_facing_vector.k = 0.0f;
        if (vector3d_normalize_with_length((real_vector3d *)(obj + 0x224)) == 0.0f) {
            *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i = *global_forward3d_pointer;
        }
    }
    switch (obj[0x2a3]) {
    case 0: case 2: case 3:
        obj[0x4d2] = 0;
        break;
    case 4: case 5: case 6: case 7:
        obj[0x4d2] = 1;
        break;
    default:
        obj[0x4d2] = 2;
        break;
    }
    {
        float *v = (float *)(obj + 0x278);

        if (v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < 0.01f) {
            *(real_point3d *)&((unit_object *)obj)->unit.throttle.i = *global_origin3d_pointer;
        }
    }
    if ((obj[0x4cc] & 1) != 0) {
        if ((int8_t)obj[0x501] < 0x7f) {
            obj[0x501]++;
        }
    } else {
        obj[0x501] = 0;
    }
    if ((obj[0x4cc] & 2) != 0) {
        if ((int8_t)obj[0x502] < 0x7f) {
            obj[0x502]++;
        }
    } else {
        obj[0x502] = 0;
    }
    state[1] = (int8_t)(obj[0x208] & 1);
    state[0] = 0;
    if ((obj[0x106] & 4) == 0) {
        biped_update_facing(object_index, state);
    }
    biped_integrate_movement_with_collision(object_index, state);
    if ((obj[0x106] & 4) != 0) {
        biped_update_idle_basis(object_index, (uint8_t *)state);
    } else if ((obj[0x4cc] & 1) != 0) {
        biped_apply_idle_fidget(object_index, (uint8_t *)state);
    } else if (((struct biped_object *)obj)->biped.landing_type != -1) {
        biped_advance_frame_counter_trigger(object_index, (char *)state);
    } else if ((obj[0x4cc] & 2) != 0) {
        biped_trigger_on_velocity_threshold(object_index);
    }
    if (unit_updates_suppressed) {
        goto tail;
    }
    // 0x559cc2: melee
    if (obj[0x505] == 0) {
        if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none && (int8_t)obj[0x208] < 0) {
            datum_index weapon = unit_get_weapon_object_index(object_index,
                *(int16_t *)(OBJECT_DATA(object_index) + 0x2f2));

            if (!weapon_prevents_melee_attack(weapon) && obj[0x320] == 0xff) {
                int8_t total;
                int8_t quarter;
                int8_t tail_time;

                unit_start_seat_overlay_animation_a(object_index, 7);
                weapon_reset_triggers(weapon);
                weapon_action_notify_for_unit(object_index, 4);
                total = (int8_t)weapon_get_first_person_animation_time(weapon, 0xd, 0, -1);
                quarter = (int8_t)(total >> 2);
                obj[0x505] = (uint8_t)(total - quarter);
                tail_time = (int8_t)weapon_get_first_person_animation_time(weapon, 0xd, 1, -1);
                obj[0x506] = (uint8_t)(total - quarter - tail_time);
                if (unit_updates_suppressed) {
                    goto tail;
                }
            }
        }
    } else {
        if (obj[0x505] == obj[0x506]) {
            unit_melee_attack_scan(object_index);
        }
        obj[0x505]--;
        if (unit_updates_suppressed) {
            goto tail;
        }
    }
    unit_update_footstep_and_idle_triggers(object_index);
    if (!unit_updates_suppressed) {
        biped_check_evade_reaction(object_index);
        unit_check_fell_off_level(object_index);
    }

tail:
    if (unit_update_animation_state_machine(object_index, state) == 1) {
        unit_snap_to_min_ground_height(object_index);
    }
    if ((obj[0x106] & 4) != 0 && (obj[0x10] & 0x20) != 0) {
        (*(int16_t *)&((struct biped_object *)obj)->base.unknown_0bc)++;
    } else {
        *(int16_t *)&((struct biped_object *)obj)->base.unknown_0bc = 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5590a0):

undefined4 unit_update(uint param_1)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  undefined *puVar6;
  char cVar7;
  undefined2 uVar8;
  short sVar9;
  uint *puVar10;
  int iVar11;
  int iVar12;
  undefined4 uVar13;
  undefined2 extraout_var;
  short sVar14;
  int iVar15;
  float10 fVar16;
  undefined1 local_ec [96];
  float local_8c;
  float local_88;
  float local_84;
  undefined1 local_7c [4];
  uint local_78;
  uint local_74;
  uint local_70;
  uint local_60;
  uint local_5c;
  uint local_58;
  uint *local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  undefined4 local_28;
  undefined4 local_24;
  float local_20;
  uint *local_1c;
  int local_18;
  undefined4 local_14;
  int local_10;
  uint local_c;
  byte local_8;
  byte local_7;

  local_10 = (param_1 & 0xffff) * 0xc;
  puVar10 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
  local_c = *(uint *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_44 = puVar10;
  if (((puVar10[1] == 1) && ((char)puVar10[6] == '\x01')) && (puVar10[0x47] == 0xffffffff)) {
    FUN_00558eb0();
  }
  local_8 = 0;
  local_7 = 0;
  if (puVar10[0x47] != 0xffffffff) {
    local_18 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar10[0x47] & 0xffff) * 0xc);
    if (*(short *)(local_18 + 0xb4) != 1) {
      if (*(short *)(local_18 + 0xb4) == 0) {
        local_8 = *(byte *)(local_18 + 0x106) & 4 | 0x20;
      }
      goto LAB_00559dd2;
    }
    FUN_0055e2d0();
    if ((((puVar10[0x82] & 0x40) != 0) &&
        (puVar10 = (uint *)object_try_and_get(3), local_1c = puVar10, puVar10 != (uint *)0x0)) &&
       ((DAT_00719720 != 1 &&
        ((local_c = puVar10[0x47], local_c != 0xffffffff && ((short)puVar10[0xbc] != -1)))))) {
      if ((short)puVar10[0x2d] == 1) {
        iVar15 = *(int *)(DAT_008603b0 + 0x34);
        puVar10 = *(uint **)(iVar15 + 8 + local_10);
        local_c = puVar10[0x47];
        if ((local_c == 0xffffffff) || ((short)puVar10[0xbc] == -1)) {
LAB_005594ef:
          iVar15 = DAT_0087a480;
          if (DAT_00719720 == 1) {
LAB_00559505:
            uVar4 = puVar10[0x86];
            if (((uVar4 != 0xffffffff) && (sVar9 = (short)uVar4, -1 < sVar9)) &&
               (sVar9 < *(short *)(iVar15 + 0x20))) {
              iVar11 = (int)*(short *)(iVar15 + 0x22) * (int)sVar9;
              sVar9 = *(short *)(iVar11 + *(int *)(iVar15 + 0x34));
              if ((((sVar9 != 0) &&
                   ((sVar14 = (short)(uVar4 >> 0x10), sVar14 == 0 || (sVar9 == sVar14)))) &&
                  (*(short *)(iVar11 + *(int *)(iVar15 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0))
              {
                player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
              }
            }
          }
        }
        else {
          local_14 = *(uint **)(iVar15 + 8 + (local_c & 0xffff) * 0xc);
          iVar15 = *(int *)(iVar15 + 8 + local_10);
          iVar15 = *(short *)(iVar15 + 0x1f2) + iVar15;
          object_get_node_local_transform
                    (local_c,*(int *)(*(int *)((*local_14 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                     0x2e8) + 0x24 + (short)puVar10[0xbc] * 0x11c,local_ec,1);
          local_34 = *(float *)(iVar15 + 0x28) - local_8c;
          local_30 = *(float *)(iVar15 + 0x2c) - local_88;
          iVar11 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 +
                                                        DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                     0x14 + DAT_0087bc14) + 0xbc);
          local_1c = (uint *)(iVar11 + 0x68);
          local_28 = *(undefined4 *)(iVar11 + 0x28);
          local_2c = *(float *)(iVar15 + 0x30) - local_84;
          local_24 = *(undefined4 *)(iVar11 + 0x2c);
          local_20 = *(float *)(iVar11 + 0x30);
          if ((local_14[0xc9] == param_1) &&
             ((*(char *)((int)local_14 + 0x2a3) != '%' && (puVar10[0x47] != 0xffffffff)))) {
            unit_try_set_animation_state(puVar10[0x47],0x25);
          }
          iVar15 = DAT_006f1d6c;
          puVar10[0xcb] = local_c;
          puVar10[0xcc] = *(uint *)(iVar15 + 0xc);
          if (puVar10[0xc9] == param_1) {
            puVar10[0xc9] = 0xffffffff;
          }
          if (puVar10[0xca] == param_1) {
            puVar10[0xca] = 0xffffffff;
          }
          FUN_004f6610(param_1);
          local_40 = local_34 + (float)puVar10[0x17];
          local_3c = local_30 + (float)puVar10[0x18];
          local_38 = (local_2c + (float)puVar10[0x19]) - local_20;
          object_set_position_and_orientation(param_1,0,0);
          iVar11 = local_10;
          iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
          (*(code *)PTR_matrix4x3_multiply_00696664)
                    (*(short *)(iVar15 + 0x1f2) + iVar15,local_1c,local_7c);
          puVar10[0x1d] = local_78;
          puVar10[0x1e] = local_74;
          puVar10[0x1f] = local_70;
          puVar10[0x20] = local_60;
          puVar10[0x21] = local_5c;
          iVar15 = DAT_008603b0;
          puVar10[0x22] = local_58;
          puVar3 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + iVar11);
          local_1c = *(uint **)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if ((local_1c[0xd] != 0xffffffff) && ((puVar3[4] & 1) != 0)) {
            object_for_each_light_attachment(0,1);
          }
          if (local_1c[0xd] != 0xffffffff) {
            iVar15 = *(int *)(DAT_008603b0 + 0x34);
            puVar3[4] = puVar3[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar15 + local_10 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
          *(undefined2 *)(puVar10 + 0xbc) = 0xffff;
          *(undefined1 *)((int)puVar10 + 0x2a7) = 2;
          if (local_14[0xc9] == param_1) {
            local_14[0xc9] = 0xffffffff;
          }
          if (local_14[0xca] == param_1) {
            local_14[0xca] = 0xffffffff;
          }
          FUN_0056ce30();
          FUN_0056d6a0();
          local_14._0_3_ = CONCAT12(0x14,(undefined2)local_14);
          local_14 = (uint *)(uint)(uint3)local_14;
          FUN_00565420(param_1);
          puVar2 = (undefined4 *)(*(short *)((int)puVar10 + 0x1ea) + 0x10 + (int)puVar10);
          *puVar2 = local_28;
          puVar2[1] = local_24;
          puVar2[2] = local_20;
          if ((short)puVar10[0x2d] == 0) {
            FUN_0055add0(param_1);
          }
          object_recalculate_bounding_radius_recursive(param_1);
          cVar7 = FUN_00566910();
          if ((cVar7 == '\x01') && (iVar15 = object_try_and_get(2), iVar15 != 0)) {
            *(undefined4 *)(iVar15 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
          }
          iVar15 = DAT_0087a480;
          if (DAT_00719720 == 1) {
            iVar11 = datum_get();
            if ((iVar11 != 0) && (*(short *)(iVar11 + 2) == -1)) {
              *(undefined4 *)(iVar11 + 0x180) = 0;
              *(undefined4 *)(iVar11 + 0x17c) = 0;
              *(undefined4 *)(iVar11 + 0x1e0) = 0;
              *(undefined4 *)(iVar11 + 0x1dc) = 0;
              goto LAB_005594ef;
            }
            goto LAB_00559505;
          }
        }
      }
      else {
        cVar7 = FUN_00565c60();
        if (cVar7 == '\0') {
          iVar15 = *(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          iVar11 = *(int *)(*(int *)((*(uint *)(iVar15 + 0x44) & 0xffff) * 0x20 + 0x14 +
                                    DAT_0087bc14) + 0x10);
          iVar12 = (char)puVar10[0xa8] * 100;
          if ((8 < *(int *)(iVar12 + 0x40 + iVar11)) &&
             (*(short *)(*(int *)(iVar12 + iVar11 + 0x44) + 0x10) != -1)) {
            if (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_c & 0xffff) * 0xc) +
                         0x324) == param_1) {
              FUN_0056ab10();
            }
            uVar13 = FUN_004d6280(1);
            unit_set_custom_animation(*(undefined4 *)(iVar15 + 0x44),uVar13);
            puVar10 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
            iVar15 = *(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (*(int *)(iVar15 + 0x34) != -1) {
              if ((puVar10[4] & 1) != 0) {
                object_for_each_light_attachment(0,1);
              }
              if (*(int *)(iVar15 + 0x34) != -1) {
                iVar15 = *(int *)(DAT_008603b0 + 0x34);
                puVar10[4] = puVar10[4] & 0xfffffffe;
                pbVar1 = (byte *)(iVar15 + local_10 + 2);
                *pbVar1 = *pbVar1 | 2;
              }
            }
            puVar10 = local_1c;
            *(undefined1 *)((int)local_1c + 0x2a3) = 0x1b;
            FUN_0042c370();
            if (puVar10[1] == 0) {
              FUN_0056c370(0);
            }
          }
        }
      }
    }
    if (((DAT_006893cc != '\0') && (*(float *)(local_18 + 0x88) < 0.0)) &&
       (((*(byte *)(local_18 + 0x10) & 2) != 0 && (DAT_00719720 != 1)))) {
      iVar15 = *(int *)(DAT_008603b0 + 0x34);
      puVar10 = *(uint **)(iVar15 + 8 + local_10);
      local_c = puVar10[0x47];
      if ((local_c != 0xffffffff) && ((short)puVar10[0xbc] != -1)) {
        puVar3 = *(uint **)(iVar15 + 8 + (local_c & 0xffff) * 0xc);
        iVar15 = *(int *)(iVar15 + 8 + local_10);
        iVar15 = *(short *)(iVar15 + 0x1f2) + iVar15;
        object_get_node_local_transform
                  (local_c,*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                   0x2e8) + 0x24 + (short)puVar10[0xbc] * 0x11c,local_ec,1);
        local_34 = *(float *)(iVar15 + 0x28) - local_8c;
        local_30 = *(float *)(iVar15 + 0x2c) - local_88;
        iVar11 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 +
                                                      DAT_0087bc14) + 0x34) & 0xffff) * 0x20 + 0x14
                                  + DAT_0087bc14) + 0xbc);
        local_18 = iVar11 + 0x68;
        local_28 = *(undefined4 *)(iVar11 + 0x28);
        local_2c = *(float *)(iVar15 + 0x30) - local_84;
        local_24 = *(undefined4 *)(iVar11 + 0x2c);
        local_20 = *(float *)(iVar11 + 0x30);
        if ((puVar3[0xc9] == param_1) &&
           ((*(char *)((int)puVar3 + 0x2a3) != '%' && (puVar10[0x47] != 0xffffffff)))) {
          unit_try_set_animation_state(puVar10[0x47],0x25);
        }
        iVar15 = DAT_006f1d6c;
        puVar10[0xcb] = local_c;
        puVar10[0xcc] = *(uint *)(iVar15 + 0xc);
        if (puVar10[0xc9] == param_1) {
          puVar10[0xc9] = 0xffffffff;
        }
        if (puVar10[0xca] == param_1) {
          puVar10[0xca] = 0xffffffff;
        }
        FUN_004f6610(param_1);
        local_40 = local_34 + (float)puVar10[0x17];
        local_3c = local_30 + (float)puVar10[0x18];
        local_38 = (local_2c + (float)puVar10[0x19]) - local_20;
        object_set_position_and_orientation(param_1,0,0);
        iVar11 = local_10;
        iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
        (*(code *)PTR_matrix4x3_multiply_00696664)
                  (*(short *)(iVar15 + 0x1f2) + iVar15,local_18,local_7c);
        puVar10[0x1d] = local_78;
        puVar10[0x1e] = local_74;
        puVar10[0x1f] = local_70;
        puVar10[0x20] = local_60;
        puVar10[0x21] = local_5c;
        iVar15 = DAT_008603b0;
        puVar10[0x22] = local_58;
        puVar5 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + iVar11);
        local_18 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if ((*(int *)(local_18 + 0x34) != -1) && ((puVar5[4] & 1) != 0)) {
          object_for_each_light_attachment(0,1);
        }
        if (*(int *)(local_18 + 0x34) != -1) {
          iVar15 = *(int *)(DAT_008603b0 + 0x34);
          puVar5[4] = puVar5[4] & 0xfffffffe;
          pbVar1 = (byte *)(iVar15 + local_10 + 2);
          *pbVar1 = *pbVar1 | 2;
        }
        *(undefined2 *)(puVar10 + 0xbc) = 0xffff;
        *(undefined1 *)((int)puVar10 + 0x2a7) = 2;
        if (puVar3[0xc9] == param_1) {
          puVar3[0xc9] = 0xffffffff;
        }
        if (puVar3[0xca] == param_1) {
          puVar3[0xca] = 0xffffffff;
        }
        FUN_0056ce30();
        FUN_0056d6a0();
        local_14._0_3_ = CONCAT12(0x14,(undefined2)local_14);
        local_14 = (uint *)(uint)(uint3)local_14;
        FUN_00565420(param_1);
        puVar2 = (undefined4 *)(*(short *)((int)puVar10 + 0x1ea) + 0x10 + (int)puVar10);
        *puVar2 = local_28;
        puVar2[1] = local_24;
        puVar2[2] = local_20;
        if ((short)puVar10[0x2d] == 0) {
          FUN_0055add0(param_1);
        }
        object_recalculate_bounding_radius_recursive(param_1);
        cVar7 = FUN_00566910();
        if ((cVar7 == '\x01') && (iVar15 = object_try_and_get(2), iVar15 != 0)) {
          *(undefined4 *)(iVar15 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
        }
        if (((DAT_00719720 == 1) && (iVar15 = datum_get(), iVar15 != 0)) &&
           (*(short *)(iVar15 + 2) == -1)) {
          *(undefined4 *)(iVar15 + 0x180) = 0;
          *(undefined4 *)(iVar15 + 0x17c) = 0;
          *(undefined4 *)(iVar15 + 0x1e0) = 0;
          *(undefined4 *)(iVar15 + 0x1dc) = 0;
        }
      }
      if (puVar10[1] == 0) {
        FUN_0056c370(1);
      }
      if (((DAT_00719720 == 1) && (uVar4 = puVar10[0x86], uVar4 != 0xffffffff)) &&
         ((sVar9 = (short)uVar4, -1 < sVar9 && (sVar9 < *(short *)(DAT_0087a480 + 0x20))))) {
        iVar15 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar9;
        sVar9 = *(short *)(iVar15 + *(int *)(DAT_0087a480 + 0x34));
        if (((sVar9 != 0) && ((sVar14 = (short)(uVar4 >> 0x10), sVar14 == 0 || (sVar9 == sVar14))))
           && ((*(short *)(iVar15 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1 && (DAT_0071c2d8 != 0))
              )) {
          player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
        }
      }
    }
    goto LAB_00559dd2;
  }
  FUN_00560800();
  if (((*(byte *)((int)puVar10 + 0x106) & 4) != 0) || ((*(byte *)(local_c + 0x2f4) & 0x44) == 0)) {
    puVar10[0x8b] = 0;
    fVar16 = (float10)vector3d_normalize_with_length();
    puVar6 = PTR_DAT_00696718;
    if ((float10)0.0 == fVar16) {
      puVar10[0x89] = *(uint *)PTR_DAT_00696718;
      puVar10[0x8a] = *(uint *)(puVar6 + 4);
      puVar10[0x8b] = *(uint *)(puVar6 + 8);
    }
  }
  switch(*(undefined1 *)((int)puVar10 + 0x2a3)) {
  case 0:
  case 2:
  case 3:
    *(undefined1 *)((int)puVar10 + 0x4d2) = 0;
    break;
  default:
    *(undefined1 *)((int)puVar10 + 0x4d2) = 2;
    break;
  case 4:
  case 5:
  case 6:
  case 7:
    *(undefined1 *)((int)puVar10 + 0x4d2) = 1;
  }
  puVar6 = PTR_DAT_00696714;
  if ((float)puVar10[0xa0] * (float)puVar10[0xa0] +
      (float)puVar10[0x9f] * (float)puVar10[0x9f] + (float)puVar10[0x9e] * (float)puVar10[0x9e] <
      0.010000001) {
    puVar10[0x9e] = *(uint *)PTR_DAT_00696714;
    puVar10[0x9f] = *(uint *)(puVar6 + 4);
    puVar10[0xa0] = *(uint *)(puVar6 + 8);
  }
  if ((puVar10[0x133] & 1) == 0) {
    *(undefined1 *)((int)puVar10 + 0x501) = 0;
  }
  else if (*(char *)((int)puVar10 + 0x501) < '\x7f') {
    *(char *)((int)puVar10 + 0x501) = *(char *)((int)puVar10 + 0x501) + '\x01';
  }
  if ((puVar10[0x133] & 2) == 0) {
    *(undefined1 *)((int)puVar10 + 0x502) = 0;
  }
  else if (*(char *)((int)puVar10 + 0x502) < '\x7f') {
    *(char *)((int)puVar10 + 0x502) = *(char *)((int)puVar10 + 0x502) + '\x01';
  }
  local_7 = (byte)puVar10[0x82] & 1;
  local_8 = 0;
  if ((*(byte *)((int)puVar10 + 0x106) & 4) == 0) {
    unit_update_facing(&local_8);
  }
  FUN_0055cfd0(param_1,&local_8);
  if ((*(byte *)((int)puVar10 + 0x106) & 4) == 0) {
    if ((puVar10[0x133] & 1) == 0) {
      if ((short)puVar10[0x142] == -1) {
        if ((puVar10[0x133] & 2) != 0) {
          FUN_0055ec20();
        }
      }
      else {
        FUN_0055eb90(&local_8);
      }
    }
    else {
      FUN_0055e940(&local_8);
    }
  }
  else {
    FUN_0055e840();
  }
  if (DAT_0071c419 != '\0') goto LAB_00559dd2;
  if (*(char *)((int)puVar10 + 0x505) == '\0') {
    if ((puVar10[0x86] != 0xffffffff) && ((char)puVar10[0x82] < '\0')) {
      uVar13 = unit_get_weapon_object_index();
      cVar7 = FUN_004c2ee0();
      if ((cVar7 == '\0') && ((char)puVar10[200] == -1)) {
        FUN_00565e00(param_1,7);
        FUN_004c4b50(uVar13);
        FUN_00492730(4);
        uVar8 = FUN_004c2f80(0,0xffffffff);
        cVar7 = (char)uVar8;
        local_18 = CONCAT22(extraout_var,(short)(cVar7 >> 2));
        *(char *)((int)puVar10 + 0x505) = cVar7;
        local_14 = (uint *)CONCAT22(uVar8,(undefined2)local_14);
        *(char *)((int)puVar10 + 0x505) = cVar7 - (cVar7 >> 2);
        cVar7 = FUN_004c2f80(1,0xffffffff);
        *(char *)((int)puVar10 + 0x506) = (local_14._2_1_ - (char)local_18) - cVar7;
        goto LAB_00559da9;
      }
    }
  }
  else {
    if (*(char *)((int)puVar10 + 0x505) == *(char *)((int)puVar10 + 0x506)) {
      unit_melee_attack_scan(param_1);
    }
    *(char *)((int)puVar10 + 0x505) = *(char *)((int)puVar10 + 0x505) + -1;
LAB_00559da9:
    if (DAT_0071c419 != '\0') goto LAB_00559dd2;
  }
  FUN_00560410();
  if (DAT_0071c419 == '\0') {
    FUN_0055e190(param_1);
    FUN_0055e4a0();
  }
LAB_00559dd2:
  sVar9 = FUN_00565420(param_1);
  if (sVar9 == 1) {
    FUN_0055ecf0(param_1);
  }
  if (((*(byte *)((int)local_44 + 0x106) & 4) != 0) && ((local_44[4] & 0x20) != 0)) {
    *(short *)(local_44 + 0x2f) = (short)local_44[0x2f] + 1;
    return 1;
  }
  *(undefined2 *)(local_44 + 0x2f) = 0;
  return 1;
}
#endif
