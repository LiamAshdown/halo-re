// actor_mode_charge_enter  (not a Ghidra function: actor mode table 0x65524c, mode "charge" slot +0x10)
// address 0x401d50, size 79 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x401d50..0x401d9f (no C existed: a mode entered in game would have hit a trap).
//   Charging starts: a berserk charge (kind 4 at +0xa0) by an actor whose Actor tag +0x156 is 3 uses up one of its
//   charges (+0x5fe).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX

void actor_mode_charge_enter(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->mode_data.charge.stage == 4 &&
        *(int16_t *)((uint8_t *)actor_get_actor_definition(actor_index) + 0x156) == 3 &&
        ((struct actor *)act)->unknown_5fe > 0) {
        ((struct actor *)act)->unknown_5fe -= 1;
    }
}
