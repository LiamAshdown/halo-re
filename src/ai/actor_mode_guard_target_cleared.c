// actor_mode_guard_target_cleared  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x30)
// address 0x405180, size 50 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x405180..0x4051b2 (no C existed: a mode entered in game would have hit a trap).
//   The actor's target was cleared: a guard of kind 2 (+0xc0) forgets the guarded target (+0xd0).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_guard_target_cleared(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (*(int16_t *)(act + 0xc0) == 2) {
        *(int32_t *)(act + 0xd0) = -1;
    }
}
