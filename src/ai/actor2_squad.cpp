#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

namespace halo::ai {

/**
 * Actor AI behaviour: obey member advance.
 *
 * @address 0x406f80
 */
void ActorView::obey_member_advance(datum_index unit_index, uint16_t command_list_index, void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    uint8_t *record = (uint8_t *)component_record;

    (void)actor_index;
    (void)unit_index;
    (void)command_list_index;
    (void)secondary_record;
    (void)callback_extra;
    record[4] = (uint8_t)((record[4] & 0xef) | 8);
}

namespace actor_obey_member_enter_local {
extern "C" {
extern data_array *object_data;
extern Scenario *global_scenario;
}
}

/**
 * Actor AI behaviour: obey member enter.
 *
 * @address 0x406f30
 */
void ActorView::obey_member_enter(datum_index unit_index, uint16_t command_list_index, void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    using namespace actor_obey_member_enter_local;
    uint8_t *list = (uint8_t *)global_scenario->command_lists.pointer + (int16_t)command_list_index * 0x60;

    (void)actor_index;
    (void)component_record;
    (void)secondary_record;
    (void)callback_extra;
    if (list[0x20] & 0x10) {
        uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & halo::k_slot_mask].data;

        ((unit_object *)unit)->unit.flags |= 0x1000;
    }
}

namespace actor_obey_member_exit_local {
extern "C" {
extern data_array *object_data;
extern void actor_squad_action_reset_entry(uint32_t actor_index, uint32_t check_object_index, uint8_t *state,
    int16_t command_list_index, uint8_t *aim_state, uint8_t *next_action_index_out);
}
}

/**
 * Actor AI behaviour: obey member exit.
 *
 * @address 0x406fa0
 */
void ActorView::obey_member_exit(datum_index unit_index, uint16_t command_list_index, void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    using namespace actor_obey_member_exit_local;
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & halo::k_slot_mask].data;
    uint8_t *record = (uint8_t *)component_record;

    (void)callback_extra;
    if ((record[4] & 2) == 0) {
        uint8_t next_action = 0;

        actor_squad_action_reset_entry(actor_index, unit_index, record, (int16_t)command_list_index,
            (uint8_t *)secondary_record, &next_action);
    }
    ((unit_object *)unit)->unit.flags &= ~0x1000u;
}

namespace actor_obey_member_tick_local {
extern "C" {
extern void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_axis_request *request);
}
}

/**
 * Actor AI behaviour: obey member tick.
 *
 * @address 0x406ff0
 */
void ActorView::obey_member_tick(datum_index unit_index, uint16_t command_list_index, void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    using namespace actor_obey_member_tick_local;
    uint8_t *record = (uint8_t *)component_record;
    uint8_t flags;

    (void)command_list_index;
    (void)secondary_record;
    (void)callback_extra;
    if (*(int16_t *)(record + 2) > 0) {
        *(int16_t *)(record + 2) = (int16_t)(*(int16_t *)(record + 2) - 1);
    }
    flags = record[5];
    if ((flags & 4) && *(int16_t *)(record + 8) > 0) {
        *(int16_t *)(record + 8) = (int16_t)(*(int16_t *)(record + 8) - 1);
    }
    if ((flags & 1) && (flags & 2)) {
        actor_get_body_axis_vector(actor_index, unit_index, (actor_axis_request *)record);
    }
}

namespace actor_squad_action_execute_local {
extern "C" {
extern double fcos(double x);
extern double fsin(double x);
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *player_data;
extern Scenario *global_scenario;
extern void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_axis_request *request);
extern uint8_t actor_play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index,
                                                   char *seat_name, int16_t seat_flags, int16_t count);
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant,
                                        actor_vocalization_context *context);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra);
extern void actor_movement_actions_cancel(datum_index actor_index);
extern void actor_fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context);
extern void ai_communication_target_result_reset(ai_communication_target_result *record);
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator);
extern prop *actor_prop_iterator_next(actor_prop_iterator *iterator);
extern char hs_call_script_by_name(char *name);
extern void object_reset_velocity_and_wake(uint32_t object_index);
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
                                                real_vector3d *up, real_point3d *position);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern datum_index object_lookup_table_get(int16_t name_index);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern int32_t object_iterator_next(object_iterator *iterator);
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index);
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_communication_hold_tick, int16_t *dialogue_index,
    int32_t *chain_value);
extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode);
extern void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out);
extern void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out);
extern int32_t unit_set_grenade_type_and_count_delta(uint32_t unit_index, int16_t grenade_type, int8_t delta);
extern uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name,
    uint8_t interpolate);
extern const char k_empty_string[];
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
}
}

/**
 * state: +0x00 command index (byte), +0x02 timer ticks, +0x04 flags, +0x05 movement flags, +0x08 kind, +0x0c
 * direction / facing, +0x18 start position. aim_state (EAX): the mode's aim / look / fire record.
 *
 * @address 0x405520
 */
char ActorOps::squad_action_execute(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state)
{
    using namespace actor_squad_action_execute_local;
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint8_t *variant_tag = TAG_DATA(((actor *)act)->actor_variant_tag);
    ScenarioCommandList *list = &((ScenarioCommandList *)global_scenario->command_lists.pointer)[command_list_index];
    int32_t command_count = (int32_t)list->commands.count;
    int32_t command_index = state[0];
    ScenarioCommand *entry;
    ScenarioCommandPoint *points = (ScenarioCommandPoint *)list->points.pointer;
    int32_t point_count = (int32_t)list->points.count;
    datum_index unit_index = ((actor *)act)->unit_index;
    char result = 0;

    if (command_index >= command_count) {
        return 0;
    }
    entry = &((ScenarioCommand *)list->commands.pointer)[command_index];

    switch (entry->atom_type) {
    case 0:
        *(int16_t *)(state + 0x2) = (int16_t)(int32_t)(entry->parameter1 * 30.0f);
        return 1;

    case 1:
    case 2: {
        int16_t point_index = (int16_t)entry->point_1;
        ScenarioCommandPoint *point;

        if (aim_state == 0 || point_index < 0 || point_index >= point_count) {
            return 0;
        }
        point = &points[point_index];
        *(int16_t *)(state + 0x2) = 0;
        aim_state[0x4] = 1;
        aim_state[0x5] = (uint8_t)(entry->atom_modifier == 1);
        *(real_point3d *)(aim_state + 0x8) = *(real_point3d *)&point->position;
        *(int32_t *)(aim_state + 0x14) = (int32_t)point->surface_index;
        result = (char)actor_movement_set_destination_point((real_point3d *)(aim_state + 0x8), actor_index,
                                                            (int32_t)point->surface_index, halo::k_dword_none);
        if (!result) {
            return 0;
        }
        if (aim_state[0x5]) {
            actor_movement_actions_cancel(actor_index);
        }
        if (entry->atom_type == 2) {
            int16_t face_index = (int16_t)entry->point_2;

            if (face_index >= 0 && face_index < point_count) {
                aim_state[0x18] = 1;
                *(real_point3d *)(aim_state + 0x1c) = *(real_point3d *)&points[face_index].position;
            }
        }
        return result;
    }

    case 3: {
        real_point3d *start = (real_point3d *)(state + 0x18);
        int16_t point_index = (int16_t)entry->point_1;
        int16_t kind;

        if (check_object_index == unit_index) {
            *start = *(real_point3d *)&((actor *)act)->body_position.x;
        } else {
            object_get_position(start, check_object_index);
        }
        if (point_index >= 0 && point_index < point_count) {
            real_vector3d *direction = (real_vector3d *)(state + 0xc);

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
            *(float *)(state + 0x14) = 0.0f;
            *(float *)(state + 0xc) = (float)fcos(angle);
            *(float *)(state + 0x10) = (float)fsin(angle);
        }
        kind = entry->atom_modifier;
        *(int16_t *)(state + 0x8) = (kind >= 0 && kind <= 3) ? kind : -1;
        if (check_object_index == unit_index) {
            actor_movement_action_stop(actor_index);
        }
        state[0x5] = (uint8_t)((state[0x5] & 0xfd) | 1);
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
                    duration = halo::math::random_real_range(*(float *)(actor_tag + 0xec), *(float *)(actor_tag + 0xf0));
                } else {
                    duration = halo::math::random_real_range(entry->parameter1, entry->parameter2);
                }
            }
        } else if (entry->atom_type == 0x18) {
            actor_prop_iterator iterator;
            prop *p;
            float nearest = 3.4028235e38f;

            actor_prop_iterator_init(actor_index, &iterator);
            for (p = actor_prop_iterator_next(&iterator); p != 0; p = actor_prop_iterator_next(&iterator)) {
                if (p->state >= 2 && p->state <= 3 && p->is_parented && nearest > p->distance) {
                    look_prop = iterator.current;
                    nearest = p->distance;
                }
            }
            if (look_prop == k_datum_index_none) {
                data_iterator players;
                uint8_t *player;
                float best = 3.4028235e38f;

                players.data = player_data;
                players.next_index = 0;
                players.index = k_datum_index_none;
                players.signature = (uint32_t)(uintptr_t)player_data ^ 0x69746572;
                for (player = (uint8_t *)halo::memory::data_iterator_next(&players); player != 0; player = (uint8_t *)halo::memory::data_iterator_next(&players)) {
                    datum_index player_unit = ((struct player *)player)->unit;

                    if (player_unit != k_datum_index_none) {
                        real_point3d eye;
                        float distance;

                        unit_get_primary_eye_marker_position(player_unit, &eye);
                        distance = halo::math::vector3d_distance_squared(((struct actor *)act)->aim_origin, eye);
                        if (distance <= best) {
                            best = distance;
                            look_object = player_unit;
                        }
                    }
                }
            }
        } else if (entry->atom_type == 0x19) {
            int16_t name = (int16_t)entry->object_name;

            if (name >= 0 && name < *(int32_t *)((uint8_t *)global_scenario + 0x204)) {
                datum_index object_index = object_lookup_table_get(name);

                if (object_try_and_get(object_index, 3) != 0) {
                    look_prop = actor_find_prop_for_object(object_index, actor_index);
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
                unit_get_primary_eye_marker_position(look_object, &context.payload.point);
            } else {
                context.payload.point = *(real_point3d *)&points[(int16_t)look_point].position;
            }
        }
        actor_begin_vocalization(actor_index, 0xd, variant, &context);
        *(int16_t *)(state + 0x2) = (int16_t)(int32_t)(duration * 30.0f);
        return 1;
    }

    case 5:
        if (aim_state == 0 || entry->atom_modifier < 0 || entry->atom_modifier >= 4) {
            return 0;
        }
        *(int16_t *)(aim_state + 0x2) = entry->atom_modifier;
        return 1;

    case 6:
        if (aim_state == 0) {
            return 0;
        }
        aim_state[0x0] = (uint8_t)(entry->atom_modifier == 1);
        return 1;

    case 7: {
        int16_t p = (int16_t)entry->point_1;

        if (aim_state == 0 || p < 0 || p >= point_count) {
            return 0;
        }
        aim_state[0x36] = 1;
        *(real_point3d *)(aim_state + 0x38) = *(real_point3d *)&points[p].position;
        *(float *)(aim_state + 0x44) = entry->parameter1;
        return 1;
    }

    case 8: {
        int16_t grenade_type;
        int16_t p = (int16_t)entry->point_1;

        if (aim_state == 0) {
            return 0;
        }
        grenade_type = *(int16_t *)(variant_tag + 0x180);
        if (grenade_type == -1 || p < 0 || p >= point_count) {
            return 0;
        }
        unit_set_grenade_type_and_count_delta(unit_index, grenade_type, 1);
        aim_state[0x49] = 0;
        aim_state[0x48] = 0;
        *(real_point3d *)(aim_state + 0x4c) = *(real_point3d *)&points[p].position;
        *(int16_t *)(aim_state + 0x4a) = 0;
        if (entry->atom_modifier >= 0 && entry->atom_modifier < 3) {
            *(int16_t *)(aim_state + 0x4a) = entry->atom_modifier;
        }
        *(int16_t *)(state + 0x2) = 0x3c;
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
        while (object_iterator_next(&it) != 0) {
            datum_index vehicle_index = it.handle;
            real_point3d position;
            float distance;

            object_get_position(&position, vehicle_index);
            distance = halo::math::vector3d_distance_squared(position, ((struct actor *)act)->body_position);
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
            if (actor_play_first_valid_vocalization(0, samples[i].vehicle_index, actor_index, (char *)k_empty_string,
                                                    seat_flags, 0)) {
                state[0x4] |= 4;
                return 1;
            }
        }
        return 0;
    }

    case 0xa: {
        uint8_t moving_forward;

        if (check_object_index == unit_index && ((actor *)act)->active_unit_index != k_datum_index_none) {
            return 0;
        }
        state[0x5] = (uint8_t)((state[0x5] & 0xe7) | 4);
        if (check_object_index == unit_index) {
            moving_forward = (uint8_t)(act[0x504] != 0 && ((struct actor *)act)->moving_facing_direction == 0);
        } else {
            uint8_t *obj = OBJECT_DATA(check_object_index);

            moving_forward = (uint8_t)(((object *)obj)->parent_object == k_datum_index_none &&
                                       ((object *)obj)->velocity.k * ((object *)obj)->forward.k +
                                       ((object *)obj)->velocity.j * ((object *)obj)->forward.j +
                                       ((object *)obj)->forward.i * ((object *)obj)->velocity.i > 0.06666667f);
        }
        *(int16_t *)(state + 0x2) = 0x3c;
        *(int16_t *)(state + 0x8) = moving_forward ? 0 : 10;
        return 1;
    }

    case 0xb:
        if (check_object_index == unit_index && ((actor *)act)->active_unit_index != k_datum_index_none) {
            return 0;
        }
        state[0x5] = (uint8_t)((state[0x5] & 0xf7) | 0x14);
        *(int16_t *)(state + 0x8) = 0;
        *(float *)(state + 0xc) = entry->parameter1;
        *(float *)(state + 0x10) = entry->parameter2;
        *(int16_t *)(state + 0x2) = 0x3c;
        return 1;

    case 0xc: {
        int16_t script = (int16_t)entry->script;

        if (script < 0 || script >= *(int32_t *)((uint8_t *)global_scenario + 0x450)) {
            return 0;
        }
        return hs_call_script_by_name(*(char **)((uint8_t *)global_scenario + 0x454) + script * 0x28);
    }

    case 0xd: {
        uint8_t *reference;
        datum_index graph;
        uint8_t interpolate = 1;
        uint8_t flag_4 = 0;
        uint8_t flag_8 = 0;
        uint8_t *biped;

        if ((int16_t)entry->animation == -1) {
            return 0;
        }
        reference = *(uint8_t **)((uint8_t *)global_scenario + 0x448) + (int16_t)entry->animation * 0x3c;
        graph = *(datum_index *)(reference + 0x2c);
        if (graph == k_datum_index_none) {
            graph = *(datum_index *)(TAG_DATA(*(datum_index *)OBJECT_DATA(check_object_index)) + 0x44);
        }
        switch (entry->atom_modifier) {
        case 1: flag_4 = 1; break;
        case 2: flag_8 = 1; flag_4 = 1; break;
        case 3: interpolate = 0; break;
        case 4: interpolate = 0; flag_4 = 1; break;
        case 5: interpolate = 0; flag_8 = 1; flag_4 = 1; break;
        default: break;
        }
        if (!unit_start_user_animation(check_object_index, graph, (const char *)reference, interpolate)) {
            return 0;
        }
        biped = (uint8_t *)object_try_and_get(check_object_index, 1);
        if (biped != 0) {
            uint32_t *flags = (uint32_t *)(biped + 0x4cc);

            *flags = flag_4 ? (*flags | 4) : (*flags & ~4u);
            *flags = flag_8 ? (*flags | 8) : (*flags & ~8u);
        }
        return 1;
    }

    case 0xe: {
        int16_t recording = (int16_t)entry->recording;
        int16_t animation_index;

        if (recording < 0 || recording >= *(int32_t *)((uint8_t *)global_scenario + 0x45c)) {
            return 0;
        }
        animation_index = halo::cutscene::recorded_animation_find_by_name(
            *(char **)((uint8_t *)global_scenario + 0x460) + recording * 0x28, global_scenario);
        if (animation_index == -1) {
            return 0;
        }
        return (char)halo::cutscene::recorded_animation_start(check_object_index, animation_index, 0);
    }

    case 0xf:
        if (aim_state == 0) {
            return 0;
        }
        aim_state[0x30] = 0;
        switch (entry->atom_modifier) {
        case 0: *(int16_t *)(aim_state + 0x32) = 0; *(int16_t *)(aim_state + 0x34) = 0x2a; aim_state[0x30] = 1; break;
        case 1: *(int16_t *)(aim_state + 0x32) = 4; *(int16_t *)(aim_state + 0x34) = 0x29; aim_state[0x30] = 1; break;
        case 2: *(int16_t *)(aim_state + 0x32) = 5; *(int16_t *)(aim_state + 0x34) = 0x29; aim_state[0x30] = 1; break;
        case 3: *(int16_t *)(aim_state + 0x32) = 6; *(int16_t *)(aim_state + 0x34) = -1; aim_state[0x30] = 1; break;
        case 4: *(int16_t *)(aim_state + 0x32) = 7; *(int16_t *)(aim_state + 0x34) = -1; aim_state[0x30] = 1; break;
        case 5: *(int16_t *)(aim_state + 0x32) = 8; *(int16_t *)(aim_state + 0x34) = 0x2c; aim_state[0x30] = 1; break;
        case 6: *(int16_t *)(aim_state + 0x32) = 9; *(int16_t *)(aim_state + 0x34) = 0x2c; aim_state[0x30] = 1; break;
        case 7: *(int16_t *)(aim_state + 0x32) = 0xa; *(int16_t *)(aim_state + 0x34) = 0x2c; aim_state[0x30] = 1; break;
        case 8: *(int16_t *)(aim_state + 0x32) = 0xb; *(int16_t *)(aim_state + 0x34) = 0x2c; aim_state[0x30] = 1; break;
        case 9: *(int16_t *)(aim_state + 0x34) = 0x26; *(int16_t *)(aim_state + 0x32) = -1; aim_state[0x30] = 1; break;
        case 10: *(int16_t *)(aim_state + 0x34) = 0x27; *(int16_t *)(aim_state + 0x32) = -1; aim_state[0x30] = 1; break;
        default: break;
        }
        return (char)aim_state[0x30];

    case 0x10: {
        int16_t dialogue = (int16_t)(uint16_t)entry->atom_modifier;
        int32_t chain = -1;
        int32_t mode;
        unit_speech speech;

        mode = unit_animation_change_priority_check(check_object_index, 1, 6, 1, 0, &dialogue, &chain);
        if ((int16_t)mode <= 0) {
            return 0;
        }
        memset(&speech, 0, sizeof(speech));
        speech.priority = 6;
        speech.scream_type = dialogue;
        speech.sound_tag = (datum_index)chain;
        ai_communication_target_result_reset((ai_communication_target_result *)((uint8_t *)&speech + 0x10));
        unit_commit_speech(check_object_index, &speech, (int16_t)mode);
        return 1;
    }

    case 0x11:
        if (entry->atom_modifier == 0) {
            state[0x4] |= 1;
        } else {
            state[0x4] &= 0xfe;
        }
        return 1;

    case 0x12:
        if (check_object_index != unit_index) {
            return 0;
        }
        act[0x9e] = (uint8_t)(entry->atom_modifier == 0);
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
        uint8_t *obj = OBJECT_DATA(check_object_index);

        obj[0x106] |= entry->atom_modifier == 1 ? 0x40 : 0x20;
        return 1;
    }

    case 0x16: {
        int16_t kind = entry->atom_modifier;

        *(int16_t *)(state + 0x8) = (kind >= 0 && kind <= 3) ? kind : 0;
        actor_get_body_axis_vector(actor_index, check_object_index, (actor_axis_request *)state);
        *(int16_t *)(state + 0x2) = (int16_t)(int32_t)(entry->parameter1 * 30.0f);
        state[0x5] |= 3;
        return 1;
    }

    case 0x1a:
        if (aim_state == 0 || !(entry->parameter1 > 0.0f)) {
            return 0;
        }
        aim_state[0x28] = 1;
        *(float *)(aim_state + 0x2c) = entry->parameter1;
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
        unit_get_forward_vector_or_marker_normal(check_object_index, &forward);
        if (facing_point >= 0 && facing_point < point_count) {
            real_point3d *facing = (real_point3d *)&points[facing_point].position;
            uint8_t *biped = (uint8_t *)object_try_and_get(check_object_index, 1);
            uint8_t *biped_tag = biped != 0 ? TAG_DATA(*(datum_index *)biped) : 0;
            float length;

            forward.i = facing->x - destination->x;
            forward.j = facing->y - destination->y;
            forward.k = facing->z - destination->z;
            if (biped_tag != 0 && (biped_tag[0x2f4] & 0x44) != 0) {
                length = halo::math::vector3d_normalize_with_length(forward);
            } else {
                forward.k = 0.0f;
                length = halo::math::vector2d_normalize_with_length(*((real_vector2d *)&forward));
            }
            if (length == 0.0f) {
                unit_get_forward_vector_or_marker_normal(check_object_index, &forward);
            }
        }
        object_set_position_and_orientation(check_object_index, &forward, 0, destination);
        object_reset_velocity_and_wake(check_object_index);
        object_recalculate_bounding_radius_recursive(check_object_index);
        if (check_object_index == unit_index) {
            actor_fill_unit_position_context(unit_index, (actor_unit_position_context *)(act + 0x120));
            actor_movement_action_stop(actor_index);
        }
        return 1;
    }

    default:
        return 0;
    }
}

#undef OBJECT_DATA
#undef TAG_DATA

namespace actor_squad_action_is_complete_local {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern Scenario *global_scenario;
extern uint32_t actor_commit_grenade_toss(datum_index actor_index, real_point3d *point, uint32_t object_handle,
                                          uint32_t exclude_object_index);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_action_in_progress(datum_index actor_index);
extern float actor_compute_accuracy_scale(datum_index actor_index);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index);
extern uint32_t unit_get_biped_specific_value(uint32_t object_index);
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
}
}

/**
 * Actor AI behaviour: squad action is complete.
 *
 * @address 0x4066d0
 */
uint8_t ActorOps::squad_action_is_complete(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state)
{
    using namespace actor_squad_action_is_complete_local;
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    ScenarioCommandList *list = &((ScenarioCommandList *)global_scenario->command_lists.pointer)[command_list_index];
    datum_index unit_index = ((actor *)act)->unit_index;
    ScenarioCommand *entry;
    uint8_t done;

    if (state[0] >= (int32_t)list->commands.count) {
        return 1;
    }
    entry = &((ScenarioCommand *)list->commands.pointer)[state[0]];

    switch (entry->atom_type) {
    case 0: case 4: case 0x16: case 0x17: case 0x18: case 0x19:
        return *(int16_t *)(state + 0x2) == 0;

    case 1:
    case 2: {
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        done = actor_movement_action_in_progress(actor_index);
        if (!done && aim_state[0x5] && aim_state[0x4]) {
            float range = actor_compute_accuracy_scale(actor_index);
            real_vector3d delta;
            float distance_squared;

            delta.i = *(float *)(aim_state + 0x8) - ((actor *)act)->body_position.x;
            delta.j = *(float *)(aim_state + 0xc) - ((actor *)act)->body_position.y;
            delta.k = *(float *)(aim_state + 0x10) - ((actor *)act)->body_position.z;
            distance_squared = halo::math::vector3d_magnitude_squared(delta);
            if (distance_squared <= range * range) {
                done = 1;
            } else if (distance_squared <= (range + 0.5f) * (range + 0.5f)) {
                uint8_t *unit = OBJECT_DATA(unit_index);

                if (delta.k * ((unit_object *)unit)->base.velocity.k + delta.j * ((unit_object *)unit)->base.velocity.j +
                    delta.i * ((unit_object *)unit)->base.velocity.i <= 0.0f) {
                    done = 1;
                }
            }
        }
        if (entry->atom_type == 2 || entry->atom_modifier == 0) {
            if (act[0x504]) {
                *(int16_t *)(state + 0x2) = 10;
            }
            if (!done || *(int16_t *)(state + 0x2) != 0) {
                return 0;
            }
            done = 1;
        } else if (!done) {
            return 0;
        }
        if (aim_state[0x18]) {
            if (act[0x99]) {
                real_vector3d direction;

                direction.i = *(float *)(aim_state + 0x1c) - ((actor *)act)->body_position.x;
                direction.j = *(float *)(aim_state + 0x20) - ((actor *)act)->body_position.y;
                direction.k = *(float *)(aim_state + 0x24) - ((actor *)act)->body_position.z;
                if (halo::math::vector3d_normalize_with_length(direction) > 0.0f &&
                    direction.k * ((actor *)act)->facing.k + direction.j * ((actor *)act)->facing.j +
                    direction.i * ((actor *)act)->facing.i < 0.984f) {
                    return 0;
                }
            } else {
                real_vector2d direction;

                direction.i = *(float *)(aim_state + 0x1c) - ((actor *)act)->body_position.x;
                direction.j = *(float *)(aim_state + 0x20) - ((actor *)act)->body_position.y;
                if (halo::math::vector2d_normalize_with_length(direction) > 0.0f &&
                    direction.j * ((actor *)act)->facing.j + direction.i * ((actor *)act)->facing.i < 0.984f) {
                    return 0;
                }
            }
        }
        actor_movement_action_stop(actor_index);
        return done;
    }

    case 3: {
        real_point3d position;

        if (check_object_index == unit_index) {
            position = *(real_point3d *)&((actor *)act)->body_position.x;
        } else {
            object_get_position(&position, check_object_index);
        }
        if ((position.x - *(float *)(state + 0x18)) * *(float *)(state + 0xc) +
            (position.y - *(float *)(state + 0x1c)) * *(float *)(state + 0x10) +
            (position.z - *(float *)(state + 0x20)) * *(float *)(state + 0x14) > entry->parameter1) {
            return 1;
        }
        return 0;
    }

    case 7:
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        if (((struct actor *)act)->firing_target_type != 2 ||
            !(halo::math::vector3d_distance_squared(*(real_point3d *)(act + 0x610), *(real_point3d *)(aim_state + 0x38)) < 0.25f)) {
            int16_t ticks = (int16_t)(int32_t)(*(float *)(TAG_DATA(((actor *)act)->actor_variant_tag) + 0x84) * 30.0f);

            *(int16_t *)(state + 0x2) = ticks > 0x3c ? ticks : 0x3c;
        }
        return *(int16_t *)(state + 0x2) == 0;

    case 8:
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        if (aim_state[0x49]) {
            int16_t ticks = OBJECT_DATA(check_object_index)[0x28d] != 0 ? 0x1e : 0;

            *(int16_t *)(state + 0x2) = ticks;
            return ticks == 0;
        }
        if (!unit_is_in_busy_animation_state(check_object_index)) {
            real_point3d target = *(real_point3d *)(aim_state + 0x4c);

            if (actor_commit_grenade_toss(actor_index, &target, halo::k_dword_none, halo::k_dword_none)) {
                aim_state[0x48] = 1;
            }
        }
        return *(int16_t *)(state + 0x2) == 0;

    case 0xa:
    case 0xb: {
        uint8_t landed;

        if ((state[0x5] & 4) == 0) {
            return 1;
        }
        if (check_object_index == unit_index) {
            landed = act[0x15c];
        } else {
            landed = (uint8_t)unit_get_biped_specific_value(check_object_index);
        }
        if ((state[0x5] & 8) && landed) {
            *(int16_t *)(state + 0x2) = 0;
        }
        return *(int16_t *)(state + 0x2) == 0;
    }

    case 0xd:
        return OBJECT_DATA(check_object_index)[0x2a3] != 0x1c;

    case 0xe:
        return halo::cutscene::recorded_animation_object_is_playing(check_object_index) == 0;

    case 0xf:
        if (aim_state != 0 && aim_state[0x30]) {
            return 0;
        }
        return 1;

    case 0x10:
        return *(int16_t *)(OBJECT_DATA(check_object_index) + 0x388) != 6;

    case 0x13:
        switch (entry->atom_modifier) {
        case 0:
            return ((struct actor *)act)->combat_status > 0;
        case 1:
            return ((struct actor *)act)->combat_status >= 7;
        case 2:
            if ((state[0x4] & 8) == 0) {
                state[0x4] |= 0x10;
                return 0;
            }
            state[0x4] &= 0xe7;
            return 1;
        default:
            return 1;
        }

    default:
        return 1;
    }
}

#undef OBJECT_DATA
#undef TAG_DATA

namespace actor_squad_action_list_process_local {
extern "C" {
extern Scenario *global_scenario;
extern char actor_squad_action_execute(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state);
extern uint8_t actor_squad_action_is_complete(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state);
extern void actor_squad_action_reset_entry(uint32_t actor_index, uint32_t check_object_index, uint8_t *state, int16_t command_list_index, uint8_t *aim_state, uint8_t *next_action_index_out);
}
}

/**
 * Drives the actor's current squad action list (command_list_index) forward: while the current entry is not
 * complete, stops; otherwise resets it, advances to the next entry (or marks the list finished if that runs past
 * the end), and executes the new entry, repeating until the entry signals it wants to
 *
 * @address 0x406e30
 */
void ActorView::squad_action_list_process(uint32_t check_object_index, int16_t command_list_index, uint8_t *state, uint8_t *aim_state, uint8_t *out)
{
    using namespace actor_squad_action_list_process_local;
    ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];
    uint8_t have_current_entry;
    uint8_t next_action_index;

    if ((state[4] & 2) != 0) {
        goto finish;
    }

    have_current_entry = state[0] < (int32_t)list->commands.count;
    state[1] = 0;
    do {
        if (have_current_entry != 0 && actor_squad_action_is_complete(aim_state, actor_index, check_object_index, command_list_index, state) == 0) {
            break;
        }

        next_action_index = (state[0] == 0xff) ? 0 : (uint8_t)(state[0] + 1);

        if (have_current_entry != 0) {
            actor_squad_action_reset_entry(actor_index, check_object_index, state, command_list_index, aim_state, &next_action_index);
        }
        if (next_action_index >= (int32_t)list->commands.count) {
            state[4] |= 2;
            break;
        }
        state[0] = next_action_index;
        have_current_entry = actor_squad_action_execute(aim_state, actor_index, check_object_index, command_list_index, state);
    } while ((state[4] & 4) == 0);

finish:
    if ((state[4] & 2) == 0) {
        *out = 0;
    }
}

namespace actor_squad_action_reset_entry_local {
extern "C" {
extern data_array *actor_data;
extern Scenario *global_scenario;
extern void actor_clear_vocalization(uint32_t actor_index);
extern void actor_movement_action_stop(datum_index actor_index);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
}
}

/**
 * Actor AI behaviour: squad action reset entry.
 *
 * @address 0x406c50
 */
void ActorView::squad_action_reset_entry(uint32_t check_object_index, uint8_t *state, int16_t command_list_index, uint8_t *aim_state, uint8_t *next_action_index_out)
{
    using namespace actor_squad_action_reset_entry_local;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];
    uint32_t current_action_index = state[0];

    if (current_action_index >= (uint32_t)list->commands.count) {
        return;
    }

    {
        ScenarioCommand *entry = (ScenarioCommand *)((uint8_t *)list->commands.pointer + current_action_index * sizeof(ScenarioCommand));

        switch (entry->atom_type) {
        case 1:
        case 2:
            if (check_object_index == a->unit_index) {
                actor_movement_action_stop(actor_index);
            }
            if (aim_state != 0) {
                aim_state[4] = 0;
                aim_state[0x18] = 0;
            }
            break;
        case 3:
        case 0x16:
            state[5] &= 0xfe;
            state[8] = 0xff;
            state[9] = 0xff;
            return;
        case 4:
        case 0x17:
        case 0x18:
        case 0x19:
            if (check_object_index == a->unit_index) {
                actor_clear_vocalization(actor_index);
                return;
            }
            break;
        case 7:
            if (aim_state != 0) {
                aim_state[0x36] = 0;
                return;
            }
            break;
        case 10:
        case 0xb:
            state[5] &= 0xfb;
            state[8] = 0;
            state[9] = 0;
            return;
        case 0xd: {
            uint32_t *obj = (uint32_t *)object_try_and_get(check_object_index, 1);
            if (obj != 0) {
                uint32_t *flags = (uint32_t *)((uint8_t *)obj + 0x4cc);
                *flags &= 0xfffffff3;
                return;
            }
            break;
        }
        case 0x14:
            if (entry->atom_modifier == 1) {
                uint8_t flags = state[4];
                state[4] = flags & 0xf7;
                if ((~(flags >> 3) & 1) == 0) {
                    state[4] = flags & 0xe7;
                    return;
                }
                state[4] = (flags & 0xf7) | 0x10;
            }
            if ((int16_t)entry->command != (int32_t)current_action_index && state[1] < 10) {
                *next_action_index_out = *((uint8_t *)entry + 0x16);
                state[1] = state[1] + 1;
                return;
            }
            break;
        default:
            break;
        }
    }
}

namespace actor_squad_action_status_broadcast_local {
extern "C" {
extern data_array *actor_data;
extern Scenario *global_scenario;
extern uint16_t global_structure_bsp_index;
extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, uint16_t *caller_record);
extern void actor_clear_vocalization(uint32_t actor_index);
extern void actor_command_list_reset_record(uint32_t actor_index, datum_index unit_index, uint16_t extra, void *component_record, int32_t secondary_record, uint32_t callback_extra);
}
}

/**
 * FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original
 * never reads EAX; actor_index arrive(s) on the stack (2 stack argument(s)).
 *
 * @address 0x407140
 */
int32_t ActorView::squad_action_status_broadcast(int16_t command_list_index, int16_t *record)
{
    using namespace actor_squad_action_status_broadcast_local;
    actor *a = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    int16_t *zero_cursor = record;
    int32_t i;

    for (i = 0x21; i != 0; i--) {
        zero_cursor[0] = 0;
        zero_cursor[1] = 0;
        zero_cursor += 2;
    }

    if (command_list_index < 0 || command_list_index >= (int32_t)global_scenario->command_lists.count) {
        return 0;
    }

    {
        ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
        ScenarioCommandList *list = &lists[command_list_index];

        if (a->swarm == 0 || a->swarm_index != (datum_index)k_datum_index_none) {
            if (list->precomputed_bsp_index == halo::k_word_none || list->precomputed_bsp_index == global_structure_bsp_index) {
                uint8_t allow_initiative = (uint8_t)(list->flags & 1);
                uint8_t allow_look = (uint8_t)(~(list->flags >> 2)) & 1;
                uint8_t allow_communication = (uint8_t)(~(list->flags >> 3)) & 1;

                record[0] = command_list_index;
                if (allow_look == 0) {
                    actor_clear_vocalization(actor_index);
                }
                *((uint8_t *)record + 4) = allow_communication;
                *((uint8_t *)record + 2) = allow_initiative;
                *((uint8_t *)record + 3) = allow_look;
                uint8_t flag_bit_1 = (uint8_t)((list->flags >> 1) & 1);

                actor_swarm_for_each_component(actor_index, 1, actor_command_list_reset_record,
                    (uint32_t)(uintptr_t)&flag_bit_1, (uint16_t *)record);
                return 1;
            }
        } else if (a->active == 0) {
            a->pending_command_list = command_list_index;
        }
    }
    return 0;
}

namespace actor_squad_react_to_grenade_local {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *encounter_data;
extern void actor_queue_recognized_target_dialogue(datum_index actor_index, datum_index target_prop_index);
extern void actor_react_to_seen_target(datum_index actor_index, datum_index target_prop_index);
extern void actor_scan_backup_and_panic_reaction(datum_index target_prop_index, datum_index actor_index);
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant);
extern uint32_t actor_target_data_release(datum_index target_prop_index, uint32_t actor_index, uint8_t *out_conflict_flag);
}
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
    actor *self = &((actor *)actor_data->data)[actor_index & halo::k_slot_mask];
    prop *target = &((prop *)prop_data->data)[target_prop_index & halo::k_slot_mask];
    uint8_t encounter_forbids = 0;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & halo::k_slot_mask];
        encounter_forbids = *((uint8_t *)enc + 0x41) != 0;
    }

    if (target->stimulus_type == -1 || target->stimulus_type <= grenade_type) {
        target->stimulus_type = grenade_type;
        target->stimulus_timer = (uint16_t)(((grenade_type != 3) - 1) & 0x78) + 0x1e;
    }

    switch (grenade_type) {
    case 0:
        if (target->disregarded == 0) {
            *(int16_t *)((uint8_t *)&target->auditory_perception + 2) = 3;
            target->perception_level = 3;
            target->combat_dirty = 1;
            actor_queue_recognized_target_dialogue(actor_index, target_prop_index);
        }
        break;
    case 1:
        if (encounter_forbids == 0 && target->disregarded == 0) {
            target->shooting = 1;
            target->combat_dirty = 1;
            *(int16_t *)&target->auditory_perception = 3;
            target->perception_level = 3;
            if (target->is_parented != 0) {
                actor_set_units_active(actor_index, 0);
            }
            actor_react_to_seen_target(actor_index, target_prop_index);
        }
        break;
    case 2:
        if (encounter_forbids == 0 && target->disregarded == 0) {
            target->dead = 1;
            *(int16_t *)&target->auditory_perception = 3;
            target->perception_level = 3;
            target->combat_dirty = 1;
            actor_set_units_active(actor_index, 0);
            actor_scan_backup_and_panic_reaction(target_prop_index, actor_index);
        }
        break;
    case 3:
        if (target->disregarded == 0) {
            *(int16_t *)((uint8_t *)&target->auditory_perception + 2) = 3;
            target->perception_level = 3;
            target->combat_dirty = 1;
            if (target->is_parented != 0) {
                actor_set_units_active(actor_index, 0);
            }
            actor_target_data_release(target_prop_index, actor_index, 0);
        }
        break;
    }
}

namespace actor_squad_react_to_grenade_for_vehicle_occupants_local {
extern "C" {
extern data_array *object_data;
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag);
extern void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index,
    int16_t grenade_type);
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & halo::k_slot_mask].data)
}
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
    uint8_t *occupant;
    datum_index occupant_index;
    datum_index actor;
    datum_index prop;

    if (vehicle_object_index == k_datum_index_none) {
        return;
    }
    vehicle = (uint8_t *)object_try_and_get(vehicle_object_index, 3);
    if (vehicle == 0) {
        return;
    }
    occupant_index = ((vehicle_object *)vehicle)->unit.driver_unit_index;
    if (occupant_index == k_datum_index_none) {
        occupant_index = vehicle_object_index;
    }
    occupant = OBJECT_DATA(occupant_index);
    if (*(int16_t *)(occupant + 0xb4) != 0) {
        return;
    }
    actor = *(datum_index *)(OBJECT_DATA(other_object_index) + 0x1f4);
    if (actor != k_datum_index_none) {
        prop = actor_find_or_create_shared_prop(occupant_index, actor, 1, 0);
        if (prop != k_datum_index_none) {
            actor_squad_react_to_grenade(actor, prop, 0);
        }
    }
    actor = *(datum_index *)(occupant + 0x1f4);
    if (actor != k_datum_index_none) {
        prop = actor_find_or_create_shared_prop(other_object_index, actor, 1, 0);
        if (prop != k_datum_index_none) {
            actor_squad_react_to_grenade(actor, prop, 0);
        }
    }
}

#undef OBJECT_DATA

}
