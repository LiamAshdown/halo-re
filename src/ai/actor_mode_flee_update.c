// actor_mode_flee_update  (not a Ghidra function: actor mode table 0x65524c, mode "flee" slot +0x1c)
// address 0x403b90, size 392 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x403b90..0x403d18 (no C existed: a mode entered in game would have hit a trap).
//   Firing request while fleeing: in panic (+0xa8) kind 6 / style 0 (+0x456); against a target that can be seen
//   (prop +0x32) kind 7 / style 2 shooting blind (+0x454); away from a flee source (+0xb8) kind 3 / style 1 at it;
//   else nothing. Fire kind 4, +0x428 in panic, +0x429 for panic kinds 9..12, +0x426 / +0x424 set. Then the flee
//   destination (firing position +0xa4): none means stop; while a path is wanted it is walked to (0x417830, EDI
//   actor) and remembered (+0x3b8 / +0x3ba), or on failure the remembered one is recognised as bad (0x4141a0),
//   movement stops and a new destination is asked for (+0xa2).
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
extern data_array *prop_data; // 0x008802c0

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot,
                                                              path_find_context *path_context); // 0x417830, EDI, stack
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type); // 0x4141a0, EAX, CX, DL

void actor_mode_flee_update(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    int16_t panic = *(int16_t *)(act + 0xa8);
    datum_index target = ((actor *)act)->target_unit_index;
    int16_t destination;

    if (panic > 0) {
        ((actor *)act)->vocalization_unknown_3e8 = 6;
        ((actor *)act)->vocalization_unknown_3ec = 0;
        act[0x456] = 1;
    } else if (target != k_datum_index_none && *(int16_t *)(PROP(target) + 0x32) > 0) {
        ((actor *)act)->vocalization_unknown_3e8 = 7;
        ((actor *)act)->vocalization_unknown_3ec = 2;
        act[0x454] = 1;
    } else if (*(datum_index *)(act + 0xb8) != k_datum_index_none) {
        ((actor *)act)->vocalization_unknown_3e8 = 3;
        ((actor *)act)->vocalization_unknown_3ec = 1;
        *(datum_index *)(act + 0x3f0) = *(datum_index *)(act + 0xb8);
    } else {
        ((actor *)act)->vocalization_unknown_3e8 = 0;
    }
    *(int16_t *)(act + 0x3fc) = 4;
    act[0x428] = (uint8_t)(*(int16_t *)(act + 0xa8) > 0);
    act[0x429] = (uint8_t)(*(int16_t *)(act + 0xa8) >= 9 && *(int16_t *)(act + 0xa8) <= 12);
    act[0x426] = 1;
    act[0x427] = 0;
    act[0x424] = 1;
    act[0x425] = 0;

    destination = *(int16_t *)(act + 0xa4);
    if (destination == -1) {
        actor_movement_action_stop(actor_index);
        return;
    }
    if (!act[0x4c]) {
        return;
    }
    if (actor_movement_set_destination_firing_position(actor_index, destination, 0)) {
        ((actor *)act)->firing_position_index = *(int16_t *)(act + 0xa4);
        act[0x3ba] = act[0xa6];
        return;
    }
    if (((actor *)act)->firing_position_index != -1) {
        actor_push_recognition_entry(actor_index, ((actor *)act)->firing_position_index, 0);
        actor_movement_action_stop(actor_index);
        ((actor *)act)->firing_position_index = -1;
    }
    *(int16_t *)(act + 0xa4) = -1;
    act[0xa2] = 1;
}
