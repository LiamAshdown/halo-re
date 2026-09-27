// actor_alert_from_damage  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40a500, size 209 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40a500..0x40a5d0: a damaged flag (+0x1b5) raises the alert to at least 12; its source is the prop of
//   the unit that last damaged the actor's unit (unit +0x410, resolved through a vehicle's gunner +0x328 or driver
//   +0x324 when it is a unit, object_try_and_get mask 3), found with actor_find_prop_for_object.
// blam-cc: stack -> actor_index

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

static void actor_raise_alert(uint8_t *actor, int16_t level, uint32_t source)
{
    if (W(0x308) == 0 || D(0x30c) == 0xffffffff) {
        D(0x30c) = source;
    }
    if (W(0x308) <= level) {
        W(0x308) = level;
    }
}

extern data_array *object_data; // 0x008603b0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, ECX actor, stack object

uint8_t actor_alert_from_damage(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);
    uint32_t source = 0xffffffff;

    if (B(0x1b5) == 0) {
        return 0;
    }
    if (D(0x18) != 0xffffffff) {
        uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[D(0x18) & 0xffff].data;
        datum_index attacker = *(datum_index *)(unit + 0x410);

        if (attacker != k_datum_index_none) {
            uint8_t *attacker_unit = (uint8_t *)object_try_and_get(attacker, 3);

            if (attacker_unit != 0) {
                if (*(datum_index *)(attacker_unit + 0x328) != k_datum_index_none) {
                    attacker = *(datum_index *)(attacker_unit + 0x328);
                } else if (*(datum_index *)(attacker_unit + 0x324) != k_datum_index_none) {
                    attacker = *(datum_index *)(attacker_unit + 0x324);
                }
                if (attacker != k_datum_index_none) {
                    source = actor_find_prop_for_object(attacker, actor_index);
                }
            }
        }
    }
    actor_raise_alert(actor, 0xc, source);
    return 1;
}
