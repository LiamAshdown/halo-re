// actor_escalate_check_shield_damage  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40a9e0, size 138 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40a9e0..0x40aa6a.
//   After fresh damage (+0x2ec, consumed here) above the Actor tag's amount (+0x1c0 over +0x398) while the body is
//   still healthy (+0x1b8 under +0x39c) the actor escalates: level (+0x310) at least 3.
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

uint8_t actor_escalate_check_shield_damage(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);

    if (!act[0x2ec] || !(((struct actor *)act)->recent_damage_taken > ((Actor *)actor_tag)->berserk_damage_amount) ||
        !(((struct actor *)act)->unknown_1b8 < ((Actor *)actor_tag)->berserk_damage_threshold)) {
        return 0;
    }
    if (*(int16_t *)(act + 0x310) <= 3) {
        *(int16_t *)(act + 0x310) = 3;
    }
    act[0x2ec] = 0;
    return 1;
}
