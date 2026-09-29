// actor_alert_from_disturbance  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40a3d0, size 130 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40a3d0..0x40a451: a reacted disturbance (+0x2f0) of an actor whose definition (+0x58) has flag
//   0x400 raises the alert (+0x308) to at least 7 with the prop +0x2f4 as its source (+0x30c, when unset) and
//   clears +0x2f0. Returns whether it did.
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "fn_ai.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & 0xffff) * 0x724)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

static void actor_raise_alert(uint8_t *actor, int16_t level, uint32_t source)
{
    if (W(0x308) == 0 || D(0x30c) == 0xffffffff) {
        D(0x30c) = source;
    }
    if (W(0x308) <= level) {
        W(0x308) = level;
    }
}

uint8_t actor_alert_from_disturbance(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);

    if (B(0x2f0) == 0 || (*(uint32_t *)tag_instances[D(0x58) & 0xffff].data & 0x400) == 0) {
        return 0;
    }
    actor_raise_alert(actor, 7, D(0x2f4));
    B(0x2f0) = 0;
    return 1;
}
