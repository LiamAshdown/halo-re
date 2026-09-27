// actor_react_to_disturbance  (not a Ghidra function; AI shared behaviour, called by the actor type update procs)
// address 0x40a1e0, size 490 bytes
// name confidence: 0.3  rewrite confidence: 0.85
// objdump 0x40a1e0..0x40a3c9: unless busy (+0x160), a disturbance level (+0x2ee) at or above threshold turns the
//   actor: toward +0x2fc (a 2D direction; reversed with action 5 when it points behind the body facing +0x5a4) when
//   +0x2f8 is set, else toward +0x174 (action 4), through actor_queue_secondary_action; broadcasts event 0x29 about
//   the prop +0x2f4 (its object +0x18, 2 or 3 by its +0x60); the actor definition (+0x5c) +0x90 / +0x8c seconds set
//   a +0x5f2 = 4 / +0x5f4 timer and raise +0x5f6 (0x40f7a0); marks +0x2f0 and considers the prop as a target
//   (actor_consider_target_candidate). +0x2ee is cleared on every path past the busy check. Returns whether it reacted.
// blam-cc: stack -> actor_index, threshold

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & 0xffff) * 0x724)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

extern data_array *prop_data; // 0x008802c0
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2]); // 0x417a60
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340
extern void actor_raise_timer_5f6(datum_index actor_index, int32_t ticks); // 0x40f7a0
extern uint16_t actor_consider_target_candidate(datum_index actor_index, datum_index candidate_prop_index); // 0x4208a0
extern int32_t __ftol(void);

uint8_t actor_react_to_disturbance(datum_index actor_index, int16_t threshold)
{
    uint8_t *actor = ACTOR(actor_index);
    uint8_t *definition = (uint8_t *)tag_instances[D(0x5c) & 0xffff].data;
    real_vector2d direction;
    int16_t action = 4;
    datum_index object = k_datum_index_none;
    int32_t reason = 0;

    if (B(0x160) != 0 || W(0x2ee) < threshold) {
        W(0x2ee) = 0;
        return 0;
    }
    if (B(0x2f8) != 0) {
        direction.i = F(0x2fc);
        direction.j = F(0x300);
        vector2d_normalize_with_length(&direction);
        if (direction.j * F(0x5a8) + direction.i * F(0x5a4) < 0.0f) {
            direction.i = -direction.i;
            direction.j = -direction.j;
            action = 5;
        }
    } else {
        direction.i = F(0x174);
        direction.j = F(0x178);
        vector2d_normalize_with_length(&direction);
    }
    actor_queue_secondary_action(actor_index, action, (uint32_t *)&direction);
    if (D(0x2f4) != 0xffffffff) {
        uint8_t *prop = (uint8_t *)prop_data->data + (D(0x2f4) & 0xffff) * 0x138;

        object = *(datum_index *)(prop + 0x18);
        reason = (prop[0x60] != 0) + 2;
    }
    ai_communication_broadcast(0x29, D(0x18), object, reason, 0xffffffff, 0xffffffff, 0);
    if (*(float *)(definition + 0x90) > 0.0f) {
        W(0x5f2) = 4;
        W(0x5f4) = (int16_t)(int32_t)(*(float *)(definition + 0x90) * 30.0f);
    }
    if (*(float *)(definition + 0x8c) > 0.0f) {
        actor_raise_timer_5f6(actor_index, (int32_t)(*(float *)(definition + 0x8c) * 30.0f));
    }
    B(0x2f0) = 1;
    if (D(0x2f4) != 0xffffffff) {
        actor_consider_target_candidate(actor_index, D(0x2f4));
    }
    W(0x2ee) = 0;
    return 1;
}
