// actor_mode_charge_tick  (not a Ghidra function: actor mode table 0x65524c, mode "charge" slot +0x18)
// address 0x402aa0, size 74 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x402aa0..0x402aea (no C existed: a mode entered in game would have hit a trap).
//   Per tick: a charge of kind 3 (+0xa0) that has closed in (+0xa7) without striking (+0xa2) and is on its feet
//   (+0x15c clear) counts the ticks spent there (+0xaa).
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

void actor_mode_charge_tick(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->mode_data.charge.stage == 3 && act[0xa7] && !act[0xa2] && !act[0x15c]) {
        ((struct actor *)act)->mode_data.charge.stage_ticks += 1;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
