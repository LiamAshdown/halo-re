// actor_escalate_to_guard_or_combat  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40aaf0, size 140 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40aaf0..0x40ab7b: an actor below awareness 3 (+0x6a) with a pending escalation (+0x312) becomes aware
//   (3) and enters guard mode (actor_set_mode 6) when actor_build_guard_mode_data 0x404360 fills a record, else
//   runs actor_evaluate_combat_state_transition; +0x312 is cleared. Returns whether it escalated.
// blam-cc: EDI -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & 0xffff) * 0x724)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

extern uint8_t actor_build_guard_mode_data(datum_index actor_index, uint8_t *out); // 0x404360, EDX actor, stack out
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0
extern char actor_evaluate_combat_state_transition(uint32_t actor_index); // 0x40c620

uint8_t actor_escalate_to_guard_or_combat(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);
    uint8_t record[0x84];

    if (!(W(0x6a) < 3) || W(0x312) == 0) {
        return 0;
    }
    W(0x6a) = 3;
    if (actor_build_guard_mode_data(actor_index, record)) {
        actor_set_mode(actor_index, 6, record);
    } else {
        actor_evaluate_combat_state_transition(actor_index);
    }
    W(0x312) = 0;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
