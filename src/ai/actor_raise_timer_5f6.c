// actor_raise_timer_5f6  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40f7a0, size 52 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40f7a0..0x40f7d3: actor +0x5f6 (a countdown in ticks) becomes the larger of itself and ticks.
// blam-cc: EAX -> actor_index, EDX -> ticks

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

void actor_raise_timer_5f6(datum_index actor_index, int32_t ticks)
{
    uint8_t *actor = ACTOR(actor_index);

    if ((int32_t)W(0x5f6) > ticks) {
        W(0x5f6) = W(0x5f6);
    } else {
        W(0x5f6) = (int16_t)ticks;
    }
}
