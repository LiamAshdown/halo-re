// actor_mode_wait_process  (not a Ghidra function: actor mode table 0x65524c, mode "wait" slot +0x14)
// address 0x409b30, size 394 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x409b30..0x409cba (no C existed: a mode entered in game would have hit a trap).
//   Waiting (returns +0x9c, "done"): only while a path is wanted (+0x4c). The nearest grenade-carrying ally is looked
//   up (0x40e540). Waiting for someone (+0x9d): with nobody (+0x1d0 none) a 150-tick wait starts; with someone the
//   wait ends 2700 ticks after it began (+0xa4). Otherwise the wait is done unless the ally (a live prop, kind 2+,
//   within 8) should be followed: beyond 3.5 of it the actor walks to within 8 of it (0x417910), failing marks
//   +0xa0; else it stops.
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

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

extern data_array *prop_data;          // 0x008802c0
extern game_time_globals *game_time;    // 0x006f1d6c
extern int32_t actor_find_nearest_grenade_ally(datum_index actor_index, uint8_t widen_search); // 0x40e540
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius); // 0x417910, EAX, stack

uint8_t actor_mode_wait_process(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (!act[0x4c]) {
        return act[0x9c];
    }
    ((struct actor *)act)->mode_data.wait.unknown_03 = 0;
    actor_find_nearest_grenade_ally(actor_index, act[0x1cc]);
    if (act[0x9d]) {
        if (((struct actor *)act)->unknown_1d0 == k_datum_index_none) {
            if (((struct actor *)act)->mode_data.wait.countdown_150 == 0) {
                ((struct actor *)act)->mode_data.wait.countdown_150 = 150;
            }
        } else if (game_time->game_time >= ((struct actor *)act)->mode_data.wait.start_game_time + 2700) {
            act[0x9c] = 1;
        }
    } else {
        act[0x9c] = 1;
        if (((struct actor *)act)->unknown_1d0 != k_datum_index_none) {
            uint8_t *ally = (uint8_t *)prop_data->data + (((struct actor *)act)->unknown_1d0 & 0xffff) * 0x138;
            float distance = ((prop *)ally)->distance;
            uint8_t follow;

            if (act[0x9e] && !act[0xa0]) {
                follow = 1;
            } else if (((struct prop *)ally)->visual_perception < 2 || !(distance < 8.0f)) {
                follow = 0; // stays done
                goto decided;
            } else {
                follow = act[0xa0] == 0;
            }
            if (follow && distance > 3.5f) {
                ((struct actor *)act)->mode_data.wait.unknown_03 = 1;
                act[0x9c] = 0;
            } else {
                ((struct actor *)act)->mode_data.wait.unknown_03 = 0;
                act[0x9c] = 0;
            }
        }
    }
decided:
    if (act[0x6]) {
        return act[0x9c];
    }
    if (((struct actor *)act)->mode_data.wait.unknown_03) {
        uint8_t done = act[0x9c];

        if (!actor_movement_set_destination_near_target(((struct actor *)act)->unknown_1d0, actor_index, 8.0f)) {
            act[0xa0] = 1;
        }
        return done;
    }
    actor_movement_action_stop(actor_index);
    return act[0x9c];
}
