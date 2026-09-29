// actor_alert_from_squad_attack  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40a460, size 159 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40a460..0x40a4fe: a squad-attacked flag (+0x2ec) whose accumulated value (+0x1c0) exceeds the actor
//   definition (+0x58) +0x2ac raises the alert to at least 1, its source the squad's recent attacker
//   (actor_get_squad_recent_attacker_target(actor, 1)), and clears +0x2ec.
// blam-cc: EDX -> actor_index

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


uint8_t actor_alert_from_squad_attack(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);
    uint8_t *definition;

    if (B(0x2ec) == 0) {
        return 0;
    }
    definition = (uint8_t *)tag_instances[D(0x58) & 0xffff].data;
    if (!(F(0x1c0) > *(float *)(definition + 0x2ac))) {
        return 0;
    }
    if (W(0x308) == 0 || D(0x30c) == 0xffffffff) {
        D(0x30c) = actor_get_squad_recent_attacker_target(actor_index, 1);
    }
    if (W(0x308) <= 1) {
        W(0x308) = 1;
    }
    B(0x2ec) = 0;
    return 1;
}
