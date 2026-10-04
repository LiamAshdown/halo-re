#include "halo/math/constants.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/objects/flags.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/core/flag_bits.hpp"
#include "models.h"
#include "halo/game/records.hpp"
#include "halo/tags/objects_layout.hpp"
#include "halo/tags/units_layout.hpp"
#include "halo/units/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/hs/records.hpp"
#include "halo/ai/records.hpp"
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
#include "halo/hs/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"

static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &network_client = halo::link::ref<uint8_t *>(halo::networking::vars().network_client);
static auto &object_headers = halo::link::ref<data_array *>(halo::hs::vars().object_headers);

#undef OBJ
#define OBJ(i) ((uint8_t *)halo::ai::object_at((i)))
static const int8_t k_unit_exit_seat_request[2] = {0x14, 0};

static unit_object *unit_record(uint32_t object_index)
{
    return reinterpret_cast<unit_object *>(OBJ(object_index));
}

static void hs_unit_leave_seat(uint32_t object_index)
{
    unit_object *unit = unit_record(object_index);
    datum_index parent_index = unit->base.parent_object;
    if (parent_index != k_datum_index_none && unit->unit.vehicle_seat_index != -1) {
        unit_object *parent = unit_record(parent_index);
        Unit *parent_tag = halo::objects::tag_as<Unit>(parent->base.definition_tag);
        UnitSeat &seat = halo::objects::block_element<UnitSeat>(parent_tag->seats, unit->unit.vehicle_seat_index);
        real_matrix4x3 *nodes = halo::objects::object_block<real_matrix4x3>(unit->base, unit->base.nodes);
        Unit *unit_tag = halo::objects::tag_as<Unit>(unit->base.definition_tag);
        GBXModel *model = halo::objects::tag_as<GBXModel>(unit_tag->base.model.tag_id.index);
        ModelNode &root_node = *halo::objects::block_elements<ModelNode>(model->nodes);
        real_vector3d root_offset = *reinterpret_cast<real_vector3d *>(&root_node.default_translation);
        real_matrix4x3 *root_matrix = reinterpret_cast<real_matrix4x3 *>(&root_node.scale);
        object_marker marker;
        real_vector3d delta;
        real_point3d position;
        real_matrix4x3 basis;
        halo::objects::object_get_node_local_transform(parent_index, seat.marker_name.string, &marker, 1);
        delta.i = nodes->position.x - marker.node_transform.position.x;
        delta.j = nodes->position.y - marker.node_transform.position.y;
        delta.k = nodes->position.z - marker.node_transform.position.z;
        if (parent->unit.driver_unit_index == object_index && (int8_t)static_cast<uint8_t>(parent->unit.animation_state) != halo::units::animation_state_value(halo::units::unit_animation_state_id::opening) &&
            unit->base.parent_object != k_datum_index_none) {
            halo::units::unit_try_set_animation_state(unit->base.parent_object, halo::units::animation_state_value(halo::units::unit_animation_state_id::opening));
        }
        unit->unit.last_parent_object_index = parent_index;
        unit->unit.last_seat_change_tick = halo::game::globals().game_time->game_time;
        if (unit->unit.driver_unit_index == object_index) {
            unit->unit.driver_unit_index = k_datum_index_none;
        }
        if (unit->unit.gunner_unit_index == object_index) {
            unit->unit.gunner_unit_index = k_datum_index_none;
        }
        halo::objects::object_snap_to_parent_marker_and_detach(object_index);
        position.x = delta.i + unit->base.position.x;
        position.y = delta.j + unit->base.position.y;
        position.z = delta.k + unit->base.position.z - root_offset.k;
        halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
        unit = unit_record(object_index);
        halo::math::globals().matrix4x3_multiply_procedure(halo::objects::object_block<real_matrix4x3>(unit->base, unit->base.nodes), root_matrix, &basis);
        unit->base.forward = basis.forward;
        unit->base.up = basis.up;
        unit = unit_record(object_index);
        unit_tag = halo::objects::tag_as<Unit>(unit->base.definition_tag);
        if (halo::objects::tag_handle(unit_tag->base.model) != k_datum_index_none) {
            if (halo::ai::flag_set(unit->base.flags, halo::objects::object_flag::no_collision)) {
                halo::objects::object_for_each_light_attachment(object_index, 0, 1);
            }
            if (halo::objects::tag_handle(unit_tag->base.model) != k_datum_index_none) {
                unit->base.flags &= ~halo::to_bits(halo::objects::object_flag::no_collision);
                halo::set_flag(halo::objects::object_header_of(object_index).flags, halo::objects::object_header_flag::visible);
            }
        }
        unit->unit.vehicle_seat_index = -1;
        unit->unit.base_animation_state = 2;
        if (parent->unit.driver_unit_index == object_index) {
            parent->unit.driver_unit_index = k_datum_index_none;
        }
        if (parent->unit.gunner_unit_index == object_index) {
            parent->unit.gunner_unit_index = k_datum_index_none;
        }
        halo::units::unit_recompute_seat_occupants(parent_index);
        halo::units::unit_pick_and_ready_next_weapon(object_index);
        halo::units::unit_update_animation_state_machine(object_index, k_unit_exit_seat_request);
        unit = unit_record(object_index);
        {
            real_point3d &translation = halo::objects::object_block<real_orientation>(unit->base, unit->base.node_function_values)[0].translation;

            translation.x = root_offset.i;
            translation.y = root_offset.j;
            translation.z = root_offset.k;
        }
        if (unit->base.type == _object_type_biped) {
            halo::units::unit_reset_orientation_and_find_position(object_index, parent_index);
        }
        halo::objects::object_recalculate_bounding_radius_recursive(object_index);
        if (halo::units::unit_all_seats_unoccupied(parent_index) == 1) {
            vehicle_object *vehicle = reinterpret_cast<vehicle_object *>(halo::objects::object_try_and_get(parent_index, 2));
            if (vehicle != 0) {
                vehicle->vehicle.network_update_tick = halo::game::globals().game_time->game_time;
            }
        }
        unit = unit_record(object_index);
        if (halo::networking::globals().game_mode == 1) {
            player *player_record = (player *)halo::memory::datum_get(unit->unit.controlling_player, halo::game::globals().player_data);
            if (player_record != 0 && player_record->local_player_index == -1) {
                player_record->position_updates.read_index = static_cast<int32_t>(0);
                player_record->position_updates.write_index = static_cast<int32_t>(0);
                player_record->vehicle_updates.read_index = static_cast<int32_t>(0);
                player_record->vehicle_updates.write_index = static_cast<int32_t>(0);
            }
        }
    }
    {
        unit_object *unit = unit_record(object_index);
        if (unit->base.network_role == 0) {
            halo::units::unit_dispatch_scripted_event_9(1, (int32_t)object_index);
            unit = unit_record(object_index);
        }
        if (halo::networking::globals().game_mode == 1) {
            datum_index player_index = unit->unit.controlling_player;
            int16_t index = (int16_t)player_index;
            int16_t salt = (int16_t)(player_index >> 16);
            if (player_index != k_datum_index_none && index >= 0 && index < halo::game::globals().player_data->maximum_count) {
                player *player_record = halo::game::player_at(index);
                int16_t identifier = player_record->identifier;
                if (identifier != 0 && (salt == 0 || identifier == salt) && player_record->local_player_index != -1 &&
                    network_client != 0) {
                    halo::networking::player_update_history_free_all((player_update_history *)(*(void **)(network_client + 0xf48)));
                }
            }
        }
    }
}

static hs_object_record *hs_object_record_get(datum_index object_index)
{
    return reinterpret_cast<hs_object_record *>(halo::objects::object_record_bytes(object_index));
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
        point = marker.node_transform.position;
    } else {
        unit_object *object = (unit_object *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index)));

        point = *&object->base.bounding_center;
    }
    return halo::units::unit_point_within_look_cone(angle_degrees * halo::math::k_degrees_to_radians, viewer_unit, &point);
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
    ScenarioCutsceneFlag *flag;
    unit_object *placed;
    player *player = 0;
    real_vector3d forward;
    real_vector3d local_forward;
    object *unit;

    if (object_index == k_datum_index_none) {
        return;
    }
    flag = &halo::objects::block_element<ScenarioCutsceneFlag>(halo::scenario::globals().scenario->cutscene_flags, location_index);
    placed = unit_record(object_index);

    if (detach_from_parent && placed->base.parent_object != k_datum_index_none) {
        if (halo::objects::object_try_and_get(object_index, 3) == 0) {
            halo::objects::object_snap_to_parent_marker_and_detach(object_index);
        } else if (halo::networking::globals().game_mode != 1) {
            hs_unit_leave_seat(object_index);
        }
    }

    forward.i = (float)(halo::libm::cos((double)flag->facing.yaw) * halo::libm::cos((double)flag->facing.pitch));
    forward.j = (float)(halo::libm::sin((double)flag->facing.yaw) * halo::libm::cos((double)flag->facing.pitch));
    forward.k = (float)halo::libm::sin((double)flag->facing.pitch);
    halo::objects::object_reset_velocity_and_wake(object_index);

    unit = halo::objects::object_try_and_get(object_index, 3);
    if (unit != 0) {
        unit_object *unit_bytes = reinterpret_cast<unit_object *>(unit);
        datum_index player_index = halo::game::player_index_from_unit_index(object_index);

        if (unit_bytes->base.parent_object != k_datum_index_none) {
            unit_object *parent = unit_record(unit_bytes->base.parent_object);
            real_matrix4x3 *node = &halo::objects::object_block<real_matrix4x3>(parent->base, parent->base.nodes)[(int8_t)unit_bytes->base.parent_marker_index];
            real_matrix4x3 inverse;

            halo::math::matrix4x3_inverse(&inverse, *node);
            halo::math::matrix4x3_transform_normal(local_forward, forward, inverse);
        } else {
            local_forward = forward;
        }
        if (reorient) {
            unit_bytes->unit.desired_facing_vector = forward;
            unit_bytes->unit.desired_aiming_vector = forward;
            unit_bytes->unit.desired_looking_vector = forward;
        }
        if (player_index != k_datum_index_none) {
            player = halo::game::player_at(player_index);
            if (detach_from_parent) {
                halo::game::player_attach_unit_to_parent(player_index, halo::k_dword_none, &flag->position);
            }
            if (reorient && player->local_player_index != -1) {
                halo::game::game_engine_compute_look_angles_from_vector(&local_forward, player->local_player_index);
            }
        }
    }

    halo::objects::object_set_position_and_orientation(object_index,
        (reorient && player == 0) ? &forward : 0, 0,
        (detach_from_parent && player == 0) ? reinterpret_cast<real_point3d *>(&flag->position) : 0);
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
        if (halo::hs::hs_object_hierarchy_test(child) != 0) {
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

    if ((halo::objects::object_type_mask_of(object->type) & _object_mask_item) != 0 && (object->flags_1f4 & 2) != 0) {
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
        header = halo::hs::object_list_header_at(header_index);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = halo::hs::object_list_reference_at(next);
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
            entry = reinterpret_cast<hs_object_header_entry *>(&halo::objects::object_header_of(index));
            if (entry->identifier != 0) {
                salt = (int16_t)((uint32_t)object_index >> 0x10);
                if ((salt == 0 || entry->identifier == salt) &&
                    (halo::objects::object_type_mask_of(entry->type_flag) & _object_mask_unit) != 0 && entry->data != 0 &&
                    halo::hs::hs_object_angle_predicate_helper(target_object, object_index, angle_degrees) != 0) {

                    return 1;
                }
            }
        }
        if (next == halo::k_dword_none) {
            object_index = -1;
        } else {
            reference = halo::hs::object_list_reference_at(next);
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
        header = halo::hs::object_list_header_at(header_index);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = halo::hs::object_list_reference_at(next);
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
            entry = reinterpret_cast<hs_object_header_entry *>(&halo::objects::object_header_of(index));
            if (entry->identifier != 0) {
                salt = (int16_t)((uint32_t)object_index >> 0x10);
                if ((salt == 0 || entry->identifier == salt) &&
                    (halo::objects::object_type_mask_of(entry->type_flag) & _object_mask_unit) != 0 && entry->data != 0 &&
                    gate != 0 && halo::units::unit_point_within_look_cone(angle_degrees * halo::math::k_degrees_to_radians, object_index,
                        reinterpret_cast<real_point3d *>(&halo::objects::block_element<ScenarioCutsceneFlag>(halo::scenario::globals().scenario->cutscene_flags, gate).position)) != 0) {

                    return 1;
                }
            }
        }
        if (next == halo::k_dword_none) {
            object_index = -1;
        } else {
            reference = halo::hs::object_list_reference_at(next);
            next = reference->next;
            object_index = reference->object_index;
        }
    } while (object_index != -1);
    return 0;
}

/**
 * Allocates a fresh object_list header and chains a reference node onto it for the unit of every live player, in
 * ascending player-slot order. Returns the new list's handle (or k_datum_index_none if the header itself could
 * not be allocated).
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
        header = halo::hs::object_list_header_at(header_index);
        header->count = 0;
        header->first_reference = k_datum_index_none;
    }

    player_index = halo::memory::datum_next(-1, halo::game::globals().player_data);
    while (player_index != k_datum_index_none) {
        unit = halo::game::player_at(player_index)->unit;
        if (unit != k_datum_index_none) {
            header = halo::hs::object_list_header_at(header_index);
            reference_index = halo::memory::datum_new(halo::objects::globals().object_list_reference_data);
            if (reference_index != k_datum_index_none) {
                reference = halo::hs::object_list_reference_at(reference_index);
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
        header = halo::hs::object_list_header_at(header_index);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = halo::hs::object_list_reference_at(next);
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
            reference = halo::hs::object_list_reference_at(next);
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
            header = halo::hs::object_list_header_at(header_index);
            header->count = 0;
            header->first_reference = k_datum_index_none;
        }
        halo::hs::object_list_reference_add(header_index, object_index);
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
        datum_index first = halo::objects::object_list_header_at(header_index)->first_reference;

        if (first != k_datum_index_none) {
            object_list_reference *reference = halo::objects::object_list_reference_at(first);

            next = reference->next;
            object_index = reference->object_index;
        }
    }
    while (object_index != k_datum_index_none) {
        unit_object *object = (unit_object *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index)));

        if (halo::scenario::scenario_query::trigger_volume_contains_point((int16_t)trigger_volume_index, &object->base.bounding_center)) {
            if (!all_mode) {
                return 1;
            }
        } else if (all_mode) {
            return 0;
        }
        if (next != k_datum_index_none) {
            object_list_reference *reference = halo::objects::object_list_reference_at(next);

            next = reference->next;
            object_index = reference->object_index;
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
    if (object_index != k_datum_index_none && !halo::hs::hs_object_hierarchy_test(object_index)) {
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
        unit = static_cast<player *>(player_element)->unit;
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
            halo::hs::hs_object_hierarchy_test(object_index) == 0) {
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
        object = hs_object_record_get(object_index);
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
    uint32_t referenced_tag_id;
    GBXModel *definition;
    int32_t permutation_count;
    int32_t index;
    int32_t match_index;

    if (object_index != k_datum_index_none) {
        match_index = -1;
        if (name[0] != '\0') {
            referenced_tag_id = halo::objects::tag_handle(halo::objects::tag_as<Object>(halo::objects::object_as<object>(object_index)->definition_tag)->model);
            if (referenced_tag_id != halo::k_dword_none) {
                definition = halo::objects::tag_as<GBXModel>(referenced_tag_id);
                permutation_count = static_cast<int32_t>(definition->regions.count);
                index = 0;
                if (0 < permutation_count) {
                    do {
                        if (_stricmp(halo::objects::block_element<ModelRegion>(definition->regions, index).name.string,
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
            object = hs_object_record_get(object_index);

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
