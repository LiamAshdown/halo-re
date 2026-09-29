// actor_mode_guard_update  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x1c)
// address 0x404d60, size 933 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x404d60..0x405105 (no C existed: a mode entered in game would have hit a trap).
//   Guarding: fire permission (+0x426 / +0x427: Actor flag 0x40 while calm, an ambush unless +0xa6 needs flag
//   0x800000, flag 0x80 once alert). With a path wanted (not a swarm) by guard kind (+0xc0): 0 / 1 stand; 2 walk to
//   the guard point (+0xc4, surface +0xd0) until within its radius (+0xd4), in place within 3; 3 take the firing
//   position (+0xc4, recognised as bad on failure) and be in place within 3 of the path end (+0x4ac) or when not
//   moving. In place (+0xa0): a watched object (+0xab / +0xac) becomes a perception (0x422070 event 2, 600) and is
//   announced (event 7); a firing position holder reports (0x4048b0) and a kind-2 guard of command 9 attacks (+0xa3).
//   Firing request: attacking kind 7 / style 2 at the guard point raised 0.05 (+0x454, +0x45d, +0x460); a watched
//   object (+0xd8) kind 5 / style 1; a look point (+0xb0) style 4, kind 3 or 5 (+0xb1) at +0xb4; a known target when
//   alert kind 3 / style 1; else nothing. Fire kind 2, or 4 in combat (+0x6e 4+).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments
extern const real_vector3d *global_up3d_pointer; // 0x00696720
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX, ECX
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, EAX, stack
extern uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot,
                                                              path_find_context *path_context); // 0x417830, EDI, stack
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type); // 0x4141a0, EAX, CX, DL
extern void actor_record_perception_event(datum_index actor_index, int16_t event, int32_t data); // 0x422070, EAX, EDX, ESI
extern int32_t actor_report_command_status(uint32_t actor_index); // 0x4048b0, EAX

void actor_mode_guard_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint32_t actor_flags = *(uint32_t *)actor_tag;

    if ((actor_flags & 0x40) && ((struct actor *)act)->combat_status == 0) {
        act[0x426] = 1;
        act[0x427] = 1;
    } else {
        act[0x427] = 0;
        if (act[0xa4]) {
            act[0x426] = act[0xa6] ? (uint8_t)((actor_flags >> 23) & 1) : 1;
        } else {
            act[0x426] = (uint8_t)((actor_flags & 0x80) && ((struct actor *)act)->combat_status > 0);
        }
    }
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;

    if (act[0x4c] && !act[0x6]) {
        uint8_t in_place = 0;

        switch (((struct actor *)act)->mode_data.guard.stage) { // 0x4050f0
        case 0:
        case 1:
            actor_movement_action_stop(actor_index);
            in_place = 1;
            break;
        case 2: {
            float distance_squared = vector3d_distance_squared((real_point3d *)(act + 0xc4), (real_point3d *)(act + 0x12c));
            float radius = *(float *)(act + 0xd4);

            if (distance_squared < radius * radius) {
                actor_movement_action_stop(actor_index);
            } else {
                actor_movement_set_destination_point((real_point3d *)(act + 0xc4), actor_index, *(int32_t *)(act + 0xd0), -1);
            }
            in_place = (uint8_t)(distance_squared < 9.0f);
            break;
        }
        case 3: {
            int16_t position = ((struct actor *)act)->mode_data.guard.firing_position;

            if (position != -1) {
                ((actor *)act)->firing_position_index = position;
                act[0x3ba] = 0;
                if (!actor_movement_set_destination_firing_position(actor_index, position, 0)) {
                    actor_push_recognition_entry(actor_index, ((struct actor *)act)->mode_data.guard.firing_position, 0);
                    ((actor *)act)->firing_position_index = -1;
                }
            }
            if (!act[0x4a8]) {
                in_place = 1;
            } else {
                in_place = (uint8_t)(vector3d_distance_squared((real_point3d *)(act + 0x12c), (real_point3d *)(act + 0x4ac)) < 9.0f);
            }
            break;
        }
        default:
            break;
        }
        act[0xa0] = 1;
        if (in_place) {
            if (act[0xab]) {
                uint8_t *watched = PROP(((struct actor *)act)->mode_data.guard.hold_reference);

                act[0xab] = 0;
                *(int32_t *)(act + 0xac) = -1;
                actor_record_perception_event(actor_index, 2, 600);
                ai_communication_broadcast(7, ((actor *)act)->unit_index, *(datum_index *)(watched + 0x18), -1, -1, 2, 0);
            }
            if (act[0xa1]) {
                actor_report_command_status(actor_index);
                if (((struct actor *)act)->unknown_1e4 == 9 && ((struct actor *)act)->mode_data.guard.stage == 2) {
                    act[0xa3] = 1;
                }
            }
        }
    }

    if (act[0xa3]) {
        ((actor *)act)->vocalization_unknown_3e8 = 7;
        ((actor *)act)->vocalization_unknown_3ec = 2;
        act[0x454] = 1;
        act[0x45d] = 1;
        *(float *)(act + 0x460) = global_up3d_pointer->i * 0.05f + *(float *)(act + 0xc4);
        *(float *)(act + 0x464) = global_up3d_pointer->j * 0.05f + *(float *)(act + 0xc8);
        *(float *)(act + 0x468) = global_up3d_pointer->k * 0.05f + *(float *)(act + 0xcc);
    } else if (((struct actor *)act)->mode_data.guard.guard_target != k_datum_index_none) {
        ((actor *)act)->vocalization_unknown_3e8 = 5;
        ((actor *)act)->vocalization_unknown_3ec = 1;
        *(datum_index *)(act + 0x3f0) = ((struct actor *)act)->mode_data.guard.guard_target;
    } else if (act[0xb0]) {
        ((actor *)act)->vocalization_unknown_3ec = 4;
        ((actor *)act)->vocalization_unknown_3e8 = act[0xb1] ? 5 : 3;
        *(real_point3d *)(act + 0x3f0) = *(real_point3d *)(act + 0xb4);
    } else if (((struct actor *)act)->combat_status > 0 && ((actor *)act)->target_unit_index != k_datum_index_none) {
        ((actor *)act)->vocalization_unknown_3e8 = 3;
        ((actor *)act)->vocalization_unknown_3ec = 1;
        *(datum_index *)(act + 0x3f0) = ((actor *)act)->target_unit_index;
    } else {
        ((actor *)act)->vocalization_unknown_3e8 = 0;
    }
    ((struct actor *)act)->unknown_3fc = ((struct actor *)act)->combat_status >= 4 ? 4 : 2;
}
