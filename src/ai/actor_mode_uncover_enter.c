// actor_mode_uncover_enter  (not a Ghidra function: actor mode table 0x65524c, mode "uncover" slot +0x10)
// address 0x4081e0, size 277 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x4081e0..0x4082f5 (no C existed: a mode entered in game would have hit a trap).
//   Uncovering starts: the uncover time (+0xc4, copied to +0xc8) is a random 30 * [lo, hi) seconds from the Actor tag
//   (+0x33c / +0x340, raised to at least +0x344 / +0x348 unless the actor is a leader, +0x9f). A plain uncover
//   (+0xa4 == 0) of a known target by an actor not yet in combat (+0x6e below 3) tells the others (event 0x15).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

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

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments
extern data_array *prop_data;       // 0x008802c0
extern uint32_t random_seed_global;  // 0x00719cd0

void actor_mode_uncover_enter(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    float lo = *(float *)(actor_tag + 0x33c);
    float hi = *(float *)(actor_tag + 0x340);
    float t;
    int32_t ticks;

    if (!act[0x9f]) {
        if (!(lo > *(float *)(actor_tag + 0x344))) {
            lo = *(float *)(actor_tag + 0x344);
        }
        if (!(hi > *(float *)(actor_tag + 0x348))) {
            hi = *(float *)(actor_tag + 0x348);
        }
    }
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    t = (float)(random_seed_global >> 16) * 1.5259022e-05f;
    ticks = (int32_t)(((hi - lo) * t + lo) * 30.0f);
    ((struct actor *)act)->mode_data.uncover.duration_ticks = ticks;
    ((struct actor *)act)->mode_data.uncover.remaining_ticks = ticks;
    if (((struct actor *)act)->mode_data.uncover.stage == 0 && ((actor *)act)->target_unit_index != k_datum_index_none &&
        ((struct actor *)act)->combat_status < 3) {
        uint8_t *target = (uint8_t *)prop_data->data + (((actor *)act)->target_unit_index & 0xffff) * 0x138;

        ai_communication_broadcast(0x15, ((actor *)act)->unit_index, ((prop *)target)->object_index, -1, -1, -1, 0);
    }
}
