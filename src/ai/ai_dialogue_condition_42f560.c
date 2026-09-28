// ai_dialogue_condition_42f560  (not a Ghidra function; the "can say" condition of AI dialogue record(s) 0x89 in the
//   dialogue table at 0x656eb4 (0x24-byte records, procedure at +0x1c); no C existed, so reaching it trapped as
//   unlisted_42f560)
// address 0x42f560, size 66 bytes
// name confidence: 0.3 (named by address)   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x42f560: actor mode (+0x6c) 5 -> +0xa4 == 1; mode 7 -> 1; otherwise (or no actor) 0.
// blam-cc: stack -> object_index, unused, actor_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"

extern data_array *actor_data;  // 0x00880360, 0x724-byte actors
extern data_array *prop_data;   // 0x008802c0, 0x138-byte props
extern data_array *object_data; // 0x008603b0
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX object, stack
extern uint8_t actor_target_is_close_and_recognized(datum_index object_index, uint32_t param_2,
    datum_index actor_index); // 0x42f480

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
#define OBJECT(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

uint8_t ai_dialogue_condition_42f560(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    uint8_t *actor;

    if (actor_index == k_datum_index_none) {
        return 0;
    }
    actor = ACTOR(actor_index);
    if (((struct actor *)actor)->mode == 5) {
        return (uint8_t)(*(int16_t *)(actor + 0xa4) == 1);
    }
    return (uint8_t)(((struct actor *)actor)->mode == 7);
}
