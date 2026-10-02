// ai_dialogue_condition_42f6f0  (not a Ghidra function; the "can say" condition of AI dialogue record(s) 0x8b in the
//   dialogue table at 0x656eb4 (0x24-byte records, procedure at +0x1c); no C existed, so reaching it trapped as
//   unlisted_42f6f0)
// address 0x42f6f0, size 187 bytes
// name confidence: 0.3 (named by address)   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x42f6f0: close and recognized (0x42f480), and the object own-unit own actor (unit +0x1f4) and actor_index both have a target prop (+0x270) tracking the same object (prop +0x18).
// blam-cc: stack -> object_index, unused, actor_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

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

uint8_t ai_dialogue_condition_42f6f0(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    datum_index own_actor;
    datum_index a_target;
    datum_index b_target;

    if (!actor_target_is_close_and_recognized(object_index, param_2, actor_index)) {
        return 0;
    }
    own_actor = *(datum_index *)(OBJECT(object_index) + 0x1f4);
    if (own_actor == k_datum_index_none || actor_index == k_datum_index_none) {
        return 0;
    }
    a_target = *(datum_index *)(ACTOR(own_actor) + 0x270);
    if (a_target == k_datum_index_none) {
        return 0;
    }
    b_target = *(datum_index *)(ACTOR(actor_index) + 0x270);
    if (b_target == k_datum_index_none) {
        return 0;
    }
    return (uint8_t)(*(datum_index *)(PROP(a_target) + 0x18) == *(datum_index *)(PROP(b_target) + 0x18));
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
