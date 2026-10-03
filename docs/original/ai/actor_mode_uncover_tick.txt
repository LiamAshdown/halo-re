// actor_mode_uncover_tick  (not a Ghidra function: actor mode table 0x65524c, mode "uncover" slot +0x18)
// address 0x408470, size 526 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x408470..0x40867e (no C existed: a mode entered in game would have hit a trap).
//   Per tick while uncovering (until done, +0x9d): whether to push in (+0x9c: Actor tag kind 4 at +0x2f8 unless at
//   morale 6, or flag 2 with morale 5 against a weakly defended target; for an uncover point, kind 4 or flag 4 within
//   10 of it). Time moving (+0xc0) counts while walking; after 30 ticks the firing position is recognised as used
//   (0x4141a0). The uncover time (+0xc8) runs down (+0x9e started, +0xcc total) unless the actor holds a position it
//   should keep (+0x3b8, when told to or seeing its target or walking), which restarts it (+0xc4); it is done when the
//   time is out, after 360 ticks, or at the point (+0xbc) for kind 1.
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
extern data_array *prop_data; // 0x008802c0

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX, ECX
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type); // 0x4141a0, EAX, CX, DL

void actor_mode_uncover_tick(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag;
    int16_t kind;
    uint8_t keep_going = 1;
    uint8_t target_visible = 0;
    uint8_t done;

    if (act[0x9d]) {
        return;
    }
    actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    kind = ((struct actor *)act)->mode_data.uncover.stage;
    act[0x9c] = 0;
    if (kind == 0) {
        if (*(int16_t *)&((Actor *)actor_tag)->defensive_crouch_type == 4) {
            act[0x9c] = (uint8_t)(((actor *)act)->target_combat_status != 6);
        } else if ((actor_tag[0] & 2) && ((actor *)act)->target_combat_status == 5 &&
                   (int8_t)PROP(((actor *)act)->target_unit_index)[0x121] <= 2) {
            act[0x9c] = 1;
        }
    } else if (kind == 1) {
        if (*(int16_t *)&((Actor *)actor_tag)->defensive_crouch_type == 4 ||
            ((actor_tag[0] & 4) &&
             vector3d_distance_squared(&((struct actor *)act)->mode_data.uncover.position, (real_point3d *)(act + 0x12c)) < 100.0f)) {
            act[0x9c] = 1;
        }
    }
    if (act[0x504]) {
        ((struct actor *)act)->mode_data.uncover.stage_ticks = 0;
    } else {
        ((struct actor *)act)->mode_data.uncover.stage_ticks += 1;
        if (kind == 0 && ((struct actor *)act)->mode_data.uncover.stage_ticks >= 30) {
            actor_push_recognition_entry(actor_index, ((actor *)act)->firing_position_index, 0);
        }
    }
    kind = ((struct actor *)act)->mode_data.uncover.stage;
    if (kind == 0) {
        if (((actor *)act)->target_unit_index != k_datum_index_none) {
            target_visible = (uint8_t)(*(int16_t *)(PROP(((actor *)act)->target_unit_index) + 0x32) > 0);
            keep_going = (uint8_t)!(target_visible && ((actor *)act)->target_combat_status < 5);
        }
    } else {
        keep_going = (uint8_t)(act[0xbc] == 0);
    }
    if (((actor *)act)->firing_position_index != -1 && keep_going && (act[0x162] || target_visible || act[0x504])) {
        ((struct actor *)act)->mode_data.uncover.remaining_ticks = ((struct actor *)act)->mode_data.uncover.duration_ticks;
    } else {
        act[0x9e] = 1;
        if (((struct actor *)act)->mode_data.uncover.remaining_ticks > 0) {
            ((struct actor *)act)->mode_data.uncover.remaining_ticks -= 1;
        }
        *(int32_t *)(act + 0xcc) += 1;
    }
    done = (uint8_t)(((struct actor *)act)->mode_data.uncover.remaining_ticks == 0 || *(int32_t *)(act + 0xcc) >= 360);
    if (kind == 1 && act[0xbc]) {
        done = 1;
    }
    act[0x9d] = done;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
