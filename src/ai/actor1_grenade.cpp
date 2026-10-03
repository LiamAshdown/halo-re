#include "halo/ai/actor_grenade.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"

namespace c_actor_attempt_grenade_throw {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern ai_globals *ai_globals_ptr;

extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index);
extern void unit_set_control_countdown(uint32_t unit_index, int32_t countdown, uint32_t extra_control_flags);
extern void encounter_recompute_morale(datum_index encounter_index);
extern void actor_delete(datum_index actor_index, uint32_t flag);

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)

static uint32_t actor_death_random_16(void)
{
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    return halo::math::globals().random_seed_global >> 16;
}
}
}

extern "C" void actor_attempt_grenade_throw(datum_index actor_index);

/**
 * actor_attempt_grenade_throw: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_attempt_grenade_throw.c.txt.
 *
 * @address 0x428ab0
 */
void halo::ai::grenade_ops::attempt_grenade_throw()
{
    using namespace c_actor_attempt_grenade_throw;
    datum_index actor_index = datum;
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *variant = (uint8_t *)halo::cache::globals().tag_instances[((actor *)a)->actor_variant_tag & halo::k_slot_mask].data;
    datum_index encounter = ((actor *)a)->encounter_index;
    uint8_t *unit;
    datum_index weapon;
    real roll;

    if (((actor *)a)->awareness_level == 3 && ((struct actor *)a)->combat_status >= 2) {
        unit = OBJECT_DATA(((actor *)a)->unit_index);
        if (((((unit_object *)unit)->unit.flags >> 6) & 1) &&
            unit_get_weapon_object_index(((actor *)a)->unit_index, ((unit_object *)unit)->unit.current_weapon_index) != k_datum_index_none &&
            ((struct unit_object *)unit)->unit.delayed_weapon_drop_ticks > 0) {
            float chance = ((ActorVariant *)variant)->death_fire_wildly_chance;

            if (!(chance >= 0.1f)) {
                chance = 0.1f;
            } else if (!(chance <= 0.6f)) {
                chance = 0.6f;
            }
            if (a[0x378] || (((struct actor *)a)->firing_target_type > 0 && *(float *)(a + 0x648) < 3.0f)) {
                float boosted = chance * 4.0f;

                if (!(boosted <= 0.6f)) {
                    boosted = 0.6f;
                }
                if (!(chance > boosted)) {
                    chance = boosted;
                }
            }
            if (halo::math::random_real() < chance) {
                float seconds = ((ActorVariant *)variant)->death_fire_wildly_time;
                int16_t ticks;

                if (seconds == 0.0f) {
                    seconds = halo::math::random_real_range(0.8f, 1.3f);
                } else if (!(seconds >= 0.8f)) {
                    seconds = 0.8f;
                } else if (!(seconds <= 1.3f)) {
                    seconds = 1.3f;
                }
                ticks = (int16_t)(int32_t)(seconds * 30.0f);
                unit_set_control_countdown(((actor *)a)->unit_index, ticks, 0x800);
                unit[0x28c] = (uint8_t)ticks;
            }
        }
    }

    roll = (real)(int32_t)actor_death_random_16() * 1.5259022e-05f;
    unit = OBJECT_DATA(((actor *)a)->unit_index);
    weapon = ((unit_object *)unit)->unit.current_weapon_index != -1 ? *(datum_index *)(unit + 0x2f8 + ((unit_object *)unit)->unit.current_weapon_index * 4)
                                              : k_datum_index_none;
    if (!ai_globals_ptr->grenades_enabled || roll < ((ActorVariant *)variant)->don_t_drop_grenades_chance) {
        *(int16_t *)(unit + 0x31e) = 0;
    }
    if (weapon != k_datum_index_none) {
        float lo = *(float *)(variant + 0x1d8);
        float hi = *(float *)(variant + 0x1dc);
        int16_t least = *(int16_t *)(variant + 0x1e0);
        int16_t most = *(int16_t *)(variant + 0x1e2);

        if (lo > 0.0f || hi > 0.0f) {
            real r = (real)(int32_t)actor_death_random_16() * 1.5259022e-05f;

            halo::items::weapon_set_loaded_ammo_fraction(weapon, (hi - lo) * r + lo);
        }
        if (least > 0 || most > 0) {
            int16_t counts[2] = {0, 0};
            uint32_t r = actor_death_random_16();

            counts[0] = (int16_t)((uint32_t)(((int32_t)(int16_t)(most + 1) - least) * (int32_t)r) >> 16) + least;
            halo::items::weapon_set_ammo_counts(weapon, counts);
        }
    }
    actor_delete(actor_index, 1);
    if (encounter != k_datum_index_none) {
        encounter_recompute_morale(encounter);
    }
}

extern "C" void actor_attempt_grenade_throw(datum_index actor_index)
{
    halo::ai::grenade_ops(actor_index).attempt_grenade_throw();
}

#undef OBJECT_DATA

namespace c_actor_can_throw_grenade_at_target {
extern "C" {
extern data_array *actor_data;
extern data_array *encounter_data;
extern game_time_globals *game_time;

extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);

extern uint8_t actor_find_grenade_landing_spot(datum_index actor_index, real_point3d *out_point, datum_index *out_target_handle, int32_t *out_relationship);
extern uint8_t actor_score_blast_area_clear(datum_index actor_index, float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count);
extern uint32_t actor_commit_grenade_toss(datum_index actor_index, real_point3d *point, uint32_t object_handle, uint32_t exclude_object_index);
}
}

extern "C" uint8_t actor_can_throw_grenade_at_target(datum_index actor_index);

/**
 * actor_can_throw_grenade_at_target: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_can_throw_grenade_at_target.c.txt.
 *
 * @address 0x40d9c0
 */
uint8_t halo::ai::grenade_ops::can_throw_grenade_at_target()
{
    using namespace c_actor_can_throw_grenade_at_target;
    datum_index actor_index = datum;
    actor *self;
    ActorVariant *variant;
    int32_t now;
    int16_t hostile_count;
    real_point3d point;
    datum_index target_handle;
    int32_t relationship;
    float random_wait;
    int16_t random_wait_ticks;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    variant = (ActorVariant *)halo::cache::globals().tag_instances[self->actor_variant_tag & halo::k_slot_mask].data;
    now = game_time->game_time;

    if (self->active_unit_index != (datum_index)k_datum_index_none) {
        return 0;
    }

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = (encounter *)((uint8_t *)encounter_data->data +
                                        (self->encounter_index & halo::k_slot_mask) * sizeof(encounter));
        int32_t squad_deadline = enc->last_grenade_time;

        random_wait = ((ActorVariant *)variant)->encounter_grenade_timeout *
                      weapon_get_zoom_fov_resolved(0x18, ((struct encounter *)enc)->team);
        if (self->playfight != 0) {
            random_wait = random_wait + random_wait;
        }
        if (squad_deadline != -1) {
            random_wait_ticks = (int16_t)(int32_t)(random_wait * 30.0f);
            if (now < random_wait_ticks + squad_deadline) {
                return 0;
            }
        }
    }

    if (actor_find_grenade_landing_spot(actor_index, &point, &target_handle, &relationship) != 0 &&
        actor_score_blast_area_clear(actor_index, ((ActorVariant *)variant)->enemy_radius,
                     ((ActorVariant *)variant)->collateral_damage_radius, &point, &hostile_count) != 0 &&
        ((ActorVariant *)variant)->minimum_enemy_count <= (int16_t)hostile_count &&
        actor_commit_grenade_toss(actor_index, &point, target_handle, (uint32_t)relationship) != 0) {
        return 1;
    }
    return 0;
}

extern "C" uint8_t actor_can_throw_grenade_at_target(datum_index actor_index)
{
    return halo::ai::grenade_ops(actor_index).can_throw_grenade_at_target();
}

namespace c_actor_check_grenade_facing_and_commit {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *encounter_data;
extern game_time_globals *game_time;

extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index);
extern uint8_t actor_can_throw_grenade_at_target(datum_index actor_index);
}
}

extern "C" uint8_t actor_check_grenade_facing_and_commit(datum_index actor_index, uint8_t force_commit);

/**
 * actor_check_grenade_facing_and_commit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_check_grenade_facing_and_commit.c.txt.
 *
 * @address 0x40db00
 */
uint8_t halo::ai::grenade_ops::check_grenade_facing_and_commit(uint8_t force_commit)
{
    using namespace c_actor_check_grenade_facing_and_commit;
    datum_index actor_index = datum;
    actor *self;
    datum_index unit_index;
    object_header *unit_header;
    object *unit_obj;
    real body_damage;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    unit_index = self->unit_index;

    if (unit_is_in_busy_animation_state(self->unit_index) != 0) {
        return 0;
    }

    unit_header = (object_header *)object_data->data + (unit_index & halo::k_slot_mask);
    unit_obj = unit_header->data;
    body_damage = unit_obj->current_body_damage;
    if (body_damage > 0.0f) {
        return 0;
    }

    if (force_commit == 0) {
        if (actor_can_throw_grenade_at_target(actor_index) == 0) {
            self->grenade_throw_pending = 0;
        }
    }

    if (self->grenade_throw_pending != 0) {
        real_vector2d delta;
        float dot;

        delta.i = self->grenade_impact_point.x - self->body_position.x;
        delta.j = self->grenade_impact_point.y - self->body_position.y;

        if (halo::math::vector2d_normalize_with_length(delta) > 0.0f) {
            dot = delta.i * self->facing.i + delta.j * self->facing.j;
            if (dot >= 0.8660254f) {
                self->throw_grenade = 1;
                self->grenade_throw_pending = 0;
                if (self->encounter_index != (datum_index)k_datum_index_none) {
                    encounter *enc = (encounter *)((uint8_t *)encounter_data->data +
                                                    (self->encounter_index & halo::k_slot_mask) * sizeof(encounter));
                    enc->last_grenade_time = game_time->game_time;
                }
                return 1;
            }
        }
    }
    return 0;
}

extern "C" uint8_t actor_check_grenade_facing_and_commit(datum_index actor_index, uint8_t force_commit)
{
    return halo::ai::grenade_ops(actor_index).check_grenade_facing_and_commit(force_commit);
}

namespace c_actor_commit_grenade_toss {
extern "C" {
extern data_array *actor_data;

extern uint8_t actor_get_grenade_launch_velocity(int16_t grenade_type, real_vector3d *direction, void *origin,
    float range, real_point3d *point, int32_t max_time, float *speed, void *out_time_or_fraction, real_vector3d *out_velocity,
    float *out_gravity);
extern uint8_t actor_grenade_parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index,
    real_point3d *start_position, real total_time, real vertical_acceleration, datum_index exclude_object_index,
    uint8_t wide_mask);
}
}

extern "C" uint32_t actor_commit_grenade_toss(datum_index actor_index, real_point3d *point, uint32_t object_handle, uint32_t exclude_object_index);

/**
 * actor_commit_grenade_toss: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_commit_grenade_toss.c.txt.
 *
 * @address 0x411180
 */
uint32_t halo::ai::grenade_ops::commit_grenade_toss(real_point3d *point, uint32_t object_handle, uint32_t exclude_object_index)
{
    using namespace c_actor_commit_grenade_toss;
    datum_index actor_index = datum;
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *variant = (uint8_t *)halo::cache::globals().tag_instances[((actor *)a)->actor_variant_tag & halo::k_slot_mask].data;
    real_point3d origin = *(real_point3d *)&((actor *)a)->aim_origin.x;
    real_vector3d direction;
    real_vector3d velocity;
    float speed;
    float flight_time;
    float gravity;

    if (!actor_get_grenade_launch_velocity(*(int16_t *)&((ActorVariant *)variant)->grenade_type, &direction, &origin,
                                           ((ActorVariant *)variant)->grenade_velocity, point, 0, &speed, &flight_time,
                                           &velocity, &gravity)) {
        return 0;
    }
    if (!actor_grenade_parabolic_path_clear(&velocity, actor_index, &origin, flight_time, gravity, exclude_object_index,
                                            (uint8_t)(((actor *)a)->active_unit_index != k_datum_index_none))) {
        return 0;
    }
    *(real_point3d *)&((actor *)a)->grenade_impact_point.x = *point;
    ((struct actor *)a)->grenade_target_prop_index = object_handle;
    ((actor *)a)->grenade_throw_direction = direction;
    *(uint32_t *)(a + 0x6b8) = exclude_object_index;
    ((actor *)a)->grenade_throw_speed = speed;
    a[0x6a1] = 0;
    return 1;
}

extern "C" uint32_t actor_commit_grenade_toss(datum_index actor_index, real_point3d *point, uint32_t object_handle, uint32_t exclude_object_index)
{
    return halo::ai::grenade_ops(actor_index).commit_grenade_toss(point, object_handle, exclude_object_index);
}

namespace c_actor_compute_grenade_aim_direction {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

extern void actor_get_aim_from_position(datum_index actor_index, uint32_t out_position[3]);
}
}

extern "C" uint32_t actor_compute_grenade_aim_direction(datum_index actor_index, real_point3d *target_point, real_vector3d *out_direction, float *out_698);

/**
 * actor_compute_grenade_aim_direction: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_compute_grenade_aim_direction.c.txt.
 *
 * @address 0x40f7e0
 */
uint32_t halo::ai::grenade_ops::compute_grenade_aim_direction(real_point3d *target_point, real_vector3d *out_direction, float *out_698)
{
    using namespace c_actor_compute_grenade_aim_direction;
    datum_index actor_index = datum;
    actor *self;
    uint32_t result;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    result = (uint32_t)-1;

    if (self->firing_state == 2) {
        real_vector3d aim_from;

        if (self->firing_target_type == 1 && self->firing_target_prop_index != (uint32_t)-1) {
            prop *p = (prop *)((uint8_t *)prop_data->data + (self->firing_target_prop_index & halo::k_slot_mask) * sizeof(prop));
            if (1 < p->state && p->state < 4) {
                result = p->object_index;
            }
        }

        if (self->firing_vector_ballistic == 0) {
            out_direction->i = self->grenade_aim_direction.i - target_point->x;
            out_direction->j = self->grenade_aim_direction.j - target_point->y;
            out_direction->k = self->grenade_aim_direction.k - target_point->z;
            halo::math::vector3d_normalize_with_length(*out_direction);
        } else {
            *out_direction = self->firing_vector;
        }

        actor_get_aim_from_position(actor_index, (uint32_t *)&aim_from);

        if (!(aim_from.i * out_direction->i + aim_from.j * out_direction->j + aim_from.k * out_direction->k >= 0.8660254f)) {
            uint8_t should_rotate = 1;
            real_vector3d axis;

            halo::math::vector3d_cross_product(axis, *out_direction, aim_from);
            if (halo::math::vector3d_normalize_with_length(axis) == 0.0f) {
                halo::math::vector3d_build_perpendicular(axis, aim_from);
                if (halo::math::vector3d_normalize_with_length(axis) == 0.0f) {
                    should_rotate = 0;
                }
            }
            *out_direction = aim_from;
            if (should_rotate) {
                halo::math::vector3d_rotate_about_axis(*out_direction, axis, 0.5f, 0.86602539f);
            }
        }
        *out_698 = self->projectile_error;
    }
    return result;
}

extern "C" uint32_t actor_compute_grenade_aim_direction(datum_index actor_index, real_point3d *target_point, real_vector3d *out_direction, float *out_698)
{
    return halo::ai::grenade_ops(actor_index).compute_grenade_aim_direction(target_point, out_direction, out_698);
}

namespace c_actor_consider_grenade_throw {
extern "C" {
extern data_array *actor_data;
extern ai_globals *ai_globals_ptr;
extern game_time_globals *game_time;

extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);
extern uint8_t actor_can_throw_grenade_at_target(datum_index actor_index);
extern uint8_t actor_check_grenade_facing_and_commit(datum_index actor_index, uint8_t force_commit);
}
}

extern "C" uint8_t actor_consider_grenade_throw(datum_index actor_index);

/**
 * actor_consider_grenade_throw: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_consider_grenade_throw.c.txt.
 *
 * @address 0x40dc30
 */
uint8_t halo::ai::grenade_ops::consider_grenade_throw()
{
    using namespace c_actor_consider_grenade_throw;
    datum_index actor_index = datum;
    actor *self;
    ActorVariant *variant;
    int32_t now;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    variant = (ActorVariant *)halo::cache::globals().tag_instances[self->actor_variant_tag & halo::k_slot_mask].data;

    if (self->grenade_throw_pending != 0) {
        return 1;
    }
    if (ai_globals_ptr->grenades_enabled == 0 || variant->grenade_stimulus == -1 ||
        variant->minimum_enemy_count == -1) {
        return 0;
    }

    now = game_time->game_time;

    if (self->last_grenade_check_time != (uint32_t)-1 &&
        (variant->grenade_check_time * 30.0f + (float)(int32_t)self->last_grenade_check_time) > (float)now) {
        return 0;
    }

    {

        float scaled = variant->grenade_chance * weapon_get_zoom_fov_resolved(0x17, ((struct actor *)self)->team);
        float roll;

        self->last_grenade_check_time = now;
        roll = halo::math::random_real();
        if (roll < scaled && actor_can_throw_grenade_at_target(actor_index) != 0) {
            self->grenade_throw_pending = 1;
            actor_check_grenade_facing_and_commit(actor_index, 1);
            return 1;
        }
    }
    return 0;
}

extern "C" uint8_t actor_consider_grenade_throw(datum_index actor_index)
{
    return halo::ai::grenade_ops(actor_index).consider_grenade_throw();
}

namespace c_actor_evaluate_grenade_target_position {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *prop_data;
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index);
extern uint8_t actor_probe_step_direction(datum_index actor_index, float step_distance, real_vector2d *direction,
    uint16_t *variant, float step_up, uint8_t *out_flag, void *extra_param);
extern uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command);
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2]);
}
}

extern "C" uint8_t actor_evaluate_grenade_target_position(datum_index actor_index);

/**
 * actor_evaluate_grenade_target_position: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_evaluate_grenade_target_position.c.txt.
 *
 * @address 0x40de70
 */
uint8_t halo::ai::grenade_ops::evaluate_grenade_target_position()
{
    using namespace c_actor_evaluate_grenade_target_position;
    datum_index actor_index = datum;
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t queued = 0;
    uint8_t *unit_tag;
    uint8_t *actor_tag;
    uint8_t *p;
    float *facing = (float *)(a + 0x174);

    if (((actor *)a)->active_unit_index != k_datum_index_none || ((actor *)a)->secondary_action != -1) {
        return 0;
    }
    if (((actor *)a)->unit_index != k_datum_index_none && unit_is_in_busy_animation_state(((actor *)a)->unit_index)) {
        return 0;
    }
    if (a[0x504] || ((actor *)a)->target_unit_index == k_datum_index_none) {
        return 0;
    }
    unit_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)((uint8_t *)((object_header *)object_data->data)
        [((actor *)a)->unit_index & halo::k_slot_mask].data) & halo::k_slot_mask].data;
    p = (uint8_t *)prop_data->data + (((actor *)a)->target_unit_index & halo::k_slot_mask) * k_prop_size;
    if (!(*(float *)(unit_tag + 0x234) > 0.0f)) {
        return 0;
    }
    actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((actor *)a)->actor_definition_tag & halo::k_slot_mask].data;
    if (*(uint32_t *)actor_tag & 0x200000) {
        float dot = *(float *)(p + 0xe8) * facing[2] + *(float *)(p + 0xe4) * facing[1] + *(float *)(p + 0xe0) * facing[0];

        if (!(dot > 0.4f)) {
            return 0;
        }
    } else {
        real_vector2d flat;

        flat.i = *(float *)(p + 0xe0);
        flat.j = *(float *)(p + 0xe4);
        if (halo::math::vector2d_normalize_with_length(flat) > 0.0f && !(flat.j * facing[1] + flat.i * facing[0] > 0.4f)) {
            return 0;
        }
    }
    {
        real_vector2d direction;
        uint16_t side = 4;
        uint8_t flag;
        float extra[4];
        int16_t action;

        direction.i = *(float *)(p + 0xe0);
        direction.j = *(float *)(p + 0xe4);
        halo::math::vector2d_normalize_with_length(direction);
        if (!actor_probe_step_direction(actor_index, *(float *)(unit_tag + 0x234), &direction, &side, 0.0f, &flag, extra)) {
            return 0;
        }
        action = (int16_t)side == 1 ? 7 : 6;
        if (unit_scripted_action_animation_exists(((actor *)a)->unit_index, action)) {
            queued = actor_queue_secondary_action(actor_index, action, (uint32_t *)&direction);
        }
    }
    return queued;
}

extern "C" uint8_t actor_evaluate_grenade_target_position(datum_index actor_index)
{
    return halo::ai::grenade_ops(actor_index).evaluate_grenade_target_position();
}

namespace c_actor_find_grenade_landing_spot {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern void actor_choose_random_point_near(real_point3d *inout_point, float radius);
}
}

extern "C" uint8_t actor_find_grenade_landing_spot(datum_index actor_index, real_point3d *out_point, datum_index *out_target_handle, int32_t *out_relationship);

/**
 * actor_find_grenade_landing_spot: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_grenade_landing_spot.c.txt.
 *
 * @address 0x410c90
 */
uint8_t halo::ai::grenade_ops::find_grenade_landing_spot(real_point3d *out_point, datum_index *out_target_handle, int32_t *out_relationship)
{
    using namespace c_actor_find_grenade_landing_spot;
    datum_index actor_index = datum;
    actor *self;
    ActorVariant *variant;
    uint8_t result = 0;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    variant = (ActorVariant *)halo::cache::globals().tag_instances[self->actor_variant_tag & halo::k_slot_mask].data;

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *target_prop = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & halo::k_slot_mask) * sizeof(prop));
        if (target_prop->enemy != 0 && target_prop->dead == 0) {
            int16_t kind = target_prop->state;
            if ((1 < kind && kind < 4) || kind == 4) {
                float min_range = *(float *)((uint8_t *)variant + 0x194);
                float max_range = *(float *)((uint8_t *)variant + 0x198);
                if (min_range < target_prop->distance && target_prop->distance < max_range) {
                    *out_point = target_prop->last_known_position;
                    result = 1;
                    out_point->z = out_point->z + 0.2f;
                    *out_target_handle = self->target_unit_index;
                    *out_relationship = target_prop->relationship_object_index;
                    if (self->playfight != 0) {
                        actor_choose_random_point_near(out_point, 1.5f);
                    }
                }
            }
        }
    }
    return result;
}

extern "C" uint8_t actor_find_grenade_landing_spot(datum_index actor_index, real_point3d *out_point, datum_index *out_target_handle, int32_t *out_relationship)
{
    return halo::ai::grenade_ops(actor_index).find_grenade_landing_spot(out_point, out_target_handle, out_relationship);
}

namespace c_actor_find_nearest_grenade_ally {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern ai_globals *ai_globals_ptr;

extern uint8_t actor_validate_grenade_ally_candidate(datum_index candidate_actor, uint8_t caller_type_flag);
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor);
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag);

extern double sqrt(double x);

#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

extern "C" int32_t actor_find_nearest_grenade_ally(datum_index actor_index, uint8_t widen_search);

/**
 * actor_find_nearest_grenade_ally: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_find_nearest_grenade_ally.c.txt.
 *
 * @address 0x40e540
 */
int32_t halo::ai::grenade_ops::find_nearest_grenade_ally(uint8_t widen_search)
{
    using namespace c_actor_find_nearest_grenade_ally;
    datum_index actor_index = datum;
    uint8_t *self = ACTOR(actor_index);
    int32_t seen = 0;
    int32_t limit = (widen_search != 0) + 1;
    datum_index best = k_datum_index_none;
    float best_distance = 3.4028235e+38f;
    datum_index prop_index;

    for (prop_index = ((struct actor *)self)->first_prop; prop_index != k_datum_index_none;) {
        uint8_t *p = PROP(prop_index);
        datum_index current = prop_index;

        prop_index = ((struct prop *)p)->next_in_actor;
        if (p[0x60] || p[0x127] || ((struct prop *)p)->owner_actor_index == k_datum_index_none) {
            continue;
        }
        if (widen_search && !(((struct prop *)p)->state >= 2 && ((struct prop *)p)->state <= 3)) {
            continue;
        }
        if (!actor_validate_grenade_ally_candidate(((struct prop *)p)->owner_actor_index, widen_search)) {
            continue;
        }
        seen++;
        if (((struct prop *)p)->distance < best_distance) {
            best = current;
            best_distance = ((struct prop *)p)->distance;
        }
    }
    if (seen < (int16_t)limit && ((struct actor *)self)->encounter_index != k_datum_index_none) {
        datum_index cursor[3];
        datum_index candidate;

        ai_reference_actor_iterator_init_cursor(*(int32_t *)&((struct actor *)self)->encounter_index, cursor);
        candidate = cursor[2];
        while (ai_globals_ptr->actors_valid && candidate != k_datum_index_none) {
            uint8_t *other = ACTOR(candidate);
            datum_index unit = ((struct actor *)other)->unit_index;
            datum_index current = candidate;
            datum_index prop;

            candidate = ((struct actor *)other)->next_in_encounter;
            if (unit == k_datum_index_none || !actor_validate_grenade_ally_candidate(current, widen_search)) {
                continue;
            }
            prop = actor_find_prop_for_object(unit, actor_index);
            if (prop == k_datum_index_none) {
                prop = actor_find_or_create_shared_prop(unit, actor_index, 1, 0);
                if (prop == k_datum_index_none) {
                    continue;
                }
            }
            {
                float dx = ((struct actor *)other)->body_position.x - ((struct actor *)self)->body_position.x;
                float dy = ((struct actor *)other)->body_position.y - ((struct actor *)self)->body_position.y;
                float dz = ((struct actor *)other)->body_position.z - ((struct actor *)self)->body_position.z;
                float distance = (float)sqrt(dz * dz + dx * dx + dy * dy);

                seen++;
                if (distance < best_distance) {
                    best_distance = distance;
                    best = prop;
                }
            }
            if (!(seen < (int16_t)limit)) {
                break;
            }
        }
    }
    ((struct actor *)self)->nearby_friend_prop_index = best;
    return seen;
}

extern "C" int32_t actor_find_nearest_grenade_ally(datum_index actor_index, uint8_t widen_search)
{
    return halo::ai::grenade_ops(actor_index).find_nearest_grenade_ally(widen_search);
}

#undef ACTOR
#undef PROP

namespace c_actor_gather_nearby_grenade_targets {
extern "C" {
extern data_array *actor_data;
extern data_array *encounter_data;
extern data_array *prop_data;
extern data_array *object_data;
extern ai_globals *ai_globals_ptr;

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
extern void actor_grenade_avoidance_entry_init(ai_grenade_avoidance_entry *entry,
                                                datum_index object_index,
                                                datum_index prop_index);
}
}

extern "C" int16_t actor_gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count, ai_grenade_avoidance_entry *out_entries);

/**
 * actor_gather_nearby_grenade_targets: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_gather_nearby_grenade_targets.c.txt.
 *
 * @address 0x42afc0
 */
int16_t halo::ai::grenade_ops::gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count, ai_grenade_avoidance_entry *out_entries)
{
    using namespace c_actor_gather_nearby_grenade_targets;
    actor *self;
    actor *other;
    datum_index cursor;
    prop *p;
    datum_index prop_cursor;
    datum_index next_prop;
    object *tracked_object;
    actor *prop_owner_actor;
    datum_index prop_index_out;
    int16_t count;

    self = &((actor *)actor_data->data)[source_actor_index & halo::k_slot_mask];
    count = 0;
    cursor = self->encounter_index;

    if (cursor != (datum_index)k_datum_index_none) {
        if (ai_globals_ptr->actors_valid) {
            cursor = ((encounter *)encounter_data->data)[cursor & halo::k_slot_mask].first_actor;
        }
        while (ai_globals_ptr->actors_valid && cursor != (datum_index)k_datum_index_none) {
            other = &((actor *)actor_data->data)[cursor & halo::k_slot_mask];
            if (cursor != source_actor_index && count < maximum_count &&
                other->unit_index != (datum_index)k_datum_index_none &&
                other->active_unit_index == (datum_index)k_datum_index_none) {
                prop_index_out = actor_find_prop_for_object(other->unit_index, source_actor_index);
                actor_grenade_avoidance_entry_init(&out_entries[count], other->unit_index, prop_index_out);
                count = count + 1;
            }
            cursor = other->next_in_encounter;
        }
    }

    prop_cursor = self->first_prop;
    while (prop_cursor != (datum_index)k_datum_index_none) {
        p = &((prop *)prop_data->data)[prop_cursor & halo::k_slot_mask];
        next_prop = p->next_in_actor;

        if (p->enemy == 0 && p->dead == 0 && p->state == 3 &&
            p->relationship_object_index == -1) {
            tracked_object = ((object_header *)object_data->data)[p->object_index & halo::k_slot_mask].data;
            if (tracked_object->type == _object_type_biped) {
                int excluded = 0;
                if (self->encounter_index != (datum_index)k_datum_index_none &&
                    p->owner_actor_index != (datum_index)k_datum_index_none) {
                    prop_owner_actor = &((actor *)actor_data->data)[p->owner_actor_index & halo::k_slot_mask];
                    excluded = (prop_owner_actor->encounter_index == self->encounter_index);
                }
                if (!excluded && count < maximum_count) {
                    actor_grenade_avoidance_entry_init(&out_entries[count], p->object_index, prop_cursor);
                    count = count + 1;
                }
            }
        }
        prop_cursor = next_prop;
    }

    return count;
}

extern "C" int16_t actor_gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count, ai_grenade_avoidance_entry *out_entries)
{
    return halo::ai::grenade_ops::gather_nearby_grenade_targets(source_actor_index, maximum_count, out_entries);
}

namespace c_actor_get_grenade_launch_velocity {
extern "C" {
extern Globals *global_globals;

extern uint8_t projectile_get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag,
    real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override,
    uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed,
    real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line);
}
}

extern "C" uint8_t actor_get_grenade_launch_velocity(int16_t grenade_type, real_vector3d *direction, void *origin, float range, real_point3d *point, int32_t max_time, float *speed, void *out_time_or_fraction, real_vector3d *out_velocity, float *out_gravity);

/**
 * actor_get_grenade_launch_velocity: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_grenade_launch_velocity.c.txt.
 *
 * @address 0x410980
 */
uint8_t halo::ai::grenade_ops::get_grenade_launch_velocity(int16_t grenade_type, real_vector3d *direction, void *origin, float range, real_point3d *point, int32_t max_time, float *speed, void *out_time_or_fraction, real_vector3d *out_velocity, float *out_gravity)
{
    using namespace c_actor_get_grenade_launch_velocity;
    uint8_t *entry;
    uint32_t projectile_tag;
    void *projectile_definition;
    float scale;
    uint8_t used_straight_line;

    entry = (uint8_t *)global_globals->grenades.pointer + (int32_t)grenade_type * 0x44;
    if (entry == (uint8_t *)0) {
        return 0;
    }
    projectile_tag = *(uint32_t *)(entry + 0x40);
    if (projectile_tag == halo::k_dword_none) {
        return 0;
    }

    projectile_definition = halo::cache::globals().tag_instances[projectile_tag & halo::k_slot_mask].data;
    if (projectile_definition == (void *)0) {
        return 0;
    }

    used_straight_line = 0;
    if (projectile_get_aiming_vector(point, &range, (Projectile *)projectile_definition,
            (real_point3d *)origin, 0, (real *)(uintptr_t)max_time, 0, 0, direction, speed,
            (real *)out_time_or_fraction, 0, &used_straight_line) == 0) {
        return 0;
    }

    if (out_velocity != (real_vector3d *)0) {
        scale = *speed;
        out_velocity->i = scale * direction->i;
        out_velocity->j = scale * direction->j;
        out_velocity->k = scale * direction->k;
    }
    if (out_gravity != (float *)0) {

        if (used_straight_line) {
            *out_gravity = 0.0f;
        } else {
            *out_gravity = -(halo::physics::globals().gravity *
                             *(float *)((uint8_t *)projectile_definition + 0x1cc));
        }
    }
    return 1;
}

extern "C" uint8_t actor_get_grenade_launch_velocity(int16_t grenade_type, real_vector3d *direction, void *origin, float range, real_point3d *point, int32_t max_time, float *speed, void *out_time_or_fraction, real_vector3d *out_velocity, float *out_gravity)
{
    return halo::ai::grenade_ops::get_grenade_launch_velocity(grenade_type, direction, origin, range, point, max_time, speed, out_time_or_fraction, out_velocity, out_gravity);
}

namespace c_actor_grenade_avoidance_entry_init {
extern "C" {
extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out);
}
}

extern "C" void actor_grenade_avoidance_entry_init(ai_grenade_avoidance_entry *entry, datum_index object_index, datum_index prop_index);

/**
 * actor_grenade_avoidance_entry_init: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_grenade_avoidance_entry_init.c.txt.
 *
 * @address 0x42af50
 */
void halo::ai::grenade_ops::avoidance_entry_init(ai_grenade_avoidance_entry *entry, datum_index object_index, datum_index prop_index)
{
    using namespace c_actor_grenade_avoidance_entry_init;
    float offset;
    float deadline;

    unit_get_crouch_height_offset(&entry->target_position, object_index, &offset, &deadline);
    entry->already_clear = (offset == 0.0f);
    entry->unknown_10 = 0;
    entry->unknown_14 = 0;
    entry->crouch_offset = offset;
    entry->avoid_until = deadline + 0.15f;
    entry->prop_index = prop_index;
    entry->object_index = object_index;
}

extern "C" void actor_grenade_avoidance_entry_init(ai_grenade_avoidance_entry *entry, datum_index object_index, datum_index prop_index)
{
    halo::ai::grenade_ops::avoidance_entry_init(entry, object_index, prop_index);
}

namespace c_actor_grenade_behavior_kind_allowed {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" uint8_t actor_grenade_behavior_kind_allowed(datum_index actor_index, int16_t kind);

/**
 * actor_grenade_behavior_kind_allowed: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_grenade_behavior_kind_allowed.c.txt.
 *
 * @address 0x40f670
 */
uint8_t halo::ai::grenade_ops::behavior_kind_allowed(int16_t kind)
{
    using namespace c_actor_grenade_behavior_kind_allowed;
    datum_index actor_index = datum;
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    if (kind == 1) {
        return self->firing_target_type == 1 && 7 < self->target_combat_status;
    }
    if (kind == 2) {
        return self->firing_target_type == 0 && 4 < self->target_combat_status &&
               self->target_alive != 0 && 0x4a < self->ticks_since_engaged;
    }
    if (kind == 3) {
        return self->firing_target_type == 1 && 7 < self->target_combat_status && self->vehicle_gunner != 0;
    }
    return 0;
}

extern "C" uint8_t actor_grenade_behavior_kind_allowed(datum_index actor_index, int16_t kind)
{
    return halo::ai::grenade_ops(actor_index).behavior_kind_allowed(kind);
}

namespace c_actor_grenade_parabolic_path_clear {
extern "C" {
extern int16_t actor_gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count,
                                                     ai_grenade_avoidance_entry *out_entries);
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch);
}
}

extern "C" uint8_t actor_grenade_parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index, real_point3d *start_position, real total_time, real vertical_acceleration, datum_index exclude_object_index, uint8_t wide_mask);

/**
 * actor_grenade_parabolic_path_clear: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_grenade_parabolic_path_clear.c.txt.
 *
 * @address 0x42b5d0
 */
uint8_t halo::ai::grenade_ops::parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index, real_point3d *start_position, real total_time, real vertical_acceleration, datum_index exclude_object_index, uint8_t wide_mask)
{
    using namespace c_actor_grenade_parabolic_path_clear;
    ai_grenade_avoidance_entry entries[32];
    int16_t nearby_count;
    uint32_t collision_mask;
    real_point3d position;
    real_point3d next_position;
    real_vector3d segment_delta;
    real t;
    real previous_t;
    uint8_t clear;
    uint8_t scratch[0x50];
    int16_t i;

    nearby_count = actor_gather_nearby_grenade_targets(source_actor_index, 32, entries);
    collision_mask = wide_mask ? 0xc0b3u : 0xc2b3u;

    position = *start_position;
    previous_t = 0.0f;
    t = (6.0f <= total_time) ? 6.0f : total_time;
    clear = 0;

    do {
        real dt = t - previous_t;
        next_position.x = initial_velocity->i * dt + position.x;
        next_position.y = initial_velocity->j * dt + position.y;
        next_position.z = position.z + initial_velocity->k * dt + dt * dt * vertical_acceleration * 0.5f;

        segment_delta.i = next_position.x - position.x;
        segment_delta.j = next_position.y - position.y;
        segment_delta.k = next_position.z - position.z;

        clear = (halo::physics::collision_test_movement_segment(collision_mask, &position, &segment_delta, exclude_object_index, (collision_result *)scratch) == 0);
        if (!clear) {
            return 0;
        }

        for (i = 0; i < nearby_count; i++) {
            real_vector3d target_offset;
            int hit;
            target_offset.i = 0.0f;
            target_offset.j = 0.0f;
            target_offset.k = entries[i].crouch_offset;
            hit = halo::math::segment3d_within_radius_of_segment(&position, &entries[i].target_position, &segment_delta,
                                                       &target_offset, entries[i].avoid_until);
            if (hit) {
                return 0;
            }
        }

        position = next_position;
        previous_t = t;
        t = t + 6.0f;
        if (total_time < t) {
            t = total_time;
        }
    } while (previous_t < total_time);

    return clear;
}

extern "C" uint8_t actor_grenade_parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index, real_point3d *start_position, real total_time, real vertical_acceleration, datum_index exclude_object_index, uint8_t wide_mask)
{
    return halo::ai::grenade_ops::parabolic_path_clear(initial_velocity, source_actor_index, start_position, total_time, vertical_acceleration, exclude_object_index, wide_mask);
}

namespace c_actor_grenade_trace_from_source {
extern "C" {
extern data_array *actor_data;

extern void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point,
    uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator);

}
}

extern "C" int32_t actor_grenade_trace_from_source(uint32_t actor_index, real_point3d *target_point);

/**
 * actor_grenade_trace_from_source: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_grenade_trace_from_source.c.txt.
 *
 * @address 0x4029e0
 */
int32_t halo::ai::grenade_ops::trace_from_source(real_point3d *target_point)
{
    using namespace c_actor_grenade_trace_from_source;
    uint32_t actor_index = datum;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    real_point3d source;
    real_vector3d delta;
    uint8_t trace_result[80];

    if (a->movement_completed == 0) {
        if (a->movement_action_complete == 0) {
            return 0;
        }
        unit_add_marker_relative_offset(a->unit_index, 1, (float *)((uint8_t *)a + 0x4ac), 0, 0, &source);
    } else {
        source.x = a->aim_origin.x;
        source.y = a->aim_origin.y;
        source.z = a->aim_origin.z;
    }

    delta.i = target_point->x - source.x;
    delta.j = target_point->y - source.y;
    delta.k = target_point->z - source.z;
    halo::physics::collision_test_movement_segment(0x33, &source, &delta, halo::k_dword_none, (collision_result *)trace_result);
    return 1;
}

extern "C" int32_t actor_grenade_trace_from_source(uint32_t actor_index, real_point3d *target_point)
{
    return halo::ai::grenade_ops(actor_index).trace_from_source(target_point);
}

namespace c_actor_grenade_trajectory_blocked {
extern "C" {
extern int16_t actor_gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count,
                                                     ai_grenade_avoidance_entry *out_entries);
}
}

extern "C" uint8_t actor_grenade_trajectory_blocked(real_vector3d *trajectory_direction, datum_index source_actor_index, datum_index exclude_object_index, real_point3d *landing_position, int32_t *out_blocking_prop);

/**
 * actor_grenade_trajectory_blocked: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_grenade_trajectory_blocked.c.txt.
 *
 * @address 0x42b190
 */
uint8_t halo::ai::grenade_ops::trajectory_blocked(real_vector3d *trajectory_direction, datum_index source_actor_index, datum_index exclude_object_index, real_point3d *landing_position, int32_t *out_blocking_prop)
{
    using namespace c_actor_grenade_trajectory_blocked;
    ai_grenade_avoidance_entry entries[32];
    int16_t count;
    int16_t i;
    uint8_t clear;
    int32_t blocking_prop;
    uint8_t hit;

    clear = 1;
    blocking_prop = -1;
    count = actor_gather_nearby_grenade_targets(source_actor_index, 32, entries);

    for (i = 0; i < count; i++) {
        if (entries[i].object_index == exclude_object_index) {
            continue;
        }
        if (entries[i].already_clear) {
            hit = halo::math::ray_intersects_sphere_test(entries[i].target_position, *landing_position,
                                              *trajectory_direction, entries[i].avoid_until);
        } else {
            real_vector3d target_offset;
            target_offset.i = 0.0f;
            target_offset.j = 0.0f;
            target_offset.k = entries[i].crouch_offset;
            hit = (uint8_t)halo::math::segment3d_within_radius_of_segment(landing_position, &entries[i].target_position,
                                                                trajectory_direction, &target_offset,
                                                                entries[i].avoid_until);
        }
        if (hit) {
            blocking_prop = entries[i].prop_index;
            clear = 0;
            break;
        }
    }

    if (out_blocking_prop != 0) {
        *out_blocking_prop = blocking_prop;
    }
    return clear;
}

extern "C" uint8_t actor_grenade_trajectory_blocked(real_vector3d *trajectory_direction, datum_index source_actor_index, datum_index exclude_object_index, real_point3d *landing_position, int32_t *out_blocking_prop)
{
    return halo::ai::grenade_ops::trajectory_blocked(trajectory_direction, source_actor_index, exclude_object_index, landing_position, out_blocking_prop);
}

