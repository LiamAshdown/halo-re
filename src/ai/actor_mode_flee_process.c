// actor_mode_flee_process  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x14)
// address 0x4037f0, size 760 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x4037f0..0x403ae8 (no C existed: a mode entered in game would have hit a trap).
//   Fleeing (returns 1 when over: +0xaa gave up or +0xab safe). Panic kinds 9..12 (+0xa8) hold the flee delay at 180
//   ticks. The flee destination (+0xa4, from the firing position +0x3b8) is dropped while cowering (+0x9e) and asked
//   for when missing; once the target is out of range (0x403dc0) a finished delay commits to it (+0xab) and the
//   flee source prop becomes a searched, unseen target (0x4200d0, 0x420290). Panic kinds end when their cause is gone
//   (+0x1b0 / +0x1b4 / +0x1b5). With a path wanted: a reachable weapon (0x4041d0) cancels the destination; a committed
//   order gives up (+0xaa, +0x398); asked for a destination, a melee target (0x403f00) is tried, else give up. A
//   panicking unit that stopped screaming (+0x388) may scream again (+0xac); a fleeing unit screams (reaction
//   animation 1 or 2 for kinds 9..12, else events 0x1f / 0x20 once, then 0x21 every 60 ticks, +0xb0).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data; // 0x008802c0
extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments
extern uint8_t actor_is_target_within_engagement_range(uint32_t actor_index); // 0x403dc0, EAX
extern void actor_update_target_combat_status(datum_index actor_index); // 0x4200d0, EAX
extern void actor_update_awareness_level(datum_index actor_index); // 0x420290, EAX
extern uint8_t actor_check_weapon_pickup_reachable(uint32_t actor_index, uint8_t *record); // 0x4041d0, EAX, stack
extern void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order); // 0x403f00, stack, EBX
extern uint8_t unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code); // 0x5614a0, ESI, stack

uint8_t actor_mode_flee_process(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *mode_data = act + 0x9c;
    int16_t kind;

    if (!act[0x6]) {
        kind = *(int16_t *)(mode_data + 0xc);
        if (kind >= 9 && kind <= 12) {
            *(int16_t *)(mode_data + 0x0) = 180;
        }
        if (*(int16_t *)(mode_data + 0x2) > 0) {
            *(int16_t *)(mode_data + 0x8) = -1;
        } else if (*(int16_t *)(mode_data + 0x8) == -1) {
            mode_data[0x6] = 1;
        } else if (*(int16_t *)(act + 0x3b8) == -1) {
            *(int16_t *)(mode_data + 0x8) = -1;
            mode_data[0x6] = 1;
        } else if (actor_is_target_within_engagement_range(actor_index)) {
            if (*(int16_t *)(mode_data + 0x0) != 0) {
                mode_data[0x6] = 1;
            } else {
                *(int16_t *)(mode_data + 0x8) = *(int16_t *)(act + 0x3b8);
                mode_data[0xa] = act[0x3ba];
                mode_data[0xf] = 1;
                mode_data[0x6] = 0;
                if (*(datum_index *)(mode_data + 0x1c) != k_datum_index_none) {
                    uint8_t *source = PROP(*(datum_index *)(mode_data + 0x1c));
                    int16_t a = *(int16_t *)(source + 0x34);
                    int16_t b = *(int16_t *)(source + 0x36);

                    *(int16_t *)(source + 0x32) = 0;
                    *(int16_t *)(source + 0x30) = a > b ? a : b;
                    *(int16_t *)(source + 0x38) = 2;
                    source[0x74] = 0;
                    actor_update_target_combat_status(actor_index);
                    actor_update_awareness_level(actor_index);
                }
            }
        }
        switch (*(int16_t *)(mode_data + 0xc)) { // 0x403ad8
        case 9:
        case 10:
            if (*(datum_index *)(act + 0x1b0) == k_datum_index_none) {
                mode_data[0xf] = 1;
            }
            break;
        case 11:
            if (!act[0x1b4]) {
                mode_data[0xf] = 1;
            }
            break;
        case 12:
            if (!act[0x1b5]) {
                mode_data[0xf] = 1;
            }
            break;
        default:
            break;
        }
        if (act[0x4c] && !mode_data[0xf]) {
            if (*(int16_t *)(mode_data + 0x8) != -1 && *(int16_t *)(mode_data + 0x0) == 0 &&
                actor_check_weapon_pickup_reachable(actor_index, mode_data)) {
                *(int16_t *)(mode_data + 0x8) = -1;
                mode_data[0x6] = 1;
            }
            if (act[0x160]) {
                mode_data[0x6] = 0;
                mode_data[0xe] = 1;
                *(int32_t *)(act + 0x398) = game_time->game_time;
            } else if (mode_data[0x6]) {
                actor_check_melee_target_reachable(actor_index, (int16_t *)mode_data);
                if (*(int16_t *)(mode_data + 0x8) == -1) {
                    mode_data[0xe] = 1;
                    *(int32_t *)(act + 0x398) = game_time->game_time;
                }
            }
        }
    }

    kind = *(int16_t *)(mode_data + 0xc);
    if (kind >= 9 && kind <= 12 && *(datum_index *)(act + 0x18) != k_datum_index_none) {
        uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[*(datum_index *)(act + 0x18) & 0xffff].data;

        if (*(int16_t *)(unit + 0x388) <= 0) {
            mode_data[0x10] = 0;
        }
    }
    if (kind > 0 && *(int16_t *)(mode_data + 0x8) != -1) {
        datum_index unit_index;
        int32_t now;
        uint8_t announced;

        if (mode_data[0xe]) {
            return 1;
        }
        unit_index = *(datum_index *)(act + 0x18);
        if (unit_index != k_datum_index_none) {
            announced = mode_data[0x10];
            now = game_time->game_time;
            if (!announced || *(int32_t *)(mode_data + 0x14) + 60 >= now) {
                if (kind == 11 || kind == 12) {
                    unit_dispatch_reaction_animation(unit_index, 2);
                } else if (kind == 9 || kind == 10) {
                    unit_dispatch_reaction_animation(unit_index, 1);
                } else {
                    datum_index source_object = k_datum_index_none;

                    if (*(datum_index *)(mode_data + 0x1c) != k_datum_index_none) {
                        source_object = *(datum_index *)(PROP(*(datum_index *)(mode_data + 0x1c)) + 0x18);
                    }
                    if (!announced) {
                        ai_communication_broadcast(0x1f + (kind == 8), unit_index, source_object, -1, -1, 4, 0);
                        mode_data[0x10] = 1;
                    } else {
                        ai_communication_broadcast(0x21, unit_index, source_object, -1, -1, -1, 0);
                    }
                }
                *(int32_t *)(mode_data + 0x14) = now;
            }
        }
    }
    return (uint8_t)(mode_data[0xe] || mode_data[0xf]);
}
