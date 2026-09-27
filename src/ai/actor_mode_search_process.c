// actor_mode_search_process  (not a Ghidra function: actor mode table 0x65524c, mode "search" slot +0x14)
// address 0x407a10, size 869 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x407a10..0x407d75 (no C existed: a mode entered in game would have hit a trap).
//   Searching (returns +0x9c, "done"): only for a non-swarm actor wanting a path (+0x4c) and not yet done. Arrived
//   (+0x9e): a plain search within 1.7 (0.7 for a target of kind 1+) of the target's last position (prop +0xbc); a
//   point search within 0.7 of its point (+0xb0), or within 2.5 with the point in view (0x569190 marker offset,
//   0x42b270 reachability). Not arrived and not told to keep going (+0xa1): allies (props of kind 2..3 with an actor
//   searching the same thing, 0x40e380) are counted; with enough of them (4, or 2 for a point search) the search is
//   done and noted for the encounter (0x436b10); otherwise a close, idle ally means arrived. Arrived: stop; else walk
//   to the target (within 2.5, 0x417910) or the search firing position (0x417830), giving up when that fails.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data; // 0x008802c0

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX, ECX
extern void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t param_2, float *world_point,
    uint32_t param_4, uint32_t param_5, real_point3d *accumulator); // 0x569190, stack, EAX
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying); // 0x42b270, AX, CX, ESI, EDI, stack
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator); // 0x43ecd0, EAX, stack
extern uint8_t actor_targets_share_descriptor(datum_index actor_a, datum_index actor_b); // 0x40e380, EAX, ECX
extern uint8_t ai_pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type,
                                      int32_t min_last_tick); // 0x436b10, EDX, stack, CX, EAX
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius); // 0x417910, EAX, stack
extern uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot,
                                                              path_find_context *path_context); // 0x417830, EDI, stack

uint8_t actor_mode_search_process(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    int16_t kind;
    uint8_t ok;

    if (act[0x6] || !act[0x4c] || act[0x9c]) {
        return act[0x9c];
    }
    kind = *(int16_t *)(act + 0xa4);
    act[0x9e] = 1;
    if (kind == 0 && *(datum_index *)(act + 0x270) != k_datum_index_none) {
        uint8_t *target = PROP(*(datum_index *)(act + 0x270));
        float radius = *(int16_t *)(target + 0x38) == 0 ? 1.7f : 0.7f;
        float distance_squared = vector3d_distance_squared((real_point3d *)(act + 0x12c), (real_point3d *)(target + 0xbc));

        act[0x9e] = (uint8_t)(radius * radius > distance_squared);
    } else if (kind == 1 && *(int16_t *)(act + 0xa6) != -1) {
        float distance_squared = vector3d_distance_squared((real_point3d *)(act + 0x12c), (real_point3d *)(act + 0xb0));

        if (distance_squared < 0.49f) {
            act[0x9e] = 1;
        } else if (distance_squared > 6.25f) {
            act[0x9e] = 0;
        } else {
            real_point3d in_view;

            // 0x407b33: the marker call's five stack arguments stay pushed under the reachability call's four
            unit_add_marker_relative_offset(*(datum_index *)(act + 0x18), 1, (float *)(act + 0xb0), 0, 0, &in_view);
            act[0x9e] = (uint8_t)(actor_evaluate_engagement_reachability(*(int16_t *)(act + 0x148), *(int16_t *)(act + 0xa8),
                                                                         &in_view, (real_point3d *)(act + 0x120), 0, 0, -1,
                                                                         (uint8_t)(*(datum_index *)(act + 0x158) !=
                                                                                   k_datum_index_none)) == 0);
        }
    }
    if (!act[0x9e] && !act[0xa1]) {
        actor_prop_iterator iterator;
        datum_index prop_index;
        int16_t sharing = 0;
        int32_t close_idle = 0;

        actor_prop_iterator_init(actor_index, &iterator);
        for (prop_index = iterator.next; prop_index != k_datum_index_none;) {
            uint8_t *p = PROP(prop_index);
            int16_t prop_kind = *(int16_t *)(p + 0x24);

            prop_index = *(datum_index *)(p + 0x8);
            if (prop_kind >= 2 && prop_kind <= 3 && !p[0x60] && !p[0x127] &&
                *(datum_index *)(p + 0x1c) != k_datum_index_none &&
                actor_targets_share_descriptor(actor_index, *(datum_index *)(p + 0x1c))) {
                uint8_t *other = ACTOR(*(datum_index *)(p + 0x1c));

                sharing++;
                if (!other[0x6] && !other[0x504] &&
                    vector3d_distance_squared((real_point3d *)(other + 0x12c), (real_point3d *)(act + 0x12c)) < 0.64000005f) {
                    close_idle++;
                }
            }
        }
        if (!act[0xa0] && sharing >= (*(int16_t *)(act + 0xa4) != 1 ? 4 : 2)) {
            datum_index last_seen = -1;

            act[0x9c] = 1;
            if (*(datum_index *)(act + 0x270) != k_datum_index_none) {
                last_seen = *(datum_index *)(PROP(*(datum_index *)(act + 0x270)) + 0x7c);
            }
            if (*(datum_index *)(act + 0x34) != k_datum_index_none) {
                ai_pursuit_note_object(actor_index, *(datum_index *)(act + 0x34), *(int16_t *)(act + 0xa6), last_seen);
            }
        } else if ((int16_t)close_idle > 0) {
            act[0x9e] = 1;
        }
    }
    if (act[0x9e]) {
        actor_movement_action_stop(actor_index);
        return act[0x9c];
    }
    kind = *(int16_t *)(act + 0xa4);
    if (kind == 0) {
        ok = actor_movement_set_destination_near_target(*(datum_index *)(act + 0x270), actor_index, 2.5f);
    } else if (kind == 1) {
        *(int16_t *)(act + 0x3b8) = -1;
        ok = actor_movement_set_destination_firing_position(actor_index, *(int16_t *)(act + 0xa6), 0);
    } else {
        actor_movement_action_stop(actor_index);
        return act[0x9c];
    }
    if (!ok) {
        act[0x9c] = 1;
        act[0x9d] = 1;
    }
    return act[0x9c];
}
