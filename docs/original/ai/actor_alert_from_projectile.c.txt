// actor_alert_from_projectile  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40a5e0, size 195 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40a5e0..0x40a6a2: an object noticed at +0x1b0 (a projectile or similar, object_try_and_get -1) raises the
//   alert to at least 10; its source is the prop of the object's creator (+0xc4), resolved through a vehicle's
//   gunner/driver when that creator is a unit.
// blam-cc: stack -> actor_index

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

static void actor_raise_alert(uint8_t *actor, int16_t level, uint32_t source)
{
    if (W(0x308) == 0 || D(0x30c) == 0xffffffff) {
        D(0x30c) = source;
    }
    if (W(0x308) <= level) {
        W(0x308) = level;
    }
}

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, ECX actor, stack object

uint8_t actor_alert_from_projectile(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);
    uint32_t source = 0xffffffff;
    uint8_t *noticed;

    if (D(0x1b0) == 0xffffffff) {
        return 0;
    }
    noticed = (uint8_t *)object_try_and_get(D(0x1b0), 0xffffffff);
    if (noticed != 0 && *(datum_index *)(noticed + 0xc4) != k_datum_index_none) {
        datum_index creator = *(datum_index *)(noticed + 0xc4);
        uint8_t *creator_unit = (uint8_t *)object_try_and_get(creator, 3);

        if (creator_unit != 0) {
            datum_index who;

            if (*(datum_index *)(creator_unit + 0x328) != k_datum_index_none) {
                who = *(datum_index *)(creator_unit + 0x328);
            } else if (*(datum_index *)(creator_unit + 0x324) != k_datum_index_none) {
                who = *(datum_index *)(creator_unit + 0x324);
            } else {
                who = creator;
            }
            if (who != k_datum_index_none) {
                source = actor_find_prop_for_object(who, actor_index);
            }
        }
    }
    actor_raise_alert(actor, 0xa, source);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
