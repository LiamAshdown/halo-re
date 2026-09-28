// actor_try_grenade_evasion  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40c530, size 238 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40c530..0x40c61e.
//   An actor wanting a path (+0x4c) that has taken more than the Actor tag's share of damage (+0x1bc vs +0x2dc)
//   while fighting (grade 3 / 4, not retreating, +0x6e 2+) tries, at most every 30 ticks (+0x370), to get away from
//   the target's grenade (0x40b840): a dive (0x40dd50 1, 0) or, if allowed, a pain reaction (0x40de20, CX 4).
// blam-cc: EAX -> actor_index, stack -> allow_pain_reaction, use_alt_base

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254
extern uint8_t actor_should_throw_grenade(uint32_t actor_index, char force); // 0x40b840, EAX, stack
extern uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3); // 0x40dd50
extern uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base, uint16_t order_code,
                                         datum_index actor_index); // 0x40de20, stack, DL, CX, ESI

uint8_t actor_try_grenade_evasion(datum_index actor_index, uint8_t allow_pain_reaction, uint8_t use_alt_base)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    int16_t grade;
    int32_t now;

    if (!act[0x4c] || !(*(float *)(act + 0x1bc) <= ((Actor *)actor_tag)->hide_shield_fraction)) {
        return 0;
    }
    grade = actor_mode_definitions[((actor *)act)->mode].combat_grade;
    if (act[0x378] || (grade != 4 && grade != 3) || *(int16_t *)(act + 0x6e) < 2) {
        return 0;
    }
    now = game_time->game_time;
    if (*(int32_t *)(act + 0x370) != -1 && now < *(int32_t *)(act + 0x370) + 30) {
        return 0;
    }
    *(int32_t *)(act + 0x370) = now;
    if (!actor_should_throw_grenade(actor_index, 0)) {
        return 0;
    }
    if (actor_handle_death(actor_index, 1, 0)) {
        return 1;
    }
    if (allow_pain_reaction &&
        actor_check_pain_reaction(((actor *)act)->target_unit_index, use_alt_base, 4, actor_index)) {
        return 1;
    }
    return 0;
}
