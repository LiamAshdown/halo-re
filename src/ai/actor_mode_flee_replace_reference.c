// actor_mode_flee_replace_reference  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x28)
// address 0x404300, size 52 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x404300..0x404334 (no C existed: a mode entered in game would have hit a trap).
//   Slot +0x28: an object reference changed; the flee source (+0xb8) follows it.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_flee_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->mode_data.flee.reference == old_reference) {
        ((struct actor *)act)->mode_data.flee.reference = new_reference;
    }
}
