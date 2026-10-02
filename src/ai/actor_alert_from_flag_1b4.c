// actor_alert_from_flag_1b4  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40a6b0, size 78 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40a6b0..0x40a6fd: with +0x1b4 set the alert rises to at least 11 and its source is cleared (-1).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & 0xffff) * 0x724)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

uint8_t actor_alert_from_flag_1b4(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);

    if (B(0x1b4) == 0) {
        return 0;
    }
    if (W(0x308) <= 0xb) {
        W(0x308) = 0xb;
    }
    D(0x30c) = 0xffffffff;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
