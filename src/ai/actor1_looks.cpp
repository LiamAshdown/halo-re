#include "halo/core/bit_cast.hpp"
#include "halo/ai/actor_looks.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
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
#include "halo/game/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"

namespace c_actor_apply_queued_look_to_unit {
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &actor_control_animation_state_table = halo::link::ref<const uint8_t []>(halo::ai::vars().actor_control_animation_state_table);
}


/**
 * actor_apply_queued_look_to_unit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_apply_queued_look_to_unit.c.txt.
 *
 * @address 0x42a640
 */
void halo::ai::look_ops::apply_queued_look_to_unit()
{
    using namespace c_actor_apply_queued_look_to_unit;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    uint32_t unit_index = actor->unit_index;
    unit_object *unit = (unit_object *)halo::ai::object_at(unit_index);
    unit_control_data control;

    control.animation_state = (int8_t)actor_control_animation_state_table[actor->control_animation_mode * 2];
    control.aiming_speed = (int8_t)actor->unknown_6ee[10];
    control.control_flags = *(uint16_t *)&actor->control_flags;
    control.weapon_index = -1;
    control.grenade_index = -1;
    control.zoom_level = -1;
    control.unknown_0a = 0;
    control.throttle = *(real_vector3d *)&actor->throttle.i;
    control.primary_trigger = halo::bit_cast<float>(actor->override_target);
    control.facing_vector = *(real_vector3d *)&actor->snapshot_facing.i;
    control.aiming_vector = *(real_vector3d *)&actor->aiming_vector_snapshot.i;
    control.looking_vector = *(real_vector3d *)&actor->looking_vector_snapshot.i;

    if (((struct unit_object *)unit)->unit.controlling_player != halo::k_dword_none && halo::game::globals().local_player_globals->input_disabled == 0) {
        return;
    }
    if (actor->unit_control_pending != 0) {
        halo::units::unit_refresh_targeting_flag_and_weapons(unit_index, 1);
        actor->unit_control_pending = 0;
    }
    halo::units::unit_apply_control_block(actor->unit_index, &control, -1);
    if (actor->control_animation_impulse != -1) {
        halo::units::unit_try_start_scripted_action_animation(actor->unit_index, actor->control_animation_impulse,
            reinterpret_cast<const real_vector2d *>(&actor->unknown_6ee[2]));
    }
    if (actor->persistent_control_ticks > 0) {
        unit_object *object = (unit_object *)halo::ai::object_at(actor->unit_index);

        object->unit.persistent_control_ticks = actor->persistent_control_ticks;
        object->unit.persistent_control_flags = actor->persistent_control_flags;
    }
}

namespace halo::ai {
void actor_apply_queued_look_to_unit(datum_index actor_index)
{
    halo::ai::look_ops(actor_index).apply_queued_look_to_unit();
}
}

namespace c_actor_begin_vocalization {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &actor_vocalization_duration = halo::link::ref<float [14]>(halo::ai::vars().actor_vocalization_duration);
static auto &actor_vocalization_variant = halo::link::ref<int16_t [14][2]>(halo::ai::vars().actor_vocalization_variant);
}


/**
 * actor_begin_vocalization: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_begin_vocalization.c.txt.
 *
 * @address 0x4142d0
 */
uint8_t halo::ai::look_ops::begin_vocalization(int16_t line, int16_t variant, actor_vocalization_context *context)
{
    using namespace c_actor_begin_vocalization;
    datum_index actor_index = datum;
    actor *self;
    Actor *actor_definition;
    prop *target;
    int16_t awareness;
    uint8_t urgent;
    float duration;
    float low;
    float high;
    int32_t ticks;

    self = halo::ai::actor_at(actor_index);
    awareness = self->awareness_level;
    actor_definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    if ((awareness < 2 && line < 13) || line < self->vocalization_line) {
        return 0;
    }

    urgent = (uint8_t)(self->flee_reason > 6);

    if ((line < 13 && self->mode == halo::ai::actor_mode::obey && self->mode_data.obey.allow_look == 0) ||
        (urgent != 0 && line < 4)) {
        return 0;
    }

    if (context->code == 1) {

        target = (prop *)halo::memory::datum_get(context->payload.handle, halo::ai::globals().prop_data);
        if (target == (prop *)0) {
            return 0;
        }
        if (line < 8) {
            if ((target->enemy == 0 && target->dead == 0) ||
                (target->dead != 0 && awareness > 2)) {
                if (urgent != 0 ||
                    (((target->is_parented == 0 || line < 4) && target->last_attention_time != -1) &&
                     halo::game::globals().game_time->game_time < target->last_attention_time + 600)) {
                    return 0;
                }
                target->last_attention_time = halo::game::globals().game_time->game_time;
                if (target->interest_satisfied <= target->interest) {
                    target->interest_satisfied = target->interest;
                }
            }
        }
    }

    duration = actor_vocalization_duration[line];
    if (self->awareness_level < 3 || self->combat_status == 0) {
        duration = duration + duration;
    }

    if (actor_definition->event_look_time_modifier[0] != 0.0f || actor_definition->event_look_time_modifier[1] != 0.0f) {
        low = (actor_definition->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_definition->event_look_time_modifier[0];
        high = (actor_definition->event_look_time_modifier[1] <= 2.0f) ? actor_definition->event_look_time_modifier[1] : 2.0f;
        duration = halo::math::random_real_range(low, high) * duration;
    }

    ticks = (int32_t)(duration * 30.0f + 0.5f);
    if (ticks > INT16_MAX) {
        ticks = INT16_MAX;
    }

    if (variant == 1) {
        variant = actor_vocalization_variant[line][self->combat_status > 3 ? 1 : 0];
    }

    self->vocalization_line = line;
    self->vocalization_state = (int16_t)ticks;
    self->vocalization_variant = variant;
    memcpy(&self->vocalization_source, context, sizeof(self->vocalization_source));
    return 1;
}

namespace halo::ai {
uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant, actor_vocalization_context *context)
{
    return halo::ai::look_ops(actor_index).begin_vocalization(line, variant, context);
}
}

namespace c_actor_clear_vocalization {
}


/**
 * actor_clear_vocalization: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_clear_vocalization.c.txt.
 *
 * @address 0x414560
 */
void halo::ai::look_ops::clear_vocalization()
{
    using namespace c_actor_clear_vocalization;
    datum_index actor_index = datum;
    actor *self;

    self = halo::ai::actor_at(actor_index);
    self->vocalization_variant = 0;
    self->vocalization_line = 0;
    self->vocalization_state = 0;
}

namespace halo::ai {
void actor_clear_vocalization(datum_index actor_index)
{
    halo::ai::look_ops(actor_index).clear_vocalization();
}
}

namespace c_actor_dispatch_look_handler_by_posture {



static const float k_perception_range_class_scale[4] = {0.4f, 0.6f, 0.8f, 1.0f};
}


/**
 * actor_dispatch_look_handler_by_posture: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_dispatch_look_handler_by_posture.c.txt.
 *
 * @address 0x41bb30
 */
int16_t halo::ai::look_ops::dispatch_look_handler_by_posture(int16_t posture, uint32_t actor_index, void *origin, void *target, uint8_t stance_a, uint8_t check_facing, uint16_t range_class)
{
    using namespace c_actor_dispatch_look_handler_by_posture;
    struct actor *actor = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(actor->actor_definition_tag);
    ActorVariant *definition;
    float *from = (float *)origin;
    float *to = (float *)target;
    float base;
    float range;
    float scale;
    float current;
    float dx, dy, dz;
    float distance_squared;

    if (posture != 0 && posture != 1) {
        return 0;
    }
    definition = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);
    base = actor_tag->max_vision_distance;
    if (definition->modified_vision_range > 0.0f) {
        base = definition->modified_vision_range;
    }
    range = base * k_perception_range_class_scale[(int16_t)range_class & 3];
    dx = to[0] - from[0];
    dy = to[1] - from[1];
    dz = to[2] - from[2];
    distance_squared = dz * dz + dy * dy + dx * dx;
    if (!(range * range > distance_squared)) {
        return 0;
    }
    scale = 1.0f;
    if ((static_cast<uint8_t>(actor_tag->flags) & 1) == 0) {
        if ((int8_t)stance_a == 0) {
            scale = 0.3f;
        } else if ((int8_t)stance_a == 1) {
            scale = 0.7f;
        }
    }
    current = scale * range;
    if (!(distance_squared < current * current)) {
        return 0;
    }

    if (actor->swarm == 0 && check_facing != 0) {
        float forward = dz * actor->unit_looking_vector.k + dy * actor->unit_looking_vector.j + dx * actor->unit_looking_vector.i;
        float left = dz * actor->looking_left_vector.k + dy * actor->looking_left_vector.j + dx * actor->looking_left_vector.i;
        float up = dz * actor->looking_up_vector.k + dy * actor->looking_up_vector.j + dx * actor->looking_up_vector.i;
        float elevation = (float)halo::libm::atan2((double)up, halo::libm::sqrt((double)(left * left + forward * forward)));

        if (elevation > 0.5235988f || !(elevation > -0.78539819f)) {
            range = 0.0f;
            current = 0.0f;
        } else {
            float azimuth = (float)halo::libm::fabs(halo::libm::atan2((double)left, (double)forward));

            halo::ai::unit_get_move_speed_for_range(actor_index, range, scale, azimuth, &range, &scale);
            current = scale;
        }
    } else {
        range = current * 0.7f;
    }

    if (posture == 0 && range * range > distance_squared) {
        return distance_squared < 36.0f ? 3 : 2;
    }
    return distance_squared < current * current ? 1 : 0;
}

namespace halo::ai {
int16_t actor_dispatch_look_handler_by_posture(int16_t posture, uint32_t actor_index, void *origin, void *target, uint8_t stance_a, uint8_t check_facing, uint16_t range_class)
{
    return halo::ai::look_ops::dispatch_look_handler_by_posture(posture, actor_index, origin, target, stance_a, check_facing, range_class);
}
}

namespace c_actor_flee_look_away {
}


/**
 * actor_flee_look_away: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_flee_look_away.c.txt.
 *
 * @address 0x40d4c0
 */
uint32_t halo::ai::look_ops::flee_look_away()
{
    using namespace c_actor_flee_look_away;
    datum_index actor_index = datum;
    actor *self;
    uint32_t result;

    self = halo::ai::actor_at(actor_index);
    result = 0;
    if (self->mode == halo::ai::actor_mode::flee && self->mode_data.flee.finished != 0) {

        uint8_t order[0x84];

        memset(order, 0, sizeof(order));
        halo::ai::actor_build_order_look(actor_index, (actor_order *)order, reinterpret_cast<actor_look_request *>(&self->mode_data));
        halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::guard, order);
        result = 1;
    }
    return result;
}

namespace halo::ai {
uint32_t actor_flee_look_away(datum_index actor_index)
{
    return halo::ai::look_ops(actor_index).flee_look_away();
}
}

namespace c_actor_get_idle_facing_range {
}


/**
 * actor_get_idle_facing_range: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_idle_facing_range.c.txt.
 *
 * @address 0x4150f0
 */
float * halo::ai::look_ops::get_idle_facing_range()
{
    using namespace c_actor_get_idle_facing_range;
    datum_index actor_index = datum;
    actor *self;
    Actor *definition;
    int16_t state;

    self = halo::ai::actor_at(actor_index);
    state = self->look_posture;
    definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    if (state == 2) {
        return definition->guard_idle_facing;
    }
    if (2 < state && state < 5) {
        return definition->combat_idle_facing;
    }
    return definition->noncombat_idle_facing;
}

namespace halo::ai {
float * actor_get_idle_facing_range(datum_index actor_index)
{
    return halo::ai::look_ops(actor_index).get_idle_facing_range();
}
}

namespace c_actor_issue_order_or_vocalize {
}


/**
 * actor_issue_order_or_vocalize: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_issue_order_or_vocalize.c.txt.
 *
 * @address 0x4302e0
 */
void halo::ai::look_ops::issue_order_or_vocalize(datum_index prop_index, datum_index actor_index, datum_index vehicle_object_index, int16_t line, int16_t variant)
{
    using namespace c_actor_issue_order_or_vocalize;
    void *vehicle_obj;
    actor_vocalization_context context;
    prop *p;
    int16_t kind;

    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }
    if (variant < 1) {
        return;
    }
    if (vehicle_object_index == (datum_index)k_datum_index_none) {
        return;
    }
    vehicle_obj = halo::objects::object_try_and_get(vehicle_object_index, 3);
    if (vehicle_obj == 0) {
        return;
    }

    if (prop_index == (datum_index)k_datum_index_none) {
        prop_index = halo::ai::actor_find_prop_for_object(vehicle_object_index, actor_index);
    }
    kind = -1;
    if (prop_index != (datum_index)k_datum_index_none) {
        p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];
        kind = p->state;
    }

    if (prop_index == (datum_index)k_datum_index_none || kind < 2 || 3 < kind) {
        context.code = 3;
        halo::units::unit_get_primary_eye_marker_position(vehicle_object_index, &context.payload.point);
    } else {
        context.code = 1;
        context.payload.handle = prop_index;
    }
    halo::ai::actor_begin_vocalization(actor_index, line, variant, &context);
}

namespace halo::ai {
void actor_issue_order_or_vocalize(datum_index prop_index, datum_index actor_index, datum_index vehicle_object_index, int16_t line, int16_t variant)
{
    halo::ai::look_ops::issue_order_or_vocalize(prop_index, actor_index, vehicle_object_index, line, variant);
}
}

namespace c_actor_look_get_wait_ticks {
}


/**
 * actor_look_get_wait_ticks: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_look_get_wait_ticks.c.txt.
 *
 * @address 0x415150
 */
int32_t halo::ai::look_ops::look_get_wait_ticks(int16_t mode, uint32_t flags, float *deviation_table)
{
    using namespace c_actor_look_get_wait_ticks;
    datum_index actor_index = datum;
    float lo, hi;
    float fraction;
    float ticks;
    uint32_t rng;
    Weapon *weapon_definition;
    int32_t result;

    switch (mode) {
    case 0:
        lo = deviation_table[0];
        hi = deviation_table[1];
        break;
    case 1:
        lo = deviation_table[2];
        hi = deviation_table[3];
        break;
    case 2:
        lo = deviation_table[4];
        hi = deviation_table[5];
        break;
    default:
        lo = *(float *)&flags;
        hi = *(float *)&flags;
        break;
    }

    if (((lo < 0.0f) == (lo == 0.0f)) || ((hi < 0.0f) == (hi == 0.0f))) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        rng = halo::math::globals().random_seed_global;
        fraction = (hi - lo) * (float)(rng >> 0x10) * 1.5259022e-05f + lo;
    } else {
        fraction = 0.5f;
    }

    {
        datum_index weapon = halo::ai::actor_get_threat_weapon_object_index(actor_index);

        weapon_definition = weapon == k_datum_index_none ? 0 :
            halo::ai::tag_data<Weapon>(halo::ai::object_at(weapon)->definition_tag);
    }
    if (weapon_definition != 0 && 0.0f < weapon_definition->looking_time_modifier) {
        fraction = fraction * weapon_definition->looking_time_modifier;
    }

    if ((uint8_t)flags != 0) {
        fraction = fraction * 1.5f;
    }

    ticks = fraction * 30.0f;
    result = halo::x87::fistp_round(ticks);
    if (result < 2) {
        result = 1;
    }
    return result;
}

namespace halo::ai {
int32_t actor_look_get_wait_ticks(datum_index actor_index, int16_t mode, uint32_t flags, float *deviation_table)
{
    return halo::ai::look_ops(actor_index).look_get_wait_ticks(mode, flags, deviation_table);
}
}

namespace c_actor_look_pick_random_point_in_cone {
}


/**
 * actor_look_pick_random_point_in_cone: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_look_pick_random_point_in_cone.c.txt.
 *
 * @address 0x415260
 */
uint8_t halo::ai::look_ops::look_pick_random_point_in_cone(void *origin, float yaw_min, float yaw_max, float pitch_min, float pitch_max, real_vector3d *base_direction, uint8_t check_obstruction, real_point3d *out)
{
    using namespace c_actor_look_pick_random_point_in_cone;
    real_vector3d right_axis;
    real_vector3d direction;
    float yaw, pitch;
    float sin_a, cos_a;
    uint32_t rng;
    int16_t attempt;
    uint8_t trace_buffer[80];

    right_axis.i = -base_direction->j;
    right_axis.j = base_direction->i;
    right_axis.k = 0.0f;
    if (halo::math::vector3d_normalize_with_length(right_axis) == 0.0f) {
        right_axis = *halo::math::globals().global_left3d_pointer;
    }

    for (attempt = 0; attempt < 10; attempt++) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        rng = halo::math::globals().random_seed_global;
        yaw = (float)(rng >> 0x10) * 1.5259022e-05f * (yaw_max - yaw_min) + yaw_min;

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        rng = halo::math::globals().random_seed_global;
        pitch = (float)(rng >> 0x10) * 1.5259022e-05f * (pitch_max - pitch_min) + pitch_min;

        direction = *base_direction;

        cos_a = (float)halo::libm::cos((double)pitch);
        sin_a = (float)halo::libm::sin((double)pitch);
        halo::math::vector3d_rotate_about_axis(direction, right_axis, sin_a, cos_a);

        cos_a = (float)halo::libm::cos((double)yaw);
        sin_a = (float)halo::libm::sin((double)yaw);
        halo::math::vector3d_rotate_about_axis(direction, *(real_vector3d *)halo::math::globals().global_up3d_pointer, sin_a, cos_a);

        if (!check_obstruction) {
            break;
        }

        {
            real_vector3d scaled;
            scaled.i = direction.i * 3.0f;
            scaled.j = direction.j * 3.0f;
            scaled.k = direction.k * 3.0f;

            if (!halo::physics::collision_test_movement_segment(0x21, (real_point3d *)origin, &scaled, (uint32_t)k_datum_index_none, (collision_result *)trace_buffer)) {
                break;
            }
        }
    }
    if (check_obstruction && attempt >= 10) {
        return 0;
    }

    {
        double length = halo::libm::sqrt((double)(direction.k * direction.k + direction.j * direction.j + direction.i * direction.i));
        if ((double)0.0001f <= ((length < 0.0) ? -length : length)) {
            double inv = 1.0 / length;
            direction.i = (float)(direction.i * inv);
            direction.j = (float)(direction.j * inv);
            direction.k = (float)(direction.k * inv);
        }
        out->x = direction.i;
        out->y = direction.j;
        out->z = direction.k;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_look_pick_random_point_in_cone(void *origin, float yaw_min, float yaw_max, float pitch_min, float pitch_max, real_vector3d *base_direction, uint8_t check_obstruction, real_point3d *out)
{
    return halo::ai::look_ops::look_pick_random_point_in_cone(origin, yaw_min, yaw_max, pitch_min, pitch_max, base_direction, check_obstruction, out);
}
}

namespace c_actor_look_randomize_direction {
}


/**
 * actor_look_randomize_direction: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_look_randomize_direction.c.txt.
 *
 * @address 0x414f50
 */
void halo::ai::look_ops::look_randomize_direction(float *deviation_table, real_vector3d *base_direction)
{
    using namespace c_actor_look_randomize_direction;
    datum_index actor_index = datum;
    actor *self;
    Actor *definition;
    uint32_t out_in_front;
    float yaw_max, pitch_max;
    float delta_l, delta_r;
    float yaw_min;
    int32_t wait_ticks;
    real_point3d look_point;

    self = halo::ai::actor_at(actor_index);
    definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    out_in_front = 0;
    self->idle_minor_active = 0;

    if (!halo::ai::actor_select_facing_target_prop(actor_index, 0, 0, reinterpret_cast<actor_recognition_scan_result *>(&self->idle_look_direction_type),
            (uint8_t *)&out_in_front)) {
        yaw_max = (definition->maximum_looking_deviation.yaw <= definition->idle_looking_range.yaw)
                      ? definition->maximum_looking_deviation.yaw
                      : definition->idle_looking_range.yaw;
        pitch_max = (definition->maximum_looking_deviation.pitch <= definition->idle_looking_range.pitch)
                        ? definition->maximum_looking_deviation.pitch
                        : definition->idle_looking_range.pitch;

        if (self->awareness_level == 3) {
            delta_l = definition->combat_look_delta_l;
            delta_r = definition->combat_look_delta_r;
        } else {
            delta_l = definition->noncombat_look_delta_l;
            delta_r = definition->noncombat_look_delta_r;
        }

        yaw_min = -delta_l;
        if (yaw_min < -yaw_max) {
            yaw_min = -yaw_max;
        }
        if (delta_r < yaw_max) {
            yaw_max = delta_r;
        }

        if (!halo::ai::actor_look_pick_random_point_in_cone(&self->aim_origin, yaw_min, yaw_max, -pitch_max, pitch_max,
                                                    base_direction, 0, &look_point)) {
            return;
        }
        self->idle_look_point = look_point;
        out_in_front = 0;
        self->idle_look_direction_type = 4;
    }

    wait_ticks = halo::ai::actor_look_get_wait_ticks(actor_index, 2, out_in_front, deviation_table);
    self->idle_minor_timer = wait_ticks;
    if (wait_ticks != 0) {
        self->idle_minor_active = 1;
    }
}

namespace halo::ai {
void actor_look_randomize_direction(datum_index actor_index, float *deviation_table, real_vector3d *base_direction)
{
    halo::ai::look_ops(actor_index).look_randomize_direction(deviation_table, base_direction);
}
}

namespace c_actor_lookup_small_table_entry {
static auto &actor_lookup_table_006555a8 = halo::link::ref<int16_t [12]>(halo::ai::vars().actor_lookup_table_006555a8);
}


/**
 * actor_lookup_small_table_entry: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_lookup_small_table_entry.c.txt.
 *
 * @address 0x40e790
 */
int32_t halo::ai::look_ops::lookup_small_table_entry(int16_t index)
{
    using namespace c_actor_lookup_small_table_entry;
    if (-1 < index && index < 0xc) {
        return actor_lookup_table_006555a8[index];
    }
    return 0;
}

namespace halo::ai {
int32_t actor_lookup_small_table_entry(int16_t index)
{
    return halo::ai::look_ops::lookup_small_table_entry(index);
}
}

