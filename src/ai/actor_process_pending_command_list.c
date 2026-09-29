// actor_process_pending_command_list  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40a140, size 155 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40a140..0x40a1da: a command list queued on the actor (+0x90 != -1) starts when forced (+0x8e) or when
//   the actor is aware (+0x6a) and not reloading (actor_wants_reload_or_swap 0x40ab80): the obey mode record is
//   built (0x407140, ESI record) and, if accepted, actor_set_mode(actor, 11, record). The queue is cleared either
//   way. Returns whether the mode was set.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "fn_ai.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & 0xffff) * 0x724)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

extern uint8_t actor_wants_reload_or_swap(uint32_t actor_index); // 0x40ab80, EAX


uint8_t actor_process_pending_command_list(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);
    uint8_t started = 0;
    int16_t record[0x42];

    if (W(0x90) == -1) {
        return 0;
    }
    if (B(0x8e) == 0 && (W(0x6a) == 0 || actor_wants_reload_or_swap(actor_index))) {
        return 0;
    }
    if (actor_squad_action_status_broadcast(actor_index, (int16_t)W(0x90), record) != 0) {
        actor_set_mode(actor_index, 0xb, record);
        started = 1;
    }
    B(0x8e) = 0;
    W(0x90) = -1;
    return started;
}
