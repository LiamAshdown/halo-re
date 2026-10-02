// actor_mode_search_tick  (not a Ghidra function: actor mode table 0x65524c, mode "search" slot +0x18)
// address 0x407d80, size 373 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x407d80..0x407ef5 (no C existed: a mode entered in game would have hit a trap).
//   Per tick while searching (until done, +0x9c): whether to search aggressively (+0x9f: Actor tag kind 4 at +0x2f8,
//   or flag 2 with morale 5 against a weakly defended target). A started search (+0x9e) runs its time (+0xc0) down
//   and is done at zero; the unit announces it (event 0xd for a plain search once past its first 90 ticks or when
//   done, once, +0x3bd; event 0x12 when a follow-up search ends). Not yet started, standing still and not a swarm, it
//   gives up after 120 ticks (+0xc4).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments
extern data_array *prop_data; // 0x008802c0
extern datum_index actor_get_target_prop_object_index(datum_index actor_index); // 0x4283d0, EAX

void actor_mode_search_tick(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag;
    datum_index unit_index;

    if (act[0x9c]) {
        return;
    }
    actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    if (*(int16_t *)&((Actor *)actor_tag)->defensive_crouch_type == 4) {
        act[0x9f] = 1;
    } else {
        act[0x9f] = 0;
        if ((actor_tag[0] & 2) && ((struct actor *)act)->mode_data.search.stage == 0 && ((actor *)act)->target_combat_status == 5 &&
            (int8_t)((uint8_t *)prop_data->data + (((actor *)act)->target_unit_index & 0xffff) * 0x138)[0x121] <= 2) {
            act[0x9f] = 1;
        }
    }
    if (!((struct actor *)act)->mode_data.search.reachable) {
        if (!act[0x504] && !act[0x6]) {
            ((struct actor *)act)->mode_data.search.elapsed_ticks += 1;
            if (((struct actor *)act)->mode_data.search.elapsed_ticks > 120) {
                act[0x9d] = 1;
                act[0x9c] = 1;
            }
        }
        return;
    }
    if (((struct actor *)act)->mode_data.search.remaining_ticks > 0) {
        ((struct actor *)act)->mode_data.search.remaining_ticks -= 1;
    }
    if (((struct actor *)act)->mode_data.search.remaining_ticks == 0) {
        act[0x9c] = 1;
    }
    unit_index = ((actor *)act)->unit_index;
    if (unit_index == k_datum_index_none) {
        return;
    }
    if (((struct actor *)act)->mode_data.search.stage == 0) {
        if (act[0x3bd]) {
            return;
        }
        if (act[0x9c] || ((struct actor *)act)->mode_data.search.remaining_ticks + 90 < ((struct actor *)act)->mode_data.search.duration_ticks) {
            ai_communication_broadcast(0xd, unit_index, actor_get_target_prop_object_index(actor_index), -1, -1, -1, 0);
            act[0x3bd] = 1;
        }
    } else if (((struct actor *)act)->mode_data.search.remaining_ticks == 0) {
        ai_communication_broadcast(0x12, unit_index, actor_get_target_prop_object_index(actor_index), -1, -1, -1, 0);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
