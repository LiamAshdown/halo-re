#include "halo/hs/hs3_objects.hpp"
#include "halo/scenario/api.hpp"
#include "game.h"
#include "units.h"
#include <stdint.h>
#include "crt.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern char ai_marker_name_a[];
extern int16_t network_game_mode;
extern uint8_t *network_client;
extern double cos(double x);
extern double sin(double x);
extern void player_update_history_free_all(void *history);
extern uint8_t hs_object_angle_predicate_helper(datum_index object_index, datum_index viewer_unit, float angle_degrees);
extern void object_list_reference_add(datum_index header_index, datum_index object_index);
extern char hs_object_hierarchy_test(datum_index object_index);
extern data_array *object_headers;
}

#undef OBJ
#define OBJ(i) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(i) & halo::k_slot_mask].data)
static const int8_t k_unit_exit_seat_request[2] = {0x14, 0};

static void hs_unit_leave_seat(uint32_t object_index)
{
    uint8_t *unit = OBJ(object_index);
    datum_index parent_index = ((unit_object *)unit)->base.parent_object;
    if (parent_index != k_datum_index_none && ((unit_object *)unit)->unit.vehicle_seat_index != -1) {
        uint8_t *parent = OBJ(parent_index);
        uint8_t *parent_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)parent & halo::k_slot_mask].data;
        uint8_t *seat = *(uint8_t **)(parent_tag + 0x2e8) + ((unit_object *)unit)->unit.vehicle_seat_index * 0x11c;
        real_matrix4x3 *nodes = (real_matrix4x3 *)(unit + ((unit_object *)unit)->base.nodes.offset);
        uint8_t *unit_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)unit & halo::k_slot_mask].data;
        uint8_t *model = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id & halo::k_slot_mask].data;
        uint8_t *root_node = *(uint8_t **)(model + 0xbc);
        real_vector3d root_offset = *(real_vector3d *)(root_node + 0x28);
        real_matrix4x3 *root_matrix = (real_matrix4x3 *)(root_node + 0x68);
        object_marker marker;
        real_vector3d delta;
        real_point3d position;
        real_matrix4x3 basis;
        halo::objects::object_get_node_local_transform(parent_index, (char *)(seat + 0x24), &marker, 1);
        delta.i = nodes->position.x - marker.node_transform.position.x;
        delta.j = nodes->position.y - marker.node_transform.position.y;
        delta.k = nodes->position.z - marker.node_transform.position.z;
        if (*(datum_index *)(parent + 0x324) == object_index && (int8_t)parent[0x2a3] != 0x25 &&
            ((unit_object *)unit)->base.parent_object != k_datum_index_none) {
            halo::units::unit_try_set_animation_state(((unit_object *)unit)->base.parent_object, 0x25);
        }
        ((unit_object *)unit)->unit.last_parent_object_index = parent_index;
        ((unit_object *)unit)->unit.last_seat_change_tick = halo::game::globals().game_time->game_time;
        if (((unit_object *)unit)->unit.driver_unit_index == object_index) {
            ((unit_object *)unit)->unit.driver_unit_index = k_datum_index_none;
        }
        if (((unit_object *)unit)->unit.gunner_unit_index == object_index) {
            ((unit_object *)unit)->unit.gunner_unit_index = k_datum_index_none;
        }
        halo::objects::object_snap_to_parent_marker_and_detach(object_index);
        position.x = delta.i + ((unit_object *)unit)->base.position.x;
        position.y = delta.j + ((unit_object *)unit)->base.position.y;
        position.z = delta.k + ((unit_object *)unit)->base.position.z - root_offset.k;
        halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
        unit = OBJ(object_index);
        halo::math::globals().matrix4x3_multiply_procedure((real_matrix4x3 *)(unit + ((unit_object *)unit)->base.nodes.offset), root_matrix, &basis);
        *(real_vector3d *)&((unit_object *)unit)->base.forward.i = basis.forward;
        *(real_vector3d *)&((unit_object *)unit)->base.up.i = basis.up;
        unit = OBJ(object_index);
        unit_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)unit & halo::k_slot_mask].data;
        if (*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id != k_datum_index_none) {
            if ((((unit_object *)unit)->base.flags & 1) != 0) {
                halo::objects::object_for_each_light_attachment(object_index, 0, 1);
            }
            if (*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id != k_datum_index_none) {
                ((unit_object *)unit)->base.flags &= ~1u;
                ((uint8_t *)&((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask])[2] |= 2;
            }
        }
        ((unit_object *)unit)->unit.vehicle_seat_index = -1;
        unit[0x2a7] = 2;
        if (*(datum_index *)(parent + 0x324) == object_index) {
            *(datum_index *)(parent + 0x324) = k_datum_index_none;
        }
        if (*(datum_index *)(parent + 0x328) == object_index) {
            *(datum_index *)(parent + 0x328) = k_datum_index_none;
        }
        halo::units::unit_recompute_seat_occupants(parent_index);
        halo::units::unit_pick_and_ready_next_weapon(object_index);
        halo::units::unit_update_animation_state_machine(object_index, k_unit_exit_seat_request);
        unit = OBJ(object_index);
        *(real_vector3d *)(unit + ((unit_object *)unit)->base.node_function_values.offset + 0x10) = root_offset;
        if (((unit_object *)unit)->base.type == 0) {
            halo::units::unit_reset_orientation_and_find_position(object_index, parent_index);
        }
        halo::objects::object_recalculate_bounding_radius_recursive(object_index);
        if (halo::units::unit_all_seats_unoccupied(parent_index) == 1) {
            uint8_t *vehicle = (uint8_t *)halo::objects::object_try_and_get(parent_index, 2);
            if (vehicle != 0) {
                ((vehicle_object *)vehicle)->vehicle.network_update_tick = halo::game::globals().game_time->game_time;
            }
        }
        unit = OBJ(object_index);
        if (network_game_mode == 1) {
            uint8_t *player = (uint8_t *)halo::memory::datum_get(((unit_object *)unit)->unit.controlling_player, halo::game::globals().player_data);
            if (player != 0 && ((struct player *)player)->local_player_index == -1) {
                *(uint32_t *)&((struct player *)player)->position_updates.read_index = 0;
                *(uint32_t *)&((struct player *)player)->position_updates.write_index = 0;
                *(uint32_t *)&((struct player *)player)->vehicle_updates.read_index = 0;
                *(uint32_t *)&((struct player *)player)->vehicle_updates.write_index = 0;
            }
        }
    }
    {
        uint8_t *unit = OBJ(object_index);
        if (((unit_object *)unit)->base.network_role == 0) {
            halo::units::unit_dispatch_scripted_event_9(1, (int32_t)object_index);
            unit = OBJ(object_index);
        }
        if (network_game_mode == 1) {
            datum_index player_index = ((unit_object *)unit)->unit.controlling_player;
            int16_t index = (int16_t)player_index;
            int16_t salt = (int16_t)(player_index >> 16);
            if (player_index != k_datum_index_none && index >= 0 && index < halo::game::globals().player_data->maximum_count) {
                uint8_t *player = (uint8_t *)halo::game::globals().player_data->data + index * halo::game::globals().player_data->size;
                int16_t identifier = *(int16_t *)player;
                if (identifier != 0 && (salt == 0 || identifier == salt) && ((struct player *)player)->local_player_index != -1 &&
                    network_client != 0) {
                    player_update_history_free_all(*(void **)(network_client + 0xf48));
                }
            }
        }
    }
}

static hs_object_record *hs_object_record_get(datum_index object_index)
{
    return *(hs_object_record **)((uint8_t *)halo::objects::globals().object_data->data +
        (object_index & halo::k_slot_mask) * 0x0c + 8);
}

namespace halo::hs::part3 {

/**
 * 0x56c100, blam-cc: stack, ECX, EDI
 *
 * @address 0x4878f0
 */
uint8_t ScriptObjects::object_angle_predicate_helper(datum_index object_index, datum_index viewer_unit, float angle_degrees) const
{
    real_point3d point;

    if (object_index == k_datum_index_none) {
        return 0;
    }
    if (halo::objects::object_try_and_get(object_index, 3) != 0) {
        object_marker marker;

        halo::objects::object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1);
        point = *(real_point3d *)((uint8_t *)&marker + 0x60);
    } else {
        uint8_t *object = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (object_index & halo::k_slot_mask) * 0xc + 8);

        point = *(real_point3d *)(object + 0xa0);
    }
    return halo::units::unit_point_within_look_cone(angle_degrees * 0.017453292f, viewer_unit, &point);
}

/**
 * Behaviour of the original `hs_object_create_name_index_if_absent` function, moved unchanged into the class.
 *
 * @address 0x487cf0
 */
void ScriptObjects::object_create_name_index_if_absent(int32_t name_index) const
{
    int16_t name = (int16_t)name_index;

    if (name != -1 && (name < 0 || name >= 0x200 || halo::objects::globals().object_name_list[name] == k_datum_index_none)) {
        halo::objects::object_new_from_scenario_name(name);
    }
}

/**
 * Behaviour of the original `hs_object_detach_and_place_at_location` function, moved unchanged into the class.
 *
 * @address 0x487f50
 */
void ScriptObjects::object_detach_and_place_at_location(int16_t location_index, datum_index object_index, char detach_from_parent, char reorient) const
{
    uint8_t *flag;
    uint8_t *placed;
    uint8_t *player = 0;
    real_vector3d forward;
    real_vector3d local_forward;
    object *unit;

    if (object_index == k_datum_index_none) {
        return;
    }
    flag = (uint8_t *)halo::scenario::globals().scenario->cutscene_flags.pointer + location_index * 0x5c;
    placed = OBJ(object_index);

    if (detach_from_parent && *(datum_index *)(placed + 0x11c) != k_datum_index_none) {
        if (halo::objects::object_try_and_get(object_index, 3) == 0) {
            halo::objects::object_snap_to_parent_marker_and_detach(object_index);
        } else if (network_game_mode != 1) {
            hs_unit_leave_seat(object_index);
        }
    }

    forward.i = (float)(cos((double)*(float *)(flag + 0x30)) * cos((double)*(float *)(flag + 0x34)));
    forward.j = (float)(sin((double)*(float *)(flag + 0x30)) * cos((double)*(float *)(flag + 0x34)));
    forward.k = (float)sin((double)*(float *)(flag + 0x34));
    halo::objects::object_reset_velocity_and_wake(object_index);

    unit = halo::objects::object_try_and_get(object_index, 3);
    if (unit != 0) {
        uint8_t *unit_bytes = (uint8_t *)unit;
        datum_index player_index = halo::game::player_index_from_unit_index(object_index);

        if (*(datum_index *)(unit_bytes + 0x11c) != k_datum_index_none) {
            uint8_t *parent = OBJ(*(datum_index *)(unit_bytes + 0x11c));
            real_matrix4x3 *node = (real_matrix4x3 *)(parent + *(int16_t *)(parent + 0x1f2) +
                (int8_t)unit_bytes[0x120] * 0x34);
            real_matrix4x3 inverse;

            halo::math::matrix4x3_inverse(&inverse, *node);
            halo::math::matrix4x3_transform_normal(local_forward, forward, inverse);
        } else {
            local_forward = forward;
        }
        if (reorient) {
            *(real_vector3d *)(unit_bytes + 0x224) = forward;
            *(real_vector3d *)(unit_bytes + 0x230) = forward;
            *(real_vector3d *)(unit_bytes + 0x254) = forward;
        }
        if (player_index != k_datum_index_none) {
            player = (uint8_t *)halo::game::globals().player_data->data + (player_index & halo::k_slot_mask) * 0x200;
            if (detach_from_parent) {
                halo::game::player_attach_unit_to_parent(player_index, halo::k_dword_none, flag + 0x24);
            }
            if (reorient && ((struct player *)player)->local_player_index != -1) {
                halo::game::game_engine_compute_look_angles_from_vector(&local_forward, ((struct player *)player)->local_player_index);
            }
        }
    }

    halo::objects::object_set_position_and_orientation(object_index,
        (reorient && player == 0) ? &forward : 0, 0,
        (detach_from_parent && player == 0) ? (real_point3d *)(flag + 0x24) : 0);
}

/**
 * Returns 1 if `object_index` itself has an association via player_index_from_unit_index, or any object in its
 * child subtree does (recursively), or any object in its ancestor chain does, or its own type is one of bits
 * 2/3/4 (mask 0x1c) with flags_1f4 bit 1 set; otherwise 0.
 *
 * @address 0x487c10
 */
char ScriptObjects::object_hierarchy_test(datum_index object_index) const
{
    hs_object_record *object;
    hs_object_record *node;
    datum_index child;
    datum_index ancestor;

    object = hs_object_record_get(object_index);
    if (halo::game::player_index_from_unit_index(object_index) != halo::k_dword_none) {
        return 1;
    }

    child = object->child;
    while (child != k_datum_index_none) {
        node = hs_object_record_get(child);
        if (hs_object_hierarchy_test(child) != 0) {
            return 1;
        }
        child = node->sibling;
    }

    ancestor = object->parent;
    while (ancestor != k_datum_index_none) {
        node = hs_object_record_get(ancestor);
        if (halo::game::player_index_from_unit_index(ancestor) != halo::k_dword_none) {
            return 1;
        }
        ancestor = node->parent;
    }

    if ((1 << (object->type & 0x1f) & 0x1c) != 0 && (object->flags_1f4 & 2) != 0) {
        return 1;
    }
    return 0;
}

/**
 * Returns 1 if any object referenced by the list headed by `header_index` is a live, valid biped or vehicle (per
 * its object_data entry) whose data pointer is set and which satisfies hs_object_angle_predicate_helper for
 * `angle_degrees`; otherwise 0.
 *
 * @address 0x4879b0
 */
uint32_t ScriptObjects::object_list_any_angle_match(datum_index header_index, datum_index target_object, float angle_degrees) const
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;
    hs_object_header_entry *entry;
    int16_t index;
    int16_t salt;

    object_index = -1;
    next = halo::k_dword_none;
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)halo::objects::globals().object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = (object_list_reference *)((uint8_t *)halo::objects::globals().object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            next = reference->next;
            object_index = reference->object_index;
        }
    }

    if (object_index == -1) {
        return 0;
    }

    do {
        index = (int16_t)object_index;
        if (object_index != -1 && index >= 0 && index < halo::objects::globals().object_data->maximum_count) {
            entry = (hs_object_header_entry *)((uint8_t *)halo::objects::globals().object_data->data +
                index * halo::objects::globals().object_data->size);
            if (entry->identifier != 0) {
                salt = (int16_t)((uint32_t)object_index >> 0x10);
                if ((salt == 0 || entry->identifier == salt) &&
                    (1 << (entry->type_flag & 0x1f) & 3) != 0 && entry->data != 0 &&
                    hs_object_angle_predicate_helper(target_object, object_index, angle_degrees) != 0) {

                    return 1;
                }
            }
        }
        if (next == halo::k_dword_none) {
            object_index = -1;
        } else {
            reference = (object_list_reference *)((uint8_t *)halo::objects::globals().object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            next = reference->next;
            object_index = reference->object_index;
        }
    } while (object_index != -1);
    return 0;
}

/**
 * As hs_object_list_any_angle_match, but requires `gate` to be nonzero (in addition to the object being a live,
 * valid biped/vehicle with a data pointer) before evaluating the angle test itself.
 *
 * @address 0x487ad0
 */
uint32_t ScriptObjects::object_list_any_angle_match_gated(datum_index header_index, int16_t gate, float angle_degrees) const
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;
    hs_object_header_entry *entry;
    int16_t index;
    int16_t salt;

    object_index = -1;
    next = halo::k_dword_none;
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)halo::objects::globals().object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = (object_list_reference *)((uint8_t *)halo::objects::globals().object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
    }

    if (object_index == -1) {
        return 0;
    }

    do {
        index = (int16_t)object_index;
        if (object_index != -1 && index >= 0 && index < halo::objects::globals().object_data->maximum_count) {
            entry = (hs_object_header_entry *)((uint8_t *)halo::objects::globals().object_data->data +
                index * halo::objects::globals().object_data->size);
            if (entry->identifier != 0) {
                salt = (int16_t)((uint32_t)object_index >> 0x10);
                if ((salt == 0 || entry->identifier == salt) &&
                    (1 << (entry->type_flag & 0x1f) & 3) != 0 && entry->data != 0 &&
                    gate != 0 && halo::units::unit_point_within_look_cone(angle_degrees * 0.017453292f, object_index,
                        (real_point3d *)((uint8_t *)halo::scenario::globals().scenario->cutscene_flags.pointer + gate * 0x5c + 0x24)) != 0) {

                    return 1;
                }
            }
        }
        if (next == halo::k_dword_none) {
            object_index = -1;
        } else {
            reference = (object_list_reference *)((uint8_t *)halo::objects::globals().object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            next = reference->next;
            object_index = reference->object_index;
        }
    } while (object_index != -1);
    return 0;
}

/**
 * Allocates a fresh object_list header and chains a reference node onto it for the unit of every live player, in
 * ascending player-slot order. Returns the new list's handle (or k_datum_index_none if the header itself could
 * not be allocated -- see the UNSURE note above).
 *
 * @address 0x487630
 */
datum_index ScriptObjects::object_list_collect_player_units() const
{
    datum_index header_index;
    object_list_header *header;
    datum_index player_index;
    datum_index unit;
    datum_index reference_index;
    object_list_reference *reference;

    header_index = halo::memory::datum_new(halo::objects::globals().object_list_header_data);
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)halo::objects::globals().object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);
        header->count = 0;
        header->first_reference = k_datum_index_none;
    }

    player_index = halo::memory::datum_next(-1, halo::game::globals().player_data);
    while (player_index != k_datum_index_none) {
        unit = ((hs_player_record *)((uint8_t *)halo::game::globals().player_data->data +
            (player_index & halo::k_slot_mask) * 0x200))->unit;
        if (unit != k_datum_index_none) {
            header = (object_list_header *)((uint8_t *)halo::objects::globals().object_list_header_data->data +
                (header_index & halo::k_slot_mask) * 0x0c);
            reference_index = halo::memory::datum_new(halo::objects::globals().object_list_reference_data);
            if (reference_index != k_datum_index_none) {
                reference = (object_list_reference *)((uint8_t *)halo::objects::globals().object_list_reference_data->data +
                    (reference_index & halo::k_slot_mask) * 0x0c);
                reference->object_index = unit;
                reference->next = header->first_reference;
                header->first_reference = reference_index;
            }
            header->count = header->count + 1;
        }
        player_index = halo::memory::datum_next((int16_t)player_index, halo::game::globals().player_data);
    }
    return header_index;
}

/**
 * Calls object_notify_children_recursive(object_index) for every object referenced by the list headed by
 * `header_index`, in list order.
 *
 * @address 0x488740
 */
void ScriptObjects::object_list_for_each(datum_index header_index) const
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;

    object_index = -1;
    next = halo::k_dword_none;
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)halo::objects::globals().object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = (object_list_reference *)((uint8_t *)halo::objects::globals().object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
    }

    while (object_index != -1) {
        halo::objects::object_notify_children_recursive(object_index);
        if (next == halo::k_dword_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = (object_list_reference *)((uint8_t *)halo::objects::globals().object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
    }
}

/**
 * Allocates a new, empty object_list and, if `object_index` is valid, adds it as the list's only element.
 * Returns the new list's handle (k_datum_index_none if the header itself could not be allocated).
 *
 * @address 0x48ac10
 */
datum_index ScriptObjects::object_list_new_singleton(datum_index object_index) const
{
    datum_index header_index;
    object_list_header *header;

    header_index = k_datum_index_none;
    if (object_index != k_datum_index_none) {
        header_index = halo::memory::datum_new(halo::objects::globals().object_list_header_data);
        if (header_index != k_datum_index_none) {
            header = (object_list_header *)((uint8_t *)halo::objects::globals().object_list_header_data->data +
                (header_index & halo::k_slot_mask) * 0x0c);
            header->count = 0;
            header->first_reference = k_datum_index_none;
        }
        object_list_reference_add(header_index, object_index);
    }
    return header_index;
}

/**
 * Behaviour of the original `hs_object_list_test_trigger_volume` function, moved unchanged into the class.
 *
 * @address 0x487820
 */
char ScriptObjects::object_list_test_trigger_volume(int32_t trigger_volume_index, datum_index header_index, char all_mode) const
{
    datum_index object_index = k_datum_index_none;
    datum_index next = k_datum_index_none;

    if (header_index != k_datum_index_none) {
        datum_index first = *(datum_index *)((uint8_t *)halo::objects::globals().object_list_header_data->data + (header_index & halo::k_slot_mask) * 0xc + 8);

        if (first != k_datum_index_none) {
            uint8_t *reference = (uint8_t *)halo::objects::globals().object_list_reference_data->data + (first & halo::k_slot_mask) * 0xc;

            next = *(datum_index *)(reference + 8);
            object_index = *(datum_index *)(reference + 4);
        }
    }
    while (object_index != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (object_index & halo::k_slot_mask) * 0xc + 8);

        if (halo::scenario::scenario_trigger_volume_contains_point((int16_t)trigger_volume_index, (real_point3d *)(object + 0xa0))) {
            if (!all_mode) {
                return 1;
            }
        } else if (all_mode) {
            return 0;
        }
        if (next != k_datum_index_none) {
            uint8_t *reference = (uint8_t *)halo::objects::globals().object_list_reference_data->data + (next & halo::k_slot_mask) * 0xc;

            next = *(datum_index *)(reference + 8);
            object_index = *(datum_index *)(reference + 4);
        } else {
            object_index = k_datum_index_none;
        }
    }
    return all_mode;
}

/**
 * Behaviour of the original `hs_object_name_destroy` function, moved unchanged into the class.
 *
 * @address 0x487d90
 */
void ScriptObjects::object_name_destroy(int32_t object_name_index) const
{
    int16_t name = (int16_t)object_name_index;
    datum_index object_index;

    if (name == -1 || name < 0 || name >= 0x200) {
        return;
    }
    object_index = halo::objects::globals().object_name_list[name];
    if (object_index != k_datum_index_none && !hs_object_hierarchy_test(object_index)) {
        halo::objects::object_delete(object_index);
    }
}

/**
 * Behaviour of the original `hs_object_orient` function, moved unchanged into the class.
 *
 * @address 0x48ab80
 */
int32_t ScriptObjects::object_orient(float x) const
{
    return (int32_t)x;
}

/**
 * Phase 1: for every live player whose unit is nested somewhere under another object (its parent chain does not
 * lead back to itself... more precisely, walking up via `parent` repeatedly lands on something other than the
 * unit itself), resets that attachment via unit_detach_from_seat. Phase 2: for every top-level object (no
 * parent) that fails hs_object_hierarchy_test, either deletes it (object_delete_unparented, when object+4 == 0)
 * or, if object+4 == 3, resets it via object_delete_recursive; any other object+4 value is left untouched.
 *
 * @address 0x487dd0
 */
void ScriptObjects::object_runtime_cleanup() const
{
    data_iterator player_iter;
    void *player_element;
    datum_index unit;
    datum_index walk;
    datum_index top;
    hs_object_iterator_state object_iter;
    void *object_element;
    hs_object_record *object;
    datum_index object_index;

    player_iter.data = halo::game::globals().player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)halo::k_dword_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = halo::memory::data_iterator_next(&player_iter);
    while (player_element != 0) {
        unit = *(datum_index *)((uint8_t *)player_element + 0x34);
        if (unit != k_datum_index_none) {
            top = unit;
            do {
                walk = top;
                top = hs_object_record_get(walk)->parent;
            } while (top != k_datum_index_none);
            if (walk != unit) {
                halo::units::unit_detach_from_seat(unit, 0, 1, 1);
            }
        }
        player_element = halo::memory::data_iterator_next(&player_iter);
    }

    object_iter.type_filter = -1;
    object_iter.next_index = 0;
    object_iter.index = (datum_index)halo::k_dword_none;
    object_element = halo::objects::object_iterator_next((object_iterator *)(&object_iter));
    for (;;) {
        if (object_element == 0) {
            return;
        }
        object_index = object_iter.index;
        if (((hs_object_record *)object_element)->parent == k_datum_index_none &&
            hs_object_hierarchy_test(object_index) == 0) {
            object = hs_object_record_get(object_index);

            if (object->network_role == 0) {
                halo::objects::object_delete_unparented(object_index);
                halo::objects::object_delete_recursive(object_index, 0);
            } else if (object->network_role == 3) {
                halo::objects::object_delete_recursive(object_index, 0);
            }
        }
        object_element = halo::objects::object_iterator_next((object_iterator *)&object_iter);
    }
}

/**
 * Sets an object's current_health to `fraction` (clamped to [0, 1]) of its maximum_health. A negative fraction
 * is treated as 0 via `maximum_health * 0.0` rather than a literal 0, matching the decompiled multiply exactly
 * (so a non-finite maximum_health still propagates as NaN).
 *
 * @address 0x488600
 */
void ScriptObjects::object_set_health_fraction(datum_index object_index, float fraction) const
{
    hs_object_record *object;

    if (object_index != k_datum_index_none) {
        object = *(hs_object_record **)((uint8_t *)halo::objects::globals().object_data->data +
            (object_index & halo::k_slot_mask) * 0x0c + 8);
        if (fraction < 0.0f) {
            object->current_health = object->maximum_health * 0.0f;
            return;
        }
        if (1.0f < fraction) {
            fraction = 1.0f;
        }
        object->current_health = fraction * object->maximum_health;
    }
}

/**
 * Resolves `name` against the case-insensitive list of permutation names on the tag referenced by object_index's
 * own tag definition (offset 0x34 of that definition), and forwards the matched index (or -1 for an empty name,
 * not found, or a missing reference) to object_set_permutation_by_name.
 *
 * @address 0x488670
 */
void ScriptObjects::hs_object_set_permutation_by_name(datum_index object_index, void *permutation_name, char *name) const
{
    void **object_data;
    uint32_t referenced_tag_id;
    uint8_t *definition;
    int32_t permutation_count;
    int32_t index;
    int32_t match_index;

    if (object_index != k_datum_index_none) {
        match_index = -1;
        if (name[0] != '\0') {
            object_data = *(void ***)((uint8_t *)object_headers->data +
                (object_index & halo::k_slot_mask) * 0x0c + 8);
            referenced_tag_id = *(uint32_t *)((uint8_t *)halo::cache::globals().tag_instances[
                (*(uint32_t *)object_data & halo::k_slot_mask) & halo::k_slot_mask].data + 0x34);
            if (referenced_tag_id != halo::k_dword_none) {
                definition = (uint8_t *)halo::cache::globals().tag_instances[(referenced_tag_id & halo::k_slot_mask) & halo::k_slot_mask].data;
                permutation_count = *(int32_t *)(definition + 0xc4);
                index = 0;
                if (0 < permutation_count) {
                    do {
                        if (_stricmp((char *)(*(int32_t *)(definition + 200) + index * 0x4c),
                                name) == 0) {
                            match_index = index;
                            break;
                        }
                        index = index + 1;
                    } while (index < permutation_count);
                }
            }
        }

        halo::objects::object_set_permutation_by_name(object_index, (char *)permutation_name, (int16_t)match_index, 1);
    }
}

/**
 * Deletes (or resets, for field_04 == 3) every live object whose tag_id matches `tag_id`, then runs a garbage
 * collection pass and compacts the block list.
 *
 * @address 0x4887d0
 */
void ScriptObjects::objects_delete_by_type(uint32_t tag_id) const
{
    hs_object_iterator_state iter;
    hs_object_record *element;
    datum_index object_index;
    hs_object_record *object;

    iter.type_filter = -1;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    element = (hs_object_record *)halo::objects::object_iterator_next((object_iterator *)(&iter));
    for (;;) {
        object_index = iter.index;
        if (element == 0) {
            halo::objects::objects_garbage_collection();
            halo::memory::block_list_compact(halo::objects::globals().object_memory_pool);
            return;
        }
        if (element->tag_id == tag_id) {
            object = *(hs_object_record **)((uint8_t *)halo::objects::globals().object_data->data +
                (object_index & halo::k_slot_mask) * 0x0c + 8);

            if (object->network_role == 0) {
                halo::objects::object_delete_unparented(object_index);
                halo::objects::object_delete_recursive(object_index, 0);
            } else if (object->network_role == 3) {
                halo::objects::object_delete_recursive(object_index, 0);
            }
        }
        element = (hs_object_record *)halo::objects::object_iterator_next((object_iterator *)&iter);
    }
}

}
