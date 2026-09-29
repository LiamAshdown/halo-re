// actor_mode_converse_update  (not a Ghidra function: actor mode table 0x65524c, mode "converse" slot +0x1c)
// address 0x402e70, size 144 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x402e70..0x402f00 (no C existed: a mode entered in game would have hit a trap).
//   Firing request while conversing: look at the partner -- the prop at +0xac, else the prop of the conversation
//   (+0x9c, record +0x10) partner object (0x43ea80) -- kind 3 / style 1; fire kind 1.
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

extern data_array *ai_conversation_data; // 0x008802d4
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX

void actor_mode_converse_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    datum_index conversation = ((struct actor *)act)->mode_data.converse.conversation;
    uint8_t *record = 0;
    datum_index look_prop = k_datum_index_none;

    if (conversation != k_datum_index_none) {
        record = (uint8_t *)ai_conversation_data->data + (conversation & 0xffff) * 0x64;
    }
    if (((struct actor *)act)->mode_data.converse.partner_prop != k_datum_index_none) {
        look_prop = ((struct actor *)act)->mode_data.converse.partner_prop;
    } else if (record != 0 && *(datum_index *)(record + 0x10) != k_datum_index_none) {
        look_prop = actor_find_prop_for_object(*(datum_index *)(record + 0x10), actor_index);
    }
    ((struct actor *)act)->look_posture = 1;
    if (look_prop != k_datum_index_none) {
        ((actor *)act)->vocalization_unknown_3e8 = 3;
        ((actor *)act)->vocalization_unknown_3ec = 1;
        *(datum_index *)(act + 0x3f0) = look_prop;
    }
}
