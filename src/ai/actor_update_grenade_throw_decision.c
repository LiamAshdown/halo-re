// actor_update_grenade_throw_decision  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40b770, size 203 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40b770..0x40b83b.
//   A willing actor (morale +0x268 5+, not panicking in flee) decides on a grenade by its variant's grenade style
//   (+0x184): style 1 when fighting (+0x6e 5+), style 2 at a target that is out of the open (prop +0x14) or while
//   fleeing without a panic kind (0x40dc30). A pending throw (+0x6a0) is committed when facing (0x40db00); an
//   unwilling actor drops it.
// blam-cc: EDI -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern uint8_t actor_consider_grenade_throw(datum_index actor_index); // 0x40dc30
extern uint8_t actor_check_grenade_facing_and_commit(datum_index actor_index, uint8_t force_commit); // 0x40db00

uint8_t actor_update_grenade_throw_decision(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *variant = TAG_DATA(((actor *)act)->actor_variant_tag);
    int16_t mode = ((actor *)act)->mode;
    uint8_t result = 0;

    if (((actor *)act)->target_combat_status < 5 || (mode == 4 && ((struct actor *)act)->mode_data.flee.panic > 0)) {
        act[0x6a0] = 0;
        return 0;
    }
    switch (*(int16_t *)&((ActorVariant *)variant)->grenade_stimulus) {
    case 1:
        if (((struct actor *)act)->combat_status >= 5) {
            result = actor_consider_grenade_throw(actor_index);
        }
        break;
    case 2:
        if (PROP(((actor *)act)->target_unit_index)[0x14] || (mode == 4 && ((struct actor *)act)->mode_data.flee.panic == 0)) {
            result = actor_consider_grenade_throw(actor_index);
        }
        break;
    default:
        break;
    }
    if (act[0x6a0]) {
        actor_check_grenade_facing_and_commit(actor_index, 0);
    }
    return result;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
