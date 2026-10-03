// ai_dialogue_condition_42f690  (not a Ghidra function; the "can say" condition of AI dialogue record(s) 0x9c in the
//   dialogue table at 0x656eb4 (0x24-byte records, procedure at +0x1c); no C existed, so reaching it trapped as
//   unlisted_42f690)
// address 0x42f690, size 93 bytes
// name confidence: 0.3 (named by address)   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x42f690: close and recognized (0x42f480), then grade (+0x6e) >= 7 except in mode 4 with +0xa8 > 0.
// blam-cc: stack -> object_index, unused, actor_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

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

uint8_t ai_dialogue_condition_42f690(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    uint8_t *actor;

    if (!actor_target_is_close_and_recognized(object_index, param_2, actor_index)) {
        return 0;
    }
    actor = ACTOR(actor_index);
    if (((struct actor *)actor)->combat_status < 7) {
        return 0;
    }
    if (((struct actor *)actor)->mode == 4 && ((struct actor *)actor)->mode_data.flee.panic > 0) {
        return 0;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
