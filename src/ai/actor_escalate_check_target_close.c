// actor_escalate_check_target_close  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40a950, size 141 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40a950..0x40a9dd.
//   A fighting actor (+0x6e 5+) whose target is nearer than the Actor tag's distance (+0x3a0) escalates: level
//   (+0x310) at least 2.
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

uint8_t actor_escalate_check_target_close(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);

    if (((struct actor *)act)->alert_level < 5) {
        return 0;
    }
    if (!(*(float *)(PROP(((actor *)act)->target_unit_index) + 0x11c) <
          *(float *)(TAG_DATA(((actor *)act)->actor_definition_tag) + 0x3a0))) {
        return 0;
    }
    if (*(int16_t *)(act + 0x310) <= 2) {
        *(int16_t *)(act + 0x310) = 2;
    }
    return 1;
}
