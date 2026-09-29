// actor_squad_action_execute  (Ghidra: actor_squad_action_execute; really: start one command-list atom)
// address 0x405520, size 4338 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x405520..0x406612, every atom checked (the draft dropped the register arguments of
//   actor_movement_actions_cancel, actor_begin_vocalization, object_try_and_get, unit_get_forward_vector_or_
//   marker_normal and more, returned 0 for a successful "go to", stored the look timer from a return value,
//   left the move-in-direction vector unnormalized and wrote the animation mode to aim_state +0x00). EAX: the
//   mode's aim record (may be 0); stack (actor, the unit the list drives, command list, list state). Returns 1
//   when the atom at state[0] started. This is how ai_command_list drives scripted actors such as the a10 crew.
//   Atoms: 0 pause, 1 go to, 2 go to and face, 3 move in direction, 4 look, 5 animation mode, 6 crouch,
//   7 shoot, 8 grenade, 9 vehicle, 10 running jump, 11 targeted jump, 12 script, 13 animate, 14 recording,
//   15 action, 16 vocalize, 17 targeting, 18 initiative, 19 wait, 20 loop, 21 die, 22 move immediate,
//   23 look random, 24 look player, 25 look object, 26 set radius, 27 teleport.
// blam-cc: EAX -> aim_state, stack -> (actor_index, check_object_index, command_list_index, state)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>
#include "game.h"


extern double fcos(double x); // FCOS
extern double fsin(double x); // FSIN

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;   // 0x00746f8c

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX a, ECX b
extern real random_real_range(real min, real max);             // 0x401050
extern real vector2d_normalize_with_length(real_vector2d *v);  // 0x4018e0, ECX
extern real vector3d_normalize_with_length(real_vector3d *v);  // 0x401990, ECX
extern int32_t random_int_range(int16_t min, int16_t max);      // 0x405320, ECX -> min, stack -> max
extern void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_axis_request *request); // 0x405390, EAX, EDX, ECX
extern uint8_t actor_play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index,
                                                   char *seat_name, int16_t seat_flags, int16_t count); // 0x40e260, EAX, ECX, stack
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant,
                                        actor_vocalization_context *context); // 0x4142d0, EAX, stack
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, EAX, stack
extern void actor_movement_actions_cancel(datum_index actor_index); // 0x417a30, EAX
extern void actor_fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context); // 0x4296c0, EBX, stack
extern void ai_communication_target_result_reset(ai_communication_target_result *record); // 0x42d310, EAX
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator); // 0x43ecd0, EAX, stack
extern prop *actor_prop_iterator_next(actor_prop_iterator *iterator); // 0x43ecf0, EDX
extern int16_t recorded_animation_find_by_name(const char *name, Scenario *scenario); // 0x449f80, EBX, ESI
extern uint8_t recorded_animation_start(datum_index unit_index, int16_t scenario_animation_index, uint16_t extra_flags);
    // 0x44a930, EAX, CX, stack
extern char hs_call_script_by_name(char *name); // 0x48a2d0, EAX
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, EDI
extern void object_reset_velocity_and_wake(uint32_t object_index); // 0x4f5160
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
                                                real_vector3d *up, real_point3d *position); // 0x4f51c0, stack, EDI position
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX out, ECX object
extern datum_index object_lookup_table_get(int16_t name_index); // 0x4f73c0, AX
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern int32_t object_iterator_next(object_iterator *iterator); // 0x4f6f20
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value); // 0x560d00, EAX, DL, stack
extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode); // 0x560f20, EAX, ECX, DX
extern void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out); // 0x568f50, ECX, ESI
extern void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out); // 0x569720, ECX, EAX
extern int32_t unit_set_grenade_type_and_count_delta(uint32_t unit_index, int16_t grenade_type, int8_t delta); // 0x56d160, EAX, DX, stack
extern uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name,
    uint8_t interpolate); // 0x5702a0, stack, EDI, EAX, stack
extern int32_t float_compare_ascending(const void *a, const void *b); // 0x405360
extern const char k_empty_string[]; // 0x0065512c, the empty string: a seat name filter matching every seat

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// state: +0x00 command index (byte), +0x02 timer ticks, +0x04 flags, +0x05 movement flags, +0x08 kind,
//   +0x0c direction / facing, +0x18 start position. aim_state (EAX): the mode's aim / look / fire record.
char actor_squad_action_execute(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index,
                                int16_t command_list_index, uint8_t *state)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
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
    case 0: // pause
        *(int16_t *)(state + 0x2) = (int16_t)(int32_t)(entry->parameter1 * 30.0f);
        return 1;

    case 1: // go to
    case 2: { // go to and face
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
                                                            (int32_t)point->surface_index, 0xffffffff);
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

    case 3: { // move in direction
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
            if (!(vector3d_normalize_with_length(direction) > 0.0f)) {
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

    case 4:    // look
    case 0x17: // look random
    case 0x18: // look player
    case 0x19: { // look object
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
                look_point = random_int_range(p1, (int16_t)(p2 + 1)); // FIXED: ECX = point_1 (0x40574d)
                if (entry->parameter1 == 0.0f && entry->parameter2 == 0.0f) {
                    duration = random_real_range(*(float *)(actor_tag + 0xec), *(float *)(actor_tag + 0xf0));
                } else {
                    duration = random_real_range(entry->parameter1, entry->parameter2);
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
                for (player = data_iterator_next(&players); player != 0; player = data_iterator_next(&players)) {
                    datum_index player_unit = ((struct player *)player)->unit;

                    if (player_unit != k_datum_index_none) {
                        real_point3d eye;
                        float distance;

                        unit_get_primary_eye_marker_position(player_unit, &eye);
                        distance = vector3d_distance_squared((real_point3d *)(act + 0x120), &eye);
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
            context.kind = 1;
            context.handle = look_prop;
        } else {
            context.kind = 3;
            if (look_object != k_datum_index_none) {
                unit_get_primary_eye_marker_position(look_object, (real_point3d *)&context.handle);
            } else {
                *(real_point3d *)&context.handle = *(real_point3d *)&points[(int16_t)look_point].position;
            }
        }
        actor_begin_vocalization(actor_index, 0xd, variant, &context);
        *(int16_t *)(state + 0x2) = (int16_t)(int32_t)(duration * 30.0f);
        return 1;
    }

    case 5: // animation mode
        if (aim_state == 0 || entry->atom_modifier < 0 || entry->atom_modifier >= 4) {
            return 0;
        }
        *(int16_t *)(aim_state + 0x2) = entry->atom_modifier;
        return 1;

    case 6: // crouch
        if (aim_state == 0) {
            return 0;
        }
        aim_state[0x0] = (uint8_t)(entry->atom_modifier == 1);
        return 1;

    case 7: { // shoot
        int16_t p = (int16_t)entry->point_1;

        if (aim_state == 0 || p < 0 || p >= point_count) {
            return 0;
        }
        aim_state[0x36] = 1;
        *(real_point3d *)(aim_state + 0x38) = *(real_point3d *)&points[p].position;
        *(float *)(aim_state + 0x44) = entry->parameter1;
        return 1;
    }

    case 8: { // grenade
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

    case 9: { // vehicle
        // 0x406091..0x4061ef: every vehicle (iterator mask 2) within parameter1 of the actor (any distance when
        //   parameter1 is 0), nearest first (at most 16), until the actor can board one of its seats: seats
        //   named "" (any) with the atom modifier (0..4, else -1) as seat flags (0x40e260: EAX 0, ECX vehicle).
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
            distance = vector3d_distance_squared(&position, (real_point3d *)(act + 0x12c));
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
            qsort(samples, sample_count, 8, float_compare_ascending);
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

    case 0xa: { // running jump
        uint8_t moving_forward;

        if (check_object_index == unit_index && ((actor *)act)->active_unit_index != k_datum_index_none) {
            return 0;
        }
        state[0x5] = (uint8_t)((state[0x5] & 0xe7) | 4);
        if (check_object_index == unit_index) {
            moving_forward = (uint8_t)(act[0x504] != 0 && ((struct actor *)act)->unknown_50a == 0);
        } else {
            uint8_t *obj = OBJECT_DATA(check_object_index);

            // 0x405c70: translational velocity (+0x68) along the forward vector (+0x74)
            moving_forward = (uint8_t)(((object *)obj)->parent_object == k_datum_index_none &&
                                       ((object *)obj)->velocity.k * ((object *)obj)->forward.k +
                                       ((object *)obj)->velocity.j * ((object *)obj)->forward.j +
                                       ((object *)obj)->forward.i * ((object *)obj)->velocity.i > 0.06666667f);
        }
        *(int16_t *)(state + 0x2) = 0x3c;
        *(int16_t *)(state + 0x8) = moving_forward ? 0 : 10;
        return 1;
    }

    case 0xb: // targeted jump
        if (check_object_index == unit_index && ((actor *)act)->active_unit_index != k_datum_index_none) {
            return 0;
        }
        state[0x5] = (uint8_t)((state[0x5] & 0xf7) | 0x14);
        *(int16_t *)(state + 0x8) = 0;
        *(float *)(state + 0xc) = entry->parameter1;
        *(float *)(state + 0x10) = entry->parameter2;
        *(int16_t *)(state + 0x2) = 0x3c;
        return 1;

    case 0xc: { // script
        int16_t script = (int16_t)entry->script;

        if (script < 0 || script >= *(int32_t *)((uint8_t *)global_scenario + 0x450)) {
            return 0;
        }
        return hs_call_script_by_name(*(char **)((uint8_t *)global_scenario + 0x454) + script * 0x28);
    }

    case 0xd: { // animate
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

    case 0xe: { // recording
        int16_t recording = (int16_t)entry->recording;
        int16_t animation_index;

        if (recording < 0 || recording >= *(int32_t *)((uint8_t *)global_scenario + 0x45c)) {
            return 0;
        }
        animation_index = recorded_animation_find_by_name(
            *(char **)((uint8_t *)global_scenario + 0x460) + recording * 0x28, global_scenario);
        if (animation_index == -1) {
            return 0;
        }
        return (char)recorded_animation_start(check_object_index, animation_index, 0);
    }

    case 0xf: // action
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

    case 0x10: { // vocalize
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

    case 0x11: // targeting
        if (entry->atom_modifier == 0) {
            state[0x4] |= 1;
        } else {
            state[0x4] &= 0xfe;
        }
        return 1;

    case 0x12: // initiative
        if (check_object_index != unit_index) {
            return 0;
        }
        act[0x9e] = (uint8_t)(entry->atom_modifier == 0);
        return 1;

    case 0x13: // wait
        return 1;

    case 0x14: { // loop: a valid other command to jump to
        int16_t target = (int16_t)entry->command;

        if (target < 0 || target >= command_count || target == command_index) {
            return 0;
        }
        return 1;
    }

    case 0x15: { // die
        uint8_t *obj = OBJECT_DATA(check_object_index);

        obj[0x106] |= entry->atom_modifier == 1 ? 0x40 : 0x20;
        return 1;
    }

    case 0x16: { // move immediate
        int16_t kind = entry->atom_modifier;

        *(int16_t *)(state + 0x8) = (kind >= 0 && kind <= 3) ? kind : 0;
        actor_get_body_axis_vector(actor_index, check_object_index, (actor_axis_request *)state);
        *(int16_t *)(state + 0x2) = (int16_t)(int32_t)(entry->parameter1 * 30.0f);
        state[0x5] |= 3;
        return 1;
    }

    case 0x1a: // set radius
        if (aim_state == 0 || !(entry->parameter1 > 0.0f)) {
            return 0;
        }
        aim_state[0x28] = 1;
        *(float *)(aim_state + 0x2c) = entry->parameter1;
        return 1;

    case 0x1b: { // teleport
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
                length = vector3d_normalize_with_length(&forward);
            } else {
                forward.k = 0.0f;
                length = vector2d_normalize_with_length((real_vector2d *)&forward);
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

#if 0
Original Ghidra decompilation (0x405520):

char actor_squad_action_execute(uint param_1,uint param_2,short param_3,byte *param_4)

{
  byte *pbVar1;
  int iVar2;
  byte bVar3;
  bool bVar4;
  char cVar5;
  undefined2 uVar6;
  short sVar7;
  int in_EAX;
  int iVar8;
  float *pfVar9;
  uint *puVar10;
  uint uVar11;
  short *psVar12;
  int iVar13;
  ushort uVar14;
  float *pfVar15;
  float fVar16;
  undefined4 uVar17;
  undefined4 *puVar18;
  bool bVar19;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float10 fVar20;
  char local_f7;
  int local_f4;
  float *local_f0;
  float local_ec;
  float local_e8;
  float local_e4;
  float local_e0;
  undefined4 local_dc;
  float local_d8;
  int local_d4;
  undefined2 local_d0 [2];
  float local_cc;
  undefined4 local_c8;
  undefined4 local_c4;
  uint local_c0;
  undefined2 local_bc;
  undefined4 local_b8;
  uint local_b4;
  undefined4 local_b0;
  float local_ac;
  float local_80 [32];
  
  iVar13 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_e8 = *(float *)((*(uint *)(iVar13 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar8 = *(int *)((*(uint *)(iVar13 + 0x5c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_d8 = *(float *)(param_3 * 0x60 + 0x30 + *(int *)(global_scenario + 0x43c));
  pfVar15 = (float *)(param_3 * 0x60 + *(int *)(global_scenario + 0x43c));
  local_ec = (float)(uint)*param_4;
  local_f7 = '\0';
  if ((int)local_d8 <= (int)local_ec) {
    return '\0';
  }
  psVar12 = (short *)((int)local_ec * 0x20 + (int)pfVar15[0xd]);
  local_f0 = pfVar15;
  switch(*psVar12) {
  case 0:
    uVar6 = __ftol();
    *(undefined2 *)(param_4 + 2) = uVar6;
    return '\x01';
  case 1:
  case 2:
    if (((in_EAX != 0) && (sVar7 = psVar12[6], -1 < sVar7)) && ((int)sVar7 < (int)pfVar15[0xf])) {
      puVar18 = (undefined4 *)((int)pfVar15[0x10] + sVar7 * 0x14);
      param_4[2] = 0;
      param_4[3] = 0;
      *(undefined1 *)(in_EAX + 4) = 1;
      *(bool *)(in_EAX + 5) = psVar12[1] == 1;
      *(undefined4 *)(in_EAX + 8) = *puVar18;
      *(undefined4 *)(in_EAX + 0xc) = puVar18[1];
      *(undefined4 *)(in_EAX + 0x10) = puVar18[2];
      uVar17 = puVar18[3];
      *(undefined4 *)(in_EAX + 0x14) = uVar17;
      local_f7 = actor_movement_set_destination_point(param_1,uVar17,0xffffffff);
      if (local_f7 != '\0') {
        if (*(char *)(in_EAX + 5) != '\0') {
          actor_movement_action_cancel();
        }
        if (((*psVar12 == 2) && (-1 < psVar12[7])) &&
           (iVar8 = (int)psVar12[7], iVar8 < (int)pfVar15[0xf])) {
          fVar16 = pfVar15[0x10];
          *(undefined1 *)(in_EAX + 0x18) = 1;
          iVar13 = (int)fVar16 + iVar8 * 0x14;
          *(undefined4 *)(in_EAX + 0x1c) = *(undefined4 *)((int)fVar16 + iVar8 * 0x14);
          *(undefined4 *)(in_EAX + 0x20) = *(undefined4 *)(iVar13 + 4);
          *(undefined4 *)(in_EAX + 0x24) = *(undefined4 *)(iVar13 + 8);
          return local_f7;
        }
      }
    }
    break;
  case 3:
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      pfVar9 = (float *)(param_4 + 0x18);
      *pfVar9 = *(float *)(iVar13 + 300);
      *(undefined4 *)(param_4 + 0x1c) = *(undefined4 *)(iVar13 + 0x130);
      *(undefined4 *)(param_4 + 0x20) = *(undefined4 *)(iVar13 + 0x134);
      local_f0 = pfVar9;
    }
    else {
      pfVar9 = (float *)object_get_position();
    }
    if ((psVar12[6] < 0) || (iVar8 = (int)psVar12[6], (int)pfVar15[0xf] <= iVar8)) {
      if (*(float *)(psVar12 + 4) < 0.0) {
        return '\0';
      }
      if (360.0 <= *(float *)(psVar12 + 4)) {
        return '\0';
      }
      fVar16 = *(float *)(psVar12 + 4);
      param_4[0x14] = 0;
      param_4[0x15] = 0;
      param_4[0x16] = 0;
      param_4[0x17] = 0;
      fVar20 = (float10)fcos((float10)fVar16 * (float10)0.017453292);
      *(float *)(param_4 + 0xc) = (float)fVar20;
      fVar20 = (float10)fsin((float10)fVar16 * (float10)0.017453292);
      *(float *)(param_4 + 0x10) = (float)fVar20;
    }
    else {
      iVar2 = (int)pfVar15[0x10] + iVar8 * 0x14;
      *(float *)(param_4 + 0xc) = *(float *)((int)pfVar15[0x10] + iVar8 * 0x14) - *pfVar9;
      *(float *)(param_4 + 0x10) = *(float *)(iVar2 + 4) - pfVar9[1];
      *(float *)(param_4 + 0x14) = *(float *)(iVar2 + 8) - pfVar9[2];
      fVar20 = (float10)vector3d_normalize_with_length();
      if (fVar20 <= (float10)0.0) {
        return '\0';
      }
    }
    sVar7 = psVar12[1];
    if ((sVar7 < 0) || (3 < sVar7)) {
      param_4[8] = 0xff;
      param_4[9] = 0xff;
    }
    else {
      *(short *)(param_4 + 8) = sVar7;
    }
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      actor_movement_action_stop();
    }
    param_4[5] = param_4[5] & 0xfd | 1;
    return '\x01';
  case 4:
  case 0x17:
  case 0x18:
  case 0x19:
    if (in_EAX == 0) {
      return '\0';
    }
    local_f4._0_2_ = -1;
    local_d8 = -NAN;
    local_d4 = -1;
    local_ec = *(float *)(psVar12 + 2);
    sVar7 = *psVar12;
    if (sVar7 == 4) {
      sVar7 = psVar12[6];
      if ((-1 < sVar7) && ((int)sVar7 < (int)pfVar15[0xf])) {
        local_ec = *(float *)(psVar12 + 2);
        local_f4._0_2_ = sVar7;
      }
    }
    else if (sVar7 == 0x17) {
      if ((-1 < psVar12[6]) && ((int)psVar12[6] < (int)pfVar15[0xf])) {
        uVar14 = psVar12[7];
        if ((-1 < (short)uVar14) && ((int)(short)uVar14 < (int)pfVar15[0xf])) {
          local_f4._0_2_ = random_int_range(uVar14 + 1);
          if ((*(float *)(psVar12 + 2) == 0.0) && (*(float *)(psVar12 + 4) == 0.0)) {
            local_ec = random_real_range(*(float *)((int)local_e8 + 0xec),
                                         *(float *)((int)local_e8 + 0xf0));
          }
          else {
            local_ec = random_real_range(*(float *)(psVar12 + 2),*(float *)(psVar12 + 4));
          }
        }
      }
    }
    else if (sVar7 == 0x18) {
      FUN_0043ecd0(&local_e8);
      iVar8 = FUN_0043ecf0();
      fVar16 = local_d8;
      fVar20 = extraout_ST0;
      if (iVar8 != 0) {
        do {
          if (((1 < *(short *)(iVar8 + 0x24)) && (*(short *)(iVar8 + 0x24) < 4)) &&
             ((*(char *)(iVar8 + 0x12e) != '\0' && ((float10)*(float *)(iVar8 + 0x11c) < fVar20))))
          {
            fVar16 = local_e8;
          }
          iVar8 = FUN_0043ecf0();
          fVar20 = extraout_ST0_00;
        } while (iVar8 != 0);
        local_d8 = fVar16;
        if (fVar16 != -NAN) goto LAB_00405961;
      }
      local_c0 = DAT_0087a480;
      local_b4 = DAT_0087a480 ^ 0x69746572;
      local_e8 = 3.4028235e+38;
      local_bc = 0;
      local_b8 = 0xffffffff;
      iVar8 = data_iterator_next();
      while (iVar8 != 0) {
        if (*(int *)(iVar8 + 0x34) != -1) {
          FUN_00568f50();
          fVar20 = (float10)FUN_00401020();
          pfVar15 = local_f0;
          if (fVar20 < (float10)local_e8) {
            local_d4 = *(int *)(iVar8 + 0x34);
            local_e8 = (float)fVar20;
          }
        }
        iVar8 = data_iterator_next();
      }
    }
    else if (((sVar7 == 0x19) && (-1 < psVar12[0xc])) &&
            ((int)psVar12[0xc] < *(int *)(global_scenario + 0x204))) {
      iVar8 = FUN_004f73c0();
      iVar13 = object_try_and_get(3);
      if (iVar13 != 0) {
        local_d8 = (float)FUN_0043ea80(iVar8);
        local_d4 = iVar8;
      }
    }
LAB_00405961:
    if ((0.0 < local_ec) &&
       (((local_d8 != -NAN || (local_d4 != -1)) ||
        ((-1 < (short)local_f4 && ((int)(short)local_f4 < (int)pfVar15[0xf])))))) {
      sVar7 = psVar12[1];
      uVar17 = 1;
      if (sVar7 == 1) {
        uVar17 = 5;
      }
      else if (sVar7 == 2) {
        uVar17 = 2;
      }
      else if (sVar7 == 4) {
        uVar17 = 7;
      }
      else if (sVar7 == 3) {
        uVar17 = 8;
      }
      if (local_d8 == -NAN) {
        local_d0[0] = 3;
        if (local_d4 == -1) {
          local_cc = *(float *)((int)pfVar15[0x10] + (short)local_f4 * 0x14);
          iVar8 = (int)pfVar15[0x10] + (short)local_f4 * 0x14;
          local_c8 = *(undefined4 *)(iVar8 + 4);
          local_c4 = *(undefined4 *)(iVar8 + 8);
        }
        else {
          FUN_00568f50();
        }
      }
      else {
        local_d0[0] = 1;
        local_cc = local_d8;
      }
      FUN_004142d0(0xd,uVar17,local_d0);
      uVar6 = __ftol();
      *(undefined2 *)(param_4 + 2) = uVar6;
      return '\x01';
    }
    break;
  case 5:
    if (((in_EAX != 0) && (sVar7 = psVar12[1], -1 < sVar7)) && (sVar7 < 4)) {
      *(short *)(in_EAX + 2) = sVar7;
      return '\x01';
    }
    break;
  case 6:
    if (in_EAX != 0) {
      *(bool *)in_EAX = psVar12[1] == 1;
      return '\x01';
    }
    break;
  case 7:
    if (((in_EAX != 0) && (sVar7 = psVar12[6], -1 < sVar7)) && ((int)sVar7 < (int)pfVar15[0xf])) {
      puVar18 = (undefined4 *)((int)pfVar15[0x10] + sVar7 * 0x14);
      *(undefined1 *)(in_EAX + 0x36) = 1;
      *(undefined4 *)(in_EAX + 0x38) = *puVar18;
      *(undefined4 *)(in_EAX + 0x3c) = puVar18[1];
      *(undefined4 *)(in_EAX + 0x40) = puVar18[2];
      *(undefined4 *)(in_EAX + 0x44) = *(undefined4 *)(psVar12 + 2);
      return '\x01';
    }
    break;
  case 8:
    if ((((in_EAX != 0) && (*(short *)(iVar8 + 0x180) != -1)) && (sVar7 = psVar12[6], -1 < sVar7))
       && ((int)sVar7 < (int)pfVar15[0xf])) {
      puVar18 = (undefined4 *)((int)pfVar15[0x10] + sVar7 * 0x14);
      FUN_0056d160(1);
      *(undefined1 *)(in_EAX + 0x49) = 0;
      *(undefined1 *)(in_EAX + 0x48) = 0;
      *(undefined4 *)(in_EAX + 0x4c) = *puVar18;
      *(undefined4 *)(in_EAX + 0x50) = puVar18[1];
      *(undefined4 *)(in_EAX + 0x54) = puVar18[2];
      *(undefined2 *)(in_EAX + 0x4a) = 0;
      sVar7 = psVar12[1];
      if ((-1 < sVar7) && (sVar7 < 3)) {
        *(short *)(in_EAX + 0x4a) = sVar7;
      }
      param_4[2] = 0x3c;
      param_4[3] = 0;
      return '\x01';
    }
    break;
  case 9:
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      uVar14 = 0;
      local_f0 = (float *)0xffffffff;
      local_dc = 0x86868686;
      local_e8 = 2.8026e-45;
      local_e4 = (float)(((uint)local_e4 >> 8 & 0xff) << 8);
      local_e0 = -NAN;
      iVar8 = object_iterator_next(&local_e8);
      if (iVar8 != 0) {
        do {
          fVar16 = local_e0;
          object_get_position();
          fVar20 = (float10)FUN_00401020();
          if ((*(float *)(psVar12 + 2) == 0.0) ||
             (fVar20 < (float10)*(float *)(psVar12 + 2) * (float10)*(float *)(psVar12 + 2))) {
            iVar8 = (int)(short)uVar14;
            uVar14 = uVar14 + 1;
            local_80[iVar8 * 2] = (float)fVar20;
            local_80[iVar8 * 2 + 1] = fVar16;
            if (0xf < uVar14) break;
          }
          iVar8 = object_iterator_next(&local_e8);
        } while (iVar8 != 0);
        if (1 < (short)uVar14) {
          _qsort(local_80,(int)(short)uVar14,8,float_compare_ascending);
        }
      }
      sVar7 = psVar12[1];
      if ((-1 < sVar7) && (sVar7 < 5)) {
        local_f0 = (float *)(int)sVar7;
      }
      pfVar15 = local_f0;
      sVar7 = 0;
      if (0 < (short)uVar14) {
        do {
          cVar5 = FUN_0040e260(param_1,&DAT_0065512c,pfVar15,0);
          if (cVar5 != '\0') {
            param_4[4] = param_4[4] | 4;
            return '\x01';
          }
          sVar7 = sVar7 + 1;
        } while (sVar7 < (short)uVar14);
        return '\0';
      }
    }
    break;
  case 10:
    if ((param_2 == *(uint *)(iVar13 + 0x18)) && (*(int *)(iVar13 + 0x158) != -1)) {
      return '\0';
    }
    param_4[5] = param_4[5] & 0xe7 | 4;
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      if (*(char *)(iVar13 + 0x504) != '\0') {
        bVar19 = *(short *)(iVar13 + 0x50a) == 0;
LAB_00405c91:
        if (bVar19) {
          bVar3 = 1;
          goto LAB_00405c99;
        }
      }
    }
    else {
      iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
      if (*(int *)(iVar8 + 0x11c) == -1) {
        bVar19 = 0.06666667 <
                 *(float *)(iVar8 + 0x74) * *(float *)(iVar8 + 0x68) +
                 *(float *)(iVar8 + 0x6c) * *(float *)(iVar8 + 0x78) +
                 *(float *)(iVar8 + 0x70) * *(float *)(iVar8 + 0x7c);
        goto LAB_00405c91;
      }
    }
    bVar3 = 0;
LAB_00405c99:
    param_4[2] = 0x3c;
    param_4[3] = 0;
    *(ushort *)(param_4 + 8) = bVar3 - 1 & 10;
    return '\x01';
  case 0xb:
    if ((param_2 != *(uint *)(iVar13 + 0x18)) || (*(int *)(iVar13 + 0x158) == -1)) {
      param_4[5] = param_4[5] & 0xf7 | 0x14;
      param_4[8] = 0;
      param_4[9] = 0;
      *(undefined4 *)(param_4 + 0xc) = *(undefined4 *)(psVar12 + 2);
      *(undefined4 *)(param_4 + 0x10) = *(undefined4 *)(psVar12 + 4);
      param_4[2] = 0x3c;
      param_4[3] = 0;
      return '\x01';
    }
    break;
  case 0xc:
    if ((-1 < psVar12[9]) && ((int)psVar12[9] < *(int *)(global_scenario + 0x450))) {
      cVar5 = hs_call_script_by_name();
      return cVar5;
    }
    break;
  case 0xd:
    if (psVar12[8] == -1) {
      return '\0';
    }
    bVar19 = false;
    bVar4 = false;
    local_f4._1_3_ = (uint3)((uint)iVar8 >> 8);
    local_f4 = CONCAT31(local_f4._1_3_,1);
    switch(psVar12[1]) {
    case 1:
      break;
    case 2:
      goto switchD_00406262_caseD_2;
    case 3:
      local_f4 = (uint)local_f4._1_3_ << 8;
      goto switchD_00406262_default;
    case 4:
      local_f4 = (uint)local_f4._1_3_ << 8;
      break;
    case 5:
      local_f4 = (uint)local_f4._1_3_ << 8;
switchD_00406262_caseD_2:
      bVar4 = true;
      break;
    default:
      goto switchD_00406262_default;
    }
    bVar19 = true;
switchD_00406262_default:
    cVar5 = unit_start_user_animation(param_2,local_f4);
    if (cVar5 == '\0') {
      return '\0';
    }
    iVar8 = object_try_and_get(1);
    if (iVar8 != 0) {
      if (bVar19) {
        uVar11 = *(uint *)(iVar8 + 0x4cc) | 4;
      }
      else {
        uVar11 = *(uint *)(iVar8 + 0x4cc) & 0xfffffffb;
      }
      *(uint *)(iVar8 + 0x4cc) = uVar11;
      if (!bVar4) {
        *(uint *)(iVar8 + 0x4cc) = *(uint *)(iVar8 + 0x4cc) & 0xfffffff7;
        return '\x01';
      }
      *(uint *)(iVar8 + 0x4cc) = *(uint *)(iVar8 + 0x4cc) | 8;
      return '\x01';
    }
  case 0x13:
switchD_004055d3_caseD_13:
    local_f7 = '\x01';
    break;
  case 0xe:
    if (((-1 < psVar12[10]) && ((int)psVar12[10] < *(int *)(global_scenario + 0x45c))) &&
       (sVar7 = FUN_00449f80(), sVar7 != -1)) {
      cVar5 = FUN_0044a930(0);
      return cVar5;
    }
    break;
  case 0xf:
    if (in_EAX == 0) {
      return '\0';
    }
    *(undefined1 *)(in_EAX + 0x30) = 0;
    switch(psVar12[1]) {
    case 0:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 0;
      *(undefined2 *)(in_EAX + 0x34) = 0x2a;
      return *(char *)(in_EAX + 0x30);
    case 1:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 4;
      *(undefined2 *)(in_EAX + 0x34) = 0x29;
      return *(char *)(in_EAX + 0x30);
    case 2:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 5;
      *(undefined2 *)(in_EAX + 0x34) = 0x29;
      return *(char *)(in_EAX + 0x30);
    case 3:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 6;
      *(undefined2 *)(in_EAX + 0x34) = 0xffff;
      return *(char *)(in_EAX + 0x30);
    case 4:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 7;
      *(undefined2 *)(in_EAX + 0x34) = 0xffff;
      return *(char *)(in_EAX + 0x30);
    case 5:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 8;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 6:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 9;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 7:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 10;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 8:
      *(undefined1 *)(in_EAX + 0x30) = 1;
      *(undefined2 *)(in_EAX + 0x32) = 0xb;
      *(undefined2 *)(in_EAX + 0x34) = 0x2c;
      return *(char *)(in_EAX + 0x30);
    case 9:
      *(undefined2 *)(in_EAX + 0x34) = 0x26;
      break;
    case 10:
      *(undefined2 *)(in_EAX + 0x34) = 0x27;
      break;
    default:
      goto switchD_00405e1c_default;
    }
    *(undefined2 *)(in_EAX + 0x32) = 0xffff;
    *(undefined1 *)(in_EAX + 0x30) = 1;
switchD_00405e1c_default:
    return *(char *)(in_EAX + 0x30);
  case 0x10:
    local_f0 = (float *)(uint)(ushort)psVar12[1];
    local_e8 = -NAN;
    sVar7 = FUN_00560d00(6,1,0,&local_f0,&local_e8);
    if (0 < sVar7) {
      puVar18 = &local_b0;
      for (iVar8 = 0xc; iVar8 != 0; iVar8 = iVar8 + -1) {
        *puVar18 = 0;
        puVar18 = puVar18 + 1;
      }
      local_ac = local_e8;
      local_b0._2_2_ = local_f0._0_2_;
      local_b0._0_2_ = 6;
      FUN_0042d310();
      FUN_00560f20();
      return '\x01';
    }
    break;
  case 0x11:
    if (psVar12[1] != 0) {
      param_4[4] = param_4[4] & 0xfe;
      return '\x01';
    }
    param_4[4] = param_4[4] | 1;
    return '\x01';
  case 0x12:
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      *(bool *)(iVar13 + 0x9e) = psVar12[1] == 0;
      return '\x01';
    }
    break;
  case 0x14:
    sVar7 = psVar12[0xb];
    if (((-1 < sVar7) && ((int)sVar7 < (int)local_d8)) && ((float)(int)sVar7 != local_ec)) {
      return '\x01';
    }
    break;
  case 0x15:
    iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
    if (psVar12[1] != 1) {
      pbVar1 = (byte *)(iVar8 + 0x106);
      *pbVar1 = *pbVar1 | 0x20;
      return '\x01';
    }
    pbVar1 = (byte *)(iVar8 + 0x106);
    *pbVar1 = *pbVar1 | 0x40;
    return '\x01';
  case 0x16:
    sVar7 = psVar12[1];
    if ((sVar7 < 0) || (3 < sVar7)) {
      param_4[8] = 0;
      param_4[9] = 0;
    }
    else {
      *(short *)(param_4 + 8) = sVar7;
    }
    FUN_00405390();
    uVar6 = __ftol();
    *(undefined2 *)(param_4 + 2) = uVar6;
    param_4[5] = param_4[5] | 3;
    return '\x01';
  case 0x1a:
    if ((in_EAX != 0) && (0.0 < *(float *)(psVar12 + 2))) {
      *(undefined1 *)(in_EAX + 0x28) = 1;
      *(undefined4 *)(in_EAX + 0x2c) = *(undefined4 *)(psVar12 + 2);
      return '\x01';
    }
    break;
  case 0x1b:
    sVar7 = psVar12[6];
    if (sVar7 < 0) {
      return '\0';
    }
    if ((int)pfVar15[0xf] <= (int)sVar7) {
      return '\0';
    }
    pfVar9 = (float *)((int)pfVar15[0x10] + sVar7 * 0x14);
    FUN_00569720();
    sVar7 = psVar12[7];
    if ((-1 < sVar7) && ((int)sVar7 < (int)pfVar15[0xf])) {
      pfVar15 = (float *)((int)pfVar15[0x10] + sVar7 * 0x14);
      puVar10 = (uint *)object_try_and_get(1);
      iVar8 = 0;
      if (puVar10 != (uint *)0x0) {
        iVar8 = *(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      }
      local_e8 = *pfVar15 - *pfVar9;
      local_e4 = pfVar15[1] - pfVar9[1];
      local_e0 = pfVar15[2] - pfVar9[2];
      if ((iVar8 == 0) || ((*(byte *)(iVar8 + 0x2f4) & 0x44) == 0)) {
        local_e0 = 0.0;
        fVar20 = (float10)vector2d_normalize_with_length();
      }
      else {
        fVar20 = (float10)vector3d_normalize_with_length();
      }
      if ((float10)0.0 == fVar20) {
        FUN_00569720();
      }
    }
    object_set_position_and_orientation(param_2,&local_e8,0);
    object_reset_velocity_and_wake(param_2);
    object_recalculate_bounding_radius_recursive(param_2);
    if (param_2 == *(uint *)(iVar13 + 0x18)) {
      FUN_004296c0(iVar13 + 0x120);
      actor_movement_action_stop();
    }
    goto switchD_004055d3_caseD_13;
  }
  return local_f7;
}

#endif
