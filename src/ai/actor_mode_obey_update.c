// actor_mode_obey_update  (not a Ghidra function; AI "obey" mode, the command-list mode ai_command_list switches actors to)
// address 0x407400, size 973 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: actor_mode_definitions[11] ("obey") update slot (+0x14, actor_invoke_type_handler).
// objdump 0x407400..0x4077cc, raw actor offsets (0x724 each):
//   - movement/facing/firing modes (+0x3e8/+0x3ec/+0x3fc): a script-driven target (+0xfe) gives 7/2/4 and copies
//     its point (+0x100, 12 bytes -> +0x460) and object (+0x10c -> +0x458) with +0x454/+0x457/+0x45d set; else a
//     firing point (+0xe0, unless +0x4a8 is set without +0x484) gives 4/3, copies +0xe4 -> +0x3f0 and, when +0x99
//     is clear, +0x128 -> +0x3f8, and a firing mode of 4 (1 while awareness +0x6a < 3); else a look mode (+0xca) of
//     1 or 3 gives 7/0/0; else high morale-ish +0x6e >= 5 with +0xa8 bit 0 gives 7/2/4 and +0x454; else movement 0
//     and firing (+0x9f ? (awareness < 3 ? 1 : 4) : 0);
//   - +0x110 moves to +0x45c; +0xc8 is copied to +0x426/+0x427, +0xca to +0x42c;
//   - a pending facing/speech request (+0xf8), once the actor is not mid-animation (+0x418 == -1 and its unit +0x18
//     not busy, unit_is_in_busy_animation_state), queues the secondary action +0xfa with the normalized direction
//     +0x5a4 (actor_queue_secondary_action, EAX actor) and broadcasts the speech +0xfc
//     (ai_communication_broadcast(speech, unit, -1, -1, -1, -1, 0)), then clears +0xf8;
//   - aim/look (+0xa9): bit 0 copies the aim point +0xb0 -> +0x434 and +0xac -> +0x42e with +0x430; with bit 2, bit 3
//     (or a pending count +0xac, +0x15c, or a busy unit) looks at +0x174 (-> +0x434, +0x430 = 1, +0x42e = 0); else a
//     fresh look: the normalized 2D +0x174 (or the default at *0x006966e8 when zero) goes to +0x444/+0x448 with
//     +0x440 = 1, +0x441 = (+0xb0 * 0.7 > +0xb4), +0x442 = +0xa9 bit 4, +0x44c/+0x450 = +0xb0/+0xb4, +0xa9 |= 8, and
//     +0xac = 15 unless +0xa9 bit 4.
// blam-cc: stack -> actor_index

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

extern data_array *actor_data;                 // 0x00880360
extern real_vector2d *global_forward2d_pointer; // 0x006966e8

extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX
extern real vector2d_normalize_with_length(real_vector2d *v);       // 0x4018e0, ECX
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2]); // 0x417a60, EAX, stack
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340

#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

static void copy12(uint8_t *actor, int to, int from)
{
    D(to) = D(from);
    D(to + 4) = D(from + 4);
    D(to + 8) = D(from + 8);
}

void actor_mode_obey_update(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    if (B(0xfe) != 0) {
        W(0x3e8) = 7;
        W(0x3ec) = 2;
        W(0x3fc) = 4;
        B(0x454) = 1;
        B(0x457) = 1;
        B(0x45d) = 1;
        copy12(actor, 0x460, 0x100);
        D(0x458) = D(0x10c);
    } else if (B(0xe0) != 0 && (B(0x4a8) == 0 || B(0x484) != 0)) {
        W(0x3e8) = 4;
        W(0x3ec) = 3;
        copy12(actor, 0x3f0, 0xe4);
        if (B(0x99) == 0) {
            D(0x3f8) = D(0x128);
        }
        W(0x3fc) = (int16_t)(W(0x6a) < 3 ? 1 : 4);
    } else if (W(0xca) == 3 || W(0xca) == 1) {
        W(0x3ec) = 0;
        W(0x3fc) = 0;
        W(0x3e8) = 7;
    } else if (W(0x6e) >= 5 && (B(0xa8) & 1)) {
        W(0x3ec) = 2;
        W(0x3fc) = 4;
        B(0x454) = 1;
        W(0x3e8) = 7;
    } else {
        W(0x3e8) = 0;
        if (B(0x9f) != 0) {
            W(0x3fc) = (int16_t)(W(0x6a) < 3 ? 1 : 4);
        } else {
            W(0x3fc) = 0;
        }
    }

    if (B(0x110) != 0) {
        B(0x45c) = 1;
        B(0x110) = 0;
    }
    B(0x426) = B(0xc8);
    B(0x427) = B(0xc8);
    W(0x42c) = W(0xca);

    if (B(0xf8) != 0 && W(0x418) == -1 &&
        (D(0x18) == 0xffffffff || !unit_is_in_busy_animation_state(D(0x18)))) {
        if (W(0xfa) != -1) {
            uint32_t direction[2];

            direction[0] = D(0x5a4);
            direction[1] = D(0x5a8);
            vector2d_normalize_with_length((real_vector2d *)direction);
            actor_queue_secondary_action(actor_index, W(0xfa), direction);
        }
        if (W(0xfc) != -1) {
            ai_communication_broadcast(W(0xfc), D(0x18), 0xffffffff, -1, 0xffffffff, 0xffffffff, 0);
        }
        B(0xf8) = 0;
    }

    if (B(0xa9) & 1) {
        B(0x430) = 1;
        copy12(actor, 0x434, 0xb0);
        W(0x42e) = W(0xac);
    }
    if ((B(0xa9) & 4) == 0) {
        return;
    }
    if (B(0xa9) & 8) {
        if (!(W(0xac) > 0)) {
            return;
        }
    } else if (W(0xac) == 0 && B(0x15c) == 0 && !unit_is_in_busy_animation_state(D(0x18))) {
        real_vector2d direction;
        float x;
        float y;

        direction = *(real_vector2d *)&((struct actor *)actor)->facing.i;
        if (vector2d_normalize_with_length(&direction) == 0.0f) {
            x = global_forward2d_pointer->i;
            y = global_forward2d_pointer->j;
        } else {
            x = direction.i;
            y = direction.j;
        }
        B(0x440) = 1;
        B(0x441) = (uint8_t)(F(0xb0) * 0.7f > F(0xb4));
        B(0x442) = (uint8_t)((B(0xa9) >> 4) & 1);
        F(0x448) = y;
        F(0x444) = x;
        D(0x44c) = D(0xb0);
        D(0x450) = D(0xb4);
        B(0xa9) = (uint8_t)(B(0xa9) | 8);
        if ((B(0xa9) & 0x10) == 0) {
            W(0xac) = 0xf;
        }
        return;
    }
    copy12(actor, 0x434, 0x174);
    B(0x430) = 1;
    W(0x42e) = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
