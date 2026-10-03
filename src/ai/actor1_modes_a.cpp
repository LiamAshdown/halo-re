#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/ai/actor_modes.hpp"
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

namespace c_actor_mode_alert_movement_cancelled {
}


/**
 * actor_mode_alert_movement_cancelled: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_movement_cancelled.c.txt.
 *
 * @address 0x401460
 */
void halo::ai::alert_mode::movement_cancelled()
{
    using namespace c_actor_mode_alert_movement_cancelled;
    datum_index actor_index = datum;
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;

    mode_data->alert.current_position = -1;
    mode_data->alert.next_position = -1;
}

namespace halo::ai {
void actor_mode_alert_movement_cancelled(datum_index actor_index)
{
    halo::ai::alert_mode(actor_index).movement_cancelled();
}
}


namespace c_actor_mode_alert_process {

}


/**
 * actor_mode_alert_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_process.c.txt.
 *
 * @address 0x4010e0
 */
uint8_t halo::ai::alert_mode::process()
{
    using namespace c_actor_mode_alert_process;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    int16_t count = actor->mode_data.alert.position_count;

    if (count != 0 && actor->mode_data.alert.next_position == -1) {
        int16_t current = actor->mode_data.alert.current_position;
        int ready = 1;

        if (current != -1 && actor->movement_action_complete != 0) {
            float distance_squared = halo::math::vector3d_distance_squared(actor->mode_data.alert.position, actor->body_position);
            float radius = halo::ai::actor_compute_accuracy_scale(actor_index);

            if (!(radius > 0.5f)) {
                radius = 0.5f;
            }
            if (distance_squared > radius * radius) {
                ready = 0;
            }
        }
        if (ready && !(actor->mode_data.alert.wait_ticks > 0) && actor->mode_data.alert.position_reached == 0) {
            unit_object *unit = (unit_object *)halo::ai::object_at(actor->unit_index);

            if (static_cast<uint8_t>(unit->unit.animation_state) != 0x1c) {
                actor->mode_data.alert.next_position = (int16_t)halo::ai::actor_select_move_position(actor_index, count, current, &actor->mode_data.alert.direction_flag);
            }
        }
    }

    if (actor->needs_new_path == 0 || actor->keep_unit_alive != 0 || actor->mode_data.alert.next_position == -1) {
        return 0;
    }
    if (actor->encounter_index != halo::k_dword_none) {
        uint8_t *encounter = (uint8_t *)halo::scenario::globals().scenario->encounters.pointer + (actor->encounter_index & halo::k_slot_mask) * 0xb0;
        uint8_t *squad = *(uint8_t **)(encounter + 0x84) + actor->squad_index * 0xe8;
        int16_t next = actor->mode_data.alert.next_position;

        if (next >= 0 && next < *(int32_t *)(squad + 0xc4)) {
            uint8_t *position = *(uint8_t **)(squad + 0xc8) + next * 0x50;
            float wait = halo::math::random_real_range(*(float *)(position + 0x14), *(float *)(position + 0x18)) * 30.0f;

            actor->mode_data.alert.current_position = actor->mode_data.alert.next_position;
            actor->mode_data.alert.next_position = -1;
            memcpy(&actor->mode_data.alert.position, position, sizeof(ScenarioMovePosition));
            actor->mode_data.alert.wait_ticks = (int16_t)(int32_t)wait;
            actor->mode_data.alert.position_reached = 1;
            if (halo::ai::actor_movement_set_destination_move_position(actor_index, actor->mode_data.alert.current_position)) {
                return 0;
            }
        }
    }
    actor->mode_data.alert.current_position = actor->mode_data.alert.next_position;
    actor->mode_data.alert.next_position = -1;
    actor->mode_data.alert.wait_ticks = 0;
    actor->mode_data.alert.position_reached = 0;
    return 0;
}

namespace halo::ai {
uint8_t actor_mode_alert_process(uint32_t actor_index)
{
    return halo::ai::alert_mode(actor_index).process();
}
}


namespace c_actor_mode_alert_target_cleared {
}


/**
 * actor_mode_alert_target_cleared: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_target_cleared.c.txt.
 *
 * @address 0x401490
 */
void halo::ai::alert_mode::target_cleared()
{
    using namespace c_actor_mode_alert_target_cleared;
    datum_index actor_index = datum;
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;

    mode_data->alert.cluster_index = -1;
    mode_data->alert.surface_index = -1;
}

namespace halo::ai {
void actor_mode_alert_target_cleared(datum_index actor_index)
{
    halo::ai::alert_mode(actor_index).target_cleared();
}
}


namespace c_actor_mode_alert_tick {

}


/**
 * actor_mode_alert_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_tick.c.txt.
 *
 * @address 0x4012e0
 */
void halo::ai::alert_mode::tick()
{
    using namespace c_actor_mode_alert_tick;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    if (actor->keep_unit_alive != 0 || actor->mode_data.alert.current_position == -1) {
        return;
    }
    if (actor->movement_action_complete != 0 && actor->movement_completed == 0) {
        float dx = actor->mode_data.alert.position.x - actor->body_position.x;
        float dy = actor->mode_data.alert.position.y - actor->body_position.y;
        float dz = actor->mode_data.alert.position.z - actor->body_position.z;

        if (!(dz * dz + dy * dy + dx * dx < 0.25f)) {
            return;
        }
    }
    if (actor->mode_data.alert.wait_ticks > 0) {
        actor->mode_data.alert.wait_ticks = (int16_t)(actor->mode_data.alert.wait_ticks - 1);
    }
    if (actor->mode_data.alert.position_reached == 0) {
        return;
    }
    if (actor->mode_data.alert.animation_index != -1) {
        ScenarioAIAnimationReference *animation = &halo::ai::reflexive_data<ScenarioAIAnimationReference>(halo::scenario::globals().scenario->ai_animation_references)[actor->mode_data.alert.animation_index];
        datum_index graph = halo::ai::tag_handle(animation->animation_graph);

        if (graph == k_datum_index_none) {
            object *unit = (object *)halo::ai::object_at(actor->unit_index);

            graph = halo::ai::tag_handle(halo::ai::tag_data<Object>(unit->definition_tag)->animation_graph);
        }
        halo::units::unit_start_user_animation(actor->unit_index, graph, (const char *)animation, 1);
    }
    actor->mode_data.alert.position_reached = 0;
}

namespace halo::ai {
void actor_mode_alert_tick(uint32_t actor_index)
{
    halo::ai::alert_mode(actor_index).tick();
}
}


namespace c_actor_mode_alert_update {
}


/**
 * actor_mode_alert_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_update.c.txt.
 *
 * @address 0x401410
 */
void halo::ai::alert_mode::update()
{
    using namespace c_actor_mode_alert_update;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    actor->look_posture = 1;
    if (halo::ai::flag_set(halo::ai::tag_data<Actor>(actor->actor_definition_tag)->flags, halo::tags::actor_tag_flag::crouch_when_not_in_combat)) {
        actor->crouch_decision[0] = 1;
        actor->crouch_decision[1] = 1;
    }
}

namespace halo::ai {
void actor_mode_alert_update(uint32_t actor_index)
{
    halo::ai::alert_mode(actor_index).update();
}
}


namespace c_actor_mode_avoid_update {
}


/**
 * actor_mode_avoid_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_avoid_update.c.txt.
 *
 * @address 0x401850
 */
void halo::ai::avoid_mode::update()
{
    using namespace c_actor_mode_avoid_update;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->target_combat_status >= 5) {
        act->wants_to_fire = 1;
        act->flee_reason = 7;
        act->flee_source.code = 2;
    } else {
        act->flee_reason = 5;
        act->flee_source.code = act->danger_type > 0 ? 5 : 2;
    }
    act->look_posture = 4;
    act->crouch_decision[0] = act->crouch_active;
    act->crouch_decision[1] = 0;
    act->crouch_hold = 0;
    act->unknown_424[0] = 0;
    act->unknown_424[1] = 0;
}

namespace halo::ai {
void actor_mode_avoid_update(datum_index actor_index)
{
    halo::ai::avoid_mode(actor_index).update();
}
}


namespace c_actor_mode_converse_exit {

}


/**
 * actor_mode_converse_exit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_exit.c.txt.
 *
 * @address 0x402f40
 */
void halo::ai::converse_mode::exit()
{
    using namespace c_actor_mode_converse_exit;
    datum_index actor_index = datum;
    datum_index conversation = ((struct actor *)halo::ai::actor_bytes(actor_index))->conversation_index;

    if (conversation != k_datum_index_none) {
        halo::ai::ai_conversation_stop(conversation, 0, 0);
    }
}

namespace halo::ai {
void actor_mode_converse_exit(datum_index actor_index)
{
    halo::ai::converse_mode(actor_index).exit();
}
}


namespace c_actor_mode_converse_process {

}


/**
 * actor_mode_converse_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_process.c.txt.
 *
 * @address 0x402d70
 */
uint8_t halo::ai::converse_mode::process()
{
    using namespace c_actor_mode_converse_process;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    datum_index partner;

    if (!act->needs_new_path) {
        return act->mode_data.converse.finished;
    }
    if (act->mode_data.converse.partner_prop == k_datum_index_none && act->mode_data.converse.partner_unit != k_datum_index_none) {
        act->mode_data.converse.partner_prop = halo::ai::actor_find_or_create_shared_prop(act->mode_data.converse.partner_unit, actor_index, 1, 1);
    }
    partner = act->mode_data.converse.partner_prop;
    if (partner == k_datum_index_none) {
        act->mode_data.converse.finished = 1;
        return act->mode_data.converse.finished;
    }
    if (!act->mode_data.converse.arrived) {
        prop *p = halo::ai::prop_at(partner);
        float distance = p->distance;

        if ((p->visual_perception >= 2 && distance < act->mode_data.converse.approach_distance) || distance < 0.7f) {
            act->mode_data.converse.arrived = 1;
        }
    }
    if (act->mode_data.converse.arrived) {
        halo::ai::actor_movement_action_stop(actor_index);
        return act->mode_data.converse.finished;
    }
    if (!halo::ai::actor_movement_set_destination_near_target(partner, actor_index, act->mode_data.converse.approach_distance)) {
        act->mode_data.converse.finished = 1;
    }
    return act->mode_data.converse.finished;
}

namespace halo::ai {
uint8_t actor_mode_converse_process(datum_index actor_index)
{
    return halo::ai::converse_mode(actor_index).process();
}
}


namespace c_actor_mode_converse_replace_reference {
}


/**
 * actor_mode_converse_replace_reference: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_replace_reference.c.txt.
 *
 * @address 0x402f00
 */
void halo::ai::converse_mode::replace_reference(datum_index old_reference, datum_index new_reference)
{
    using namespace c_actor_mode_converse_replace_reference;
    datum_index actor_index = datum;
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;

    if (((actor_mode_converse_data *)mode_data)->partner_prop == old_reference) {
        ((actor_mode_converse_data *)mode_data)->partner_prop = new_reference;
    }
}

namespace halo::ai {
void actor_mode_converse_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    halo::ai::converse_mode(actor_index).replace_reference(old_reference, new_reference);
}
}


namespace c_actor_mode_converse_update {

}


/**
 * actor_mode_converse_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_update.c.txt.
 *
 * @address 0x402e70
 */
void halo::ai::converse_mode::update()
{
    using namespace c_actor_mode_converse_update;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    datum_index conversation = act->mode_data.converse.conversation;
    uint8_t *record = 0;
    datum_index look_prop = k_datum_index_none;

    if (conversation != k_datum_index_none) {
        record = (uint8_t *)halo::ai::globals().conversation_data->data + (conversation & halo::k_slot_mask) * k_ai_conversation_size;
    }
    if (act->mode_data.converse.partner_prop != k_datum_index_none) {
        look_prop = act->mode_data.converse.partner_prop;
    } else if (record != 0 && *(datum_index *)(record + 0x10) != k_datum_index_none) {
        look_prop = halo::ai::actor_find_prop_for_object(*(datum_index *)(record + 0x10), actor_index);
    }
    act->look_posture = 1;
    if (look_prop != k_datum_index_none) {
        act->flee_reason = 3;
        act->flee_source.code = 1;
        act->flee_source.payload.handle = look_prop;
    }
}

namespace halo::ai {
void actor_mode_converse_update(datum_index actor_index)
{
    halo::ai::converse_mode(actor_index).update();
}
}


namespace c_actor_mode_obey_enter {
}


/**
 * actor_mode_obey_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_enter.c.txt.
 *
 * @address 0x407280
 */
void halo::ai::obey_mode::enter()
{
    using namespace c_actor_mode_obey_enter;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    halo::ai::actor_swarm_for_each_component(actor_index, 0, halo::ai::actor_obey_member_enter, 0, &actor->mode_data.obey);
}

namespace halo::ai {
void actor_mode_obey_enter(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).enter();
}
}

namespace c_actor_mode_obey_exit {
}


/**
 * actor_mode_obey_exit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_exit.c.txt.
 *
 * @address 0x4072c0
 */
void halo::ai::obey_mode::exit()
{
    using namespace c_actor_mode_obey_exit;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    halo::ai::actor_swarm_for_each_component(actor_index, 0, halo::ai::actor_obey_member_exit, 0, &actor->mode_data.obey);
}

namespace halo::ai {
void actor_mode_obey_exit(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).exit();
}
}

namespace c_actor_mode_obey_process {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}


/**
 * actor_mode_obey_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_process.c.txt.
 *
 * @address 0x407340
 */
uint8_t halo::ai::obey_mode::process()
{
    using namespace c_actor_mode_obey_process;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    actor_mode_obey_data *mode_data = &actor->mode_data.obey;
    uint8_t still_running = 1;

    halo::ai::actor_swarm_for_each_component(actor_index, 0, halo::ai::actor_squad_action_list_process,
        (uint32_t)&still_running, mode_data);
    if (still_running && mode_data->finished == 0) {
        uint8_t *list = (uint8_t *)halo::scenario::globals().scenario->command_lists.pointer + mode_data->command_list_index * 0x60;
        int mark = 1;

        if ((list[0x20] & 0x10) && actor->airborne != 0) {
            Actor *actor_tag = halo::ai::tag_data<Actor>(actor->actor_definition_tag);

            if (!halo::ai::flag_set(actor_tag->flags, halo::tags::actor_tag_flag::flying)) {
                mark = 0;
            }
        }
        if (mark) {
            actor->command_list_finished_time = game_time->game_time;
            mode_data->finished = 1;
        }
    }
    return (uint8_t)(actor->mode == halo::ai::actor_mode::obey && mode_data->finished != 0);
}

namespace halo::ai {
uint8_t actor_mode_obey_process(uint32_t actor_index)
{
    return halo::ai::obey_mode(actor_index).process();
}
}

namespace c_actor_mode_obey_tick_members {
}


/**
 * actor_mode_obey_tick_members: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_tick_members.c.txt.
 *
 * @address 0x407300
 */
void halo::ai::obey_mode::tick_members()
{
    using namespace c_actor_mode_obey_tick_members;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    halo::ai::actor_swarm_for_each_component(actor_index, 0, halo::ai::actor_obey_member_tick, 0, &actor->mode_data.obey);
}

namespace halo::ai {
void actor_mode_obey_tick_members(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).tick_members();
}
}

namespace c_actor_mode_obey_update {
static auto &global_forward2d_pointer = halo::link::ref<real_vector2d *>(halo::ai::vars().global_forward2d_pointer);

}


/**
 * actor_mode_obey_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_update.c.txt.
 *
 * @address 0x407400
 */
void halo::ai::obey_mode::update()
{
    using namespace c_actor_mode_obey_update;
    uint32_t actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);

    if (actor->mode_data.obey.aim.shoot_valid != 0) {
        actor->flee_reason = 7;
        actor->flee_source.code = 2;
        actor->look_posture = 4;
        actor->wants_to_fire = 1;
        actor->force_fire = 1;
        actor->forced_aim_valid = 1;
        actor->forced_aim_point = actor->mode_data.obey.aim.shoot_point;
        actor->burst_duration_override = actor->mode_data.obey.aim.burst_duration;
    } else if (actor->mode_data.obey.aim.look_valid != 0 && (actor->movement_action_complete == 0 || actor->movement_completed != 0)) {
        actor->flee_reason = 4;
        actor->flee_source.code = 3;
        actor->flee_source.payload.point = actor->mode_data.obey.aim.look_point;
        if (actor->flying == 0) {
            actor->flee_source.payload.point.z = actor->aim_origin.z;
        }
        actor->look_posture = (int16_t)(actor->awareness_level < 3 ? 1 : 4);
    } else if (actor->mode_data.obey.aim.movement_style == 3 || actor->mode_data.obey.aim.movement_style == 1) {
        actor->flee_source.code = 0;
        actor->look_posture = 0;
        actor->flee_reason = 7;
    } else if (actor->combat_status >= 5 && (actor->mode_data.obey.action.flags & 1)) {
        actor->flee_source.code = 2;
        actor->look_posture = 4;
        actor->wants_to_fire = 1;
        actor->flee_reason = 7;
    } else {
        actor->flee_reason = 0;
        if (actor->mode_data.obey.allow_look != 0) {
            actor->look_posture = (int16_t)(actor->awareness_level < 3 ? 1 : 4);
        } else {
            actor->look_posture = 0;
        }
    }

    if (actor->mode_data.obey.aim.grenade_pending != 0) {
        actor->throw_grenade = 1;
        actor->mode_data.obey.aim.grenade_pending = 0;
    }
    actor->crouch_decision[0] = actor->mode_data.obey.aim.crouch;
    actor->crouch_decision[1] = actor->mode_data.obey.aim.crouch;
    actor->movement_style_override = actor->mode_data.obey.aim.movement_style;

    if (actor->mode_data.obey.aim.secondary_action_pending != 0 && actor->secondary_action == -1 &&
        (actor->unit_index == halo::k_dword_none || !halo::units::unit_is_in_busy_animation_state(actor->unit_index))) {
        if (actor->mode_data.obey.aim.secondary_action != -1) {
            real_vector2d direction;

            direction.i = actor->desired_facing_vector.x;
            direction.j = actor->desired_facing_vector.y;
            halo::math::vector2d_normalize_with_length(direction);
            halo::ai::actor_queue_secondary_action(actor_index, actor->mode_data.obey.aim.secondary_action, &direction);
        }
        if (actor->mode_data.obey.aim.communication_line != -1) {
            halo::ai::ai_communication_broadcast(actor->mode_data.obey.aim.communication_line, actor->unit_index, halo::k_dword_none, -1, halo::k_dword_none, halo::k_dword_none, 0);
        }
        actor->mode_data.obey.aim.secondary_action_pending = 0;
    }

    if (actor->mode_data.obey.action.movement_flags & 1) {
        actor->move_in_direction = 1;
        actor->move_direction = actor->mode_data.obey.action.direction;
        actor->strafe_axis_override = actor->mode_data.obey.action.axis;
    }
    if ((actor->mode_data.obey.action.movement_flags & 4) == 0) {
        return;
    }
    if (actor->mode_data.obey.action.movement_flags & 8) {
        if (!(actor->mode_data.obey.action.axis > 0)) {
            return;
        }
    } else if (actor->mode_data.obey.action.axis == 0 && actor->airborne == 0 && !halo::units::unit_is_in_busy_animation_state(actor->unit_index)) {
        real_vector2d direction;
        float x;
        float y;

        direction = *(real_vector2d *)&actor->facing.i;
        if (halo::math::vector2d_normalize_with_length(direction) == 0.0f) {
            x = global_forward2d_pointer->i;
            y = global_forward2d_pointer->j;
        } else {
            x = direction.i;
            y = direction.j;
        }
        actor->jump_requested = 1;
        actor->jump_is_leap = (uint8_t)(actor->mode_data.obey.action.jump.horizontal_speed * 0.7f > actor->mode_data.obey.action.jump.vertical_speed);
        actor->jump_parameters_valid = (uint8_t)((actor->mode_data.obey.action.movement_flags >> 4) & 1);
        actor->jump_facing.j = y;
        actor->jump_facing.i = x;
        actor->jump_horizontal_velocity = actor->mode_data.obey.action.jump.horizontal_speed;
        actor->jump_vertical_velocity = actor->mode_data.obey.action.jump.vertical_speed;
        actor->mode_data.obey.action.movement_flags = (uint8_t)(actor->mode_data.obey.action.movement_flags | 8);
        if ((actor->mode_data.obey.action.movement_flags & 0x10) == 0) {
            actor->mode_data.obey.action.axis = 0xf;
        }
        return;
    }
    actor->move_direction = actor->facing;
    actor->move_in_direction = 1;
    actor->strafe_axis_override = 0;
}

namespace halo::ai {
void actor_mode_obey_update(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).update();
}
}

namespace c_actor_mode_search_enter {

}


/**
 * actor_mode_search_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_enter.c.txt.
 *
 * @address 0x407940
 */
void halo::ai::search_mode::enter()
{
    using namespace c_actor_mode_search_enter;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    float lo;
    float hi;
    float t;
    int32_t ticks;

    if (act->mode_data.search.stage == 0) {
        lo = actor_tag->target_search_time[0];
        hi = actor_tag->target_search_time[1];
    } else {
        lo = actor_tag->pursuit_position_time[0];
        hi = actor_tag->pursuit_position_time[1];
    }
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    t = (float)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f;
    ticks = (int32_t)(((hi - lo) * t + lo) * 30.0f);
    act->mode_data.search.duration_ticks = ticks;
    act->mode_data.search.remaining_ticks = ticks;
}

namespace halo::ai {
void actor_mode_search_enter(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).enter();
}
}


namespace c_actor_mode_search_movement_cancelled {
}


/**
 * actor_mode_search_movement_cancelled: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_movement_cancelled.c.txt.
 *
 * @address 0x407f00
 */
void halo::ai::search_mode::movement_cancelled()
{
    using namespace c_actor_mode_search_movement_cancelled;
    datum_index actor_index = datum;
    actor_mode_data *mode_data = &halo::ai::actor_at(actor_index)->mode_data;

    if (((actor_mode_search_data *)mode_data)->stage == 1) {
        ((actor_mode_search_data *)mode_data)->firing_position = -1;
        mode_data->search.finished = 1;
    }
}

namespace halo::ai {
void actor_mode_search_movement_cancelled(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).movement_cancelled();
}
}


namespace c_actor_mode_search_process {

}


/**
 * actor_mode_search_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_process.c.txt.
 *
 * @address 0x407a10
 */
uint8_t halo::ai::search_mode::process()
{
    using namespace c_actor_mode_search_process;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    int16_t kind;
    uint8_t ok;

    if (act->swarm || !act->needs_new_path || act->mode_data.search.finished) {
        return act->mode_data.search.finished;
    }
    kind = act->mode_data.search.stage;
    act->mode_data.search.reachable = 1;
    if (kind == 0 && act->target_unit_index != k_datum_index_none) {
        prop *target = halo::ai::prop_at(act->target_unit_index);
        float radius = ((struct actor *)target)->original_squad_index == 0 ? 1.7f : 0.7f;
        float distance_squared = halo::math::vector3d_distance_squared(act->body_position, target->last_known_position);

        act->mode_data.search.reachable = (uint8_t)(radius * radius > distance_squared);
    } else if (kind == 1 && act->mode_data.search.firing_position != -1) {
        float distance_squared = halo::math::vector3d_distance_squared(act->body_position, *(&act->mode_data.search.position));

        if (distance_squared < 0.49f) {
            act->mode_data.search.reachable = 1;
        } else if (distance_squared > 6.25f) {
            act->mode_data.search.reachable = 0;
        } else {
            real_point3d in_view;

            halo::units::unit_add_marker_relative_offset(act->unit_index, 1, &act->mode_data.alert.position.z, 0, 0, &in_view);
            act->mode_data.search.reachable = (uint8_t)(halo::ai::actor_evaluate_engagement_reachability(act->location.cluster_index, act->mode_data.search.target_cluster,
                                                                         &in_view, &act->aim_origin, 0, 0, -1,
                                                                         (uint8_t)(act->active_unit_index !=
                                                                                   k_datum_index_none)) == 0);
        }
    }
    if (!act->mode_data.search.reachable && !act->mode_data.search.unknown_05) {
        actor_prop_iterator iterator;
        datum_index prop_index;
        int16_t sharing = 0;
        int32_t close_idle = 0;

        halo::ai::actor_prop_iterator_init(actor_index, &iterator);
        for (prop_index = iterator.next; prop_index != k_datum_index_none;) {
            prop *p = halo::ai::prop_at(prop_index);
            int16_t prop_kind = p->state;

            prop_index = p->next_in_actor;
            if (prop_kind >= 2 && prop_kind <= 3 && !p->enemy && !p->dead &&
                p->owner_actor_index != k_datum_index_none &&
                halo::ai::actor_targets_share_descriptor(actor_index, p->owner_actor_index)) {
                actor *other = halo::ai::actor_at(p->owner_actor_index);

                sharing++;
                if (!other->swarm && !other->moving &&
                    halo::math::vector3d_distance_squared(other->body_position, act->body_position) < 0.64000005f) {
                    close_idle++;
                }
            }
        }
        if (!act->mode_data.search.unknown_04 && sharing >= (act->mode_data.search.stage != 1 ? 4 : 2)) {
            datum_index last_seen = -1;

            act->mode_data.search.finished = 1;
            if (act->target_unit_index != k_datum_index_none) {
                last_seen = static_cast<datum_index>(halo::ai::prop_at(act->target_unit_index)->last_perceived_time);
            }
            if (act->encounter_index != k_datum_index_none) {
                halo::ai::ai_pursuit_note_object(actor_index, act->encounter_index, act->mode_data.search.firing_position, last_seen);
            }
        } else if ((int16_t)close_idle > 0) {
            act->mode_data.search.reachable = 1;
        }
    }
    if (act->mode_data.search.reachable) {
        halo::ai::actor_movement_action_stop(actor_index);
        return act->mode_data.search.finished;
    }
    kind = act->mode_data.search.stage;
    if (kind == 0) {
        ok = halo::ai::actor_movement_set_destination_near_target(act->target_unit_index, actor_index, 2.5f);
    } else if (kind == 1) {
        act->firing_position_index = -1;
        ok = halo::ai::actor_movement_set_destination_firing_position(actor_index, act->mode_data.search.firing_position, 0);
    } else {
        halo::ai::actor_movement_action_stop(actor_index);
        return act->mode_data.search.finished;
    }
    if (!ok) {
        act->mode_data.search.finished = 1;
        act->mode_data.search.unknown_01 = 1;
    }
    return act->mode_data.search.finished;
}

namespace halo::ai {
uint8_t actor_mode_search_process(datum_index actor_index)
{
    return halo::ai::search_mode(actor_index).process();
}
}


namespace c_actor_mode_search_tick {

}


/**
 * actor_mode_search_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_tick.c.txt.
 *
 * @address 0x407d80
 */
void halo::ai::search_mode::tick()
{
    using namespace c_actor_mode_search_tick;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag;
    datum_index unit_index;

    if (act->mode_data.search.finished) {
        return;
    }
    actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    if (actor_tag->defensive_crouch_type == 4) {
        act->mode_data.search.unknown_03 = 1;
    } else {
        act->mode_data.search.unknown_03 = 0;
        if ((static_cast<uint8_t>(actor_tag->flags) & 2) && act->mode_data.search.stage == 0 && act->target_combat_status == 5 &&
            (int8_t)halo::ai::prop_at(act->target_unit_index)->distance_class <= 2) {
            act->mode_data.search.unknown_03 = 1;
        }
    }
    if (!act->mode_data.search.reachable) {
        if (!act->moving && !act->swarm) {
            act->mode_data.search.elapsed_ticks += 1;
            if (act->mode_data.search.elapsed_ticks > 120) {
                act->mode_data.search.unknown_01 = 1;
                act->mode_data.search.finished = 1;
            }
        }
        return;
    }
    if (act->mode_data.search.remaining_ticks > 0) {
        act->mode_data.search.remaining_ticks -= 1;
    }
    if (act->mode_data.search.remaining_ticks == 0) {
        act->mode_data.search.finished = 1;
    }
    unit_index = act->unit_index;
    if (unit_index == k_datum_index_none) {
        return;
    }
    if (act->mode_data.search.stage == 0) {
        if (act->target_lost_reported) {
            return;
        }
        if (act->mode_data.search.finished || act->mode_data.search.remaining_ticks + 90 < act->mode_data.search.duration_ticks) {
            halo::ai::ai_communication_broadcast(0xd, unit_index, halo::ai::actor_get_target_prop_object_index(actor_index), -1, -1, -1, 0);
            act->target_lost_reported = 1;
        }
    } else if (act->mode_data.search.remaining_ticks == 0) {
        halo::ai::ai_communication_broadcast(0x12, unit_index, halo::ai::actor_get_target_prop_object_index(actor_index), -1, -1, -1, 0);
    }
}

namespace halo::ai {
void actor_mode_search_tick(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).tick();
}
}


namespace c_actor_mode_search_update {
}


/**
 * actor_mode_search_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_update.c.txt.
 *
 * @address 0x407f40
 */
void halo::ai::search_mode::update()
{
    using namespace c_actor_mode_search_update;
    datum_index actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);

    if (act->moving) {
        act->flee_reason = 3;
        act->flee_source.code = 0;
    } else {
        int32_t total = act->mode_data.search.duration_ticks;
        int32_t third = total / 3;

        if (third <= 90) {
            third = 90;
        }
        if (total - act->mode_data.search.remaining_ticks < third && act->mode_data.search.stage == 0) {
            act->flee_reason = 3;
            act->flee_source.code = 2;
        } else if (total - act->mode_data.search.remaining_ticks < third && act->mode_data.search.stage == 1) {
            act->flee_reason = 3;
            act->flee_source.code = 3;
            act->flee_source.payload.point = act->mode_data.search.position;
        } else {
            act->flee_reason = 1;
        }
    }
    act->look_posture = 3;
    if (act->mode_data.search.stage == 0) {
        act->wants_to_fire = (uint8_t)(act->target_combat_status >= ((static_cast<uint8_t>(actor_tag->flags) & 0x10) ? 5 : 6));
    }
    act->crouch_decision[0] = act->mode_data.search.unknown_03;
    act->crouch_decision[1] = act->mode_data.search.unknown_03;
    act->crouch_hold = 0;
    act->unknown_424[0] = 0;
    act->unknown_424[1] = 1;
}

namespace halo::ai {
void actor_mode_search_update(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).update();
}
}


namespace c_actor_mode_sleep_update {
}


/**
 * actor_mode_sleep_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_sleep_update.c.txt.
 *
 * @address 0x408090
 */
void halo::ai::sleep_mode::update()
{
    using namespace c_actor_mode_sleep_update;
    datum_index actor_index = datum;
    halo::ai::actor_at(actor_index)->look_posture = 0;
}

namespace halo::ai {
void actor_mode_sleep_update(datum_index actor_index)
{
    halo::ai::sleep_mode(actor_index).update();
}
}


