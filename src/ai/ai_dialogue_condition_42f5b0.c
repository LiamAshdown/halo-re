// ai_dialogue_condition_42f5b0  (not a Ghidra function; the "can say" condition of AI dialogue record(s) 0x8d/0x9c in the
//   dialogue table at 0x656eb4 (0x24-byte records, procedure at +0x1c); no C existed, so reaching it trapped as
//   unlisted_42f5b0)
// address 0x42f5b0, size 147 bytes
// name confidence: 0.3 (named by address)   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x42f5b0: close and recognized (0x42f480), and the object own-unit own actor (unit +0x1f4) and actor_index share a valid +0x34 (encounter) and the same +0x3c word (squad).
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

uint8_t ai_dialogue_condition_42f5b0(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    datum_index own_actor;
    uint8_t *a;
    uint8_t *b;

    if (!actor_target_is_close_and_recognized(object_index, param_2, actor_index)) {
        return 0;
    }
    own_actor = *(datum_index *)(OBJECT(object_index) + 0x1f4);
    if (own_actor == k_datum_index_none || actor_index == k_datum_index_none) {
        return 0;
    }
    a = ACTOR(own_actor);
    b = ACTOR(actor_index);
    return (uint8_t)(*(datum_index *)(a + 0x34) != k_datum_index_none &&
        *(datum_index *)(a + 0x34) == *(datum_index *)(b + 0x34) &&
        *(int16_t *)(a + 0x3c) == *(int16_t *)(b + 0x3c));
}
