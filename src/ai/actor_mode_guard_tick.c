// actor_mode_guard_tick  (not a Ghidra function: actor mode table 0x65524c, mode "guard" slot +0x18)
// address 0x404b90, size 450 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x404b90..0x404d52 (no C existed: a mode entered in game would have hit a trap).
//   Per tick while guarding: an awake guard standing still (+0x484) counts its hold time (+0x9c) down; at zero (not
//   committed, not a swarm) a held firing position is given up (0x4048b0; +0x1e4 / +0x1e8 / +0xa1 / +0xa3 / +0xd8)
//   and a new spot is asked for (+0xaa). The watch time (+0x9e) runs while standing still (or unwatched, +0xdc) and
//   forgets the watched object (+0xd8). An ambush (+0xa0) standing still ends when its grenade threat is gone
//   (+0xa6, +0x3a8) or its time (+0xa8) runs out: the units wake (0x427860 BL 0), the ambush state clears, a
//   fighting actor says so (event 0x23), a guard of kind 3 gives up its spot (0x4141a0), and the guard kind goes
//   back to 1 when committed, else 0 with a new spot asked for.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments
extern int32_t actor_report_command_status(uint32_t actor_index); // 0x4048b0, EAX
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860, EAX, BL
extern datum_index actor_get_target_prop_object_index(datum_index actor_index); // 0x4283d0, EAX
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type); // 0x4141a0, EAX, CX, DL

void actor_mode_guard_tick(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t ambush_over;

    if (!act[0x13] && act[0x484] && *(int16_t *)(act + 0x9c) > 0) {
        *(int16_t *)(act + 0x9c) -= 1;
        if (*(int16_t *)(act + 0x9c) == 0 && !act[0x160] && !act[0x6]) {
            if (act[0xa1]) {
                actor_report_command_status(actor_index);
                *(int16_t *)(act + 0x1e4) = 0;
                *(int32_t *)(act + 0x1e8) = -1;
                act[0xa1] = 0;
                act[0xa3] = 0;
                *(int32_t *)(act + 0xd8) = -1;
            }
            act[0xaa] = 1;
        }
    }
    if (*(int16_t *)(act + 0x9e) > 0 && (!act[0xdc] || act[0x484])) {
        *(int16_t *)(act + 0x9e) -= 1;
        if (*(int16_t *)(act + 0x9e) == 0) {
            *(int32_t *)(act + 0xd8) = -1;
        }
    }
    if (!act[0xa0] || !act[0x484]) {
        return;
    }
    if (act[0xa6]) {
        act[0xa6] = (uint8_t)(*(int16_t *)(act + 0x3a8) > 0);
        ambush_over = (uint8_t)(act[0xa6] == 0);
    } else {
        if (*(int16_t *)(act + 0xa8) <= 0) {
            return;
        }
        *(int16_t *)(act + 0xa8) -= 1;
        ambush_over = (uint8_t)(*(int16_t *)(act + 0xa8) == 0);
    }
    if (!ambush_over) {
        return;
    }
    actor_set_units_active(actor_index, 0);
    act[0xa4] = 0;
    act[0xa5] = 0;
    act[0xa6] = 0;
    *(int16_t *)(act + 0xa8) = 0;
    if (*(int16_t *)(act + 0x6e) >= 2 && ((actor *)act)->unit_index != k_datum_index_none) {
        ai_communication_broadcast(0x23, ((actor *)act)->unit_index, actor_get_target_prop_object_index(actor_index),
                                   -1, -1, -1, 0);
    }
    if (*(int16_t *)(act + 0xc0) == 3) {
        actor_push_recognition_entry(actor_index, *(int16_t *)(act + 0xc4), 0);
        *(int16_t *)(act + 0xc4) = -1;
    }
    ((actor *)act)->firing_position_index = -1;
    if (act[0x160]) {
        *(int16_t *)(act + 0xc0) = 1;
        return;
    }
    *(int16_t *)(act + 0xc0) = 0;
    act[0xaa] = 1;
}
