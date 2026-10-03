#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

namespace halo::ai {

namespace actor_target_relationship_think_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
extern "C" {
static const uint8_t k_relationship_recheck_case[4][4] = {
    { 0, 0, 1, 3 },
    { 0, 1, 2, 3 },
    { 0, 2, 3, 4 },
    { 0, 3, 4, 4 },
};
}
}

/**
 * Actor AI behaviour: target relationship think.
 *
 * @address 0x41abd0
 */
void ActorView::target_relationship_think()
{
    using namespace actor_target_relationship_think_local;
    actor *self;
    Actor *definition;
    uint32_t reaction_ticks;
    uint8_t danger_reacted;
    datum_index best_prop;
    float best_prop_distance;
    datum_index cursor;
    datum_index target_prop_index;
    prop *target;
    datum_index next_prop_index;
    int32_t new_kind;
    uint8_t cooldown_expired;
    uint8_t refresh_needed;
    uint8_t need_aim_refresh;
    uint8_t released;
    uint8_t had_conflict;
    uint8_t scratch1[56];
    uint8_t scratch2[56];
    int16_t danger_type;
    int16_t timer;
    actor *owner;
    datum_index paired_prop;
    struct { int16_t team; int16_t object_type; char is_enemy; } payload;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & halo::k_slot_mask].data;

    reaction_ticks = 1;
    danger_reacted = 0;
    best_prop = (datum_index)k_datum_index_none;
    best_prop_distance = 3.402823e+38f;

    if (self->keep_unit_alive != 0) goto skip_danger_response;

    if (self->needs_new_path != 0) {
        halo::ai::actor_target_scan_potential_targets(actor_index);
    }
    halo::ai::actor_danger_update_reaction(actor_index);

    danger_type = self->danger_type;
    if (danger_type < 1) goto skip_danger_response;

    {
        uint8_t should_react;

        if (self->danger_is_own == 0 && self->danger_owner_relation == 0) {
            should_react = 0;
            if (self->danger_reaction_ticks > 0 && self->danger_reaction_delayed != 0) {
                if (self->ticks_since_threatened == -1 || self->ticks_since_threatened > 0x3b) {
                    self->danger_reaction_ticks -= 1;
                    should_react = (uint8_t)(self->danger_reaction_ticks == 0);
                } else {
                    self->danger_reaction_ticks = 0;
                    should_react = 1;
                }
            }
        } else {
            self->danger_reacting = 1;
            should_react = (uint8_t)(self->danger_reaction_ticks > 0);
            self->danger_reaction_ticks = 0;
        }

        if (should_react) {
            if (danger_type == 1) {
                self->danger_reacting = 1;
            } else {
                float threshold = -1.0f;
                if (danger_type == 2) {
                    threshold = definition->notice_projectile_chance;
                } else if (danger_type == 3) {
                    threshold = definition->notice_vehicle_chance;
                }
                if (threshold > 0.0f && halo::math::random_real() < threshold) {
                    self->danger_reacting = 1;
                }
            }

            if (self->danger_reacting != 0) {
                if (self->danger_is_own == 0) {
                    if (self->danger_owner_relation == 0 && danger_type != 3 && danger_type != 1) {
                        self->danger_dive = (uint8_t)(halo::math::random_real() < definition->dive_from_grenade_chance);
                    } else {
                        self->danger_dive = 1;
                    }
                } else {
                    self->danger_dive = 0;
                }
                halo::ai::actor_notify_squad_of_threat_direction(&self->flee_from_point, actor_index, (uint16_t)self->danger_type,
                    (uint16_t)self->danger_owner_relation);
            }
        }
    }

    if (self->danger_reaction_ticks == 0) {
        if (self->vocalization_line == 0xc) {
            if (self->vocalization_variant > 5) {
                self->vocalization_variant = 5;
            }
        }
        if (self->danger_is_own != 0) {
            self->danger_reacting = 1;
            self->danger_dive = 0;
        }
    }

skip_danger_response:
    cursor = self->first_prop;

restart:
    target_prop_index = cursor;

    if (cursor == (datum_index)k_datum_index_none) {
        datum_index result = (datum_index)k_datum_index_none;

        if (self->target_unit_index == (datum_index)k_datum_index_none) {
            result = best_prop;
        } else {
            prop *cur = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (self->target_unit_index & halo::k_slot_mask) * sizeof(prop));
            if (cur->state < 4 || cur->state > 5) {
                result = best_prop;
            }
        }

        if (self->target_combat_status > 5) {
            self->ever_had_target[0] = 1;
        }
        if (self->target_combat_status < 10) {
            if (self->stood_down == 0) {
                if (self->ticks_since_engaged != -1) {
                    self->ticks_since_engaged += 1;
                }
            } else {
                self->ticks_since_engaged = -1;
            }
        } else {
            self->ticks_since_engaged = 0;
        }

        self->target_reaction_threshold = (int16_t)reaction_ticks;
        self->nearest_orphan_prop_index = result;
        return;
    }

    target = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (cursor & halo::k_slot_mask) * sizeof(prop));
    next_prop_index = target->next_in_actor;
    cursor = next_prop_index;

    new_kind = -1;
    cooldown_expired = 0;
    refresh_needed = 0;
    need_aim_refresh = 0;
    released = 0;
    had_conflict = 0;

    if (target->stimulus_timer > 0) {
        target->stimulus_timer -= 1;
        if (target->stimulus_timer == 0) {
            target->stimulus_type = -1;
        }
    }
    if (target->seen_state != -1) {
        target->seen_state += 1;
        if (target->seen_state > 0x2c) {
            target->seen = 0;
        }
    }
    if (target->information_age != -1) {
        target->information_age += 1;
        if (target->information_age > 0x3b) {
            target->has_current_information = 0;
            target->information_source_actor = -1;
        }
    }
    if (target->dead == 0) {
        target->dead_ticks = 0;
    } else {
        target->dead_ticks += 1;
    }
    if (target->lost_timer > 0) {
        target->lost_timer -= 1;
    }
    if (target->retain_timer > 0 && target->just_created == 0) {
        target->retain_timer -= 1;
    }
    if (target->engaged_ticks > 0 && target->engaged_ticks < INT16_MAX) {
        target->engaged_ticks += 1;
    }
    if (target->friends_killed_timer > 0) {
        target->friends_killed_timer -= 1;
        if (target->friends_killed_timer == 0) {
            target->friends_killed -= 1;
            if (target->friends_killed > 0) {
                target->friends_killed_timer = 0x2ee;
            }
        }
    }
    if (target->visual_perception < 2) {
        target->sighted_ticks = 0;
    } else if (target->sighted_ticks < INT16_MAX) {
        target->sighted_ticks += 1;
    }

    if (self->keep_unit_alive == 0) {
        target->reaction_timer += 1;
        timer = target->reaction_timer;
        if (target->enemy == 0) {
            timer = (int16_t)(timer >> 3);
        }
        if (target->distance_class > 2) {
            timer = (int16_t)(timer >> 1);
        }
        if (danger_reacted == 0 && timer >= self->target_reaction_threshold) {
            need_aim_refresh = 1;
            refresh_needed = 1;
            timer = 0;
            target->reaction_timer = 0;
            danger_reacted = 1;
        }
        if ((int16_t)reaction_ticks < timer) {
            reaction_ticks = (uint16_t)timer;
        }

        if (target->state < 0 || target->state > 1 || target->pair_index == (datum_index)k_datum_index_none) {
            if (self->swarm == 0) {
                uint8_t important =
                    (uint8_t)((self->target_unit_index == target_prop_index) ||
                              (self->nearest_orphan_prop_index == target_prop_index) ||
                              (self->retreat_prop_index == target_prop_index) ||
                              (self->nearby_friend_prop_index == target_prop_index) ||
                              (self->vocalization_line != 0 && self->vocalization_source.code == 1 &&
                               self->vocalization_source.payload.handle == target_prop_index) ||
                              (self->idle_major_active != 0 && self->idle_major_direction_type == 1 &&
                               self->idle_major_prop_index == target_prop_index) ||
                              (self->idle_look_state[1] != 0 && self->idle_look_direction_type == 1 &&
                               self->idle_look_prop_index == target_prop_index));
                target->in_use = important;
                if (target->state > 3 && target->state < 6) {
                    prop *pair = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (target->pair_index & halo::k_slot_mask) * sizeof(prop));
                    pair->in_use = important;
                }
            } else {
                target->in_use = 0;
            }
        }

        if ((target->in_use != 0 && (target->state < 0 || target->state > 1)) || refresh_needed) {
            halo::ai::actor_target_data_refresh(actor_index, target_prop_index, scratch1, 0, need_aim_refresh);
        }
        if (need_aim_refresh != 0) {
            halo::ai::actor_target_update_tracking_speed(actor_index, target_prop_index, scratch1);
        }
    } else {
        target->in_use = 0;
        target->reaction_timer = 0;
    }

    switch (target->state) {
    case 0:
        if (target->perception_level > 0) {
            new_kind = 1;
            *(float *)&target->acknowledge_progress = 0.0f;
            goto case1_dispatch;
        }
        break;

    case 1:
    case1_dispatch:
        if (target->perception_level != 0) {
            int priority_class = halo::ai::actor_target_get_priority_class(actor_index, target_prop_index);
            int danger = target->perception_level & 3;
            float rate;

            switch (k_relationship_recheck_case[priority_class & 3][danger]) {
            case 0: rate = 0.0f; break;
            case 1: rate = definition->inverse_non_combat_perception_time; break;
            case 2: rate = definition->inverse_guard_perception_time; break;
            case 3: rate = definition->inverse_combat_perception_time; break;
            default: rate = 1.0f; break;
            }

            *(float *)&target->acknowledge_progress += rate;
            if (*(float *)&target->acknowledge_progress >= 1.0f) {
                new_kind = 3;
            }
            if (new_kind == -1) {
                goto check_cooldown;
            }
            goto apply_new_kind;
        }
        *(float *)&target->acknowledge_progress = 0.0f;
        new_kind = 0;
        goto apply_new_kind;

    case 2:
        if (target->perception_level < 1) {
            if (target->lost_timer != 0) {
                float dx = target->last_known_position.x - target->last_perceived_position.x;
                float dy = target->last_known_position.y - target->last_perceived_position.y;
                if (dx * dx + dy * dy <= 1.0f) {
                    break;
                }
            }
            owner = (target->owner_actor_index == (datum_index)k_datum_index_none)
                        ? (actor *)0
                        : (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (target->owner_actor_index & halo::k_slot_mask) * sizeof(actor));
            paired_prop = (datum_index)halo::k_dword_none;
            if (target->enemy != 0 && target->dead == 0 &&
                (target->is_parented != 0 ||
                 ((owner == (actor *)0 || (owner->active != 0 && owner->keep_unit_alive == 0)) &&
                  target->distance * target->distance <= 1600.0f))) {
                halo::ai::actor_target_data_refresh(actor_index, target_prop_index, scratch2, 0, 0);
                halo::ai::actor_target_get_relationship_object(target_prop_index);
                paired_prop = halo::ai::actor_allocate_paired_prop(actor_index, target_prop_index);
            }
            halo::ai::actor_replace_object_reference(actor_index, paired_prop, target_prop_index);
            new_kind = 0;
            goto apply_new_kind;
        replace_and_idle:
            halo::ai::actor_replace_object_reference(actor_index, halo::k_dword_none, target_prop_index);
            new_kind = 0;
        } else {
            new_kind = 3;
        }
        goto apply_new_kind;

    case 3:
        if (target->perception_level == 0) {
            owner = (target->owner_actor_index == (datum_index)k_datum_index_none)
                        ? (actor *)0
                        : (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (target->owner_actor_index & halo::k_slot_mask) * sizeof(actor));
            if (target->enemy == 0 || target->dead != 0 ||
                (target->is_parented == 0 &&
                 ((owner != (actor *)0 && (owner->active == 0 || owner->keep_unit_alive != 0)) ||
                  target->distance * target->distance > 1600.0f))) {
                goto replace_and_idle;
            }
            new_kind = 2;
            goto apply_new_kind;
        }
        break;

    case 4:
    case 5: {
        int penalty;

        if (target->state == 4 &&
            (target->visual_perception > 1 ||
             (self->firing_target_type == 1 && self->firing_target_prop_index == target_prop_index &&
              halo::game::globals().game_time->game_time % 3 == 0))) {
            char nearly_dead = self->vehicle_gunner_bombards[0];
            int16_t threshold = (int16_t)((nearly_dead != 0) ? 300 : 45);
            target->inspection_ticks += 1;
            if (target->inspection_ticks >= threshold) {
                new_kind = 5;
            }
        }

        if (target_prop_index == self->retreat_prop_index ||
            (self->mode == 4 && *(uint32_t *)&self->mode_data.raw[0x1c] == target_prop_index)) {
            penalty = 0;
        } else if (target_prop_index == self->target_unit_index) {
            penalty = (target->noticed_c != 0) ? 1 : 0;
        } else if (target_prop_index == self->nearest_orphan_prop_index) {
            penalty = (self->combat_status < 4) ? 1 : 6;
        } else {
            penalty = 10;
        }
        target->orphan_timer = (int16_t)(target->orphan_timer - penalty);
        if (target->orphan_timer < 0) {
            cooldown_expired = 1;
        }
        if (new_kind != -1) {
            goto apply_new_kind;
        }
        goto check_cooldown;
    }
    }

tail:
    if (target->combat_dirty != 0 && target->state > 1 && target->state < 4) {
        if (target->just_died != 0) {
            halo::ai::actor_scan_backup_and_panic_reaction(target_prop_index, actor_index);
            target->just_died = 0;
        }
        if (target->just_sighted != 0 || (released != 0 && target->visual_perception > 0)) {
            halo::ai::actor_notify_target_engaged(target_prop_index, actor_index, (uint8_t)(released != 0 && had_conflict == 0));
            target->just_sighted = 0;
        }
        if (self->friendly_player_greeted == 0 && target->enemy == 0 && target->is_parented != 0 &&
            target->visual_perception > 1 && target->aiming_at_actor_class < 3 && target->distance < 7.0f) {
            self->friendly_player_greeted = 1;
            halo::ai::ai_communication_broadcast(0x19, self->unit_index, target->object_index, 2, (uint32_t)-1, (uint32_t)-1, 0);
            halo::ai::actor_notify_target_engaged(target_prop_index, actor_index, 0);
        }
        if (self->unit_index != (datum_index)k_datum_index_none && target->dead == 0 &&
            target->allegiance != 0 && target->team_pair_status != 0) {
            float dist_threshold;
            payload.object_type = target->team;
            payload.team = self->team;
            payload.is_enemy = (char)halo::game::teams_are_enemies(target->team, self->team);
            if (payload.is_enemy == 0) {
                dist_threshold = (target->aiming_at_actor_class < 3) ? 10.0f : 3.0f;
            } else {
                dist_threshold = 15.0f;
            }
            if ((payload.is_enemy != 0 && target->seen != 0) || target->distance < dist_threshold) {
                halo::ai::ai_communication_broadcast(8, self->unit_index, target->object_index,
                                            (int32_t)((payload.is_enemy != 0 ? 2 : 0) + 2),
                                            (uint32_t)-1, 1, (uint32_t *)&payload);
            }
        }

        if (self->awareness_level < 3) {
            if (target->dead != 0) {
                if (target->enemy != 0) goto clear_search_and_continue;
                halo::ai::actor_start_search_timer(actor_index, target_prop_index);
                goto after_posture;
            }
            if (target->enemy != 0) {
            clear_search_and_continue:
                halo::ai::actor_queue_velocity_search_from_prop(target_prop_index, actor_index);
                goto after_posture;
            }
        } else {
        after_posture:
            if (target->enemy != 0) goto restart;
        }

        if (target->dead == 0 && target->is_parented == 0) {
            if (self->target_unit_index == (datum_index)k_datum_index_none ||
                (self->ticks_since_engaged != -1 && self->ticks_since_engaged < 0xb4)) {
                if (self->encounter_index == (datum_index)k_datum_index_none) goto restart;
                {
                    encounter *enc = (encounter *)((uint8_t *)halo::ai::globals().encounter_data->data + (self->encounter_index & halo::k_slot_mask) * sizeof(encounter));
                    if (enc->ticks_since_engaged != (datum_index)k_datum_index_none &&
                        (enc->ticks_since_engaged < 0xb4 || enc->has_live_target == 0)) {
                        goto restart;
                    }
                }
            }
            if (self->unit_index != (datum_index)k_datum_index_none) {
                if (self->awareness_level < 3) {
                    if (target->owner_stalled != 0) {
                        halo::ai::ai_communication_broadcast(0xf, target->object_index, self->unit_index, 2, (uint32_t)-1, 2, 0);
                    }
                } else {
                    char busy = (char)halo::ai::actor_is_burst_pending(actor_index);
                    if (busy != 0) {
                        char should_end = (char)halo::ai::actor_check_burst_length_exceeded(actor_index);
                        if (should_end == 0 && target->owner_not_in_combat != 0 && target->visual_perception > 1) {
                            halo::ai::ai_communication_broadcast(0xf, self->unit_index, target->object_index, 2, (uint32_t)-1, 2, 0);
                        }
                    }
                }
            }
        }
        goto restart;
    }
    if (target->state > 3 && target->state < 6 && target->distance < best_prop_distance) {
        best_prop = target_prop_index;
        best_prop_distance = target->distance;
    }
    goto restart;

apply_new_kind:
    switch (new_kind) {
    case 0:
    case 4:
    case 5:
        target->has_current_information = 0;
        target->information_source_actor = -1;
        break;
    case 2:
        target->lost_timer = (uint16_t)((target->visual_perception < 2) ? 10 : 60);
        break;
    case 3:
        released = (uint8_t)halo::ai::actor_target_data_release(target_prop_index, actor_index, &had_conflict);
        cursor = target->next_in_actor;
        break;
    }
    target->state = (int16_t)new_kind;
    target->engaged = halo::ai::actor_target_update_active_flag(actor_index, target_prop_index);
    target->desirability = halo::ai::actor_rate_potential_target(actor_index, target_prop_index);

check_cooldown:
    if (cooldown_expired == 0) {
        goto tail;
    }

    {
        prop *pair = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (target->pair_index & halo::k_slot_mask) * sizeof(prop));
        pair->pair_index = (datum_index)k_datum_index_none;
    }
    halo::ai::actor_replace_object_reference(actor_index, halo::k_dword_none, target_prop_index);
    halo::ai::actor_unlink_prop(actor_index, target_prop_index);
    halo::memory::datum_delete(halo::ai::globals().prop_data, target_prop_index);
    goto restart;
}

}
