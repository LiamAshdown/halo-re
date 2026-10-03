#include "halo/ai/actor_behavior.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"

namespace halo::ai {

namespace actor_mode_uncover_tick_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type);
}
}

/**
 * Actor AI behaviour: mode uncover tick.
 *
 * @address 0x408470
 */
void ActorView::mode_uncover_tick()
{
    using namespace actor_mode_uncover_tick_local;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag;
    int16_t kind;
    uint8_t keep_going = 1;
    uint8_t target_visible = 0;
    uint8_t done;

    if (act[0x9d]) {
        return;
    }
    actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    kind = ((struct actor *)act)->mode_data.uncover.stage;
    act[0x9c] = 0;
    if (kind == 0) {
        if (*(int16_t *)&((Actor *)actor_tag)->defensive_crouch_type == 4) {
            act[0x9c] = (uint8_t)(((actor *)act)->target_combat_status != 6);
        } else if ((actor_tag[0] & 2) && ((actor *)act)->target_combat_status == 5 &&
                   (int8_t)PROP(((actor *)act)->target_unit_index)[0x121] <= 2) {
            act[0x9c] = 1;
        }
    } else if (kind == 1) {
        if (*(int16_t *)&((Actor *)actor_tag)->defensive_crouch_type == 4 ||
            ((actor_tag[0] & 4) &&
             halo::math::vector3d_distance_squared(*(&((struct actor *)act)->mode_data.uncover.position), ((struct actor *)act)->body_position) < 100.0f)) {
            act[0x9c] = 1;
        }
    }
    if (act[0x504]) {
        ((struct actor *)act)->mode_data.uncover.stage_ticks = 0;
    } else {
        ((struct actor *)act)->mode_data.uncover.stage_ticks += 1;
        if (kind == 0 && ((struct actor *)act)->mode_data.uncover.stage_ticks >= 30) {
            actor_push_recognition_entry(actor_index, ((actor *)act)->firing_position_index, 0);
        }
    }
    kind = ((struct actor *)act)->mode_data.uncover.stage;
    if (kind == 0) {
        if (((actor *)act)->target_unit_index != k_datum_index_none) {
            target_visible = (uint8_t)(((struct prop *)PROP(((actor *)act)->target_unit_index))->visual_perception > 0);
            keep_going = (uint8_t)!(target_visible && ((actor *)act)->target_combat_status < 5);
        }
    } else {
        keep_going = (uint8_t)(act[0xbc] == 0);
    }
    if (((actor *)act)->firing_position_index != -1 && keep_going && (act[0x162] || target_visible || act[0x504])) {
        ((struct actor *)act)->mode_data.uncover.remaining_ticks = ((struct actor *)act)->mode_data.uncover.duration_ticks;
    } else {
        act[0x9e] = 1;
        if (((struct actor *)act)->mode_data.uncover.remaining_ticks > 0) {
            ((struct actor *)act)->mode_data.uncover.remaining_ticks -= 1;
        }
        *(int32_t *)(act + 0xcc) += 1;
    }
    done = (uint8_t)(((struct actor *)act)->mode_data.uncover.remaining_ticks == 0 || *(int32_t *)(act + 0xcc) >= 360);
    if (kind == 1 && act[0xbc]) {
        done = 1;
    }
    act[0x9d] = done;
}

#undef ACTOR
#undef TAG_DATA
#undef PROP

namespace actor_mode_uncover_update_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
}
}

/**
 * Actor AI behaviour: mode uncover update.
 *
 * @address 0x408680
 */
void ActorView::mode_uncover_update()
{
    using namespace actor_mode_uncover_update_local;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    datum_index target = ((actor *)act)->target_unit_index;

    if (target != k_datum_index_none) {
        uint8_t *p = PROP(target);
        uint8_t forced = 0;
        int16_t kind = ((struct prop *)p)->obstruction;

        if (((struct actor *)act)->mode_data.uncover.stage == 0) {
            if (act[0x162]) {
                act[0x454] = 1;
                act[0x455] = 1;
                forced = 1;
            } else {
                act[0x454] = (uint8_t)(((actor *)act)->target_combat_status >= ((actor_tag[0] & 0x10) ? 5 : 6));
            }
        }
        if ((act[0x454] && (kind == 0 || kind == 1)) || forced) {
            ((actor *)act)->flee_reason = 7;
        } else if (((actor *)act)->target_combat_status < 5) {
            ((actor *)act)->flee_reason = 3;
        } else if (kind == 2 || kind == 4) {
            ((actor *)act)->flee_reason = 2;
        } else {
            ((actor *)act)->flee_reason = 5;
        }
        if (((struct actor *)act)->mode_data.uncover.stage == 0) {
            ((actor *)act)->flee_source.code = 2;
        } else if (((struct actor *)act)->mode_data.uncover.stage == 1) {
            ((actor *)act)->flee_source.code = 3;
            *(real_point3d *)(act + 0x3f0) = ((struct actor *)act)->mode_data.uncover.position;
        }
    }
    ((struct actor *)act)->look_posture = 3;
    act[0x426] = act[0x9c];
    act[0x427] = act[0x9c];
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 1;
}

#undef ACTOR
#undef TAG_DATA
#undef PROP

namespace actor_mode_vehicle_enter_local {
extern "C" {
extern data_array *actor_data;
extern game_time_globals *game_time;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: mode vehicle enter.
 *
 * @address 0x408b50
 */
void ActorView::mode_vehicle_enter()
{
    using namespace actor_mode_vehicle_enter_local;
    uint8_t *act = ACTOR(actor_index);

    *(int16_t *)(act + 0xaa) = 0;
    *(int32_t *)(act + 0xac) = game_time->game_time;
    *(real_point3d *)(act + 0xb0) = *(real_point3d *)&((actor *)act)->body_position.x;
}

#undef ACTOR

namespace actor_mode_vehicle_update_local {
extern "C" {
extern data_array *actor_data;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: mode vehicle update.
 *
 * @address 0x408e80
 */
void ActorView::mode_vehicle_update()
{
    using namespace actor_mode_vehicle_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (act[0xc8]) {
        ((actor *)act)->flee_reason = 4;
        ((actor *)act)->flee_source.code = 4;
        *(real_vector3d *)(act + 0x3f0) = *(real_vector3d *)(act + 0xd8);
    } else if (act[0x4a8]) {
        ((actor *)act)->flee_reason = 3;
        ((actor *)act)->flee_source.code = 0;
    } else {
        ((actor *)act)->flee_reason = 0;
    }
    ((struct actor *)act)->look_posture = 4;
    act[0x454] = 0;
    act[0x426] = 0;
    act[0x427] = 0;
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;
}

#undef ACTOR

namespace actor_mode_wait_process_local {
extern "C" {
extern data_array *actor_data;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
extern data_array *prop_data;
extern game_time_globals *game_time;
extern int32_t actor_find_nearest_grenade_ally(datum_index actor_index, uint8_t widen_search);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius);
}
}

/**
 * Actor AI behaviour: mode wait process.
 *
 * @address 0x409b30
 */
uint8_t ActorView::mode_wait_process()
{
    using namespace actor_mode_wait_process_local;
    uint8_t *act = ACTOR(actor_index);

    if (!act[0x4c]) {
        return act[0x9c];
    }
    ((struct actor *)act)->mode_data.wait.following_friend = 0;
    actor_find_nearest_grenade_ally(actor_index, act[0x1cc]);
    if (act[0x9d]) {
        if (((struct actor *)act)->nearby_friend_prop_index == k_datum_index_none) {
            if (((struct actor *)act)->mode_data.wait.countdown_150 == 0) {
                ((struct actor *)act)->mode_data.wait.countdown_150 = 150;
            }
        } else if (game_time->game_time >= ((struct actor *)act)->mode_data.wait.start_game_time + 2700) {
            act[0x9c] = 1;
        }
    } else {
        act[0x9c] = 1;
        if (((struct actor *)act)->nearby_friend_prop_index != k_datum_index_none) {
            uint8_t *ally = (uint8_t *)prop_data->data + (((struct actor *)act)->nearby_friend_prop_index & halo::k_slot_mask) * k_prop_size;
            float distance = ((prop *)ally)->distance;
            uint8_t follow;

            if (act[0x9e] && !act[0xa0]) {
                follow = 1;
            } else if (((struct prop *)ally)->visual_perception < 2 || !(distance < 8.0f)) {
                follow = 0;
                goto decided;
            } else {
                follow = act[0xa0] == 0;
            }
            if (follow && distance > 3.5f) {
                ((struct actor *)act)->mode_data.wait.following_friend = 1;
                act[0x9c] = 0;
            } else {
                ((struct actor *)act)->mode_data.wait.following_friend = 0;
                act[0x9c] = 0;
            }
        }
    }
decided:
    if (act[0x6]) {
        return act[0x9c];
    }
    if (((struct actor *)act)->mode_data.wait.following_friend) {
        uint8_t done = act[0x9c];

        if (!actor_movement_set_destination_near_target(((struct actor *)act)->nearby_friend_prop_index, actor_index, 8.0f)) {
            act[0xa0] = 1;
        }
        return done;
    }
    actor_movement_action_stop(actor_index);
    return act[0x9c];
}

#undef ACTOR
#undef TAG_DATA

namespace actor_mode_wait_tick_local {
extern "C" {
extern data_array *actor_data;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
}
}

/**
 * Actor AI behaviour: mode wait tick.
 *
 * @address 0x409cc0
 */
void ActorView::mode_wait_tick()
{
    using namespace actor_mode_wait_tick_local;
    uint8_t *act = ACTOR(actor_index);
    datum_index unit_index = ((actor *)act)->unit_index;

    if (((struct actor *)act)->mode_data.wait.random_countdown > 0) {
        ((struct actor *)act)->mode_data.wait.random_countdown -= 1;
        if (((struct actor *)act)->mode_data.wait.random_countdown == 0) {
            if (unit_index != k_datum_index_none) {
                ai_communication_broadcast(0x11, unit_index, -1, -1, -1, -1, 0);
            }
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            ((struct actor *)act)->mode_data.wait.random_countdown = (int16_t)((((halo::math::globals().random_seed_global >> 16) * 300) >> 16) + 300);
        }
    }
    if (((struct actor *)act)->mode_data.wait.countdown_150 > 0) {
        ((struct actor *)act)->mode_data.wait.countdown_150 -= 1;
        if (((struct actor *)act)->mode_data.wait.countdown_150 == 0) {
            if (act[0x9d] && unit_index != k_datum_index_none) {
                ai_communication_broadcast(0x14, unit_index, -1, -1, -1, -1, 0);
            }
            act[0x9c] = 1;
        }
    }
    if (!((struct actor *)act)->mode_data.wait.following_friend && ((struct actor *)act)->mode_data.wait.countdown_0c > 0) {
        ((struct actor *)act)->mode_data.wait.countdown_0c -= 1;
    }
}

#undef ACTOR

namespace actor_mode_wait_update_local {
extern "C" {
extern data_array *actor_data;
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: mode wait update.
 *
 * @address 0x409dc0
 */
void ActorView::mode_wait_update()
{
    using namespace actor_mode_wait_update_local;
    uint8_t *act = ACTOR(actor_index);

    if (act[0x504]) {
        ((actor *)act)->flee_reason = 3;
        ((actor *)act)->flee_source.code = 0;
    } else if (!act[0x1cc] && *(int32_t *)&((struct actor *)act)->nearby_friend_prop_index != -1 && ((struct actor *)act)->mode_data.wait.countdown_0c > 0) {
        ((actor *)act)->flee_reason = 5;
        ((actor *)act)->flee_source.code = 1;
        *(int32_t *)(act + 0x3f0) = *(int32_t *)&((struct actor *)act)->nearby_friend_prop_index;
    } else {
        ((actor *)act)->flee_reason = 1;
    }
    ((struct actor *)act)->look_posture = 3;
    act[0x454] = 0;
    act[0x426] = 0;
    act[0x427] = 0;
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;
}

#undef ACTOR

namespace actor_run_mode_transition_loop_local {
extern "C" {
extern data_array *actor_data;
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
}
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
    actor *self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
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

    actor_set_mode(actor_index, 0, 0);
}

namespace actor_set_mode_local {
extern "C" {
extern data_array *actor_data;
}
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

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
extern "C" {
extern data_array *actor_data;
extern uint8_t actor_update_melee_combat_action(datum_index actor_index);
extern void actor_set_target_alert_stage1(datum_index target_prop_index, datum_index actor_index);
extern void actor_set_target_alert_stage2(datum_index target_prop_index, datum_index actor_index);
extern void actor_set_target_alert_stage3(datum_index target_prop_index, datum_index actor_index);
}
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    mode = self->mode;

    if (mode == 5) {
        if (self->mode_data.raw[1] == 0) {
            return 0;
        }
        if (*(int16_t *)&self->mode_data.raw[8] == 0) {
            actor_set_target_alert_stage1(self->target_unit_index, actor_index);
        }
    } else if (mode == 7) {
        if (self->mode_data.raw[0] == 0) {
            return 0;
        }
        if (*(int16_t *)&self->mode_data.raw[8] == 0) {
            actor_set_target_alert_stage2(self->target_unit_index, actor_index);
            return actor_update_melee_combat_action(actor_index);
        }
    } else if (mode == 8) {
        if (self->mode_data.raw[0] == 0) {
            return 0;
        }
        actor_set_target_alert_stage3(self->target_unit_index, actor_index);
        return actor_update_melee_combat_action(actor_index);
    } else {
        return 0;
    }
    return actor_update_melee_combat_action(actor_index);
}

}
