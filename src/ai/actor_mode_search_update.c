// actor_mode_search_update  (not a Ghidra function: actor mode table 0x65524c, mode "search" slot +0x1c)
// address 0x407f40, size 324 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x407f40..0x408084 (no C existed: a mode entered in game would have hit a trap).
//   Firing request while searching: walking (+0x504) kind 3 / style 0; in the first third of the search (at least
//   90 ticks) kind 3 with style 2 for a plain search, or style 3 at the search point (+0xb0) for kind 1; else
//   kind 1. Fire kind 3; a plain search may shoot blind (+0x454) at morale 5+ (6+ without Actor flag 0x10); the
//   request flags +0x426 / +0x427 follow +0x9f, +0x425 is set.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

void actor_mode_search_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);

    if (act[0x504]) {
        ((actor *)act)->vocalization_unknown_3e8 = 3;
        ((actor *)act)->vocalization_unknown_3ec = 0;
    } else {
        int32_t total = ((struct actor *)act)->mode_data.search.duration_ticks;
        int32_t third = total / 3;

        if (third <= 90) {
            third = 90;
        }
        if (total - ((struct actor *)act)->mode_data.search.remaining_ticks < third && ((struct actor *)act)->mode_data.search.stage == 0) {
            ((actor *)act)->vocalization_unknown_3e8 = 3;
            ((actor *)act)->vocalization_unknown_3ec = 2;
        } else if (total - ((struct actor *)act)->mode_data.search.remaining_ticks < third && ((struct actor *)act)->mode_data.search.stage == 1) {
            ((actor *)act)->vocalization_unknown_3e8 = 3;
            ((actor *)act)->vocalization_unknown_3ec = 3;
            *(real_point3d *)(act + 0x3f0) = ((struct actor *)act)->mode_data.search.position;
        } else {
            ((actor *)act)->vocalization_unknown_3e8 = 1;
        }
    }
    *(int16_t *)(act + 0x3fc) = 3;
    if (((struct actor *)act)->mode_data.search.stage == 0) {
        act[0x454] = (uint8_t)(((actor *)act)->target_combat_status >= ((actor_tag[0] & 0x10) ? 5 : 6));
    }
    act[0x426] = act[0x9f];
    act[0x427] = act[0x9f];
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 1;
}
