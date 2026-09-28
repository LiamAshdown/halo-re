// unit_apply_damage_effects  (Ghidra: FUN_005674a0)
// address 0x5674a0, size 3199 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// REWRITTEN from objdump 0x5674a0..0x56811e and its one caller, object_damage_notify_and_impulse (0x4effc7).
//   Stack: (unit, damage_data, notify flags, shield damage, body damage, region, is_local).
//   Locally a unit with recent damage (+0xf4 + +0xf8) records the damage category (+0x404), a 45 tick timer
//   (+0x406), the peak (+0x408) and the responsible object (+0x40c). Units with flag 0x10 (+0x204) lose the
//   effect's +0x1c from +0x37c. Locally a kill is "violent" when effect +0x30 >= 2; a survivor with flag 0x2000
//   whose recent body damage beats tag +0x22c is knocked down (+0x106 bit 4) for (random + tag +0x230) * 30 ticks,
//   at least one (+0x420). An AI unit killed by an effect with flag 0x80 whose tag has flag 0x40000 leaves its
//   seat: a seated vehicle (dropship cargo) is detached exactly as biped_update does (0x567729..0x567b04, the
//   same inline sequence, helpers copied from biped_update.c), a biped not in a scripted animation plays its
//   seat's death animation (slot 8) and then, like a unit killed outside any seat, is stunned (0x5705a0) and
//   no longer counts as killed. Unless the damage has flag 0x10, a kill, knock-down or unit not yet knocked
//   down (and neither +0x204 bit 0x800000 nor effect flag 0x10) builds the unit_state_change_record (direction
//   and angle in xy against the unit's forward) and runs unit_update_stance_and_jump (0x566de0). Locally: the
//   game engine's +0x64 callback between the two players, unit_record_recent_damage_and_react (0x568230) and
//   unit_choose_combat_reaction_animation (0x561140); any damage clears a pending weapon switch (0x5659c0);
//   a local biped then tells its actor (killed: 0x42b880, else unless knocked down: 0x42be40). Player units
//   with effect +0x20 accumulate screen shake (+0x424, capped by +0x24) and a ticks counter (+0x428, between the
//   globals +0x174 block's +0x8c and +0x90, times 30) in multiplayer or on a dedicated server. Finally a local
//   kill or knock-down releases transient state (0x568610) and a killed authoritative unit (role 0) broadcasts
//   the record (0x566c00), leaves the network index cache and becomes role 3.
// blam-cc: stack=(unit_index, dd, notify_flags, shield_damage, body_damage, region_index, is_local)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>  // uintptr_t only; this is a .c file, not a Ghidra-ingested header
#include <string.h>
#include "networking.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480
extern int16_t game_connection_role; // 0x00719720: 1 = client
extern game_time_globals *game_time; // 0x006f1d6c
extern network_client_globals *network_client;
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t is_dedicated_server_flag; // 0x00724a44
extern Globals *global_globals;
extern uint8_t network_index_cache_container[]; // 0x006870d8

extern real random_real_range(real min, real max); // 0x401050
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern void actor_notify_weapon_pickup_once(datum_index object_index); // 0x42c370, ECX
extern int32_t actor_reassign_vehicle_seat(datum_index vehicle_object_index, datum_index self_object_index,
    int32_t seat_selector); // 0x42b880, EBX, EDI, stack
extern void actor_react_to_threat_event(datum_index self_object_index, datum_index other_object_index,
    int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay); // 0x42be40
extern real vector2d_angle_between(real_vector2d *a, real_vector2d *b); // 0x4cd480, ESI, EDI
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0 (via 0x696664)
extern uint8_t network_index_cache_remove(uint8_t *container, int32_t key); // 0x4e9d40, EAX, ESI
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
extern uint8_t unit_choose_combat_reaction_animation(uint32_t unit_index, const datum_index *reaction_source,
    uint8_t is_scripted, uint8_t allow_second_tier, float distance_bias); // 0x561140, stack, EAX
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack, ECX
extern void unit_validate_and_clear_weapon_switch(uint32_t unit_index); // 0x5659c0
extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern void unit_broadcast_state_change_event(unit_state_change_record record); // 0x566c00, the record by value
extern void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction,
    uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle,
    int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still); // 0x566de0
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern void unit_record_recent_damage_and_react(uint32_t unit_index, float damage_amount, int16_t response_index,
    uint8_t allow_broadcast, uint32_t responsible_player, int16_t team_index, uint32_t responsible_object); // 0x568230, EAX
extern void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset); // 0x568610
extern void unit_notify_weapon_removed(int32_t object_index); // 0x56ab10, EAX
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX, DX, stack
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); // 0x56ebd0
extern void unit_enter_stunned_state(uint32_t unit_index, uint32_t responsible_object); // 0x5705a0, EDI, stack

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
        player_update_history_free_all(*(void **)&network_client->update_history);
    }
}

void unit_apply_damage_effects(datum_index unit_index, damage_data *dd, uint32_t flags, float shield_damage,
    float body_damage, int32_t region_index, uint8_t is_local)
{
    float total = shield_damage + body_damage;
    uint8_t *obj = OBJECT_DATA(unit_index);
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)obj);
    uint8_t *effect_block = TAG_DATA(dd->damage_effect_tag) + 0x1c4;
    uint8_t killed = (uint8_t)(flags & 1);
    uint8_t knocked_down = 0;
    uint8_t violent = 0;
    uint32_t unit_flags;
    unit_state_change_record record;

    memset(&record, 0, sizeof(record));
    if (is_local == 1) {
        float recent = *(float *)(obj + 0xf8) + *(float *)(obj + 0xf4);

        if (recent > 0.0f) {
            *(int16_t *)(obj + 0x404) = *(int16_t *)(effect_block + 0x2);
            *(int16_t *)(obj + 0x406) = 0x2d;
            if (recent < *(float *)(obj + 0x408)) {
                recent = *(float *)(obj + 0x408);
            }
            *(float *)(obj + 0x408) = recent;
            if (dd->responsible_object != k_datum_index_none) {
                *(datum_index *)(obj + 0x40c) = dd->responsible_object;
            }
        }
    }
    unit_flags = *(uint32_t *)(obj + 0x204);
    if (unit_flags & 0x10) {
        float left = *(float *)(obj + 0x37c) - *(float *)(effect_block + 0x1c);

        *(float *)(obj + 0x37c) = left;
        if (left < 0.0f) {
            *(float *)(obj + 0x37c) = 0.0f;
        }
    }
    if (is_local == 1) {
        violent = (uint8_t)(killed && !(*(float *)(effect_block + 0x30) < 2.0f));
        if (!killed && (unit_flags & 0x2000) && *(float *)(unit_tag + 0x22c) > 0.0f &&
            *(float *)(unit_tag + 0x230) > 0.0f && *(float *)(obj + 0xe0) > 0.0f &&
            *(float *)(obj + 0xf8) > *(float *)(unit_tag + 0x22c)) {
            float ticks = (random_real_range(0.0f, 1.0f) + *(float *)(unit_tag + 0x230)) * 30.0f;

            obj[0x106] |= 4;
            knocked_down = 1;
            if (1.0f > ticks) {
                ticks = 1.0f;
            }
            *(int16_t *)(obj + 0x420) = (int16_t)(int32_t)ticks;
        }
    }

    // 0x567691: an AI unit killed in a seat leaves it
    if (*(datum_index *)(obj + 0x218) == k_datum_index_none && killed && (effect_block[0x4] & 0x80) &&
        (*(uint32_t *)(unit_tag + 0x17c) & 0x40000)) {
        uint8_t *self;
        datum_index vehicle_index;

        if (*(datum_index *)(obj + 0x11c) == k_datum_index_none) {
            goto stunned;
        }
        self = (uint8_t *)object_try_and_get(unit_index, 3);
        if (self == 0 || game_connection_role == 1 ||
            (vehicle_index = *(datum_index *)(self + 0x11c)) == k_datum_index_none ||
            *(int16_t *)(self + 0x2f0) == -1) {
            goto record_check;
        }
        if (*(int16_t *)(self + 0xb4) == 1) {
            uint8_t *me = OBJECT_DATA(unit_index);

            if (*(datum_index *)(me + 0x11c) != k_datum_index_none && *(int16_t *)(me + 0x2f0) != -1) {
                biped_detach_from_seat(unit_index, *(datum_index *)(me + 0x11c));
            }
            biped_free_local_player_history(me);
            goto record_check;
        }
        if (unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
            goto record_check;
        }
        {
            uint8_t *self_tag = TAG_DATA(*(datum_index *)self);
            datum_index graph = *(datum_index *)(self_tag + 0x44);
            uint8_t *seat_block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)self[0x2a0] * 0x64;
            int16_t death_animation;
            uint8_t *object;
            uint8_t *object_tag;

            if (*(int32_t *)(seat_block + 0x40) <= 8) {
                goto record_check;
            }
            death_animation = (*(int16_t **)(seat_block + 0x44))[8];
            if (death_animation == -1) {
                goto record_check;
            }
            if (*(datum_index *)(OBJECT_DATA(vehicle_index) + 0x324) == unit_index) {
                unit_notify_weapon_removed((int32_t)vehicle_index);
            }
            unit_set_custom_animation(unit_index, *(datum_index *)(self_tag + 0x44),
                animation_choose_random_permutation(graph, death_animation, 1));
            object = OBJECT_DATA(unit_index);
            object_tag = TAG_DATA(*(datum_index *)object);
            if (*(int32_t *)(object_tag + 0x34) != -1 && (object[0x10] & 1) != 0) {
                object_for_each_light_attachment(unit_index, 0, 1);
            }
            if (*(int32_t *)(object_tag + 0x34) != -1) {
                *(uint32_t *)(object + 0x10) &= ~1u;
                OBJECT_HEADER(unit_index).flags |= 2;
            }
            self[0x2a3] = 0x1b;
            actor_notify_weapon_pickup_once(unit_index);
            if (*(int32_t *)(self + 4) == 0) {
                unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
            }
        }
stunned:
        if (is_local == 1) {
            unit_enter_stunned_state(unit_index, dd->responsible_object);
        }
        killed = 0;
        goto local_reactions;
    }

record_check:
    if ((dd->flags & 0x10) == 0 && (killed || knocked_down || (obj[0x106] & 4) == 0) &&
        (*(uint32_t *)(obj + 0x204) & 0x800000) == 0 && (*(uint32_t *)(effect_block + 0x4) & 0x10) == 0) {
        uint32_t effect_flags = *(uint32_t *)(effect_block + 0x4);
        real_vector2d direction;
        real_vector2d forward;
        uint8_t stunned_flag = 0;
        uint8_t special = 0;
        uint8_t has_direction = 0;
        float angle = 0.0f;

        direction.i = dd->direction.i;
        direction.j = dd->direction.j;
        forward.i = *(float *)(obj + 0x74);
        forward.j = *(float *)(obj + 0x78);
        if (vector2d_normalize_with_length(&direction) > 0.0f && vector2d_normalize_with_length(&forward) > 0.0f) {
            angle = vector2d_angle_between(&forward, &direction);
            has_direction = 1;
        }
        if ((unit_tag[0x17c] & 0x80) && (effect_flags & 4) == 0) {
            stunned_flag = 1;
        }
        if (obj[0x28b] != 0) {
            stunned_flag = 1;
        }
        if (flags & 0x8a) {
            special = 1;
        }
        record.valid = 1;
        record.killed = killed;
        record.knocked_down = knocked_down;
        record.violent = violent;
        record.stunned = stunned_flag;
        record.special = special;
        record.region_index = (int16_t)region_index;
        record.angle = angle;
        if (has_direction == 1) {
            record.no_direction = 0;
            record.direction = direction;
        } else {
            record.no_direction = 1;
        }
        record.player_value = 0;
        if (*(datum_index *)(obj + 0x218) != k_datum_index_none) {
            uint8_t *player = (uint8_t *)datum_get(*(datum_index *)(obj + 0x218), player_data);

            if (player != 0) {
                record.player_value = *(uint32_t *)(player + 0x2c);
            }
        }
        unit_update_stance_and_jump(unit_index, killed, knocked_down, violent, stunned_flag, special, angle,
            (int16_t)region_index, has_direction ? &direction : 0, is_local);
    } else {
        record.valid = 0;
    }

local_reactions:
    if (is_local == 1) {
        datum_index unit_player = *(datum_index *)(obj + 0x218);

        if (dd->responsible_player != k_datum_index_none && unit_player != k_datum_index_none &&
            current_game_engine != 0 && current_game_engine->unknown_64 != 0) {
            ((void (*)(datum_index, datum_index, uint32_t))current_game_engine->unknown_64)(
                dd->responsible_player, unit_player, (flags >> 4) & 0xffffff01);
        }
        if (dd->responsible_player != k_datum_index_none || dd->responsible_object != k_datum_index_none) {
            unit_record_recent_damage_and_react(unit_index, total, (int16_t)*(uint16_t *)(effect_block + 0x2),
                killed, dd->responsible_player, (int16_t)(uint16_t)dd->team_index, dd->responsible_object);
        }
        if ((dd->flags & 0x10) == 0 && ((flags & 1) || body_damage > 0.0f || shield_damage > 0.0f)) {
            unit_choose_combat_reaction_animation(unit_index, (const datum_index *)dd,
                (uint8_t)(knocked_down | killed), (uint8_t)((flags >> 6) & 0xffffff01), body_damage);
        }
    }
    if (body_damage > 0.0f || shield_damage > 0.0f) {
        unit_validate_and_clear_weapon_switch(unit_index);
    }
    if (is_local == 1 && *(int16_t *)(obj + 0xb4) == 0) {
        if (killed) {
            actor_reassign_vehicle_seat(dd->responsible_object, unit_index, *(uint16_t *)(effect_block + 0x2));
        } else if ((obj[0x106] & 4) == 0) {
            actor_react_to_threat_event(unit_index, dd->responsible_object, *(uint16_t *)(effect_block + 0x2), total,
                (uint32_t)(uintptr_t)&dd->direction, 0);
        }
    }

    // 0x567f73: player screen shake
    if (*(datum_index *)(obj + 0x218) != k_datum_index_none && *(float *)(effect_block + 0x20) > 0.0f &&
        (current_game_engine != 0 || is_dedicated_server_flag)) {
        uint8_t *shake = (uint8_t *)global_globals->player_information.pointer;
        float step = dd->random_blend * *(float *)(effect_block + 0x20);
        float cap = *(float *)(effect_block + 0x24) * dd->random_blend;
        int16_t add;
        int16_t low;
        int16_t high;

        if (step < 0.0f) {
            step = 0.0f;
        }
        if (cap < 0.0f) {
            cap = 0.0f;
        } else if (!(cap < 1.0f)) {
            cap = 1.0f;
        }
        if (cap > *(float *)(obj + 0x424)) {
            float value = step + *(float *)(obj + 0x424);

            *(float *)(obj + 0x424) = value;
            if (value > cap) {
                *(float *)(obj + 0x424) = cap;
            }
        }
        add = (int16_t)(int32_t)(*(float *)(effect_block + 0x28) * 30.0f);
        low = (int16_t)(int32_t)(*(float *)(shake + 0x8c) * 30.0f);
        high = (int16_t)(int32_t)(*(float *)(shake + 0x90) * 30.0f);
        if (*(int16_t *)(obj + 0x428) < low) {
            *(int16_t *)(obj + 0x428) = low;
        }
        *(int16_t *)(obj + 0x428) += add;
        if (*(int16_t *)(obj + 0x428) > high) {
            *(int16_t *)(obj + 0x428) = high;
        }
    }

    if (is_local == 1 && (killed || knocked_down)) {
        unit_release_transient_state(unit_index, knocked_down);
        if (*(int32_t *)(obj + 0x4) == 0 && killed == 1) {
            record.unit = unit_index;
            unit_broadcast_state_change_event(record);
            if ((OBJECT_HEADER(unit_index).flags & 8) == 0) {
                network_index_cache_remove(network_index_cache_container, (int32_t)unit_index);
            }
            *(int32_t *)(obj + 0x4) = 3;
        }
    }
}

#if 0
Original Ghidra decompilation (0x5674a0):

void FUN_005674a0(uint *param_1,uint *param_2,byte param_3,float param_4,float param_5,uint *param_6
                 ,char param_7)

{
  byte *pbVar1;
  undefined4 *puVar2;
  float fVar3;
  uint *puVar4;
  uint uVar5;
  float fVar6;
  bool bVar7;
  uint *puVar8;
  char cVar9;
  undefined2 uVar10;
  short sVar11;
  short sVar12;
  short sVar13;
  int iVar14;
  byte bVar15;
  uint *puVar16;
  int iVar17;
  uint **ppuVar18;
  int iVar19;
  float10 fVar20;
  uint auStack_158 [2];
  uint *puStack_150;
  uint uStack_14c;
  uint *puStack_148;
  uint *puStack_144;
  float local_c8;
  float local_c4;
  float local_c0;
  uint local_b4;
  uint local_b0;
  uint local_ac;
  uint local_9c;
  uint local_98;
  uint local_94;
  uint *local_68;
  undefined1 local_64;
  undefined1 local_63;
  undefined1 local_62;
  undefined1 local_61;
  undefined1 local_60;
  undefined1 local_5f;
  undefined1 local_5e;
  undefined2 local_5c;
  uint *local_58;
  float local_54;
  float local_50;
  undefined4 local_4c;
  uint *local_48;
  undefined4 local_44;
  uint local_40;
  uint local_3c;
  float local_38;
  float local_34;
  float local_30;
  uint local_2c;
  int local_28;
  uint *local_24;
  uint local_20;
  int local_1c;
  uint *local_18;
  undefined4 local_14;
  uint *local_10;
  uint local_c;

  local_48 = (uint *)(param_4 + param_5);
  iVar17 = ((uint)param_1 & 0xffff) * 0xc;
  puVar16 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
  local_28 = *(int *)((*puVar16 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar19 = *(int *)((*param_2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_1c = iVar19 + 0x1c4;
  bVar15 = param_3 & 1;
  local_18 = (uint *)(CONCAT31(local_18._1_3_,param_3) & 0xffffff01);
  local_20 = local_20 & 0xffffff00;
  uVar5 = local_2c >> 8;
  local_2c = local_2c & 0xffffff00;
  if ((param_7 == '\x01') && (fVar3 = (float)puVar16[0x3e] + (float)puVar16[0x3d], 0.0 < fVar3)) {
    *(undefined2 *)(puVar16 + 0x101) = *(undefined2 *)(iVar19 + 0x1c6);
    *(undefined2 *)((int)puVar16 + 0x406) = 0x2d;
    if (fVar3 < (float)puVar16[0x102]) {
      fVar3 = (float)puVar16[0x102];
    }
    puVar16[0x102] = (uint)fVar3;
    if (param_2[3] != 0xffffffff) {
      puVar16[0x103] = param_2[3];
    }
  }
  local_c = puVar16[0x81];
  if (((local_c & 0x10) != 0) &&
     (fVar3 = (float)puVar16[0xdf] - *(float *)(iVar19 + 0x1e0), puVar16[0xdf] = (uint)fVar3,
     fVar3 < 0.0)) {
    puVar16[0xdf] = 0;
  }
  local_24 = puVar16;
  if (param_7 == '\x01') {
    if (((param_3 & 1) == 0) || (local_2c = CONCAT31((int3)uVar5,1), *(float *)(iVar19 + 500) < 2.0)
       ) {
      local_2c = local_2c & 0xffffff00;
    }
    if ((((((param_3 & 1) == 0) && ((local_c & 0x2000) != 0)) &&
         (0.0 < *(float *)(local_28 + 0x22c))) &&
        ((0.0 < *(float *)(local_28 + 0x230) && (0.0 < (float)puVar16[0x38])))) &&
       (*(float *)(local_28 + 0x22c) < (float)puVar16[0x3e])) {
      puStack_144 = (uint *)0x56764b;
      random_real_range(0.0,1.0);
      *(byte *)((int)puVar16 + 0x106) = *(byte *)((int)puVar16 + 0x106) | 4;
      local_20 = CONCAT31(local_20._1_3_,1);
      uVar10 = __ftol();
      *(undefined2 *)(puVar16 + 0x108) = uVar10;
      bVar15 = (byte)local_18;
    }
  }
  if (((puVar16[0x86] == 0xffffffff) && (bVar15 != 0)) &&
     ((*(char *)(local_1c + 4) < '\0' && ((*(uint *)(local_28 + 0x17c) & 0x40000) != 0)))) {
    if (puVar16[0x47] == 0xffffffff) {
LAB_00567dbf:
      if (param_7 == '\x01') {
        FUN_005705a0();
      }
      local_18 = (uint *)((uint)local_18 & 0xffffff00);
      goto LAB_00567b40;
    }
    local_10 = (uint *)object_try_and_get();
    if ((((local_10 != (uint *)0x0) && (DAT_00719720 != 1)) &&
        (local_c = local_10[0x47], local_c != 0xffffffff)) && ((short)local_10[0xbc] != -1)) {
      if ((short)local_10[0x2d] == 1) {
        puVar16 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
        local_10 = (uint *)puVar16[0x47];
        if ((local_10 == (uint *)0xffffffff) || ((short)puVar16[0xbc] == -1)) {
LAB_00567a9a:
          iVar19 = DAT_0087a480;
          if (DAT_00719720 != 1) goto LAB_00567b07;
        }
        else {
          local_14 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)local_10 & 0xffff) * 0xc)
          ;
          iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
          iVar19 = *(short *)(iVar19 + 0x1f2) + iVar19;
          puStack_144 = (uint *)(*(int *)(*(int *)((*local_14 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                                  ) + 0x2e8) + 0x24 + (short)puVar16[0xbc] * 0x11c);
          uStack_14c = 0x5677b7;
          puStack_148 = local_10;
          object_get_node_local_transform();
          local_38 = *(float *)(iVar19 + 0x28) - local_c8;
          local_34 = *(float *)(iVar19 + 0x2c) - local_c4;
          iVar14 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar16 & 0xffff) * 0x20 + 0x14 +
                                                        DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                     0x14 + DAT_0087bc14) + 0xbc);
          local_c = iVar14 + 0x68;
          local_44 = *(undefined4 *)(iVar14 + 0x28);
          local_30 = *(float *)(iVar19 + 0x30) - local_c0;
          local_40 = *(uint *)(iVar14 + 0x2c);
          local_3c = *(uint *)(iVar14 + 0x30);
          if (((uint *)local_14[0xc9] == param_1) &&
             ((*(char *)((int)local_14 + 0x2a3) != '%' && (puVar16[0x47] != 0xffffffff)))) {
            puStack_144 = (uint *)0x567850;
            unit_try_set_animation_state();
          }
          iVar19 = DAT_006f1d6c;
          puVar16[0xcb] = (uint)local_10;
          puVar16[0xcc] = *(uint *)(iVar19 + 0xc);
          if ((uint *)puVar16[0xc9] == param_1) {
            puVar16[0xc9] = 0xffffffff;
          }
          if ((uint *)puVar16[0xca] == param_1) {
            puVar16[0xca] = 0xffffffff;
          }
          FUN_004f6610();
          puStack_144 = param_1;
          puStack_148 = (uint *)0x5678c9;
          object_set_position_and_orientation();
          iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
          puStack_144 = (uint *)(*(short *)(iVar19 + 0x1f2) + iVar19);
          puStack_148 = (uint *)0x5678f3;
          (*(code *)PTR_matrix4x3_multiply_00696664)();
          puVar16[0x1d] = local_b4;
          puVar16[0x1e] = local_b0;
          puVar16[0x1f] = local_ac;
          puVar16[0x20] = local_9c;
          puVar16[0x21] = local_98;
          iVar19 = DAT_008603b0;
          puVar16[0x22] = local_94;
          puVar4 = *(uint **)(*(int *)(iVar19 + 0x34) + 8 + iVar17);
          local_c = *(uint *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if ((*(int *)(local_c + 0x34) != -1) && ((puVar4[4] & 1) != 0)) {
            puStack_144 = (uint *)0x567971;
            object_for_each_light_attachment();
          }
          if (*(int *)(local_c + 0x34) != -1) {
            iVar19 = *(int *)(DAT_008603b0 + 0x34);
            puVar4[4] = puVar4[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar19 + iVar17 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
          *(undefined2 *)(puVar16 + 0xbc) = 0xffff;
          *(undefined1 *)((int)puVar16 + 0x2a7) = 2;
          if ((uint *)local_14[0xc9] == param_1) {
            local_14[0xc9] = 0xffffffff;
          }
          if ((uint *)local_14[0xca] == param_1) {
            local_14[0xca] = 0xffffffff;
          }
          FUN_0056ce30();
          FUN_0056d6a0();
          local_14 = (uint *)(uint)CONCAT12(0x14,(undefined2)local_14);
          FUN_00565420();
          puVar2 = (undefined4 *)(*(short *)((int)puVar16 + 0x1ea) + 0x10 + (int)puVar16);
          *puVar2 = local_44;
          puVar2[1] = local_40;
          puVar2[2] = local_3c;
          if ((short)puVar16[0x2d] == 0) {
            FUN_0055add0();
          }
          object_recalculate_bounding_radius_recursive();
          cVar9 = FUN_00566910();
          if ((cVar9 == '\x01') && (iVar19 = object_try_and_get(), iVar19 != 0)) {
            *(undefined4 *)(iVar19 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
          }
          iVar19 = DAT_0087a480;
          if (DAT_00719720 != 1) goto LAB_00567b07;
          iVar14 = datum_get();
          if ((iVar14 != 0) && (*(short *)(iVar14 + 2) == -1)) {
            *(undefined4 *)(iVar14 + 0x180) = 0;
            *(undefined4 *)(iVar14 + 0x17c) = 0;
            *(undefined4 *)(iVar14 + 0x1e0) = 0;
            *(undefined4 *)(iVar14 + 0x1dc) = 0;
            goto LAB_00567a9a;
          }
        }
        uVar5 = puVar16[0x86];
        if (((uVar5 != 0xffffffff) && (sVar11 = (short)uVar5, -1 < sVar11)) &&
           (sVar11 < *(short *)(iVar19 + 0x20))) {
          iVar14 = (int)*(short *)(iVar19 + 0x22) * (int)sVar11;
          sVar11 = *(short *)(iVar14 + *(int *)(iVar19 + 0x34));
          if ((((sVar11 != 0) &&
               ((sVar12 = (short)(uVar5 >> 0x10), sVar12 == 0 || (sVar11 == sVar12)))) &&
              (*(short *)(iVar14 + *(int *)(iVar19 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)) {
            player_update_history_free_all();
          }
        }
      }
      else {
        cVar9 = FUN_00565c60();
        if (((cVar9 == '\0') &&
            (iVar19 = (char)local_10[0xa8] * 100 +
                      *(int *)(*(int *)((*(uint *)(*(int *)((*local_10 & 0xffff) * 0x20 + 0x14 +
                                                           DAT_0087bc14) + 0x44) & 0xffff) * 0x20 +
                                        0x14 + DAT_0087bc14) + 0x10), 8 < *(int *)(iVar19 + 0x40)))
           && (sVar11 = *(short *)(*(int *)(iVar19 + 0x44) + 0x10), local_14 = (uint *)(int)sVar11,
              sVar11 != -1)) {
          if (*(uint **)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_c & 0xffff) * 0xc) +
                        0x324) == param_1) {
            FUN_0056ab10();
          }
          FUN_004d6280();
          puStack_144 = (uint *)0x567d31;
          unit_set_custom_animation();
          puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
          local_c = *(uint *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if ((*(int *)(local_c + 0x34) != -1) && ((puVar4[4] & 1) != 0)) {
            puStack_144 = (uint *)0x567d72;
            object_for_each_light_attachment();
          }
          puVar8 = local_10;
          if (*(int *)(local_c + 0x34) != -1) {
            iVar19 = *(int *)(DAT_008603b0 + 0x34);
            puVar4[4] = puVar4[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar19 + iVar17 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
          *(undefined1 *)((int)local_10 + 0x2a3) = 0x1b;
          FUN_0042c370();
          if (puVar8[1] == 0) {
            FUN_0056c370();
          }
          goto LAB_00567dbf;
        }
      }
    }
  }
LAB_00567b07:
  puVar16 = local_24;
  if (((param_2[1] & 0x10) == 0) &&
     (((((byte)local_18 != '\0' || ((byte)local_20 != '\0')) ||
       ((*(byte *)((int)local_24 + 0x106) & 4) == 0)) &&
      (((local_24[0x81] & 0x800000) == 0 && (uVar5 = *(uint *)(local_1c + 4), (uVar5 & 0x10) == 0)))
      ))) {
    local_34 = (float)param_2[0xd];
    local_40 = local_24[0x1d];
    local_30 = (float)param_2[0xe];
    local_3c = local_24[0x1e];
    local_10 = (uint *)((uint)local_10 & 0xffffff00);
    local_c = local_c & 0xffffff00;
    bVar7 = false;
    local_14 = (uint *)0x0;
    fVar20 = (float10)vector2d_normalize_with_length();
    if (((float10)0.0 < fVar20) &&
       (fVar20 = (float10)vector2d_normalize_with_length(), (float10)0.0 < fVar20)) {
      fVar20 = (float10)vector2d_angle_between();
      local_14 = (uint *)(float)fVar20;
      bVar7 = true;
      puVar16 = local_24;
    }
    if ((*(char *)(local_28 + 0x17c) < '\0') && ((uVar5 & 4) == 0)) {
      local_10 = (uint *)CONCAT31(local_10._1_3_,1);
    }
    if (*(char *)((int)puVar16 + 0x28b) != '\0') {
      local_10 = (uint *)CONCAT31(local_10._1_3_,1);
    }
    if ((param_3 & 0x8a) != 0) {
      local_c = CONCAT31(local_c._1_3_,1);
    }
    local_5e = !bVar7;
    local_63 = (byte)local_18;
    local_62 = (byte)local_20;
    local_61 = (undefined1)local_2c;
    local_64 = 1;
    local_60 = local_10._0_1_;
    local_5f = (undefined1)local_c;
    local_5c = SUB42(param_6,0);
    local_58 = local_14;
    if (!(bool)local_5e) {
      local_54 = local_34;
      local_50 = local_30;
    }
    local_4c = 0;
    if ((puVar16[0x86] != 0xffffffff) && (iVar19 = datum_get(), iVar19 != 0)) {
      local_4c = *(undefined4 *)(iVar19 + 0x2c);
    }
    puStack_144 = param_6;
    puStack_148 = local_14;
    uStack_14c = local_c;
    puStack_150 = local_10;
    auStack_158[1] = local_2c;
    auStack_158[0] = local_20;
    FUN_00566de0(param_1,local_18);
    puVar16 = local_24;
  }
  else {
    local_64 = 0;
  }
LAB_00567b40:
  if (param_7 == '\x01') {
    if (((((uint *)param_2[2] != (uint *)0xffffffff) && (puVar16[0x86] != 0xffffffff)) &&
        (DAT_006f1d20 != 0)) && (*(code **)(DAT_006f1d20 + 100) != (code *)0x0)) {
      puStack_148 = (uint *)0x567b7f;
      puStack_144 = (uint *)param_2[2];
      (**(code **)(DAT_006f1d20 + 100))();
    }
    if (((uint *)param_2[2] != (uint *)0xffffffff) || (param_2[3] != 0xffffffff)) {
      uStack_14c = (uint)*(ushort *)(local_1c + 2);
      puStack_148 = local_18;
      puStack_150 = local_48;
      auStack_158[1] = 0x567bb5;
      puStack_144 = (uint *)param_2[2];
      FUN_00568230();
    }
    if (((param_2[1] & 0x10) == 0) && ((((param_3 & 1) != 0 || (0.0 < param_5)) || (0.0 < param_4)))
       ) {
      puStack_144 = (uint *)(uint)((byte)local_20 | (byte)local_18);
      puStack_148 = param_1;
      uStack_14c = 0x567c0a;
      FUN_00561140();
    }
  }
  if ((0.0 < param_5) || (0.0 < param_4)) {
    FUN_005659c0();
  }
  if ((param_7 == '\x01') && ((short)puVar16[0x2d] == 0)) {
    if ((byte)local_18 == '\0') {
      if ((*(byte *)((int)puVar16 + 0x106) & 4) == 0) {
        puStack_148 = (uint *)(uint)*(ushort *)(local_1c + 2);
        puStack_144 = local_48;
        uStack_14c = param_2[3];
        puStack_150 = param_1;
        auStack_158[1] = 0x567f70;
        FUN_0042be40();
      }
    }
    else {
      FUN_0042b880();
      puVar16 = local_24;
    }
  }
  if (((puVar16[0x86] != 0xffffffff) && (0.0 < *(float *)(local_1c + 0x20))) &&
     ((DAT_006f1d20 != 0 || (DAT_00724a44 != '\0')))) {
    fVar3 = (float)param_2[0x10] * *(float *)(local_1c + 0x20);
    fVar6 = *(float *)(local_1c + 0x24) * (float)param_2[0x10];
    param_2 = (uint *)fVar3;
    if (fVar3 < 0.0) {
      param_2 = (uint *)0x0;
    }
    if (0.0 <= fVar6) {
      if (1.0 <= fVar6) {
        fVar6 = 1.0;
      }
    }
    else {
      fVar6 = 0.0;
    }
    if (((float)puVar16[0x109] < fVar6) &&
       (fVar3 = (float)puVar16[0x109], puVar16[0x109] = (uint)((float)param_2 + fVar3),
       fVar6 < (float)param_2 + fVar3)) {
      puVar16[0x109] = (uint)fVar6;
    }
    sVar11 = __ftol();
    sVar12 = __ftol();
    sVar13 = __ftol();
    if ((short)puVar16[0x10a] < sVar12) {
      *(short *)(puVar16 + 0x10a) = sVar12;
    }
    *(short *)(puVar16 + 0x10a) = (short)puVar16[0x10a] + sVar11;
    if (sVar13 < (short)puVar16[0x10a]) {
      *(short *)(puVar16 + 0x10a) = sVar13;
    }
  }
  if ((param_7 == '\x01') && (((byte)local_18 != '\0' || ((byte)local_20 != '\0')))) {
    puStack_144 = (uint *)0x5680c0;
    FUN_00568610();
    if ((puVar16[1] == 0) && ((byte)local_18 == '\x01')) {
      local_68 = param_1;
      ppuVar18 = &local_68;
      puVar16 = auStack_158;
      for (iVar19 = 8; iVar19 != 0; iVar19 = iVar19 + -1) {
        *puVar16 = (uint)*ppuVar18;
        ppuVar18 = ppuVar18 + 1;
        puVar16 = puVar16 + 1;
      }
      FUN_00566c00();
      if ((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + iVar17) & 8) == 0) {
        FUN_004e9d40();
      }
      local_24[1] = 3;
    }
  }
  return;
}
#endif
