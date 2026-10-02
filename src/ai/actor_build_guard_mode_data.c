// actor_build_guard_mode_data  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x404360, size 327 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x404360..0x4044a6: the 0x44-byte guard mode record: +0x00 = actor +0x33c; mode +0x24 = 1 for a busy
//   (+0x160) or swarm (+0x06) actor; else 2 with the point +0x318 (-> +0x28), surface +0x324 (-> +0x34) and +0x328
//   (-> +0x38) when +0x314 is set and actor_firing_position_near_point accepts it; else 1 with +0x14 = +0x32c and,
//   when set, the direction +0x330 (-> +0x18, normalized; +0x14 cleared when it is zero) when +0x33c > 0; else 0
//   with +0x0e = 1. An escalation of 2 (+0x312) sets +0x0f and +0x10 = +0x340; +0x3c = +0x340 and, when that is a
//   handle, +0x02 = +0x344 and +0x40 = +0x348. Returns 1.
// blam-cc: EDX -> actor_index, stack -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & 0xffff) * 0x724)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point, int32_t start_surface_index,
    int16_t kind); // 0x412960, EDX actor, stack
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX

uint8_t actor_build_guard_mode_data(datum_index actor_index, uint8_t *out)
{
    uint8_t *actor = ACTOR(actor_index);

    memset(out, 0, 0x44);
    *(int16_t *)out = W(0x33c);
    if (B(0x160) != 0 || B(6) != 0) {
        *(int16_t *)(out + 0x24) = 1;
    } else if (B(0x314) != 0 &&
               actor_firing_position_near_point(actor_index, (real_point3d *)(actor + 0x318), (int32_t)D(0x324), 1)) {
        *(int16_t *)(out + 0x24) = 2;
        memcpy(out + 0x28, actor + 0x318, 12);
        *(uint32_t *)(out + 0x34) = D(0x324);
        *(uint32_t *)(out + 0x38) = D(0x328);
    } else if (*(int16_t *)out > 0) {
        *(int16_t *)(out + 0x24) = 1;
        out[0x14] = B(0x32c);
        out[0x15] = 0;
        if (B(0x32c) != 0) {
            memcpy(out + 0x18, actor + 0x330, 12);
            if (vector3d_normalize_with_length((real_vector3d *)(out + 0x18)) == 0.0f) {
                out[0x14] = 0;
            }
        }
    } else {
        *(int16_t *)(out + 0x24) = 0;
        out[0x0e] = 1;
    }
    if (W(0x312) == 2) {
        out[0x0f] = 1;
        *(uint32_t *)(out + 0x10) = D(0x340);
    }
    *(uint32_t *)(out + 0x3c) = D(0x340);
    if (D(0x340) != 0xffffffff) {
        *(int16_t *)(out + 0x02) = W(0x344);
        out[0x40] = B(0x348);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
