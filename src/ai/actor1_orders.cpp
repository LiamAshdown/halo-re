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

namespace c_actor_build_guard_mode_data {
#define ACTOR(index) ((uint8_t *)halo::ai::globals().actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

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
    uint8_t *actor = ACTOR(actor_index);

    memset(out, 0, 0x44);
    *(int16_t *)out = W(0x33c);
    if (B(0x160) != 0 || B(6) != 0) {
        *(int16_t *)(out + 0x24) = 1;
    } else if (B(0x314) != 0 &&
               halo::ai::actor_firing_position_near_point(actor_index, &((struct actor *)actor)->search_position, (int32_t)D(0x324), 1)) {
        *(int16_t *)(out + 0x24) = 2;
        memcpy(out + 0x28, actor + 0x318, 12);
        *(uint32_t *)(out + 0x34) = D(0x324);
        *(uint32_t *)(out + 0x38) = D(0x328);
    } else if (*(int16_t *)out > 0) {
        *(int16_t *)(out + 0x24) = 1;
        out[0x14] = B(0x32c);
        out[0x15] = 0;
        if (B(0x32c) != 0) {
            memcpy(out + 0x18, actor + 0x330, 12);
            if (halo::math::vector3d_normalize_with_length(*(real_vector3d *)(out + 0x18)) == 0.0f) {
                out[0x14] = 0;
            }
        }
    } else {
        *(int16_t *)(out + 0x24) = 0;
        out[0x0e] = 1;
    }
    if (W(0x312) == 2) {
        out[0x0f] = 1;
        *(uint32_t *)(out + 0x10) = D(0x340);
    }
    *(uint32_t *)(out + 0x3c) = D(0x340);
    if (D(0x340) != halo::k_dword_none) {
        *(int16_t *)(out + 0x02) = W(0x344);
        out[0x40] = B(0x348);
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_build_guard_mode_data(datum_index actor_index, uint8_t *out)
{
    return halo::ai::order_builder(actor_index).build_guard_mode_data(out);
}
}

#undef ACTOR
#undef B
#undef D
#undef F
#undef W

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
    uint32_t *body = (uint32_t *)order;
    int32_t i;

    for (i = 0x17; i != 0; i--) {
        *body = 0;
        body++;
    }

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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = order;
    int32_t i;
    uint32_t result = 0;

    for (i = 0xd; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed == 0 && a->swarm == 0 && a->encounter_index != (datum_index)k_datum_index_none && firing_position_index != -1) {
        ScenarioEncounter *encounters = (ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer;
        ScenarioFiringPosition *fp = &((ScenarioFiringPosition *)encounters[a->encounter_index & halo::k_slot_mask].firing_positions.pointer)[firing_position_index];

        *(int16_t *)((uint8_t *)order + 0xa) = firing_position_index;
        *(int16_t *)(order + 2) = 1;
        order[5] = *(uint32_t *)&fp->position.x;
        order[6] = *(uint32_t *)&fp->position.y;
        order[7] = *(uint32_t *)&fp->position.z;
        order[4] = *(uint32_t *)&fp->surface_index;
        *(uint16_t *)(order + 3) = fp->cluster_index;
        *((uint8_t *)(order + 8)) = 0;
        *((uint8_t *)order + 3) = 1;
        a->search_firing_positions = 1;
        return 1;
    }
    return result;
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = order;
    int32_t i;
    uint32_t result = 0;

    for (i = 0xb; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed == 0 && a->swarm == 0 && a->encounter_index != (datum_index)k_datum_index_none && firing_position_index != -1) {
        ScenarioEncounter *encounters = (ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer;
        ScenarioFiringPosition *fp = &((ScenarioFiringPosition *)encounters[a->encounter_index & halo::k_slot_mask].firing_positions.pointer)[firing_position_index];

        *((uint8_t *)order + 4) = byte_a;
        *(int16_t *)((uint8_t *)order + 0xa) = firing_position_index;
        *(int16_t *)(order + 2) = 1;
        order[5] = *(uint32_t *)&fp->position.x;
        order[6] = *(uint32_t *)&fp->position.y;
        order[7] = *(uint32_t *)&fp->position.z;
        order[4] = *(uint32_t *)&fp->surface_index;
        *(uint16_t *)(order + 3) = fp->cluster_index;
        a->search_firing_positions = 1;
        return 1;
    }
    return result;
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = order;
    int32_t i;

    for (i = 0xb; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed == 0) {
        *((uint8_t *)order + 5) = byte_a;
        *(int16_t *)(order + 2) = 0;
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    int32_t i;
    uint16_t *body = order;

    if (a->order_committed != 0) {
        return 0;
    }

    for (i = 0xc; i != 0; i--) {
        body[0] = 0;
        body[1] = 0;
        body += 2;
    }

    order[0] = -(uint16_t)(use_alt_base != 0) & 0xb4;
    order[4] = halo::k_word_none;
    order[6] = order_code;
    *(uint8_t *)(order + 2) = byte_a;
    *((uint8_t *)order + 5) = byte_b;
    *(uint32_t *)(order + 0xe) = resolved_target;
    if (resolved_target != halo::k_dword_none) {
        halo::ai::actor_consider_target_candidate(actor_index, (datum_index)resolved_target);
    }

    if ((int16_t)order_code > 8 && (int16_t)order_code < 0xd) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        if ((float)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f < 0.4f) {
            order[1] = 0x2d;
            return 1;
        }
    }
    if (a->swarm == 0) {
        halo::ai::actor_check_melee_target_reachable(actor_index, (int16_t *)order);
        if (order[4] != halo::k_word_none) {
            return 1;
        }
        *((uint8_t *)order + 0xe) = 0;
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    int16_t *body = (int16_t *)order;
    int32_t i;

    for (i = 0x11; i != 0; i--) {
        body[0] = 0;
        body[1] = 0;
        body += 2;
    }

    if (a->order_committed == 0 && a->swarm == 0) {
        order->order_code = guard_at_current_position;
        if (guard_at_current_position == 0) {
            *((uint8_t *)order + 0xe) = 1;
            *(int16_t *)((uint8_t *)order + 0x24) = 0;
            *(int16_t *)((uint8_t *)order + 0x3c) = -1;
            *(int16_t *)((uint8_t *)order + 0x3e) = -1;
            return 1;
        }
        *(int16_t *)((uint8_t *)order + 0x24) = 1;
        *((uint8_t *)order + 0x14) = 1;

        *(real_vector3d *)((uint8_t *)order + 0x18) = a->facing;
        *(int16_t *)((uint8_t *)order + 0x3c) = -1;
        *(int16_t *)((uint8_t *)order + 0x3e) = -1;
        return 1;
    }
    *(int16_t *)((uint8_t *)order + 0x24) = 1;
    *(int16_t *)((uint8_t *)order + 0x3c) = -1;
    *(int16_t *)((uint8_t *)order + 0x3e) = -1;
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
    uint8_t *act = (uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *vehicle;
    real_point3d entry;
    real_vector3d direction;
    real_point3d hint;

    memset(order, 0, 0x4c);
    if (((actor *)act)->active_unit_index != k_datum_index_none || act[0x6] != 0) {
        return 0;
    }
    vehicle = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[vehicle_index & halo::k_slot_mask].data;
    if (((vehicle_object *)vehicle)->base.up.k < 0.5f || (vehicle[0x106] & 4) != 0) {
        return 0;
    }
    *(datum_index *)(order + 0x0) = vehicle_index;
    *(int16_t *)(order + 0x4) = seat_index;
    order[0x6] = 0;
    if (!halo::units::unit_seat_index_is_valid(((actor *)act)->unit_index, vehicle_index, seat_index)) {
        return 0;
    }
    if (!halo::ai::actor_evaluate_search_node(actor_index, vehicle_index, seat_index, &entry, &direction, &hint, 0, 0, 0, 0)) {
        return 0;
    }
    if (!halo::ai::actor_avoid_obstacle_and_project(actor_index, vehicle_index, &entry, &hint, 0, (real_point3d *)(order + 0x30),
                                          (int32_t *)(order + 0x48))) {
        return 0;
    }
    if (!halo::ai::actor_movement_set_destination_point((real_point3d *)(order + 0x30), actor_index, *(int32_t *)(order + 0x48),
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint8_t *o = (uint8_t *)order;
    uint32_t *body = (uint32_t *)order;
    int32_t i;
    uint8_t force_random;
    uint8_t need_random_duration;

    for (i = 0x11; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed != 0 || a->swarm != 0) {
        *(int16_t *)(o + 0x24) = 1;
        *(int32_t *)(o + 0x3c) = -1;
        return 1;
    }

    order->order_code = 0;
    o[0x08] = 1;
    *(int16_t *)(o + 0x0c) = 0;

    force_random = request->force_random > 0;
    o[0x09] = force_random;
    need_random_duration = (a->retreat_timer < 1) || force_random;
    o[0x0a] = need_random_duration ? 0 : 1;

    if (need_random_duration) {
        Actor *actor_def = (Actor *)halo::cache::globals().tag_instances[a->actor_definition_tag & halo::k_slot_mask].data;
        float min, max;

        if (o[0x09] == 0) {
            min = actor_def->hide_behind_cover_time[0];
            max = actor_def->hide_behind_cover_time[1];
        } else {
            min = actor_def->cowering_time[0];
            max = actor_def->cowering_time[1];
        }

        *(int16_t *)(o + 0x0c) = (int16_t)(int32_t)(halo::math::random_real_range(min, max) * 30.0f);
    }

    if (request->explicit_direction == -1) {
        *(int16_t *)(o + 0x24) = 0;
        o[0x0e] = 1;
        *(int32_t *)(o + 0x3c) = -1;
        return 1;
    }
    o[0x0e] = 0;
    *(int16_t *)(o + 0x24) = 3;
    *(int16_t *)(o + 0x28) = request->explicit_direction;
    if (request->has_target_point != 0) {
        real_vector3d *direction = (real_vector3d *)(o + 0x18);

        o[0x14] = 1;
        o[0x15] = 1;
        direction->i = request->target_point.x - a->body_position.x;
        direction->j = request->target_point.y - a->body_position.y;
        direction->k = request->target_point.z - a->body_position.z;

        if (halo::math::vector3d_normalize_with_length(*direction) == 0.0f) {
            o[0x14] = 0;
            *(int32_t *)(o + 0x3c) = -1;
            return 1;
        }
    }
    *(int32_t *)(o + 0x3c) = -1;
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = order;
    int32_t i;

    for (i = 0xb; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->swarm != 0) {
        *(int16_t *)(order + 2) = 2;
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
extern "C" {
extern game_time_globals *game_time;
}
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = order;
    int32_t i;

    for (i = 0; i < 6; i++) {
        body[i] = 0;
    }

    if (a->order_committed == 0) {
        *((uint8_t *)order + 1) = a->grenade_ally_phase_flag;
        *((uint8_t *)order + 2) = byte_a;
        order[2] = (uint32_t)halo::game::globals().game_time->game_time;
        *(int16_t *)((uint8_t *)order + 0xe) = 0;
        *(int16_t *)((uint8_t *)order + 0xc) = 0x78;
        *((uint8_t *)order + 3) = 1;
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        *(int16_t *)(order + 4) = (int16_t)(((halo::math::globals().random_seed_global >> 0x10) * 300) >> 0x10) + 300;
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = (uint32_t *)order;
    int32_t i;

    for (i = 0x11; i != 0; i--) {
        *body = 0;
        body++;
    }

    *(int16_t *)((uint8_t *)order + 0x24) = 1;
    *((uint8_t *)order + 0x14) = 1;

        *(real_vector3d *)((uint8_t *)order + 0x18) = a->facing;
    *(int32_t *)((uint8_t *)order + 0x3c) = -1;
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
    uint8_t *act = (uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    real_point3d entry;
    real_vector3d direction;
    real_point3d hint;
    int16_t seat;

    memset(order, 0, 0x4c);
    *(float *)(order + 0x20) = radius_a;
    *(float *)(order + 0x24) = radius_b;
    if (((actor *)act)->active_unit_index != k_datum_index_none || act[0x6] != 0 || ((actor *)act)->mode == 9) {
        return 0;
    }
    if (!halo::ai::actor_is_within_alert_range(0, radius_a, radius_b, 0, 0, actor_index, vehicle_index)) {
        return 0;
    }
    *(datum_index *)(order + 0x0) = vehicle_index;
    seat = halo::ai::actor_find_best_search_node(actor_index, vehicle_index, &entry, &direction, &hint);
    *(int16_t *)(order + 0x4) = seat;
    if (seat == -1) {
        return 0;
    }
    order[0x6] = 1;
    if (!halo::units::unit_seat_index_is_valid(((actor *)act)->unit_index, vehicle_index, seat)) {
        return 0;
    }
    if (!halo::ai::actor_avoid_obstacle_and_project(actor_index, vehicle_index, &entry, &hint, 0, (real_point3d *)(order + 0x30),
                                          (int32_t *)(order + 0x48))) {
        return 0;
    }
    if (!halo::ai::actor_movement_set_destination_point((real_point3d *)(order + 0x30), actor_index, *(int32_t *)(order + 0x48),
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = (uint32_t *)order;
    int32_t i;

    for (i = 0x11; i != 0; i--) {
        *body = 0;
        body++;
    }

    order->order_code = 0x78;
    *(int16_t *)((uint8_t *)order + 0x24) = 1;
    *((uint8_t *)order + 5) = 1;
    *(int32_t *)((uint8_t *)order + 0x3c) = -1;

    if (a->vehicle_driving_type == 4) {
        return 0;
    }

    if (a->order_committed == 0 && a->swarm == 0 && a->post_combat_prop_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)halo::ai::globals().prop_data->data)[a->post_combat_prop_index & halo::k_slot_mask];

        *(int32_t *)((uint8_t *)order + 0x3c) = a->post_combat_prop_index;
        order->unknown_02 = 0x78;
        *((uint8_t *)order + 0x40) = 1;

        switch (a->post_combat_action - 6) {
        case 0:
            *(float *)((uint8_t *)order + 0x38) = 2.0f;
            break;
        case 1:
        case 2:
            *(float *)((uint8_t *)order + 0x38) = 1.0f;
            break;
        case 3:
            *(float *)((uint8_t *)order + 0x38) = 1.5f;
            break;
        default:
            return 1;
        }

        halo::ai::actor_target_get_relationship_object(a->post_combat_prop_index);
        *(int16_t *)((uint8_t *)order + 0x24) = 2;
        *(float *)((uint8_t *)order + 0x28) = ((struct prop *)p)->pathfinding_point.x;
        *(float *)((uint8_t *)order + 0x2c) = ((struct prop *)p)->pathfinding_point.y;
        *(float *)((uint8_t *)order + 0x30) = ((struct prop *)p)->pathfinding_point.z;
        *(int32_t *)((uint8_t *)order + 0x34) = ((struct prop *)p)->pathfinding_surface_index;
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint32_t *body = order;
    int32_t i;

    for (i = 0xd; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed == 0 && a->swarm == 0) {
        *(int16_t *)(order + 2) = 0;
        *((uint8_t *)order + 3) = byte_a;
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

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    unit_index = self->unit_index;
    actor_definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & halo::k_slot_mask].data;
    radius = actor_definition->pathfinding_radius;

    if (self->vehicle_driving_type > 0) {
        unit_index = self->active_unit_index;
        unit_object = ((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask].data;
        vehicle_definition = (Vehicle *)halo::cache::globals().tag_instances[unit_object->definition_tag & halo::k_slot_mask].data;
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

