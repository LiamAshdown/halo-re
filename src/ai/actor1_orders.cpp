#include "halo/core/bit_cast.hpp"
#include "halo/ai/actor_orders.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

namespace c_actor_build_guard_mode_data {

}


/**
 * actor_build_guard_mode_data: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_guard_mode_data.c.txt.
 *
 * @address 0x404360
 */
uint8_t halo::ai::order_builder::build_guard_mode_data(uint8_t *out)
{
    using namespace c_actor_build_guard_mode_data;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_guard_data *>(out);

    memset(data, 0, sizeof(*data));
    data->countdown_00 = (int16_t)actor->search_velocity_ticks;
    if (actor->order_committed != 0 || actor->swarm != 0) {
        data->stage = 1;
    } else if (actor->search_position_valid != 0 &&
               halo::ai::actor_firing_position_near_point(actor_index, &actor->search_position, (int32_t)(uint32_t)actor->search_surface_index, 1)) {
        data->stage = 2;
        data->guard_point = actor->search_position;
        data->guard_point_surface = actor->search_surface_index;
        memcpy(&data->guard_radius, &actor->search_position_extra, sizeof(data->guard_radius));
    } else if (data->countdown_00 > 0) {
        data->stage = 1;
        data->look_point_valid = actor->search_velocity_valid;
        data->look_point_hostile = 0;
        if (actor->search_velocity_valid != 0) {
            real_vector3d velocity = actor->search_velocity;

            if (halo::math::vector3d_normalize_with_length(velocity) == 0.0f) {
                data->look_point_valid = 0;
            }
            data->look_point.x = velocity.i;
            data->look_point.y = velocity.j;
            data->look_point.z = velocity.k;
        }
    } else {
        data->stage = 0;
        data->reselect = 1;
    }
    if (actor->search_priority == 2) {
        data->watch_pending = 1;
        data->hold_reference = actor->search_prop_index;
    }
    data->guard_target = actor->search_prop_index;
    if (actor->search_prop_index != halo::k_dword_none) {
        data->countdown_02 = (int16_t)actor->search_prop_value;
        data->follow_movement = actor->search_prop_flag;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_build_guard_mode_data(datum_index actor_index, uint8_t *out)
{
    return halo::ai::order_builder(actor_index).build_guard_mode_data(out);
}
}


namespace c_actor_build_order_default {
}


/**
 * actor_build_order_default: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_default.c.txt.
 *
 * @address 0x401090
 */
int32_t halo::ai::order_builder::default_(int16_t order_code, actor_order *order, int16_t parameter)
{
    using namespace c_actor_build_order_default;
    uint32_t actor_index = datum;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    memset(order, 0, sizeof(*order));

    if (a->swarm != 0) {
        order_code = 0;
    }

    order->order_code = order_code;
    order->unknown_02 = 0;
    order->unknown_0a = 0;
    order->target_index = -1;
    order->parameter = parameter;
    order->valid = 1;
    return 1;
}

namespace halo::ai {
int32_t actor_build_order_default(uint32_t actor_index, int16_t order_code, actor_order *order, int16_t parameter)
{
    return halo::ai::order_builder(actor_index).default_(order_code, order, parameter);
}
}

namespace c_actor_build_order_face_seat_marker {
}


/**
 * actor_build_order_face_seat_marker: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_face_seat_marker.c.txt.
 *
 * @address 0x408110
 */
uint32_t halo::ai::order_builder::face_seat_marker(int16_t firing_position_index, uint32_t *order)
{
    using namespace c_actor_build_order_face_seat_marker;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_uncover_data *>(order);

    memset(data, 0, sizeof(*data));

    if (a->order_committed == 0 && a->swarm == 0 && a->encounter_index != (datum_index)k_datum_index_none && firing_position_index != -1) {
        ScenarioEncounter *encounters = (ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer;
        ScenarioFiringPosition *fp = &((ScenarioFiringPosition *)encounters[a->encounter_index & halo::k_slot_mask].firing_positions.pointer)[firing_position_index];

        data->firing_position = firing_position_index;
        data->stage = 1;
        data->position.x = fp->position.x;
        data->position.y = fp->position.y;
        data->position.z = fp->position.z;
        data->target_object = fp->surface_index;
        data->target_cluster = (int16_t)fp->cluster_index;
        data->target_reached = 0;
        data->unknown_03 = 1;
        a->search_firing_positions = 1;
        return 1;
    }
    return 0;
}

namespace halo::ai {
uint32_t actor_build_order_face_seat_marker(uint32_t actor_index, int16_t firing_position_index, uint32_t *order)
{
    return halo::ai::order_builder(actor_index).face_seat_marker(firing_position_index, order);
}
}

namespace c_actor_build_order_face_seat_marker_committed {
}


/**
 * actor_build_order_face_seat_marker_committed: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_face_seat_marker_committed.c.txt.
 *
 * @address 0x407820
 */
uint32_t halo::ai::order_builder::face_seat_marker_committed(int16_t firing_position_index, uint8_t byte_a, uint32_t *order)
{
    using namespace c_actor_build_order_face_seat_marker_committed;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_search_data *>(order);

    memset(data, 0, sizeof(*data));

    if (a->order_committed == 0 && a->swarm == 0 && a->encounter_index != (datum_index)k_datum_index_none && firing_position_index != -1) {
        ScenarioEncounter *encounters = (ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer;
        ScenarioFiringPosition *fp = &((ScenarioFiringPosition *)encounters[a->encounter_index & halo::k_slot_mask].firing_positions.pointer)[firing_position_index];

        data->unknown_04 = byte_a;
        data->firing_position = firing_position_index;
        data->stage = 1;
        data->position.x = fp->position.x;
        data->position.y = fp->position.y;
        data->position.z = fp->position.z;
        data->surface_index = (int32_t)fp->surface_index;
        data->target_cluster = (int16_t)fp->cluster_index;
        a->search_firing_positions = 1;
        return 1;
    }
    return 0;
}

namespace halo::ai {
uint32_t actor_build_order_face_seat_marker_committed(uint32_t actor_index, int16_t firing_position_index, uint8_t byte_a, uint32_t *order)
{
    return halo::ai::order_builder(actor_index).face_seat_marker_committed(firing_position_index, byte_a, order);
}
}

namespace c_actor_build_order_flee {
}


/**
 * actor_build_order_flee: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_flee.c.txt.
 *
 * @address 0x4077d0
 */
int32_t halo::ai::order_builder::flee(uint8_t byte_a, uint32_t *order)
{
    using namespace c_actor_build_order_flee;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_search_data *>(order);

    memset(data, 0, sizeof(*data));

    if (a->order_committed == 0) {
        data->unknown_05 = byte_a;
        data->stage = 0;
        a->search_firing_positions = 1;
        return 1;
    }
    return 0;
}

namespace halo::ai {
int32_t actor_build_order_flee(uint32_t actor_index, uint8_t byte_a, uint32_t *order)
{
    return halo::ai::order_builder(actor_index).flee(byte_a, order);
}
}

namespace c_actor_build_order_grenade_or_melee {
}


/**
 * actor_build_order_grenade_or_melee: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_grenade_or_melee.c.txt.
 *
 * @address 0x403630
 */
int32_t halo::ai::order_builder::grenade_or_melee(uint32_t resolved_target, uint8_t use_alt_base, uint32_t actor_index, uint16_t order_code, uint8_t byte_a, uint8_t byte_b, uint16_t *order)
{
    using namespace c_actor_build_order_grenade_or_melee;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_flee_data *>(order);

    if (a->order_committed != 0) {
        return 0;
    }

    memset(data, 0, sizeof(*data));

    data->countdown_180 = static_cast<int16_t>(-(uint16_t)(use_alt_base != 0) & 0xb4);
    data->destination = -1;
    data->panic = static_cast<int16_t>(order_code);
    data->use_last_seen_position = byte_a;
    data->cover_flag = byte_b;
    data->reference = (datum_index)resolved_target;
    if (resolved_target != halo::k_dword_none) {
        halo::ai::actor_consider_target_candidate(actor_index, (datum_index)resolved_target);
    }

    if ((int16_t)order_code > 8 && (int16_t)order_code < 0xd) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        if ((float)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f < 0.4f) {
            data->countdown_02 = 0x2d;
            return 1;
        }
    }
    if (a->swarm == 0) {
        halo::ai::actor_check_melee_target_reachable(actor_index, data);
        if (static_cast<uint16_t>(data->destination) != halo::k_word_none) {
            return 1;
        }
        data->engage = 0;
    }
    return 0;
}

namespace halo::ai {
int32_t actor_build_order_grenade_or_melee(uint32_t resolved_target, uint8_t use_alt_base, uint32_t actor_index, uint16_t order_code, uint8_t byte_a, uint8_t byte_b, uint16_t *order)
{
    return halo::ai::order_builder::grenade_or_melee(resolved_target, use_alt_base, actor_index, order_code, byte_a, byte_b, order);
}
}

namespace c_actor_build_order_guard {
}


/**
 * actor_build_order_guard: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_guard.c.txt.
 *
 * @address 0x404510
 */
int32_t halo::ai::order_builder::guard(actor_order *order, int16_t guard_at_current_position)
{
    using namespace c_actor_build_order_guard;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_guard_data *>(order);

    memset(data, 0, sizeof(*data));

    if (a->order_committed == 0 && a->swarm == 0) {
        data->countdown_00 = guard_at_current_position;
        if (guard_at_current_position == 0) {
            data->reselect = 1;
            data->stage = 0;
            data->guard_target = (datum_index)k_datum_index_none;
            return 1;
        }
        data->stage = 1;
        data->look_point_valid = 1;
        data->look_point.x = a->facing.i;
        data->look_point.y = a->facing.j;
        data->look_point.z = a->facing.k;
        data->guard_target = (datum_index)k_datum_index_none;
        return 1;
    }
    data->stage = 1;
    data->guard_target = (datum_index)k_datum_index_none;
    return 1;
}

namespace halo::ai {
int32_t actor_build_order_guard(uint32_t actor_index, actor_order *order, int16_t guard_at_current_position)
{
    return halo::ai::order_builder(actor_index).guard(order, guard_at_current_position);
}
}

namespace c_actor_build_order_investigate_encounter_point {
}


/**
 * actor_build_order_investigate_encounter_point: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_investigate_encounter_point.c.txt.
 *
 * @address 0x408a30
 */
uint8_t halo::ai::order_builder::investigate_encounter_point(uint32_t vehicle_index, uint32_t actor_index, int16_t seat_index, uint8_t *order)
{
    using namespace c_actor_build_order_investigate_encounter_point;
    actor *act = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_vehicle_data *>(order);
    object *vehicle;
    real_point3d entry;
    real_vector3d direction;
    real_point3d hint;

    memset(data, 0, sizeof(*data));
    if (act->active_unit_index != k_datum_index_none || act->swarm != 0) {
        return 0;
    }
    vehicle = (object *)halo::ai::object_at(vehicle_index);
    if (((vehicle_object *)vehicle)->base.up.k < 0.5f || (static_cast<uint8_t>(vehicle->vitality_flags) & 4) != 0) {
        return 0;
    }
    data->vehicle_index = vehicle_index;
    data->seat_index = seat_index;
    data->unknown_06 = 0;
    if (!halo::units::unit_seat_index_is_valid(act->unit_index, vehicle_index, seat_index)) {
        return 0;
    }
    if (!halo::ai::actor_evaluate_search_node(actor_index, vehicle_index, seat_index, &entry, &direction, &hint, 0, 0, 0, 0)) {
        return 0;
    }
    if (!halo::ai::actor_avoid_obstacle_and_project(actor_index, vehicle_index, &entry, &hint, 0, &data->path_destination,
                                          &data->path_surface)) {
        return 0;
    }
    if (!halo::ai::actor_movement_set_destination_point(&data->path_destination, actor_index, data->path_surface,
                                              vehicle_index)) {
        return 0;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_build_order_investigate_encounter_point(uint32_t vehicle_index, uint32_t actor_index, int16_t seat_index, uint8_t *order)
{
    return halo::ai::order_builder::investigate_encounter_point(vehicle_index, actor_index, seat_index, order);
}
}

namespace c_actor_build_order_look {
}


/**
 * actor_build_order_look: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_look.c.txt.
 *
 * @address 0x4046c0
 */
int32_t halo::ai::order_builder::look(actor_order *order, actor_look_request *request)
{
    using namespace c_actor_build_order_look;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_guard_data *>(order);
    uint8_t force_random;
    uint8_t need_random_duration;

    memset(data, 0, sizeof(*data));

    if (a->order_committed != 0 || a->swarm != 0) {
        data->stage = 1;
        data->guard_target = (datum_index)k_datum_index_none;
        return 1;
    }

    data->countdown_00 = 0;
    data->ambush_active = 1;
    data->countdown_0c = 0;

    force_random = request->force_random > 0;
    data->ambush_triggered = force_random;
    need_random_duration = (a->retreat_timer < 1) || force_random;
    data->ambush_retreat = need_random_duration ? 0 : 1;

    if (need_random_duration) {
        Actor *actor_def = halo::ai::tag_data<Actor>(a->actor_definition_tag);
        float min, max;

        if (data->ambush_triggered == 0) {
            min = actor_def->hide_behind_cover_time[0];
            max = actor_def->hide_behind_cover_time[1];
        } else {
            min = actor_def->cowering_time[0];
            max = actor_def->cowering_time[1];
        }

        data->countdown_0c = (int16_t)(int32_t)(halo::math::random_real_range(min, max) * 30.0f);
    }

    if (request->explicit_direction == -1) {
        data->stage = 0;
        data->reselect = 1;
        data->guard_target = (datum_index)k_datum_index_none;
        return 1;
    }
    data->reselect = 0;
    data->stage = 3;
    data->firing_position = request->explicit_direction;
    if (request->has_target_point != 0) {
        real_vector3d direction;

        data->look_point_valid = 1;
        data->look_point_hostile = 1;
        direction.i = request->target_point.x - a->body_position.x;
        direction.j = request->target_point.y - a->body_position.y;
        direction.k = request->target_point.z - a->body_position.z;

        float length = halo::math::vector3d_normalize_with_length(direction);

        data->look_point.x = direction.i;
        data->look_point.y = direction.j;
        data->look_point.z = direction.k;
        if (length == 0.0f) {
            data->look_point_valid = 0;
            data->guard_target = (datum_index)k_datum_index_none;
            return 1;
        }
    }
    data->guard_target = (datum_index)k_datum_index_none;
    return 1;
}

namespace halo::ai {
int32_t actor_build_order_look(uint32_t actor_index, actor_order *order, actor_look_request *request)
{
    return halo::ai::order_builder(actor_index).look(order, request);
}
}

namespace c_actor_build_order_minimal_stop {
}


/**
 * actor_build_order_minimal_stop: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_minimal_stop.c.txt.
 *
 * @address 0x4078f0
 */
int32_t halo::ai::order_builder::minimal_stop(uint32_t *order)
{
    using namespace c_actor_build_order_minimal_stop;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_search_data *>(order);

    memset(data, 0, sizeof(*data));

    if (a->swarm != 0) {
        data->stage = 2;
        a->search_firing_positions = 1;
        return 1;
    }
    return 0;
}

namespace halo::ai {
int32_t actor_build_order_minimal_stop(uint32_t actor_index, uint32_t *order)
{
    return halo::ai::order_builder(actor_index).minimal_stop(order);
}
}

namespace c_actor_build_order_random_wait {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}


/**
 * actor_build_order_random_wait: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_random_wait.c.txt.
 *
 * @address 0x409a90
 */
int32_t halo::ai::order_builder::random_wait(uint8_t byte_a, uint32_t *order)
{
    using namespace c_actor_build_order_random_wait;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_wait_data *>(order);

    memset(data, 0, sizeof(*data));

    if (a->order_committed == 0) {
        data->unknown_01 = a->grenade_ally_phase_flag;
        data->unknown_02 = byte_a;
        data->start_game_time = halo::game::globals().game_time->game_time;
        data->countdown_150 = 0;
        data->countdown_0c = 0x78;
        data->following_friend = 1;
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        data->random_countdown = (int16_t)(((halo::math::globals().random_seed_global >> 0x10) * 300) >> 0x10) + 300;
        return 1;
    }
    return 0;
}

namespace halo::ai {
int32_t actor_build_order_random_wait(uint32_t actor_index, uint8_t byte_a, uint32_t *order)
{
    return halo::ai::order_builder(actor_index).random_wait(byte_a, order);
}
}

namespace c_actor_build_order_return_to_anchor {
}


/**
 * actor_build_order_return_to_anchor: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_return_to_anchor.c.txt.
 *
 * @address 0x4044b0
 */
int32_t halo::ai::order_builder::return_to_anchor(actor_order *order)
{
    using namespace c_actor_build_order_return_to_anchor;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_guard_data *>(order);

    memset(data, 0, sizeof(*data));

    data->stage = 1;
    data->look_point_valid = 1;
    data->look_point.x = a->facing.i;
    data->look_point.y = a->facing.j;
    data->look_point.z = a->facing.k;
    data->guard_target = (datum_index)k_datum_index_none;
    return 1;
}

namespace halo::ai {
int32_t actor_build_order_return_to_anchor(uint32_t actor_index, actor_order *order)
{
    return halo::ai::order_builder(actor_index).return_to_anchor(order);
}
}

namespace c_actor_build_order_search_object {
}


/**
 * actor_build_order_search_object: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_search_object.c.txt.
 *
 * @address 0x408920
 */
uint8_t halo::ai::order_builder::search_object(uint32_t vehicle_index, uint32_t actor_index, float radius_a, float radius_b, uint8_t *order)
{
    using namespace c_actor_build_order_search_object;
    actor *act = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_vehicle_data *>(order);
    real_point3d entry;
    real_vector3d direction;
    real_point3d hint;
    int16_t seat;

    memset(data, 0, sizeof(*data));
    data->alert_range_min = radius_a;
    data->alert_range_max = radius_b;
    if (act->active_unit_index != k_datum_index_none || act->swarm != 0 || act->mode == halo::ai::actor_mode::vehicle) {
        return 0;
    }
    if (!halo::ai::actor_is_within_alert_range(0, radius_a, radius_b, 0, 0, actor_index, vehicle_index)) {
        return 0;
    }
    data->vehicle_index = vehicle_index;
    seat = halo::ai::actor_find_best_search_node(actor_index, vehicle_index, &entry, &direction, &hint);
    data->seat_index = seat;
    if (seat == -1) {
        return 0;
    }
    data->unknown_06 = 1;
    if (!halo::units::unit_seat_index_is_valid(act->unit_index, vehicle_index, seat)) {
        return 0;
    }
    if (!halo::ai::actor_avoid_obstacle_and_project(actor_index, vehicle_index, &entry, &hint, 0, &data->path_destination,
                                          &data->path_surface)) {
        return 0;
    }
    if (!halo::ai::actor_movement_set_destination_point(&data->path_destination, actor_index, data->path_surface,
                                              vehicle_index)) {
        return 0;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_build_order_search_object(uint32_t vehicle_index, uint32_t actor_index, float radius_a, float radius_b, uint8_t *order)
{
    return halo::ai::order_builder::search_object(vehicle_index, actor_index, radius_a, radius_b, order);
}
}

namespace c_actor_build_order_search_wait {
}


/**
 * actor_build_order_search_wait: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_search_wait.c.txt.
 *
 * @address 0x4045a0
 */
int32_t halo::ai::order_builder::search_wait(actor_order *order)
{
    using namespace c_actor_build_order_search_wait;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_guard_data *>(order);

    memset(data, 0, sizeof(*data));

    data->countdown_00 = 0x78;
    data->stage = 1;
    data->command_pending = 1;
    data->guard_target = (datum_index)k_datum_index_none;

    if (a->vehicle_driving_type == 4) {
        return 0;
    }

    if (a->order_committed == 0 && a->swarm == 0 && a->post_combat_prop_index != (datum_index)k_datum_index_none) {
        prop *p = halo::ai::prop_at(a->post_combat_prop_index);

        data->guard_target = a->post_combat_prop_index;
        data->countdown_02 = 0x78;
        data->follow_movement = 1;

        switch (a->post_combat_action - 6) {
        case 0:
            data->guard_radius = 2.0f;
            break;
        case 1:
        case 2:
            data->guard_radius = 1.0f;
            break;
        case 3:
            data->guard_radius = 1.5f;
            break;
        default:
            return 1;
        }

        halo::ai::actor_target_get_relationship_object(a->post_combat_prop_index);
        data->stage = 2;
        data->guard_point.x = p->pathfinding_point.x;
        data->guard_point.y = p->pathfinding_point.y;
        data->guard_point.z = p->pathfinding_point.z;
        data->guard_point_surface = p->pathfinding_surface_index;
    }
    return 1;
}

namespace halo::ai {
int32_t actor_build_order_search_wait(uint32_t actor_index, actor_order *order)
{
    return halo::ai::order_builder(actor_index).search_wait(order);
}
}

namespace c_actor_build_order_wait_byte {
}


/**
 * actor_build_order_wait_byte: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_order_wait_byte.c.txt.
 *
 * @address 0x4080c0
 */
int32_t halo::ai::order_builder::wait_byte(uint8_t byte_a, uint32_t *order)
{
    using namespace c_actor_build_order_wait_byte;
    uint32_t actor_index = datum;
    actor *a = halo::ai::actor_at(actor_index);
    auto *data = reinterpret_cast<actor_mode_uncover_data *>(order);

    memset(data, 0, sizeof(*data));

    if (a->order_committed == 0 && a->swarm == 0) {
        data->stage = 0;
        data->unknown_03 = byte_a;
        return 1;
    }
    return 0;
}

namespace halo::ai {
int32_t actor_build_order_wait_byte(uint32_t actor_index, uint8_t byte_a, uint32_t *order)
{
    return halo::ai::order_builder(actor_index).wait_byte(byte_a, order);
}
}

namespace c_actor_build_path_find_request {
}


/**
 * actor_build_path_find_request: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_build_path_find_request.c.txt.
 *
 * @address 0x41a9c0
 */
void halo::ai::order_builder::build_path_find_request(path_find_request *request)
{
    using namespace c_actor_build_path_find_request;
    datum_index actor_index = datum;
    actor *self;
    Actor *actor_definition;
    Vehicle *vehicle_definition;
    object *unit_object;
    datum_index unit_index;
    float radius;
    uint8_t ignores_glass;
    uint32_t *clear;
    int32_t i;

    self = halo::ai::actor_at(actor_index);
    unit_index = self->unit_index;
    actor_definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    radius = actor_definition->pathfinding_radius;

    if (self->vehicle_driving_type > 0) {
        unit_index = self->active_unit_index;
        unit_object = halo::ai::object_at(unit_index);
        vehicle_definition = halo::ai::tag_data<Vehicle>(unit_object->definition_tag);
        if (vehicle_definition->ai_pathfinding_radius > 0.0f) {
            radius = vehicle_definition->ai_pathfinding_radius;
        }
    }

    halo::ai::actor_update_target_lead_position(actor_index);
    ignores_glass = self->ignores_glass;

    clear = (uint32_t *)request;
    for (i = 0x12; i != 0; i--) {
        *clear = 0;
        clear++;
    }

    request->exclude_object_index_a = unit_index;
    request->pathfinding_radius = radius;
    request->ignores_glass = ignores_glass;
    request->exclude_object_index_b = (datum_index)k_datum_index_none;
    request->have_start = 1;
    request->start_position = self->pathfinding_point;
    request->start_surface_index = (uint32_t)self->pathfinding_surface_index;
}

namespace halo::ai {
void actor_build_path_find_request(datum_index actor_index, path_find_request *request)
{
    halo::ai::order_builder(actor_index).build_path_find_request(request);
}
}

