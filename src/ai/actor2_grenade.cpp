#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/ai/actor_view.hpp"
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
static auto &k_random_scale_65536 = halo::link::ref<float>(halo::ai::vars().k_random_scale_65536);
static auto &ticks_per_second = halo::link::ref<float>(halo::ai::vars().ticks_per_second);
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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *definition = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    uint8_t eligible = (uint8_t)(self->awareness_level == 3 && self->combat_status > self->minimum_combat_status);
    int16_t base_ticks = 0;
    float minimum;
    float maximum;
    float fraction;

    if (self->unit_index != (datum_index)k_datum_index_none) {
        unit_object *unit_obj = (unit_object *)halo::ai::object_at(self->unit_index);

        if (unit_obj->unit.current_speech.priority > 0) {
            base_ticks = unit_obj->unit.speech_duration_ticks;
        }
    }

    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    fraction = (float)(int32_t)(halo::math::globals().random_seed_global >> 16) * k_random_scale_65536;
    if (eligible) {
        minimum = definition->combat_idle_speech_time[0];
        maximum = definition->combat_idle_speech_time[1];
    } else {
        minimum = definition->noncombat_idle_speech_time[0];
        maximum = definition->noncombat_idle_speech_time[1];
    }

    self->grenade_eligible = eligible;
    self->grenade_recheck_ticks =
        (int16_t)(long long)((fraction * (maximum - minimum) + minimum) * ticks_per_second + (float)base_ticks);
}

namespace actor_request_path_with_grenade_arc_local {
}

/**
 * Actor AI behaviour: request path with grenade arc.
 *
 * @address 0x408300
 */
uint8_t ActorView::request_path_with_grenade_arc()
{
    using namespace actor_request_path_with_grenade_arc_local;
    struct actor *actor = halo::ai::actor_at(actor_index);
    static actor_firing_position_query query;
    static path_find_context path_context;
    actor_firing_position_candidate candidate;
    uint32_t previous_owner = halo::k_dword_none;
    uint8_t path_ok = 0;
    int16_t selected;

    if (actor->needs_new_path == 0 || actor->order_committed != 0 || actor->mode_data.uncover.done != 0) {
        return 0;
    }
    memset(&query, 0, sizeof(query));
    memset(&candidate, 0, sizeof(candidate));
    query.goal_kind = 3;
    if (actor->mode_data.uncover.stage == 1) {
        query.have_explicit_target = 1;
        query.explicit_target_position = actor->mode_data.uncover.position;
        query.explicit_target_object = actor->mode_data.uncover.target_object;
        query.explicit_target_cluster_index = actor->mode_data.uncover.target_cluster;
    } else {
        query.use_last_seen_position = actor->mode_data.uncover.use_last_seen_position;
    }
    selected = halo::ai::actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context, &path_ok);
    actor = halo::ai::actor_at(actor_index);
    if (selected != -1) {
        if (actor->mode_data.uncover.stage == 0) {
            if (candidate.request_result != 0 && candidate.request_result != 1) {
                actor->mode_data.uncover.use_last_seen_position = 1;
            }
        } else if (candidate.request_result == 0 && halo::ai::actor_compute_accuracy_scale(actor_index) > candidate.distance_from_actor) {
            actor->mode_data.uncover.target_reached = 1;
        }
    }
    if (halo::ai::actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok) == -1) {
        actor = halo::ai::actor_at(actor_index);
        actor->mode_data.uncover.unknown_02 = 1;
    }
    return 0;
}

namespace actor_schedule_grenade_throw_local {
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
    struct actor *a = halo::ai::actor_at(actor_index);
    Actor *actor_tag;
    datum_index source;
    uint8_t request[0x10];
    datum_index prop;
    float delay;
    int32_t ticks;

    if (a->conversation_index == k_datum_index_none) {
        return;
    }
    source = a->conversation_participant;
    if (source == k_datum_index_none) {
        return;
    }
    memset(request, 0, sizeof(request));
    prop = halo::ai::actor_find_prop_for_object(source, actor_index);
    if (prop != k_datum_index_none) {
        *(int16_t *)request = 1;
        *(datum_index *)(request + 0x4) = prop;
    } else {
        *(int16_t *)request = 3;
        halo::units::unit_get_primary_eye_marker_position(source, (real_point3d *)(request + 0x4));
    }
    actor_tag = halo::ai::tag_data<Actor>(a->actor_definition_tag);
    if (!(a->awareness_level > 1) || a->vocalization_line > 8) {
        return;
    }
    if (a->mode == 0xb && !a->mode_data.obey.allow_look) {
        return;
    }
    if (*(int16_t *)request == 1 && halo::memory::datum_get(*(datum_index *)(request + 0x4), halo::ai::globals().prop_data) == 0) {
        return;
    }
    delay = (a->awareness_level < 3 || a->combat_status == 0) ? 2.4f : 1.2f;
    if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
        float lo = actor_tag->event_look_time_modifier[0] > 0.5f ? actor_tag->event_look_time_modifier[0] : 0.5f;
        float hi = actor_tag->event_look_time_modifier[1] > 2.0f ? 2.0f : actor_tag->event_look_time_modifier[1];

        delay = halo::math::random_real_range(lo, hi) * delay;
    }
    ticks = halo::x87::fistp_round(delay * 30.0f);
    if (ticks > INT16_MAX) {
        ticks = INT16_MAX;
    }
    a->vocalization_state = (int16_t)ticks;
    a->vocalization_line = 8;
    a->vocalization_variant = 5;
    memcpy((uint8_t *)a + 0x54c, request, 0x10);
}

namespace actor_should_throw_grenade_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_def = halo::ai::tag_data<Actor>(a->actor_definition_tag);
    uint8_t eligible = 1;

    if (force == 0 && a->playfight == 0) {
        if (actor_def->hide_target_not_visible_time > 0.0f) {
            if (a->combat_status < 7) {
                if (a->target_last_seen_time != (datum_index)k_datum_index_none) {
                    int16_t delay = (int16_t)(actor_def->hide_target_not_visible_time * 30.0f);
                    if (halo::game::globals().game_time->game_time < delay + (int32_t)a->target_last_seen_time) {
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
static auto &global_globals = halo::link::ref<::Globals *>(halo::game::vars().global_globals);
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

    self = halo::ai::actor_at(actor_index);
    variant = halo::ai::tag_data<ActorVariant>(self->actor_variant_tag);

    entry = (uint8_t *)global_globals->grenades.pointer + (int32_t)variant->grenade_type * 0x44;
    projectile_definition = (void *)0;
    if (entry != (uint8_t *)0) {
        projectile_tag = *(uint32_t *)(entry + 0x40);
        if (projectile_tag != halo::k_dword_none) {
            projectile_definition = halo::cache::globals().tag_instances[projectile_tag & halo::k_slot_mask].data;
        }
    }

    if (halo::ai::projectile_get_aiming_vector(&self->grenade_impact_point, 0, (Projectile *)projectile_definition,
                     point, 0, 0, &self->grenade_throw_speed, self->grenade_high_arc[0], &direction,
                     &speed, &arc, 0, &flat) == 0) {
        return 0;
    }

    length = (float)halo::libm::sqrt((double)(direction.i * direction.i + direction.j * direction.j));
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

    if (halo::ai::actor_grenade_parabolic_path_clear(&velocity, actor_index, point, arc, gravity,
                                           *(datum_index *)self->grenade_exclude_object_index,
                                           (uint8_t)(self->active_unit_index != (datum_index)halo::k_dword_none)) == 0) {
        return 0;
    }

    self->grenade_throw_direction.i = direction.i;
    self->grenade_throw_direction.j = direction.j;
    self->grenade_throw_direction.k = direction.k;
    self->grenade_throw_speed = speed;
    return 1;
}

namespace actor_try_grenade_evasion_local {
extern "C" {
extern game_time_globals *game_time;
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
extern actor_mode_definition actor_mode_definitions[16];
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
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = TAG_DATA(act->actor_definition_tag);
    int16_t grade;
    int32_t now;

    if (!act->needs_new_path || !(act->shield_vitality <= ((Actor *)actor_tag)->hide_shield_fraction)) {
        return 0;
    }
    grade = actor_mode_definitions[act->mode].combat_grade;
    if (act->berserking || (grade != 4 && grade != 3) || act->combat_status < 2) {
        return 0;
    }
    now = game_time->game_time;
    if (static_cast<int32_t>(act->last_cover_attempt_time) != -1 && now < static_cast<int32_t>(act->last_cover_attempt_time) + 30) {
        return 0;
    }
    act->last_cover_attempt_time = static_cast<datum_index>(now);
    if (!halo::ai::actor_should_throw_grenade(actor_index, 0)) {
        return 0;
    }
    if (halo::ai::actor_handle_death(actor_index, 1, 0)) {
        return 1;
    }
    if (allow_pain_reaction &&
        halo::ai::actor_check_pain_reaction(act->target_unit_index, use_alt_base, 4, actor_index)) {
        return 1;
    }
    return 0;
}

#undef TAG_DATA

namespace actor_update_grenade_and_morale_reactions_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
}

/**
 * Actor AI behaviour: update grenade and morale reactions.
 *
 * @address 0x40b920
 */
char ActorView::update_grenade_and_morale_reactions()
{
    using namespace actor_update_grenade_and_morale_reactions_local;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *variant = TAG_DATA(act->actor_variant_tag);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    int32_t now = game_time->game_time;
    char result = 0;
    float threshold;
    uint8_t may_evade;
    uint8_t may_target;

    if (act->retreat_timer > 0 && act->active_unit_index == k_datum_index_none) {
        prop *threat = halo::ai::prop_at(act->retreat_prop_index);

        if (threat->engaged != 0 && (threat->obstruction == 0 || threat->obstruction == 1) &&
            (static_cast<int32_t>(act->last_evasion_time) == -1 || static_cast<int32_t>(act->last_evasion_time) + 0x1e <= now)) {
            act->last_evasion_time = static_cast<datum_index>(now);
            if (halo::ai::actor_should_throw_grenade(actor_index, 1)) {
                if (halo::ai::actor_handle_death(actor_index, 0, 1)) {
                    return 1;
                }
                if (halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::panicked_by_unopposable_enemy) &&
                    halo::ai::actor_check_pain_reaction(act->retreat_prop_index, 0, 5, actor_index)) {
                    return 1;
                }
            }
        }
    }

    if (act->defending != 0 && act->berserking == 0) {
        threshold = actor_tag->defending_evasion_threshold;
    } else {
        threshold = actor_tag->attacking_evasion_threshold;
    }
    if (act->playfight != 0 && actor_tag->evasion_seek_cover_chance > 0.0f && threshold > 1.1f) {
        threshold = 1.1f;
    }
    if (!(threshold <= act->danger_meter)) {
        return 0;
    }
    if (act->moving == 0) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    }
    if (((struct ActorVariant *)variant)->grenade_stimulus == 2 && halo::ai::actor_consider_grenade_throw(actor_index)) {
        act->danger_meter = 0.0f;
        result = 1;
    }
    may_evade = 1;
    may_target = 1;
    if (act->crouch_active != 0 && halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::try_to_stay_still_when_crouched)) {
        datum_index target = act->target_unit_index;

        may_evade = 0;
        if (target != k_datum_index_none) {
            prop *target_prop = halo::ai::prop_at(target);

            if ((int8_t)(uint8_t)target_prop->aiming_at_actor_class <= 2 && (int8_t)target_prop->distance_class <= 1) {
                may_evade = 1;
            }
        }
    }
    if (act->mode == 10 && (act->mode_data.charge.stage == 2 || act->mode_data.charge.stage == 3)) {
        may_target = 0;
    }
    if (result) {
        return result;
    }
    if (may_evade && (static_cast<int32_t>(act->last_evasion_time) == -1 || static_cast<int32_t>(act->last_evasion_time) + 0x1e <= now)) {
        act->last_evasion_time = static_cast<datum_index>(now);
        if (halo::ai::actor_should_throw_grenade(actor_index, 0) && halo::math::random_real() <= actor_tag->evasion_seek_cover_chance &&
            halo::ai::actor_handle_death(actor_index, 0, 1)) {
            halo::ai::ai_communication_broadcast(0x18, act->unit_index, halo::ai::actor_get_target_prop_object_index(actor_index),
                                       -1, -1, -1, 0);
            act->danger_meter = 0.0f;
            return 1;
        }
    }
    if (may_target && act->evasion_delay_ticks == 0 && halo::ai::actor_evaluate_grenade_target_position(actor_index)) {
        act->danger_meter = 0.0f;
        act->evasion_delay_ticks = (int16_t)(int32_t)(actor_tag->evasion_delay_time * 30.0f);
        act->grenade_evasion_active = 1;
        result = 1;
    }
    return result;
}

#undef TAG_DATA

namespace actor_update_grenade_eligibility_state_local {
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

    self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    if (self->awareness_level <= 1 || !halo::ai::globals().state->dialogue_triggers_enabled) {
        return;
    }

    eligible = (self->awareness_level == 3 && self->minimum_combat_status < self->combat_status);

    if (self->grenade_recheck_ticks == 0 || self->grenade_eligible != eligible) {
        halo::ai::actor_recompute_grenade_eligibility(actor_index);
    }

    if (0 < self->grenade_recheck_ticks) {
        self->grenade_recheck_ticks = self->grenade_recheck_ticks - 1;
        if (self->grenade_recheck_ticks == 0) {
            out_a = (int16_t)(eligible != 0);
            out_b = -1;
            result = (int16_t)halo::units::unit_animation_change_priority_check(self->unit_index, 1, 1, 0, 0, &out_a, &out_b);
            if (0 < result) {
                int i;
                for (i = 0; i < 12; i++) {
                    buffer[i] = 0;
                }
                *(int16_t *)((uint8_t *)buffer + 2) = (int16_t)out_a;
                *((uint32_t *)((uint8_t *)buffer + 4)) = out_b;
                *(int16_t *)buffer = 1;
                halo::ai::ai_communication_target_result_reset((ai_communication_target_result *)((uint8_t *)buffer + 0x10));
                halo::units::unit_commit_speech(self->unit_index, (const unit_speech *)buffer, result);
            }
        }
    }
}

namespace actor_update_grenade_throw_decision_local {
extern "C" {
extern game_time_globals *game_time;
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
}

/**
 * Actor AI behaviour: update grenade throw decision.
 *
 * @address 0x40b770
 */
uint8_t ActorView::update_grenade_throw_decision()
{
    using namespace actor_update_grenade_throw_decision_local;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *variant = TAG_DATA(act->actor_variant_tag);
    int16_t mode = act->mode;
    uint8_t result = 0;

    if (act->target_combat_status < 5 || (mode == 4 && act->mode_data.flee.panic > 0)) {
        act->grenade_throw_pending = 0;
        return 0;
    }
    switch (((struct ActorVariant *)variant)->grenade_stimulus) {
    case 1:
        if (act->combat_status >= 5) {
            result = halo::ai::actor_consider_grenade_throw(actor_index);
        }
        break;
    case 2:
        if (halo::ai::prop_at(act->target_unit_index)->swarm_owned || (mode == 4 && act->mode_data.flee.panic == 0)) {
            result = halo::ai::actor_consider_grenade_throw(actor_index);
        }
        break;
    default:
        break;
    }
    if (act->grenade_throw_pending) {
        halo::ai::actor_check_grenade_facing_and_commit(actor_index, 0);
    }
    return result;
}

#undef TAG_DATA

namespace actor_validate_grenade_ally_candidate_local {
static auto &actor_type_procs = halo::link::ref<void *[16]>(halo::ai::vars().actor_type_procs);
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
    if (candidate_actor != (datum_index)k_datum_index_none && -1 < index && index < halo::ai::globals().actor_data->maximum_count) {
        actor *slot = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (uint32_t)index * halo::ai::globals().actor_data->size);
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

    self = halo::ai::actor_at(actor_index);
    variant = halo::ai::tag_data<ActorVariant>(self->actor_variant_tag);

    if (halo::ai::actor_score_blast_area_clear(actor_index, variant->enemy_radius,
                      variant->collateral_damage_radius, candidate_point, &hostile_count) != 0) {
        self->grenade_impact_point = *candidate_point;
        return 1;
    }
    return 0;
}

}
