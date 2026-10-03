#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

namespace halo::ai {

/**
 * Actor AI behaviour: order code is grenade throw.
 *
 * @address 0x404340
 */
int32_t ActorOps::order_code_is_grenade_throw(int16_t order_code)
{
    if (order_code > _actor_order_code_grenade_first - 1 && order_code < _actor_order_code_grenade_last + 1) {
        return 1;
    }
    return 0;
}

namespace actor_recompute_grenade_eligibility_local {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern float k_random_scale_65536;
extern float ticks_per_second;
}
}

/**
 * REWRITTEN 2026-09-28 from objdump 0x42f260..0x42f36b (the draft's formula was a placeholder). The recheck time
 * is a random lerp over one of two ranges in the actor definition (actor +0x58): +0x400..+0x404 when eligible,
 * else +0x3f8..+0x3fc, in seconds, times 30, plus the unit's +0x3fa ticks when its
 *
 * @address 0x42f260
 */
void ActorView::recompute_grenade_eligibility()
{
    using namespace actor_recompute_grenade_eligibility_local;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&self->actor_definition_tag & 0xffff].data;
    uint8_t eligible = (uint8_t)(self->awareness_level == 3 && self->combat_status > self->minimum_combat_status);
    int16_t base_ticks = 0;
    float minimum;
    float maximum;
    float fraction;

    if (self->unit_index != (datum_index)k_datum_index_none) {
        uint8_t *unit_obj = (uint8_t *)((object_header *)object_data->data)[self->unit_index & 0xffff].data;

        if (*(int16_t *)(unit_obj + 0x388) > 0) {
            base_ticks = *(int16_t *)(unit_obj + 0x3fa);
        }
    }

    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    fraction = (float)(int32_t)(halo::math::globals().random_seed_global >> 16) * k_random_scale_65536;
    if (eligible) {
        minimum = *(float *)(definition + 0x400);
        maximum = *(float *)(definition + 0x404);
    } else {
        minimum = *(float *)(definition + 0x3f8);
        maximum = *(float *)(definition + 0x3fc);
    }

    self->grenade_eligible = eligible;
    self->grenade_recheck_ticks =
        (int16_t)(long long)((fraction * (maximum - minimum) + minimum) * ticks_per_second + (float)base_ticks);
}

namespace actor_request_path_with_grenade_arc_local {
extern "C" {
extern data_array *actor_data;
extern int16_t actor_select_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok);
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok);
extern float actor_compute_accuracy_scale(datum_index actor_index);
}
}

/**
 * Actor AI behaviour: request path with grenade arc.
 *
 * @address 0x408300
 */
uint8_t ActorView::request_path_with_grenade_arc()
{
    using namespace actor_request_path_with_grenade_arc_local;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    static actor_firing_position_query query;
    static path_find_context path_context;
    actor_firing_position_candidate candidate;
    uint32_t previous_owner = 0xffffffff;
    uint8_t path_ok = 0;
    int16_t selected;

    if (actor[0x4c] == 0 || actor[0x160] != 0 || actor[0x9d] != 0) {
        return 0;
    }
    memset(&query, 0, sizeof(query));
    memset(&candidate, 0, sizeof(candidate));
    query.goal_kind = 3;
    if (*(int16_t *)(actor + 0xa4) == 1) {
        query.have_explicit_target = 1;
        query.explicit_target_position = *(real_point3d *)(actor + 0xb0);
        query.explicit_target_object = *(uint32_t *)(actor + 0xac);
        query.explicit_target_unknown_34 = *(int16_t *)(actor + 0xa8);
    } else {
        query.use_last_seen_position = actor[0xa0];
    }
    selected = actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context, &path_ok);
    actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    if (selected != -1) {
        if (*(int16_t *)(actor + 0xa4) == 0) {
            if (candidate.request_result != 0 && candidate.request_result != 1) {
                actor[0xa0] = 1;
            }
        } else if (candidate.request_result == 0 && actor_compute_accuracy_scale(actor_index) > candidate.distance_from_actor) {
            actor[0xbc] = 1;
        }
    }
    if (actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok) == -1) {
        actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
        actor[0x9e] = 1;
    }
    return 0;
}

namespace actor_schedule_grenade_throw_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
extern void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out);
extern int32_t fistp_round(float x);
}
}

/**
 * REWRITTEN from objdump 0x402f80..0x403172 (misnamed: it schedules a look at whatever last hurt the actor).
 * ECX: actor. With a damage source (+0x1dc, object +0x1e0) the target is the actor's prop for it (kind 1) or the
 * object's eye (kind 3). An awake actor (+0x6a > 1) without a higher pending look (+
 *
 * @address 0x402f80
 */
void ActorView::schedule_grenade_throw()
{
    using namespace actor_schedule_grenade_throw_local;
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag;
    datum_index source;
    uint8_t request[0x10];
    datum_index prop;
    float delay;
    int32_t ticks;

    if (((actor *)a)->conversation_index == k_datum_index_none) {
        return;
    }
    source = ((actor *)a)->conversation_participant;
    if (source == k_datum_index_none) {
        return;
    }
    memset(request, 0, sizeof(request));
    prop = actor_find_prop_for_object(source, actor_index);
    if (prop != k_datum_index_none) {
        *(int16_t *)request = 1;
        *(datum_index *)(request + 0x4) = prop;
    } else {
        *(int16_t *)request = 3;
        unit_get_primary_eye_marker_position(source, (real_point3d *)(request + 0x4));
    }
    actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((actor *)a)->actor_definition_tag & 0xffff].data;
    if (!(((actor *)a)->awareness_level > 1) || ((actor *)a)->vocalization_line > 8) {
        return;
    }
    if (((actor *)a)->mode == 0xb && !a[0x9f]) {
        return;
    }
    if (*(int16_t *)request == 1 && halo::memory::datum_get(*(datum_index *)(request + 0x4), prop_data) == 0) {
        return;
    }
    delay = (((actor *)a)->awareness_level < 3 || ((struct actor *)a)->combat_status == 0) ? 2.4f : 1.2f;
    if (*(float *)(actor_tag + 0xd4) != 0.0f || *(float *)(actor_tag + 0xd8) != 0.0f) {
        float lo = *(float *)(actor_tag + 0xd4) > 0.5f ? *(float *)(actor_tag + 0xd4) : 0.5f;
        float hi = *(float *)(actor_tag + 0xd8) > 2.0f ? 2.0f : *(float *)(actor_tag + 0xd8);

        delay = halo::math::random_real_range(lo, hi) * delay;
    }
    ticks = fistp_round(delay * 30.0f);
    if (ticks > 0x7fff) {
        ticks = 0x7fff;
    }
    ((actor *)a)->vocalization_state = (int16_t)ticks;
    ((actor *)a)->vocalization_line = 8;
    ((actor *)a)->vocalization_variant = 5;
    memcpy(a + 0x54c, request, 0x10);
}

namespace actor_should_throw_grenade_local {
extern "C" {
extern data_array *actor_data;
extern game_time_globals *game_time;
}
}

/**
 * The result is produced in AL only (the Ghidra listing returns CONCAT31(<garbage>, uVar12)), so the return type
 * is a byte, not the int Ghidra recovered.
 *
 * @address 0x40b840
 */
uint8_t ActorView::should_throw_grenade(char force)
{
    using namespace actor_should_throw_grenade_local;
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_def = (Actor *)halo::cache::globals().tag_instances[a->actor_definition_tag & 0xffff].data;
    uint8_t eligible = 1;

    if (force == 0 && a->playfight == 0) {
        if (actor_def->hide_target_not_visible_time > 0.0f) {
            if (a->combat_status < 7) {
                if (a->target_last_seen_time != (datum_index)k_datum_index_none) {
                    int16_t delay = (int16_t)(actor_def->hide_target_not_visible_time * 30.0f);
                    if (game_time->game_time < delay + (int32_t)a->target_last_seen_time) {
                        eligible = 0;
                    }
                }
            } else {
                eligible = 0;
            }
        }
        if (!(actor_def->cover_damage_threshold < 0.0f) && !(actor_def->cover_damage_threshold == 0.0f)) {
            if (a->recent_body_damage < actor_def->cover_damage_threshold) {
                eligible = 0;
            }
        }
    }

    if (a->berserking != 0) {
        eligible = 0;
    }
    if (a->order_committed != 0) {
        return 0;
    }
    return eligible;
}

namespace actor_solve_grenade_lob_local {
extern "C" {
extern data_array *actor_data;
extern Globals *global_globals;
extern double sqrt(double x);
extern uint8_t projectile_get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag,
    real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override,
    uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed,
    real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line);
extern uint8_t actor_grenade_parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index,
    real_point3d *start_position, real total_time, real vertical_acceleration, datum_index exclude_object_index,
    uint8_t wide_mask);
}
}

/**
 * Solves a grenade lob at the given point and commits it only when the resulting throw is inside a 30 degree
 * cone of the actor facing. The committed values are the throw direction and the solved speed, which
 * actor_compute_grenade_throw_vector then turns into the final aim vector.
 *
 * @address 0x410780
 */
uint32_t ActorView::solve_grenade_lob(real_point3d *point)
{
    using namespace actor_solve_grenade_lob_local;
    actor *self;
    ActorVariant *variant;
    uint8_t *entry;
    void *projectile_definition;
    uint32_t projectile_tag;
    real_vector3d direction;
    real_vector3d velocity;
    float speed;
    float arc;
    float gravity;
    float length;
    uint8_t flat;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)halo::cache::globals().tag_instances[self->actor_variant_tag & 0xffff].data;

    entry = (uint8_t *)global_globals->grenades.pointer + (int32_t)variant->grenade_type * 0x44;
    projectile_definition = (void *)0;
    if (entry != (uint8_t *)0) {
        projectile_tag = *(uint32_t *)(entry + 0x40);
        if (projectile_tag != 0xffffffff) {
            projectile_definition = halo::cache::globals().tag_instances[projectile_tag & 0xffff].data;
        }
    }

    if (projectile_get_aiming_vector(&self->grenade_impact_point, 0, (Projectile *)projectile_definition,
                     point, 0, 0, &self->grenade_unknown_6c8, self->grenade_high_arc[0], &direction,
                     &speed, &arc, 0, &flat) == 0) {
        return 0;
    }

    length = (float)sqrt((double)(direction.i * direction.i + direction.j * direction.j));
    if (length < 0.0001f && length > -0.0001f) {
        return 0;
    }
    if (length <= 0.0f) {
        return 0;
    }

    if (direction.i * (1.0f / length) * self->facing.i +
        (1.0f / length) * direction.j * self->facing.j <= 0.8660254f) {
        return 0;
    }

    velocity.i = direction.i * speed;
    velocity.j = direction.j * speed;
    velocity.k = direction.k * speed;
    gravity = (flat != 0) ? 0.0f
                          : -(halo::physics::globals().gravity *
                              *(float *)((uint8_t *)projectile_definition + 0x1cc));

    if (actor_grenade_parabolic_path_clear(&velocity, actor_index, point, arc, gravity,
                                           *(datum_index *)self->grenade_exclude_object_index,
                                           (uint8_t)(self->active_unit_index != (datum_index)0xffffffff)) == 0) {
        return 0;
    }

    self->grenade_unknown_6bc = direction.i;
    self->grenade_unknown_6c0 = direction.j;
    self->grenade_unknown_6c4 = direction.k;
    self->grenade_unknown_6c8 = speed;
    return 1;
}

namespace actor_try_grenade_evasion_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
extern actor_mode_definition actor_mode_definitions[16];
extern uint8_t actor_should_throw_grenade(uint32_t actor_index, char force);
extern uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3);
extern uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base, uint16_t order_code,
                                         datum_index actor_index);
}
}

/**
 * Actor AI behaviour: try grenade evasion.
 *
 * @address 0x40c530
 */
uint8_t ActorView::try_grenade_evasion(uint8_t allow_pain_reaction, uint8_t use_alt_base)
{
    using namespace actor_try_grenade_evasion_local;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    int16_t grade;
    int32_t now;

    if (!act[0x4c] || !(*(float *)(act + 0x1bc) <= ((Actor *)actor_tag)->hide_shield_fraction)) {
        return 0;
    }
    grade = actor_mode_definitions[((actor *)act)->mode].combat_grade;
    if (act[0x378] || (grade != 4 && grade != 3) || ((struct actor *)act)->combat_status < 2) {
        return 0;
    }
    now = game_time->game_time;
    if (*(int32_t *)&((struct actor *)act)->last_cover_attempt_time != -1 && now < *(int32_t *)&((struct actor *)act)->last_cover_attempt_time + 30) {
        return 0;
    }
    *(int32_t *)&((struct actor *)act)->last_cover_attempt_time = now;
    if (!actor_should_throw_grenade(actor_index, 0)) {
        return 0;
    }
    if (actor_handle_death(actor_index, 1, 0)) {
        return 1;
    }
    if (allow_pain_reaction &&
        actor_check_pain_reaction(((actor *)act)->target_unit_index, use_alt_base, 4, actor_index)) {
        return 1;
    }
    return 0;
}

#undef ACTOR
#undef TAG_DATA
#undef PROP

namespace actor_update_grenade_and_morale_reactions_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;
extern uint8_t actor_should_throw_grenade(uint32_t actor_index, char force);
extern uint8_t actor_consider_grenade_throw(datum_index actor_index);
extern uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3);
extern uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base,
    uint16_t order_code, datum_index actor_index);
extern uint8_t actor_evaluate_grenade_target_position(datum_index actor_index);
extern datum_index actor_get_target_prop_object_index(datum_index actor_index);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
}
}

/**
 * Actor AI behaviour: update grenade and morale reactions.
 *
 * @address 0x40b920
 */
char ActorView::update_grenade_and_morale_reactions()
{
    using namespace actor_update_grenade_and_morale_reactions_local;
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *variant = TAG_DATA(((actor *)act)->actor_variant_tag);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    int32_t now = game_time->game_time;
    char result = 0;
    float threshold;
    uint8_t may_evade;
    uint8_t may_target;

    if (((struct actor *)act)->retreat_timer > 0 && ((actor *)act)->active_unit_index == k_datum_index_none) {
        uint8_t *threat = (uint8_t *)prop_data->data + (((struct actor *)act)->retreat_prop_index & 0xffff) * 0x138;

        if (threat[0xa4] != 0 && (((struct prop *)threat)->obstruction == 0 || ((struct prop *)threat)->obstruction == 1) &&
            (*(int32_t *)&((struct actor *)act)->last_evasion_time == -1 || *(int32_t *)&((struct actor *)act)->last_evasion_time + 0x1e <= now)) {
            *(int32_t *)&((struct actor *)act)->last_evasion_time = now;
            if (actor_should_throw_grenade(actor_index, 1)) {
                if (actor_handle_death(actor_index, 0, 1)) {
                    return 1;
                }
                if ((*(uint32_t *)actor_tag & 0x400000) != 0 &&
                    actor_check_pain_reaction(((struct actor *)act)->retreat_prop_index, 0, 5, actor_index)) {
                    return 1;
                }
            }
        }
    }

    if (act[0x374] != 0 && act[0x378] == 0) {
        threshold = ((Actor *)actor_tag)->defending_evasion_threshold;
    } else {
        threshold = ((Actor *)actor_tag)->attacking_evasion_threshold;
    }
    if (act[0x1ca] != 0 && ((Actor *)actor_tag)->evasion_seek_cover_chance > 0.0f && threshold > 1.1f) {
        threshold = 1.1f;
    }
    if (!(threshold <= *(float *)(act + 0x354))) {
        return 0;
    }
    if (act[0x504] == 0) {
        halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    }
    if (*(int16_t *)&((ActorVariant *)variant)->grenade_stimulus == 2 && actor_consider_grenade_throw(actor_index)) {
        *(float *)(act + 0x354) = 0.0f;
        result = 1;
    }
    may_evade = 1;
    may_target = 1;
    if (act[0x358] != 0 && (*(uint32_t *)actor_tag & 0x20) != 0) {
        datum_index target = ((actor *)act)->target_unit_index;

        may_evade = 0;
        if (target != k_datum_index_none) {
            uint8_t *target_prop = (uint8_t *)prop_data->data + (target & 0xffff) * 0x138;

            if ((int8_t)target_prop[0x122] <= 2 && (int8_t)target_prop[0x121] <= 1) {
                may_evade = 1;
            }
        }
    }
    if (((actor *)act)->mode == 10 && (*(int16_t *)(act + 0xa0) == 2 || *(int16_t *)(act + 0xa0) == 3)) {
        may_target = 0;
    }
    if (result) {
        return result;
    }
    if (may_evade && (*(int32_t *)&((struct actor *)act)->last_evasion_time == -1 || *(int32_t *)&((struct actor *)act)->last_evasion_time + 0x1e <= now)) {
        *(int32_t *)&((struct actor *)act)->last_evasion_time = now;
        if (actor_should_throw_grenade(actor_index, 0) && halo::math::random_real() <= ((Actor *)actor_tag)->evasion_seek_cover_chance &&
            actor_handle_death(actor_index, 0, 1)) {
            ai_communication_broadcast(0x18, ((actor *)act)->unit_index, actor_get_target_prop_object_index(actor_index),
                                       -1, -1, -1, 0);
            *(float *)(act + 0x354) = 0.0f;
            return 1;
        }
    }
    if (may_target && *(int16_t *)(act + 0x368) == 0 && actor_evaluate_grenade_target_position(actor_index)) {
        *(float *)(act + 0x354) = 0.0f;
        *(int16_t *)(act + 0x368) = (int16_t)(int32_t)(((Actor *)actor_tag)->evasion_delay_time * 30.0f);
        act[0x3bb] = 1;
        result = 1;
    }
    return result;
}

#undef TAG_DATA

namespace actor_update_grenade_eligibility_state_local {
extern "C" {
extern data_array *actor_data;
extern ai_globals *ai_globals_ptr;
extern void actor_recompute_grenade_eligibility(datum_index actor_index);
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value);
extern int32_t unit_commit_speech(uint32_t unit_index, const void *source, int16_t mode);
extern void ai_communication_target_result_reset(void *record);
}
}

/**
 * If the actor's awareness is above 1 and communication is valid, refreshes its cached grenade-eligibility
 * whenever that cache is stale or has never been rolled, and counts down its recheck timer; when the timer
 * reaches zero, attempts to queue a communication event reflecting the (possibly just-recomp
 *
 * @address 0x42f370
 */
void ActorView::update_grenade_eligibility_state()
{
    using namespace actor_update_grenade_eligibility_state_local;
    actor *self;
    uint8_t eligible;
    uint32_t buffer[12];
    int16_t out_a;
    int32_t out_b;
    int16_t result;

    self = &((actor *)actor_data->data)[actor_index & 0xffff];
    if (self->awareness_level <= 1 || !ai_globals_ptr->dialogue_triggers_enabled) {
        return;
    }

    eligible = (self->awareness_level == 3 && self->minimum_combat_status < self->combat_status);

    if (self->grenade_recheck_ticks == 0 || self->grenade_eligible != eligible) {
        actor_recompute_grenade_eligibility(actor_index);
    }

    if (0 < self->grenade_recheck_ticks) {
        self->grenade_recheck_ticks = self->grenade_recheck_ticks - 1;
        if (self->grenade_recheck_ticks == 0) {
            out_a = (int16_t)(eligible != 0);
            out_b = -1;
            result = (int16_t)unit_animation_change_priority_check(self->unit_index, 1, 1, 0, 0, &out_a, &out_b);
            if (0 < result) {
                int i;
                for (i = 0; i < 12; i++) {
                    buffer[i] = 0;
                }
                *(int16_t *)((uint8_t *)buffer + 2) = (int16_t)out_a;
                *((uint32_t *)((uint8_t *)buffer + 4)) = out_b;
                *(int16_t *)buffer = 1;
                ai_communication_target_result_reset((uint8_t *)buffer + 0x10);
                unit_commit_speech(self->unit_index, buffer, result);
            }
        }
    }
}

namespace actor_update_grenade_throw_decision_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern game_time_globals *game_time;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
extern uint8_t actor_consider_grenade_throw(datum_index actor_index);
extern uint8_t actor_check_grenade_facing_and_commit(datum_index actor_index, uint8_t force_commit);
}
}

/**
 * Actor AI behaviour: update grenade throw decision.
 *
 * @address 0x40b770
 */
uint8_t ActorView::update_grenade_throw_decision()
{
    using namespace actor_update_grenade_throw_decision_local;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *variant = TAG_DATA(((actor *)act)->actor_variant_tag);
    int16_t mode = ((actor *)act)->mode;
    uint8_t result = 0;

    if (((actor *)act)->target_combat_status < 5 || (mode == 4 && ((struct actor *)act)->mode_data.flee.panic > 0)) {
        act[0x6a0] = 0;
        return 0;
    }
    switch (*(int16_t *)&((ActorVariant *)variant)->grenade_stimulus) {
    case 1:
        if (((struct actor *)act)->combat_status >= 5) {
            result = actor_consider_grenade_throw(actor_index);
        }
        break;
    case 2:
        if (PROP(((actor *)act)->target_unit_index)[0x14] || (mode == 4 && ((struct actor *)act)->mode_data.flee.panic == 0)) {
            result = actor_consider_grenade_throw(actor_index);
        }
        break;
    default:
        break;
    }
    if (act[0x6a0]) {
        actor_check_grenade_facing_and_commit(actor_index, 0);
    }
    return result;
}

#undef ACTOR
#undef TAG_DATA
#undef PROP

namespace actor_validate_grenade_ally_candidate_local {
extern "C" {
extern data_array *actor_data;
extern void *actor_type_procs[16];
}
}

/**
 * TYPES (folded into types/ai.h by the review pass): see actor_update_melee_combat_action.c for the same layout.
 *
 * @address 0x40e4a0
 */
uint8_t ActorOps::validate_grenade_ally_candidate(datum_index candidate_actor, uint8_t caller_type_flag)
{
    using namespace actor_validate_grenade_ally_candidate_local;
    actor *candidate;
    int16_t index;
    int16_t salt;
    actor_type_table_entry *type_entry;

    candidate = (actor *)0;
    index = (int16_t)candidate_actor;
    if (candidate_actor != (datum_index)k_datum_index_none && -1 < index && index < actor_data->maximum_count) {
        actor *slot = (actor *)((uint8_t *)actor_data->data + (uint32_t)index * actor_data->size);
        salt = (int16_t)(candidate_actor >> 16);
        if (slot->identifier != 0 && (salt == 0 || slot->identifier == salt)) {
            candidate = slot;
        }
    }

    if (candidate != (actor *)0 && 1 < candidate->combat_status && candidate->combat_status < 4 &&
        (candidate->mode == 7 || candidate->mode == 5 ||
         (caller_type_flag == 0 && candidate->mode == 8) ||
         (candidate->mode == 6 && candidate->mode_data.raw[8] == 0 && 0 < *(int16_t *)&candidate->mode_data.raw[0]))) {
        type_entry = (actor_type_table_entry *)actor_type_procs[candidate->type];
        if (type_entry->swarm != caller_type_flag) {
            return 1;
        }
    }
    return 0;
}

namespace actor_validate_grenade_impact_point_local {
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_score_blast_area_clear(datum_index actor_index, float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count);
}
}

/**
 * Actor AI behaviour: validate grenade impact point.
 *
 * @address 0x410710
 */
uint8_t ActorView::validate_grenade_impact_point(real_point3d *candidate_point)
{
    using namespace actor_validate_grenade_impact_point_local;
    actor *self;
    ActorVariant *variant;
    int16_t hostile_count;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)halo::cache::globals().tag_instances[self->actor_variant_tag & 0xffff].data;

    if (actor_score_blast_area_clear(actor_index, ((ActorVariant *)variant)->enemy_radius,
                      ((ActorVariant *)variant)->collateral_damage_radius, candidate_point, &hostile_count) != 0) {
        self->grenade_impact_point = *candidate_point;
        return 1;
    }
    return 0;
}

}
