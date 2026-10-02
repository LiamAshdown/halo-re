// actor_mode_alert_target_cleared  (not a Ghidra function: actor mode table 0x65524c, mode "alert" slot +0x30)
// address 0x401490, size 42 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x401490..0x4014ba (no C existed: a mode entered in game would have hit a trap).
//   The actor's target was cleared: the alert target (mode data +0x34 word, +0x58 handle) is forgotten.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

void actor_mode_alert_target_cleared(datum_index actor_index)
{
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    *(int16_t *)(mode_data + 0x34) = -1;
    *(int32_t *)(mode_data + 0x58) = -1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
