#include "halo/units/animation_states.hpp"
#include "halo/objects/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/ai_constants.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/core/x87.hpp"
#include "halo/core/libm.hpp"
#include "halo/networking/api.hpp"

namespace halo::ai {

/**
 * Actor AI behaviour: obey member advance.
 *
 * @address 0x406f80
 */
void ActorView::obey_member_advance(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    (void)actor_index;
    (void)unit_index;
    (void)command_list_index;
    (void)aim;
    (void)callback_extra;
    action->flags = (uint8_t)((action->flags & 0xef) | 8);
}

namespace actor_obey_member_enter_local {
}

/**
 * Actor AI behaviour: obey member enter.
 *
 * @address 0x406f30
 */
void ActorView::obey_member_enter(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    using namespace actor_obey_member_enter_local;
    ScenarioCommandList *list = &halo::ai::reflexive_data<ScenarioCommandList>(halo::scenario::globals().scenario->command_lists)[(int16_t)command_list_index];

    (void)actor_index;
    (void)action;
    (void)aim;
    (void)callback_extra;
    if (list->flags & 0x10) {
        unit_object *unit = (unit_object *)halo::ai::object_at(unit_index);

        ((unit_object *)unit)->unit.flags |= halo::to_bits(halo::units::unit_flag::no_falling_damage);
    }
}

namespace actor_obey_member_exit_local {
}

/**
 * Actor AI behaviour: obey member exit.
 *
 * @address 0x406fa0
 */
void ActorView::obey_member_exit(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    using namespace actor_obey_member_exit_local;
    unit_object *unit = (unit_object *)halo::ai::object_at(unit_index);

    (void)callback_extra;
    if ((action->flags & 2) == 0) {
        uint8_t next_action = 0;

        halo::ai::actor_squad_action_reset_entry(actor_index, unit_index, action, (int16_t)command_list_index,
            aim, &next_action);
    }
    ((unit_object *)unit)->unit.flags &= ~halo::to_bits(halo::units::unit_flag::no_falling_damage);
}

namespace actor_obey_member_tick_local {
}

/**
 * Actor AI behaviour: obey member tick.
 *
 * @address 0x406ff0
 */
void ActorView::obey_member_tick(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    using namespace actor_obey_member_tick_local;
    uint8_t flags;

    (void)command_list_index;
    (void)aim;
    (void)callback_extra;
    if (action->timer_ticks > 0) {
        action->timer_ticks = (int16_t)(action->timer_ticks - 1);
    }
    flags = action->movement_flags;
    if ((flags & 4) && action->axis > 0) {
        action->axis = (int16_t)(action->axis - 1);
    }
    if ((flags & 1) && (flags & 2)) {
        halo::ai::actor_get_body_axis_vector(actor_index, unit_index, action);
    }
}

namespace actor_squad_action_execute_local {
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &k_empty_string = halo::link::ref<const char []>(halo::networking::vars().k_empty_string);
}

/**
 * state: +0x00 command index (byte), +0x02 timer ticks, +0x04 flags, +0x05 movement flags, +0x08 kind, +0x0c
 * direction / facing, +0x18 start position. aim_state (EAX): the mode's aim / look / fire record.
 *
 * @address 0x405520
 */
char ActorOps::squad_action_execute(actor_command_aim *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state)
{
    using namespace actor_squad_action_execute_local;
    actor *act = halo::ai::actor_at(actor_index);
    Actor *actor_tag = halo::ai::tag_data<Actor>(act->actor_definition_tag);
    ActorVariant *variant_tag = halo::ai::tag_data<ActorVariant>(act->actor_variant_tag);
    ScenarioCommandList *list = &((ScenarioCommandList *)halo::scenario::globals().scenario->command_lists.pointer)[command_list_index];
    int32_t command_count = (int32_t)list->commands.count;
    int32_t command_index = state->command_index;
    ScenarioCommand *entry;
    ScenarioCommandPoint *points = (ScenarioCommandPoint *)list->points.pointer;
    int32_t point_count = (int32_t)list->points.count;
    datum_index unit_index = act->unit_index;
    char result = 0;

    if (command_index >= command_count) {
        return 0;
    }
    entry = &((ScenarioCommand *)list->commands.pointer)[command_index];

    switch (entry->atom_type) {
    case 0:
        state->timer_ticks = (int16_t)(int32_t)(entry->parameter1 * 30.0f);
        return 1;

    case 1:
    case 2: {
        int16_t point_index = (int16_t)entry->point_1;
        ScenarioCommandPoint *point;

        if (aim_state == 0 || point_index < 0 || point_index >= point_count) {
            return 0;
        }
        point = &points[point_index];
        state->timer_ticks = 0;
        aim_state->move_requested = 1;
        aim_state->move_interrupts = (uint8_t)(entry->atom_modifier == 1);
        *&aim_state->move_point = *(real_point3d *)&point->position;
        aim_state->move_surface_index = (int32_t)point->surface_index;
        result = (char)halo::ai::actor_movement_set_destination_point(&aim_state->move_point, actor_index,
                                                            (int32_t)point->surface_index, halo::k_dword_none);
        if (!result) {
            return 0;
        }
        if (aim_state->move_interrupts) {
            halo::ai::actor_movement_actions_cancel(actor_index);
        }
        if (entry->atom_type == 2) {
            int16_t face_index = (int16_t)entry->point_2;

            if (face_index >= 0 && face_index < point_count) {
                aim_state->look_valid = 1;
                *&aim_state->look_point = *(real_point3d *)&points[face_index].position;
            }
        }
        return result;
    }

    case 3: {
        real_point3d *start = &state->start_position;
        int16_t point_index = (int16_t)entry->point_1;
        int16_t kind;

        if (check_object_index == unit_index) {
            *start = *(real_point3d *)&act->body_position.x;
        } else {
            halo::objects::object_get_position(start, check_object_index);
        }
        if (point_index >= 0 && point_index < point_count) {
            real_vector3d *direction = &state->direction;

            direction->i = points[point_index].position.x - start->x;
            direction->j = points[point_index].position.y - start->y;
            direction->k = points[point_index].position.z - start->z;
            if (!(halo::math::vector3d_normalize_with_length(*direction) > 0.0f)) {
                return 0;
            }
        } else {
            double angle;

            if (entry->parameter2 < 0.0f || !(entry->parameter2 <= 360.0f)) {
                return 0;
            }
            angle = entry->parameter2 * 0.017453292f;
            state->direction.k = 0.0f;
            state->direction.i = (float)halo::x87::fcos(angle);
            state->direction.j = (float)halo::x87::fsin(angle);
        }
        kind = entry->atom_modifier;
        state->axis = (kind >= 0 && kind <= 3) ? kind : -1;
        if (check_object_index == unit_index) {
            halo::ai::actor_movement_action_stop(actor_index);
        }
        state->movement_flags = (uint8_t)((state->movement_flags & 0xfd) | 1);
        return 1;
    }

    case 4:
    case 0x17:
    case 0x18:
    case 0x19: {
        int32_t look_point = -1;
        datum_index look_prop = k_datum_index_none;
        datum_index look_object = k_datum_index_none;
        float duration = entry->parameter1;
        int16_t variant;
        actor_vocalization_context context;

        if (aim_state == 0) {
            return 0;
        }
        if (entry->atom_type == 4) {
            int16_t p = (int16_t)entry->point_1;

            if (p >= 0 && p < point_count) {
                look_point = (uint16_t)p;
                duration = entry->parameter1;
            }
        } else if (entry->atom_type == 0x17) {
            int16_t p1 = (int16_t)entry->point_1;
            int16_t p2 = (int16_t)entry->point_2;

            if (p1 >= 0 && p1 < point_count && p2 >= 0 && p2 < point_count) {
                look_point = halo::math::random_int_range(p1, (int16_t)(p2 + 1));
                if (entry->parameter1 == 0.0f && entry->parameter2 == 0.0f) {
                    duration = halo::math::random_real_range(actor_tag->noncombat_idle_looking[0], actor_tag->noncombat_idle_looking[1]);
                } else {
                    duration = halo::math::random_real_range(entry->parameter1, entry->parameter2);
                }
            }
        } else if (entry->atom_type == 0x18) {
            actor_prop_iterator iterator;
            prop *p;
            float nearest = 3.4028235e38f;

            halo::ai::actor_prop_iterator_init(actor_index, &iterator);
            for (p = halo::ai::actor_prop_iterator_next(&iterator); p != 0; p = halo::ai::actor_prop_iterator_next(&iterator)) {
                if (p->state >= 2 && p->state <= 3 && p->is_parented && nearest > p->distance) {
                    look_prop = iterator.current;
                    nearest = p->distance;
                }
            }
            if (look_prop == k_datum_index_none) {
                data_iterator players;
                struct player *player;
                float best = 3.4028235e38f;

                players.data = halo::game::globals().player_data;
                players.next_index = 0;
                players.index = k_datum_index_none;
                players.signature = (uint32_t)(uintptr_t)halo::game::globals().player_data ^ halo::ai::k_iterator_signature_key;
                for (player = (struct player *)halo::memory::data_iterator_next(&players); player != 0; player = (struct player *)halo::memory::data_iterator_next(&players)) {
                    datum_index player_unit = player->unit;

                    if (player_unit != k_datum_index_none) {
                        real_point3d eye;
                        float distance;

                        halo::units::unit_get_primary_eye_marker_position(player_unit, &eye);
                        distance = halo::math::vector3d_distance_squared(act->aim_origin, eye);
                        if (distance <= best) {
                            best = distance;
                            look_object = player_unit;
                        }
                    }
                }
            }
        } else if (entry->atom_type == 0x19) {
            int16_t name = (int16_t)entry->object_name;

            if (name >= 0 && name < static_cast<int32_t>(halo::scenario::globals().scenario->object_names.count)) {
                datum_index object_index = halo::objects::object_lookup_table_get(name);

                if (halo::objects::object_try_and_get(object_index, 3) != 0) {
                    look_prop = halo::ai::actor_find_prop_for_object(object_index, actor_index);
                    look_object = object_index;
                }
            }
        }
        if (!(duration > 0.0f)) {
            return 0;
        }
        if (look_prop == k_datum_index_none && look_object == k_datum_index_none &&
            ((int16_t)look_point < 0 || (int16_t)look_point >= point_count)) {
            return 0;
        }
        switch (entry->atom_modifier) {
        case 1: variant = 5; break;
        case 2: variant = 2; break;
        case 4: variant = 7; break;
        case 3: variant = 8; break;
        default: variant = 1; break;
        }
        memset(&context, 0, sizeof(context));
        if (look_prop != k_datum_index_none) {
            context.code = 1;
            context.payload.handle = look_prop;
        } else {
            context.code = 3;
            if (look_object != k_datum_index_none) {
                halo::units::unit_get_primary_eye_marker_position(look_object, &context.payload.point);
            } else {
                context.payload.point = *(real_point3d *)&points[(int16_t)look_point].position;
            }
        }
        halo::ai::actor_begin_vocalization(actor_index, 0xd, variant, &context);
        state->timer_ticks = (int16_t)(int32_t)(duration * 30.0f);
        return 1;
    }

    case 5:
        if (aim_state == 0 || entry->atom_modifier < 0 || entry->atom_modifier >= 4) {
            return 0;
        }
        aim_state->movement_style = entry->atom_modifier;
        return 1;

    case 6:
        if (aim_state == 0) {
            return 0;
        }
        aim_state->crouch = (uint8_t)(entry->atom_modifier == 1);
        return 1;

    case 7: {
        int16_t p = (int16_t)entry->point_1;

        if (aim_state == 0 || p < 0 || p >= point_count) {
            return 0;
        }
        aim_state->shoot_valid = 1;
        *&aim_state->shoot_point = *(real_point3d *)&points[p].position;
        aim_state->burst_duration = entry->parameter1;
        return 1;
    }

    case 8: {
        int16_t grenade_type;
        int16_t p = (int16_t)entry->point_1;

        if (aim_state == 0) {
            return 0;
        }
        grenade_type = variant_tag->grenade_type;
        if (grenade_type == -1 || p < 0 || p >= point_count) {
            return 0;
        }
        halo::units::unit_set_grenade_type_and_count_delta(unit_index, grenade_type, 1);
        aim_state->grenade_thrown = 0;
        aim_state->grenade_pending = 0;
        *&aim_state->grenade_target = *(real_point3d *)&points[p].position;
        aim_state->grenade_style = 0;
        if (entry->atom_modifier >= 0 && entry->atom_modifier < 3) {
            aim_state->grenade_style = entry->atom_modifier;
        }
        state->timer_ticks = 0x3c;
        return 1;
    }

    case 9: {
        object_iterator it;
        struct { float distance_squared; datum_index vehicle_index; } samples[16];
        int16_t sample_count = 0;
        int16_t seat_flags = -1;
        int16_t i;

        if (check_object_index != unit_index) {
            return 0;
        }
        it.type_mask = 2;
        it.flags_mask = 0;
        it.index = 0;
        it.handle = k_datum_index_none;
        while (halo::objects::object_iterator_next(&it) != 0) {
            datum_index vehicle_index = it.handle;
            real_point3d position;
            float distance;

            halo::objects::object_get_position(&position, vehicle_index);
            distance = halo::math::vector3d_distance_squared(position, act->body_position);
            if (entry->parameter1 == 0.0f || distance <= entry->parameter1 * entry->parameter1) {
                samples[sample_count].distance_squared = distance;
                samples[sample_count].vehicle_index = vehicle_index;
                sample_count++;
                if (sample_count >= 16) {
                    break;
                }
            }
        }
        if (sample_count > 1) {
            qsort(samples, sample_count, 8, halo::math::float_compare_ascending);
        }
        if (entry->atom_modifier >= 0 && entry->atom_modifier < 5) {
            seat_flags = entry->atom_modifier;
        }
        for (i = 0; i < sample_count; i++) {
            if (halo::ai::actor_play_first_valid_vocalization(0, samples[i].vehicle_index, actor_index, (char *)k_empty_string,
                                                    seat_flags, 0)) {
                state->flags |= 4;
                return 1;
            }
        }
        return 0;
    }

    case 0xa: {
        uint8_t moving_forward;

        if (check_object_index == unit_index && act->active_unit_index != k_datum_index_none) {
            return 0;
        }
        state->movement_flags = (uint8_t)((state->movement_flags & 0xe7) | 4);
        if (check_object_index == unit_index) {
            moving_forward = (uint8_t)(act->moving != 0 && act->moving_facing_direction == 0);
        } else {
            uint8_t *obj = halo::ai::object_bytes(check_object_index);

            moving_forward = (uint8_t)(((object *)obj)->parent_object == k_datum_index_none &&
                                       ((object *)obj)->velocity.k * ((object *)obj)->forward.k +
                                       ((object *)obj)->velocity.j * ((object *)obj)->forward.j +
                                       ((object *)obj)->forward.i * ((object *)obj)->velocity.i > 0.06666667f);
        }
        state->timer_ticks = 0x3c;
        state->axis = moving_forward ? 0 : 10;
        return 1;
    }

    case 0xb:
        if (check_object_index == unit_index && act->active_unit_index != k_datum_index_none) {
            return 0;
        }
        state->movement_flags = (uint8_t)((state->movement_flags & 0xf7) | 0x14);
        state->axis = 0;
        state->direction.i = entry->parameter1;
        state->direction.j = entry->parameter2;
        state->timer_ticks = 0x3c;
        return 1;

    case 0xc: {
        int16_t script = (int16_t)entry->script;

        if (script < 0 || script >= static_cast<int32_t>(halo::scenario::globals().scenario->ai_script_references.count)) {
            return 0;
        }
        return halo::hs::hs_call_script_by_name(halo::ai::reflexive_data<char>(halo::scenario::globals().scenario->ai_script_references) + script * 0x28);
    }

    case 0xd: {
        ScenarioAIAnimationReference *reference;
        datum_index graph;
        uint8_t interpolate = 1;
        uint8_t flag_4 = 0;
        uint8_t flag_8 = 0;
        biped_object *biped;

        if ((int16_t)entry->animation == -1) {
            return 0;
        }
        reference = &halo::ai::reflexive_data<ScenarioAIAnimationReference>(halo::scenario::globals().scenario->ai_animation_references)[(int16_t)entry->animation];
        graph = halo::ai::tag_handle(reference->animation_graph);
        if (graph == k_datum_index_none) {
            graph = halo::ai::tag_handle(halo::ai::tag_data<Object>(halo::ai::object_at(check_object_index)->definition_tag)->animation_graph);
        }
        switch (entry->atom_modifier) {
        case 1: flag_4 = 1; break;
        case 2: flag_8 = 1; flag_4 = 1; break;
        case 3: interpolate = 0; break;
        case 4: interpolate = 0; flag_4 = 1; break;
        case 5: interpolate = 0; flag_8 = 1; flag_4 = 1; break;
        default: break;
        }
        if (!halo::units::unit_start_user_animation(check_object_index, graph, (const char *)reference, interpolate)) {
            return 0;
        }
        biped = (biped_object *)halo::objects::object_try_and_get(check_object_index, 1);
        if (biped != 0) {
            uint32_t *flags = &biped->biped.flags;

            *flags = flag_4 ? (*flags | 4) : (*flags & ~4u);
            *flags = flag_8 ? (*flags | 8) : (*flags & ~8u);
        }
        return 1;
    }

    case 0xe: {
        int16_t recording = (int16_t)entry->recording;
        int16_t animation_index;

        if (recording < 0 || recording >= static_cast<int32_t>(halo::scenario::globals().scenario->ai_recording_references.count)) {
            return 0;
        }
        animation_index = halo::cutscene::recorded_animation_find_by_name(
            halo::ai::reflexive_data<char>(halo::scenario::globals().scenario->ai_recording_references) + recording * 0x28, halo::scenario::globals().scenario);
        if (animation_index == -1) {
            return 0;
        }
        return (char)halo::cutscene::recorded_animation_start(check_object_index, animation_index, 0);
    }

    case 0xf:
        if (aim_state == 0) {
            return 0;
        }
        aim_state->secondary_action_pending = 0;
        switch (entry->atom_modifier) {
        case 0: aim_state->secondary_action = 0; aim_state->communication_line = 0x2a; aim_state->secondary_action_pending = 1; break;
        case 1: aim_state->secondary_action = 4; aim_state->communication_line = 0x29; aim_state->secondary_action_pending = 1; break;
        case 2: aim_state->secondary_action = 5; aim_state->communication_line = 0x29; aim_state->secondary_action_pending = 1; break;
        case 3: aim_state->secondary_action = 6; aim_state->communication_line = -1; aim_state->secondary_action_pending = 1; break;
        case 4: aim_state->secondary_action = 7; aim_state->communication_line = -1; aim_state->secondary_action_pending = 1; break;
        case 5: aim_state->secondary_action = 8; aim_state->communication_line = 0x2c; aim_state->secondary_action_pending = 1; break;
        case 6: aim_state->secondary_action = 9; aim_state->communication_line = 0x2c; aim_state->secondary_action_pending = 1; break;
        case 7: aim_state->secondary_action = 0xa; aim_state->communication_line = 0x2c; aim_state->secondary_action_pending = 1; break;
        case 8: aim_state->secondary_action = 0xb; aim_state->communication_line = 0x2c; aim_state->secondary_action_pending = 1; break;
        case 9: aim_state->communication_line = 0x26; aim_state->secondary_action = -1; aim_state->secondary_action_pending = 1; break;
        case 10: aim_state->communication_line = 0x27; aim_state->secondary_action = -1; aim_state->secondary_action_pending = 1; break;
        default: break;
        }
        return (char)aim_state->secondary_action_pending;

    case 0x10: {
        int16_t dialogue = (int16_t)(uint16_t)entry->atom_modifier;
        int32_t chain = -1;
        int32_t mode;
        unit_speech speech;

        mode = halo::units::unit_animation_change_priority_check(check_object_index, 1, 6, 1, 0, &dialogue, &chain);
        if ((int16_t)mode <= 0) {
            return 0;
        }
        memset(&speech, 0, sizeof(speech));
        speech.priority = 6;
        speech.scream_type = dialogue;
        speech.sound_tag = (datum_index)chain;
        halo::ai::ai_communication_target_result_reset(&halo::ai::speech_target(speech));
        halo::units::unit_commit_speech(check_object_index, &speech, (int16_t)mode);
        return 1;
    }

    case 0x11:
        if (entry->atom_modifier == 0) {
            state->flags |= 1;
        } else {
            state->flags &= 0xfe;
        }
        return 1;

    case 0x12:
        if (check_object_index != unit_index) {
            return 0;
        }
        ((uint8_t *)act)[0x9e] = (uint8_t)(entry->atom_modifier == 0);
        return 1;

    case 0x13:
        return 1;

    case 0x14: {
        int16_t target = (int16_t)entry->command;

        if (target < 0 || target >= command_count || target == command_index) {
            return 0;
        }
        return 1;
    }

    case 0x15: {
        object *obj = halo::ai::object_at(check_object_index);

        obj->vitality_flags |= halo::to_bits(entry->atom_modifier == 1 ? halo::objects::vitality_flag::die_act_of_god_silent : halo::objects::vitality_flag::die_act_of_god);
        return 1;
    }

    case 0x16: {
        int16_t kind = entry->atom_modifier;

        state->axis = (kind >= 0 && kind <= 3) ? kind : 0;
        halo::ai::actor_get_body_axis_vector(actor_index, check_object_index, state);
        state->timer_ticks = (int16_t)(int32_t)(entry->parameter1 * 30.0f);
        state->movement_flags |= 3;
        return 1;
    }

    case 0x1a:
        if (aim_state == 0 || !(entry->parameter1 > 0.0f)) {
            return 0;
        }
        aim_state->unknown_28 = 1;
        aim_state->unknown_2c = entry->parameter1;
        return 1;

    case 0x1b: {
        int16_t p = (int16_t)entry->point_1;
        int16_t facing_point = (int16_t)entry->point_2;
        real_point3d *destination;
        real_vector3d forward;

        if (p < 0 || p >= point_count) {
            return 0;
        }
        destination = (real_point3d *)&points[p].position;
        halo::units::unit_get_forward_vector_or_marker_normal(check_object_index, &forward);
        if (facing_point >= 0 && facing_point < point_count) {
            real_point3d *facing = (real_point3d *)&points[facing_point].position;
            unit_object *biped = (unit_object *)halo::objects::object_try_and_get(check_object_index, 1);
            Biped *biped_tag = biped != 0 ? halo::ai::tag_data<Biped>(((object *)biped)->definition_tag) : 0;
            float length;

            forward.i = facing->x - destination->x;
            forward.j = facing->y - destination->y;
            forward.k = facing->z - destination->z;
            if (biped_tag != 0 && (biped_tag->biped_flags & 0x44) != 0) {
                length = halo::math::vector3d_normalize_with_length(forward);
            } else {
                forward.k = 0.0f;
                length = halo::math::vector2d_normalize_with_length(*((real_vector2d *)&forward));
            }
            if (length == 0.0f) {
                halo::units::unit_get_forward_vector_or_marker_normal(check_object_index, &forward);
            }
        }
        halo::objects::object_set_position_and_orientation(check_object_index, &forward, 0, destination);
        halo::objects::object_reset_velocity_and_wake(check_object_index);
        halo::objects::object_recalculate_bounding_radius_recursive(check_object_index);
        if (check_object_index == unit_index) {
            halo::ai::actor_fill_unit_position_context(unit_index, halo::ai::own_firing_positions(act));
            halo::ai::actor_movement_action_stop(actor_index);
        }
        return 1;
    }

    default:
        return 0;
    }
}


namespace actor_squad_action_is_complete_local {
}

/**
 * Actor AI behaviour: squad action is complete.
 *
 * @address 0x4066d0
 */
uint8_t ActorOps::squad_action_is_complete(actor_command_aim *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state)
{
    using namespace actor_squad_action_is_complete_local;
    actor *act = halo::ai::actor_at(actor_index);
    ScenarioCommandList *list = &((ScenarioCommandList *)halo::scenario::globals().scenario->command_lists.pointer)[command_list_index];
    datum_index unit_index = act->unit_index;
    ScenarioCommand *entry;
    uint8_t done;

    if (state->command_index >= (int32_t)list->commands.count) {
        return 1;
    }
    entry = &((ScenarioCommand *)list->commands.pointer)[state->command_index];

    switch (entry->atom_type) {
    case 0: case 4: case 0x16: case 0x17: case 0x18: case 0x19:
        return state->timer_ticks == 0;

    case 1:
    case 2: {
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        done = halo::ai::actor_movement_action_in_progress(actor_index);
        if (!done && aim_state->move_interrupts && aim_state->move_requested) {
            float range = halo::ai::actor_compute_accuracy_scale(actor_index);
            real_vector3d delta;
            float distance_squared;

            delta.i = aim_state->move_point.x - act->body_position.x;
            delta.j = aim_state->move_point.y - act->body_position.y;
            delta.k = aim_state->move_point.z - act->body_position.z;
            distance_squared = halo::math::vector3d_magnitude_squared(delta);
            if (distance_squared <= range * range) {
                done = 1;
            } else if (distance_squared <= (range + 0.5f) * (range + 0.5f)) {
                uint8_t *unit = halo::ai::object_bytes(unit_index);

                if (delta.k * ((unit_object *)unit)->base.velocity.k + delta.j * ((unit_object *)unit)->base.velocity.j +
                    delta.i * ((unit_object *)unit)->base.velocity.i <= 0.0f) {
                    done = 1;
                }
            }
        }
        if (entry->atom_type == 2 || entry->atom_modifier == 0) {
            if (act->moving) {
                state->timer_ticks = 10;
            }
            if (!done || state->timer_ticks != 0) {
                return 0;
            }
            done = 1;
        } else if (!done) {
            return 0;
        }
        if (aim_state->look_valid) {
            if (act->flying) {
                real_vector3d direction;

                direction.i = aim_state->look_point.x - act->body_position.x;
                direction.j = aim_state->look_point.y - act->body_position.y;
                direction.k = aim_state->look_point.z - act->body_position.z;
                if (halo::math::vector3d_normalize_with_length(direction) > 0.0f &&
                    direction.k * act->facing.k + direction.j * act->facing.j +
                    direction.i * act->facing.i < 0.984f) {
                    return 0;
                }
            } else {
                real_vector2d direction;

                direction.i = aim_state->look_point.x - act->body_position.x;
                direction.j = aim_state->look_point.y - act->body_position.y;
                if (halo::math::vector2d_normalize_with_length(direction) > 0.0f &&
                    direction.j * act->facing.j + direction.i * act->facing.i < 0.984f) {
                    return 0;
                }
            }
        }
        halo::ai::actor_movement_action_stop(actor_index);
        return done;
    }

    case 3: {
        real_point3d position;

        if (check_object_index == unit_index) {
            position = *(real_point3d *)&act->body_position.x;
        } else {
            halo::objects::object_get_position(&position, check_object_index);
        }
        if ((position.x - state->start_position.x) * state->direction.i +
            (position.y - state->start_position.y) * state->direction.j +
            (position.z - state->start_position.z) * state->direction.k > entry->parameter1) {
            return 1;
        }
        return 0;
    }

    case 7:
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        if (act->firing_target_type != 2 ||
            !(halo::math::vector3d_distance_squared(*&act->firing_target_free_point, *&aim_state->shoot_point) < 0.25f)) {
            int16_t ticks = (int16_t)(int32_t)(halo::ai::tag_data<ActorVariant>(act->actor_variant_tag)->first_burst_delay_time[1] * 30.0f);

            state->timer_ticks = ticks > 0x3c ? ticks : 0x3c;
        }
        return state->timer_ticks == 0;

    case 8:
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        if (aim_state->grenade_thrown) {
            int16_t ticks = halo::units::unit_data_of(halo::ai::object_at(check_object_index))->throwing_grenade_state != 0 ? 0x1e : 0;

            state->timer_ticks = ticks;
            return ticks == 0;
        }
        if (!halo::units::unit_is_in_busy_animation_state(check_object_index)) {
            real_point3d target = *&aim_state->grenade_target;

            if (halo::ai::actor_commit_grenade_toss(actor_index, &target, halo::k_dword_none, halo::k_dword_none)) {
                aim_state->grenade_pending = 1;
            }
        }
        return state->timer_ticks == 0;

    case 0xa:
    case 0xb: {
        uint8_t landed;

        if ((state->movement_flags & 4) == 0) {
            return 1;
        }
        if (check_object_index == unit_index) {
            landed = act->airborne;
        } else {
            landed = (uint8_t)halo::units::unit_get_biped_specific_value(check_object_index);
        }
        if ((state->movement_flags & 8) && landed) {
            state->timer_ticks = 0;
        }
        return state->timer_ticks == 0;
    }

    case 0xd:
        return halo::units::unit_data_of(halo::ai::object_at(check_object_index))->animation_state != halo::units::animation_state_value(halo::units::unit_animation_state_id::custom_animation);

    case 0xe:
        return halo::cutscene::recorded_animation_object_is_playing(check_object_index) == 0;

    case 0xf:
        if (aim_state != 0 && aim_state->secondary_action_pending) {
            return 0;
        }
        return 1;

    case 0x10:
        return halo::units::unit_data_of(halo::ai::object_at(check_object_index))->current_speech.priority != 6;

    case 0x13:
        switch (entry->atom_modifier) {
        case 0:
            return act->combat_status > 0;
        case 1:
            return act->combat_status >= 7;
        case 2:
            if ((state->flags & 8) == 0) {
                state->flags |= 0x10;
                return 0;
            }
            state->flags &= 0xe7;
            return 1;
        default:
            return 1;
        }

    default:
        return 1;
    }
}


namespace actor_squad_action_list_process_local {
}

/**
 * Drives the actor's current squad action list (command_list_index) forward: while the current entry is not
 * complete, stops; otherwise resets it, advances to the next entry (or marks the list finished if that runs past
 * the end), and executes the new entry, repeating until the entry signals it wants to
 *
 * @address 0x406e30
 */
void ActorView::squad_action_list_process(uint32_t check_object_index, int16_t command_list_index, actor_squad_action_state *state, actor_command_aim *aim_state, uint8_t *finished_flag)
{
    using namespace actor_squad_action_list_process_local;
    ScenarioCommandList *lists = (ScenarioCommandList *)halo::scenario::globals().scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];
    uint8_t have_current_entry;
    uint8_t next_action_index;

    if ((state->flags & 2) != 0) {
        return;
    }

    have_current_entry = state->command_index < (int32_t)list->commands.count;
    state->retry_count = 0;
    do {
        if (have_current_entry != 0 && halo::ai::actor_squad_action_is_complete(aim_state, actor_index, check_object_index, command_list_index, state) == 0) {
            break;
        }

        next_action_index = (state->command_index == 0xff) ? 0 : (uint8_t)(state->command_index + 1);

        if (have_current_entry != 0) {
            halo::ai::actor_squad_action_reset_entry(actor_index, check_object_index, state, command_list_index, aim_state, &next_action_index);
        }
        if (next_action_index >= (int32_t)list->commands.count) {
            state->flags |= 2;
            break;
        }
        state->command_index = next_action_index;
        have_current_entry = halo::ai::actor_squad_action_execute(aim_state, actor_index, check_object_index, command_list_index, state);
    } while ((state->flags & 4) == 0);

    if ((state->flags & 2) == 0) {
        *finished_flag = 0;
    }
}

namespace actor_squad_action_reset_entry_local {
}

/**
 * Actor AI behaviour: squad action reset entry.
 *
 * @address 0x406c50
 */
void ActorView::squad_action_reset_entry(uint32_t check_object_index, actor_squad_action_state *state, int16_t command_list_index, actor_command_aim *aim_state, uint8_t *next_action_index_out)
{
    using namespace actor_squad_action_reset_entry_local;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    ScenarioCommandList *lists = (ScenarioCommandList *)halo::scenario::globals().scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];
    uint32_t current_action_index = state->command_index;

    if (current_action_index >= (uint32_t)list->commands.count) {
        return;
    }

    {
        ScenarioCommand *entry = &halo::ai::reflexive_data<ScenarioCommand>(list->commands)[current_action_index];

        switch (entry->atom_type) {
        case 1:
        case 2:
            if (check_object_index == a->unit_index) {
                halo::ai::actor_movement_action_stop(actor_index);
            }
            if (aim_state != 0) {
                aim_state->move_requested = 0;
                aim_state->look_valid = 0;
            }
            break;
        case 3:
        case 0x16:
            state->movement_flags &= 0xfe;
            state->axis = -1;
            return;
        case 4:
        case 0x17:
        case 0x18:
        case 0x19:
            if (check_object_index == a->unit_index) {
                halo::ai::actor_clear_vocalization(actor_index);
                return;
            }
            break;
        case 7:
            if (aim_state != 0) {
                aim_state->shoot_valid = 0;
                return;
            }
            break;
        case 10:
        case 0xb:
            state->movement_flags &= 0xfb;
            state->axis = 0;
            return;
        case 0xd: {
            biped_object *obj = (biped_object *)halo::objects::object_try_and_get(check_object_index, 1);
            if (obj != 0) {
                obj->biped.flags &= 0xfffffff3;
                return;
            }
            break;
        }
        case 0x14:
            if (entry->atom_modifier == 1) {
                uint8_t flags = state->flags;
                state->flags = flags & 0xf7;
                if ((~(flags >> 3) & 1) == 0) {
                    state->flags = flags & 0xe7;
                    return;
                }
                state->flags = (flags & 0xf7) | 0x10;
            }
            if ((int16_t)entry->command != (int32_t)current_action_index && state->retry_count < 10) {
                *next_action_index_out = (uint8_t)entry->command;
                state->retry_count = state->retry_count + 1;
                return;
            }
            break;
        default:
            break;
        }
    }
}

namespace actor_squad_action_status_broadcast_local {
}

/**
 * FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original
 * never reads EAX; actor_index arrive(s) on the stack (2 stack argument(s)).
 *
 * @address 0x407140
 */
int32_t ActorView::squad_action_status_broadcast(int16_t command_list_index, actor_mode_obey_data *record)
{
    using namespace actor_squad_action_status_broadcast_local;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    memset(record, 0, sizeof(*record));

    if (command_list_index < 0 || command_list_index >= (int32_t)halo::scenario::globals().scenario->command_lists.count) {
        return 0;
    }

    {
        ScenarioCommandList *lists = (ScenarioCommandList *)halo::scenario::globals().scenario->command_lists.pointer;
        ScenarioCommandList *list = &lists[command_list_index];

        if (a->swarm == 0 || a->swarm_index != (datum_index)k_datum_index_none) {
            if (list->precomputed_bsp_index == halo::k_word_none || list->precomputed_bsp_index == halo::scenario::globals().structure_bsp_index) {
                uint8_t allow_initiative = (uint8_t)(list->flags & 1);
                uint8_t allow_look = (uint8_t)(~(list->flags >> 2)) & 1;
                uint8_t allow_communication = (uint8_t)(~(list->flags >> 3)) & 1;

                record->command_list_index = command_list_index;
                if (allow_look == 0) {
                    halo::ai::actor_clear_vocalization(actor_index);
                }
                record->allow_communication = allow_communication;
                record->allow_initiative = allow_initiative;
                record->allow_look = allow_look;
                uint8_t flag_bit_1 = (uint8_t)((list->flags >> 1) & 1);

                halo::ai::actor_swarm_for_each_component(actor_index, 1, halo::ai::actor_command_list_reset_record,
                    (uint32_t)(uintptr_t)&flag_bit_1, record);
                return 1;
            }
        } else if (a->active == 0) {
            a->pending_command_list = command_list_index;
        }
    }
    return 0;
}

namespace actor_squad_react_to_grenade_local {
}

/**
 * Reacts a squad to an incoming grenade of a given type by triggering the corresponding avoidance behavior (0:
 * recognized-target dialogue with a "notice" flag set; 1: seen-target reaction with a marked-for-attention flag;
 * 2: a vault/cover flag and a backup/panic scan; 3: full target-data release), gat
 *
 * @address 0x42a3a0
 */
void ActorView::squad_react_to_grenade(datum_index target_prop_index, int16_t grenade_type)
{
    using namespace actor_squad_react_to_grenade_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    prop *target = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];
    uint8_t encounter_forbids = 0;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[self->encounter_index & halo::k_slot_mask];
        encounter_forbids = enc->deaf != 0;
    }

    if (target->stimulus_type == -1 || target->stimulus_type <= grenade_type) {
        target->stimulus_type = grenade_type;
        target->stimulus_timer = (uint16_t)(((grenade_type != 3) - 1) & 0x78) + 0x1e;
    }

    switch (grenade_type) {
    case 0:
        if (target->disregarded == 0) {
            target->ambient_perception = 3;
            target->perception_level = 3;
            target->combat_dirty = 1;
            halo::ai::actor_queue_recognized_target_dialogue(actor_index, target_prop_index);
        }
        break;
    case 1:
        if (encounter_forbids == 0 && target->disregarded == 0) {
            target->shooting = 1;
            target->combat_dirty = 1;
            target->auditory_perception = 3;
            target->perception_level = 3;
            if (target->is_parented != 0) {
                halo::ai::actor_set_units_active(actor_index, 0);
            }
            halo::ai::actor_react_to_seen_target(actor_index, target_prop_index);
        }
        break;
    case 2:
        if (encounter_forbids == 0 && target->disregarded == 0) {
            target->dead = 1;
            target->auditory_perception = 3;
            target->perception_level = 3;
            target->combat_dirty = 1;
            halo::ai::actor_set_units_active(actor_index, 0);
            halo::ai::actor_scan_backup_and_panic_reaction(target_prop_index, actor_index);
        }
        break;
    case 3:
        if (target->disregarded == 0) {
            target->ambient_perception = 3;
            target->perception_level = 3;
            target->combat_dirty = 1;
            if (target->is_parented != 0) {
                halo::ai::actor_set_units_active(actor_index, 0);
            }
            halo::ai::actor_target_data_release(target_prop_index, actor_index, 0);
        }
        break;
    }
}

namespace actor_squad_react_to_grenade_for_vehicle_occupants_local {
}

/**
 * REWRITTEN from objdump 0x42bd70..0x42be38. EAX: a unit, EBX: the other object. The unit's driver (or the unit
 * itself) must be a biped; then the other object's actor gets its prop for that biped (0x43eb30, create 1, flag
 * 0) and reacts to it (0x42a3a0, type 0), and the biped's actor does the same for
 *
 * @address 0x42bd70
 */
void ActorOps::squad_react_to_grenade_for_vehicle_occupants(datum_index vehicle_object_index, datum_index other_object_index)
{
    using namespace actor_squad_react_to_grenade_for_vehicle_occupants_local;
    uint8_t *vehicle;
    unit_object *occupant;
    datum_index occupant_index;
    datum_index actor;
    datum_index prop;

    if (vehicle_object_index == k_datum_index_none) {
        return;
    }
    vehicle = (uint8_t *)halo::objects::object_try_and_get(vehicle_object_index, 3);
    if (vehicle == 0) {
        return;
    }
    occupant_index = ((vehicle_object *)vehicle)->unit.driver_unit_index;
    if (occupant_index == k_datum_index_none) {
        occupant_index = vehicle_object_index;
    }
    occupant = (unit_object *)halo::ai::object_bytes(occupant_index);
    if (occupant->base.type != 0) {
        return;
    }
    actor = halo::units::unit_data_of(halo::ai::object_at(other_object_index))->actor_index;
    if (actor != k_datum_index_none) {
        prop = halo::ai::actor_find_or_create_shared_prop(occupant_index, actor, 1, 0);
        if (prop != k_datum_index_none) {
            halo::ai::actor_squad_react_to_grenade(actor, prop, 0);
        }
    }
    actor = occupant->unit.actor_index;
    if (actor != k_datum_index_none) {
        prop = halo::ai::actor_find_or_create_shared_prop(other_object_index, actor, 1, 0);
        if (prop != k_datum_index_none) {
            halo::ai::actor_squad_react_to_grenade(actor, prop, 0);
        }
    }
}


}
