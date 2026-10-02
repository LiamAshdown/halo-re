// actor_escalate_apply  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40aa70, size 122 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40aa70..0x40aaea.
//   The escalation level (+0x310) reaching the threshold escalates a non-retreating actor (+0x378): combat alert
//   (0x421a40, BL 1) and, once fighting (+0x6e 4+), a combat state transition (0x40c620) whose result is returned.
//   The level is reset either way.
// blam-cc: EDI -> actor_index, stack -> threshold

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag); // 0x421a40, EAX, EBX
extern char actor_evaluate_combat_state_transition(uint32_t actor_index); // 0x40c620

uint8_t actor_escalate_apply(datum_index actor_index, int16_t threshold)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t result = 0;

    if (*(int16_t *)(act + 0x310) >= threshold && !act[0x378]) {
        actor_set_combat_alert_flag(actor_index, 1);
        if (((struct actor *)act)->combat_status >= 4) {
            result = (uint8_t)actor_evaluate_combat_state_transition(actor_index);
        }
    }
    *(int16_t *)(act + 0x310) = 0;
    return result;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
