#include "halo/ai/actor_behavior.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

namespace halo::ai {

namespace actor_mode_uncover_tick_local {
}

/**
 * Actor AI behaviour: mode uncover tick.
 *
 * @address 0x408470
 */
void ActorView::mode_uncover_tick()
{
    using namespace actor_mode_uncover_tick_local;
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag;
    int16_t kind;
    uint8_t keep_going = 1;
    uint8_t target_visible = 0;
    uint8_t done;

    if (act->mode_data.uncover.done) {
        return;
    }
    actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    kind = act->mode_data.uncover.stage;
    act->mode_data.uncover.crouch = 0;
    if (kind == 0) {
        if (actor_tag->defensive_crouch_type == 4) {
            act->mode_data.uncover.crouch = (uint8_t)(act->target_combat_status != 6);
        } else if ((static_cast<uint8_t>(actor_tag->flags) & 2) && act->target_combat_status == 5 &&
                   (int8_t)halo::ai::prop_at(act->target_unit_index)->distance_class <= 2) {
            act->mode_data.uncover.crouch = 1;
        }
    } else if (kind == 1) {
        if (actor_tag->defensive_crouch_type == 4 ||
            ((static_cast<uint8_t>(actor_tag->flags) & 4) &&
             halo::math::vector3d_distance_squared(*(&act->mode_data.uncover.position), act->body_position) < 100.0f)) {
            act->mode_data.uncover.crouch = 1;
        }
    }
    if (act->moving) {
        act->mode_data.uncover.stage_ticks = 0;
    } else {
        act->mode_data.uncover.stage_ticks += 1;
        if (kind == 0 && act->mode_data.uncover.stage_ticks >= 30) {
            halo::ai::actor_push_recognition_entry(actor_index, act->firing_position_index, 0);
        }
    }
    kind = act->mode_data.uncover.stage;
    if (kind == 0) {
        if (act->target_unit_index != k_datum_index_none) {
            target_visible = (uint8_t)(((struct prop *)halo::ai::prop_bytes(act->target_unit_index))->visual_perception > 0);
            keep_going = (uint8_t)!(target_visible && act->target_combat_status < 5);
        }
    } else {
        keep_going = (uint8_t)(act->mode_data.uncover.target_reached == 0);
    }
    if (act->firing_position_index != -1 && keep_going && (act->vehicle_gunner_bombards[0] || target_visible || act->moving)) {
        act->mode_data.uncover.remaining_ticks = act->mode_data.uncover.duration_ticks;
    } else {
        act->mode_data.uncover.unknown_02 = 1;
        if (act->mode_data.uncover.remaining_ticks > 0) {
            act->mode_data.uncover.remaining_ticks -= 1;
        }
        act->mode_data.uncover.total_ticks += 1;
    }
    done = (uint8_t)(act->mode_data.uncover.remaining_ticks == 0 || act->mode_data.uncover.total_ticks >= 360);
    if (kind == 1 && act->mode_data.uncover.target_reached) {
        done = 1;
    }
    act->mode_data.uncover.done = done;
}


namespace actor_mode_uncover_update_local {
}

/**
 * Actor AI behaviour: mode uncover update.
 *
 * @address 0x408680
 */
void ActorView::mode_uncover_update()
{
    using namespace actor_mode_uncover_update_local;
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    datum_index target = act->target_unit_index;

    if (target != k_datum_index_none) {
        prop *p = halo::ai::prop_at(target);
        uint8_t forced = 0;
        int16_t kind = p->obstruction;

        if (act->mode_data.uncover.stage == 0) {
            if (act->vehicle_gunner_bombards[0]) {
                act->wants_to_fire = 1;
                act->unknown_455[0] = 1;
                forced = 1;
            } else {
                act->wants_to_fire = (uint8_t)(act->target_combat_status >= ((static_cast<uint8_t>(actor_tag->flags) & 0x10) ? 5 : 6));
            }
        }
        if ((act->wants_to_fire && (kind == 0 || kind == 1)) || forced) {
            act->flee_reason = 7;
        } else if (act->target_combat_status < 5) {
            act->flee_reason = 3;
        } else if (kind == 2 || kind == 4) {
            act->flee_reason = 2;
        } else {
            act->flee_reason = 5;
        }
        if (act->mode_data.uncover.stage == 0) {
            act->flee_source.code = 2;
        } else if (act->mode_data.uncover.stage == 1) {
            act->flee_source.code = 3;
            act->flee_source.payload.point = act->mode_data.uncover.position;
        }
    }
    act->look_posture = 3;
    act->crouch_decision[0] = act->mode_data.uncover.crouch;
    act->crouch_decision[1] = act->mode_data.uncover.crouch;
    act->crouch_hold = 0;
    act->unknown_424[0] = 0;
    act->unknown_424[1] = 1;
}


namespace actor_mode_vehicle_enter_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}

/**
 * Actor AI behaviour: mode vehicle enter.
 *
 * @address 0x408b50
 */
void ActorView::mode_vehicle_enter()
{
    using namespace actor_mode_vehicle_enter_local;
    actor *act = halo::ai::actor_at(actor_index);

    act->mode_data.vehicle.stuck_count = 0;
    act->mode_data.vehicle.last_progress_time = game_time->game_time;
    act->mode_data.vehicle.last_progress_position = *(real_point3d *)&act->body_position.x;
}


namespace actor_mode_vehicle_update_local {
}

/**
 * Actor AI behaviour: mode vehicle update.
 *
 * @address 0x408e80
 */
void ActorView::mode_vehicle_update()
{
    using namespace actor_mode_vehicle_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode_data.vehicle.entry_reached) {
        act->flee_reason = 4;
        act->flee_source.code = 4;
        *(real_vector3d *)((uint8_t *)act + 0x3f0) = act->mode_data.vehicle.entry_direction;
    } else if (act->movement_action_complete) {
        act->flee_reason = 3;
        act->flee_source.code = 0;
    } else {
        act->flee_reason = 0;
    }
    act->look_posture = 4;
    act->wants_to_fire = 0;
    act->crouch_decision[0] = 0;
    act->crouch_decision[1] = 0;
    act->crouch_hold = 0;
    act->unknown_424[0] = 0;
    act->unknown_424[1] = 0;
}


namespace actor_mode_wait_process_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}

/**
 * Actor AI behaviour: mode wait process.
 *
 * @address 0x409b30
 */
uint8_t ActorView::mode_wait_process()
{
    using namespace actor_mode_wait_process_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (!act->needs_new_path) {
        return act->mode_data.wait.finished;
    }
    act->mode_data.wait.following_friend = 0;
    halo::ai::actor_find_nearest_grenade_ally(actor_index, act->grenade_ally_phase_flag);
    if (act->mode_data.wait.unknown_01) {
        if (act->nearby_friend_prop_index == k_datum_index_none) {
            if (act->mode_data.wait.countdown_150 == 0) {
                act->mode_data.wait.countdown_150 = 150;
            }
        } else if (game_time->game_time >= act->mode_data.wait.start_game_time + 2700) {
            act->mode_data.wait.finished = 1;
        }
    } else {
        act->mode_data.wait.finished = 1;
        if (act->nearby_friend_prop_index != k_datum_index_none) {
            prop *ally = halo::ai::prop_at(act->nearby_friend_prop_index);
            float distance = ally->distance;
            uint8_t follow;
            bool decided = false;

            if (act->mode_data.wait.unknown_02 && !act->mode_data.wait.unknown_04) {
                follow = 1;
            } else if (ally->visual_perception < 2 || !(distance < 8.0f)) {
                follow = 0;
                decided = true;
            } else {
                follow = act->mode_data.wait.unknown_04 == 0;
            }
            if (!decided) {
                act->mode_data.wait.following_friend = (follow && distance > 3.5f) ? 1 : 0;
                act->mode_data.wait.finished = 0;
            }
        }
    }
    if (act->swarm) {
        return act->mode_data.wait.finished;
    }
    if (act->mode_data.wait.following_friend) {
        uint8_t done = act->mode_data.wait.finished;

        if (!halo::ai::actor_movement_set_destination_near_target(act->nearby_friend_prop_index, actor_index, 8.0f)) {
            act->mode_data.wait.unknown_04 = 1;
        }
        return done;
    }
    halo::ai::actor_movement_action_stop(actor_index);
    return act->mode_data.wait.finished;
}


namespace actor_mode_wait_tick_local {
}

/**
 * Actor AI behaviour: mode wait tick.
 *
 * @address 0x409cc0
 */
void ActorView::mode_wait_tick()
{
    using namespace actor_mode_wait_tick_local;
    actor *act = halo::ai::actor_at(actor_index);
    datum_index unit_index = act->unit_index;

    if (act->mode_data.wait.random_countdown > 0) {
        act->mode_data.wait.random_countdown -= 1;
        if (act->mode_data.wait.random_countdown == 0) {
            if (unit_index != k_datum_index_none) {
                halo::ai::ai_communication_broadcast(0x11, unit_index, -1, -1, -1, -1, 0);
            }
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            act->mode_data.wait.random_countdown = (int16_t)((((halo::math::globals().random_seed_global >> 16) * 300) >> 16) + 300);
        }
    }
    if (act->mode_data.wait.countdown_150 > 0) {
        act->mode_data.wait.countdown_150 -= 1;
        if (act->mode_data.wait.countdown_150 == 0) {
            if (act->mode_data.wait.unknown_01 && unit_index != k_datum_index_none) {
                halo::ai::ai_communication_broadcast(0x14, unit_index, -1, -1, -1, -1, 0);
            }
            act->mode_data.wait.finished = 1;
        }
    }
    if (!act->mode_data.wait.following_friend && act->mode_data.wait.countdown_0c > 0) {
        act->mode_data.wait.countdown_0c -= 1;
    }
}


namespace actor_mode_wait_update_local {
}

/**
 * Actor AI behaviour: mode wait update.
 *
 * @address 0x409dc0
 */
void ActorView::mode_wait_update()
{
    using namespace actor_mode_wait_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->moving) {
        act->flee_reason = 3;
        act->flee_source.code = 0;
    } else if (!act->grenade_ally_phase_flag && *(int32_t *)&act->nearby_friend_prop_index != -1 && act->mode_data.wait.countdown_0c > 0) {
        act->flee_reason = 5;
        act->flee_source.code = 1;
        act->flee_source.payload.handle = static_cast<int32_t>(act->nearby_friend_prop_index);
    } else {
        act->flee_reason = 1;
    }
    act->look_posture = 3;
    act->wants_to_fire = 0;
    act->crouch_decision[0] = 0;
    act->crouch_decision[1] = 0;
    act->crouch_hold = 0;
    act->unknown_424[0] = 0;
    act->unknown_424[1] = 0;
}


namespace actor_run_mode_transition_loop_local {
}

/**
 * Drives an actor's mode-transition state machine: each pass clears mode_changed, invokes the per-type
 * "unknown_14" callback, clears the whole look-at/search/perception scratch region, and, unless both the
 * per-mode "keep transitioning" predicate and mode_changed are clear, loops again (up to 10 times)
 *
 * @address 0x429ee0
 */
void ActorView::run_mode_transition_loop()
{
    using namespace actor_run_mode_transition_loop_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint8_t keep_going = 0;
    int iterations = 0;

    for (;;) {
        iterations = iterations + 1;
        self->mode_changed = 0;

        ActorTypeRegistry::get(self->type).transition(*this);

        memset((uint8_t *)self + 0x2ec, 0, 0x19 * sizeof(uint32_t));

        if ((keep_going != 0 && self->mode_changed == 0) || iterations > 9) {
            break;
        }

        keep_going = ActorModeRegistry::get(self->mode).process(*this);

        if (keep_going == 0 && self->mode_changed == 0) {
            return;
        }
    }

    halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::none, 0);
}

namespace actor_set_mode_local {
}

/**
 * Central actor mode setter: runs the outgoing mode's exit callback, clamps or raises awareness_level depending
 * on the new mode's combat_grade, clears the recognition-history ring, copies up to data_size bytes of
 * mode-specific data into actor.mode_data.raw, commits the new mode number and mode_changed
 *
 * @address 0x40d8d0
 */
void ActorView::set_mode(int32_t mode, void *mode_data)
{
    using namespace actor_set_mode_local;
    actor *self;
    uint32_t data_size;
    int i;

    self = halo::ai::actor_at(actor_index);

    const TableActorMode incoming = ActorModeRegistry::get(mode);
    ActorModeRegistry::get(self->mode).exit(*this);

    if (incoming.combat_grade() == 0) {
        if (2 < self->awareness_level) {
            self->awareness_level = 2;
        }
    } else if (self->awareness_level < 3) {
        self->awareness_level = 3;
    }

    self->recognition_cursor = 0;
    for (i = 0; i < 4; i++) {
        self->recognition[i].firing_position_index = -1;
    }
    if (self->recognition_valid != 0) {
        self->recognition_valid = 0;
    }

    data_size = incoming.data_size();
    if (data_size != 0 && mode_data != 0) {
        uint8_t *src = (uint8_t *)mode_data;
        uint8_t *dst = self->mode_data.raw;
        uint32_t n;
        for (n = data_size >> 2; n != 0; n--) {
            *(uint32_t *)dst = *(uint32_t *)src;
            src += 4;
            dst += 4;
        }
        for (n = data_size & 3; n != 0; n--) {
            *dst = *src;
            src++;
            dst++;
        }
    }

    self->mode = (int16_t)mode;
    self->mode_changed = 1;

    incoming.enter(*this);
}

namespace actor_update_special_mode_local {
}

/**
 * Actor AI behaviour: update special mode.
 *
 * @address 0x40d820
 */
uint8_t ActorView::update_special_mode()
{
    using namespace actor_update_special_mode_local;
    actor *self;
    int16_t mode;

    self = halo::ai::actor_at(actor_index);
    mode = self->mode;

    if (mode == halo::ai::actor_mode::uncover) {
        if (self->mode_data.uncover.done == 0) {
            return 0;
        }
        if (self->mode_data.uncover.stage == 0) {
            halo::ai::actor_set_target_alert_stage1(self->target_unit_index, actor_index);
        }
    } else if (mode == halo::ai::actor_mode::search) {
        if (self->mode_data.search.finished == 0) {
            return 0;
        }
        if (self->mode_data.search.stage == 0) {
            halo::ai::actor_set_target_alert_stage2(self->target_unit_index, actor_index);
            return halo::ai::actor_update_melee_combat_action(actor_index);
        }
    } else if (mode == halo::ai::actor_mode::wait) {
        if (self->mode_data.wait.finished == 0) {
            return 0;
        }
        halo::ai::actor_set_target_alert_stage3(self->target_unit_index, actor_index);
        return halo::ai::actor_update_melee_combat_action(actor_index);
    } else {
        return 0;
    }
    return halo::ai::actor_update_melee_combat_action(actor_index);
}

}
