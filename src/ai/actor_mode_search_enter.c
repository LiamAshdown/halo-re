// actor_mode_search_enter  (not a Ghidra function: actor mode table 0x65524c, mode "search" slot +0x10)
// address 0x407940, size 201 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x407940..0x407a09 (no C existed: a mode entered in game would have hit a trap).
//   Searching starts: the search time (+0xbc, copied to +0xc0) is a random 30 * [lo, hi) seconds from the Actor tag
//   (+0x344 / +0x348 for a first search, +0x34c / +0x350 for a follow-up at +0xa4).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global;  // 0x00719cd0

void actor_mode_search_enter(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = (uint8_t *)tag_instances[*(datum_index *)(act + 0x58) & 0xffff].data;
    float lo;
    float hi;
    float t;
    int32_t ticks;

    if (*(int16_t *)(act + 0xa4) == 0) {
        lo = *(float *)(actor_tag + 0x344);
        hi = *(float *)(actor_tag + 0x348);
    } else {
        lo = *(float *)(actor_tag + 0x34c);
        hi = *(float *)(actor_tag + 0x350);
    }
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    t = (float)(random_seed_global >> 16) * 1.5259022e-05f;
    ticks = (int32_t)(((hi - lo) * t + lo) * 30.0f);
    *(int32_t *)(act + 0xbc) = ticks;
    *(int32_t *)(act + 0xc0) = ticks;
}
