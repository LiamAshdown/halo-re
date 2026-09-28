// actor_mode_charge_update  (not a Ghidra function: actor mode table 0x65524c, mode "charge" slot +0x1c)
// address 0x402af0, size 511 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x402af0..0x402cef (no C existed: a mode entered in game would have hit a trap).
//   Firing request while charging: style 2, fire kind 4; request kind 4 while a leaping / berserk charge (kinds 2 / 3)
//   has closed in (+0xa5) and stands, 7 for a fighting actor (+0x6e 5+) not on a kind-1 charge, else 5. Kind-1
//   charges fire only before striking (+0xc1); otherwise +0x426 / +0x427 follow +0x358 for Actor flag 0x10000
//   unless +0x428. A pending melee strike (+0xa8) is issued (+0x440..0x450 from +0xb0..0xbc, +0x441 when the
//   reach is less than 0.7 of the gap) and timed (+0xa7 / +0xac / +0xaa). Actor flag 0x100000 lets a charge (or a
//   retreat, +0x378) keep firing while +0xc4 (+0x428). +0x42a set, +0x454 unless kind 1.
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

extern game_time_globals *game_time; // 0x006f1d6c

void actor_mode_charge_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint32_t actor_flags = *(uint32_t *)actor_tag;
    int16_t kind = ((struct actor *)act)->mode_data.charge.stage;

    ((actor *)act)->vocalization_unknown_3ec = 2;
    ((struct actor *)act)->unknown_3fc = 4;
    if ((kind == 2 || kind == 3) && act[0xa5] && !act[0x504] && !act[0x4a8]) {
        ((actor *)act)->vocalization_unknown_3e8 = 4;
    } else if (((struct actor *)act)->unknown_6e >= 5 && kind != 1) {
        ((actor *)act)->vocalization_unknown_3e8 = 7;
    } else {
        ((actor *)act)->vocalization_unknown_3e8 = 5;
    }
    if (((struct actor *)act)->mode_data.charge.stage == 1) {
        act[0x426] = (uint8_t)(act[0xc1] == 0);
        act[0x427] = (uint8_t)(act[0xc1] == 0);
    } else if (!act[0x428] && (actor_flags & 0x10000)) {
        act[0x426] = act[0x358];
        act[0x427] = act[0x358];
    } else {
        act[0x426] = 0;
        act[0x427] = 0;
    }
    if (act[0xa8]) {
        act[0x440] = 1;
        act[0x441] = (uint8_t)(*(float *)(act + 0xb8) * 0.7f > *(float *)(act + 0xbc));
        act[0x442] = 1;
        *(int32_t *)&((struct actor *)act)->unknown_444.i = *(int32_t *)(act + 0xb0);
        *(int32_t *)&((struct actor *)act)->unknown_444.j = *(int32_t *)(act + 0xb4);
        *(int32_t *)&((struct actor *)act)->unknown_44c = *(int32_t *)(act + 0xb8);
        *(int32_t *)&((struct actor *)act)->unknown_450 = *(int32_t *)(act + 0xbc);
        act[0xa7] = 1;
        act[0xa8] = 0;
        ((struct actor *)act)->mode_data.charge.stage_start_time = game_time->game_time;
        ((struct actor *)act)->mode_data.charge.stage_ticks = 0;
    }
    if (actor_flags & 0x100000) {
        if (act[0x378] || ((struct actor *)act)->mode_data.charge.stage == 2 || ((struct actor *)act)->mode_data.charge.stage == 3) {
            act[0x428] = (uint8_t)(act[0xc4] && !act[0x427]);
        }
    }
    act[0x424] = 0;
    act[0x425] = 0;
    act[0x42a] = 1;
    act[0x454] = (uint8_t)(((struct actor *)act)->mode_data.charge.stage != 1);
}
