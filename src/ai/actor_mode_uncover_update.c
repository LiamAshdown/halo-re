// actor_mode_uncover_update  (not a Ghidra function: actor mode table 0x65524c, mode "uncover" slot +0x1c)
// address 0x408680, size 376 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x408680..0x4087f8 (no C existed: a mode entered in game would have hit a trap).
//   Firing request while uncovering a target: a plain uncover shoots blind -- forced by +0x162 (also +0x455), else at
//   morale 5+ (6+ without Actor flag 0x10). Kind 7 when shooting blind at a target of kind 0 / 1 (or forced), else 3
//   below morale 5, 2 for targets of kind 2 / 4, else 5; style 2 for a plain uncover, 3 at the uncover point (+0xb0)
//   for kind 1. Fire kind 3; +0x426 / +0x427 follow +0x9c, +0x425 set.
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
extern data_array *prop_data; // 0x008802c0

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

void actor_mode_uncover_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    datum_index target = ((actor *)act)->target_unit_index;

    if (target != k_datum_index_none) {
        uint8_t *p = PROP(target);
        uint8_t forced = 0;
        int16_t kind = *(int16_t *)(p + 0x38);

        if (*(int16_t *)(act + 0xa4) == 0) {
            if (act[0x162]) {
                act[0x454] = 1;
                act[0x455] = 1;
                forced = 1;
            } else {
                act[0x454] = (uint8_t)(((actor *)act)->target_combat_status >= ((actor_tag[0] & 0x10) ? 5 : 6));
            }
        }
        if ((act[0x454] && (kind == 0 || kind == 1)) || forced) {
            ((actor *)act)->vocalization_unknown_3e8 = 7;
        } else if (((actor *)act)->target_combat_status < 5) {
            ((actor *)act)->vocalization_unknown_3e8 = 3;
        } else if (kind == 2 || kind == 4) {
            ((actor *)act)->vocalization_unknown_3e8 = 2;
        } else {
            ((actor *)act)->vocalization_unknown_3e8 = 5;
        }
        if (*(int16_t *)(act + 0xa4) == 0) {
            ((actor *)act)->vocalization_unknown_3ec = 2;
        } else if (*(int16_t *)(act + 0xa4) == 1) {
            ((actor *)act)->vocalization_unknown_3ec = 3;
            *(real_point3d *)(act + 0x3f0) = *(real_point3d *)(act + 0xb0);
        }
    }
    *(int16_t *)(act + 0x3fc) = 3;
    act[0x426] = act[0x9c];
    act[0x427] = act[0x9c];
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 1;
}
