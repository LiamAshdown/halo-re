// actor_mode_converse_process  (not a Ghidra function: actor mode table 0x65524c, mode "converse" slot +0x14)
// address 0x402d70, size 248 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x402d70..0x402e68 (no C existed: a mode entered in game would have hit a trap).
//   Walking over to the conversation partner (returns +0xa0, "arrived / gave up"): only while a path is wanted
//   (+0x4c). The partner prop (+0xac) is found or made for the partner object (+0xa8, 0x43eb30); none means give up.
//   Close enough (a partner prop of kind 2+ nearer than the talking distance +0xa4, or anything within 0.7) the
//   actor stops (+0xa1); otherwise it walks to within +0xa4 of it (0x417910), giving up when that fails.
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern data_array *prop_data; // 0x008802c0
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
                                                    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius); // 0x417910, EAX, stack

uint8_t actor_mode_converse_process(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    datum_index partner;

    if (!act[0x4c]) {
        return act[0xa0];
    }
    if (((struct actor *)act)->mode_data.converse.partner_prop == k_datum_index_none && ((struct actor *)act)->mode_data.converse.partner_unit != k_datum_index_none) {
        ((struct actor *)act)->mode_data.converse.partner_prop = actor_find_or_create_shared_prop(((struct actor *)act)->mode_data.converse.partner_unit, actor_index, 1, 1);
    }
    partner = ((struct actor *)act)->mode_data.converse.partner_prop;
    if (partner == k_datum_index_none) {
        act[0xa0] = 1;
        return act[0xa0];
    }
    if (!act[0xa1]) {
        uint8_t *p = (uint8_t *)prop_data->data + (partner & 0xffff) * 0x138;
        float distance = ((prop *)p)->distance;

        if ((((struct prop *)p)->visual_perception >= 2 && distance < ((struct actor *)act)->mode_data.converse.approach_distance) || distance < 0.7f) {
            act[0xa1] = 1;
        }
    }
    if (act[0xa1]) {
        actor_movement_action_stop(actor_index);
        return act[0xa0];
    }
    if (!actor_movement_set_destination_near_target(partner, actor_index, ((struct actor *)act)->mode_data.converse.approach_distance)) {
        act[0xa0] = 1;
    }
    return act[0xa0];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
