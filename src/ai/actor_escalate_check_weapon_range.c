// actor_escalate_check_weapon_range  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40a860, size 225 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40a860..0x40a941.
//   A fighting actor (+0x6e 5+) with a weapon threat (+0x1b0) whose target is inside the actor definition's range
//   (+0x16c, 0x40fa70) escalates with the Actor tag's chance (+0x3a8): level (+0x310) at least 4.
// blam-cc: EAX -> actor_index

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

extern uint32_t random_seed_global; // 0x00719cd0
extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX

uint8_t actor_escalate_check_weapon_range(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    uint8_t *definition = (uint8_t *)actor_get_actor_definition(actor_index);

    if (*(datum_index *)&((struct actor *)act)->stuck_projectile_index == k_datum_index_none || ((struct actor *)act)->combat_status < 5) {
        return 0;
    }
    if (!(*(float *)(PROP(((actor *)act)->target_unit_index) + 0x11c) < *(float *)(definition + 0x16c))) {
        return 0;
    }
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    if (!((float)(random_seed_global >> 16) * 1.5259022e-05f < ((Actor *)actor_tag)->berserk_grenade_chance)) {
        return 0;
    }
    if (*(int16_t *)(act + 0x310) <= 4) {
        *(int16_t *)(act + 0x310) = 4;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
