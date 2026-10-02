// actor_mode_flee_exit  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x20)
// address 0x4037a0, size 65 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x4037a0..0x4037e1 (no C existed: a mode entered in game would have hit a trap).
//   Fleeing ends: the unit's random look-around flag (unit +0x204 bit 0x2000000) is cleared.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern data_array *object_data; // 0x008603b0

void actor_mode_flee_exit(datum_index actor_index)
{
    datum_index unit_index = *(datum_index *)(ACTOR(actor_index) + 0x18);

    if (unit_index != k_datum_index_none) {
        uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;

        *(uint32_t *)(obj + 0x204) &= ~0x2000000u;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
