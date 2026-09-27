// actor_take_danger_escape  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40e060, size 479 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40e060..0x40e23f.
//   Dodge a danger (a grenade) sideways or back: an actor on foot probes the escape step (0x417e50, which may
//   change the direction) and scores the four dodge directions in the facing frame (side, other side, forward,
//   back) plus each dodge animation's bias (table 0x655558: action, direction, bias; dives score 1.5 more); the
//   best one above -0.5 the unit can play (0x569470) is queued as the secondary action with its direction
//   (0x417a60) and announced (event 0x2c). Returns 1 when queued.
// blam-cc: EAX -> path_delta, stack -> (actor_index, escape_direction, step_distance bits, step_up)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t actor_probe_step_direction(datum_index actor_index, float step_distance, real_vector2d *direction,
                                          uint16_t *variant, float step_up, uint8_t *out_flag, void *extra_param); // 0x417e50, stack, ECX
extern uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command); // 0x569470, EAX, CX
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2]); // 0x417a60, EAX, stack
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340

typedef struct actor_dodge_entry {
    int16_t action;    // unit scripted action animation
    int16_t direction; // 0 side, 1 other side, 2 forward, 3 back (in the facing frame)
    float bias;
} actor_dodge_entry;
extern const actor_dodge_entry actor_dodge_table[]; // 0x00655558, terminated by action -1

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

uint8_t actor_take_danger_escape(real_vector3d *path_delta, datum_index actor_index, uint32_t escape,
                                 uint32_t extra, float distance)
{
    uint8_t *act = ACTOR(actor_index);
    uint16_t direction_kind = (uint16_t)escape;
    float step_distance = *(float *)&extra;
    uint8_t probe_flag;
    float probe_extra[4];
    float a = 0.0f;
    float b = 0.0f;
    float scores[4];
    float fx;
    float fy;
    float best = -0.5f;
    int16_t best_action = -1;
    int16_t best_direction = -1;
    int32_t i;
    float payload[2];
    uint8_t queued;

    if (*(datum_index *)(act + 0x158) != k_datum_index_none) {
        return 0;
    }
    if (!actor_probe_step_direction(actor_index, step_distance, (real_vector2d *)path_delta, &direction_kind, distance,
                                    &probe_flag, probe_extra)) {
        return 0;
    }
    switch ((int16_t)direction_kind) { // 0x40e240
    case 0: a = -path_delta->j; b = path_delta->i; break;
    case 1: a = path_delta->j; b = -path_delta->i; break;
    case 2:
    case 3: a = path_delta->i; b = path_delta->j; break;
    default: b = probe_extra[1]; break;
    }
    fx = *(float *)(act + 0x174);
    fy = *(float *)(act + 0x178);
    scores[2] = b * fy + a * fx;  // forward
    scores[0] = fx * b - fy * a;  // side
    scores[3] = -scores[2];
    scores[1] = -scores[0];
    for (i = 0; actor_dodge_table[i].action != -1; i++) {
        float value = scores[actor_dodge_table[i].direction] + actor_dodge_table[i].bias;

        if (value > best && unit_scripted_action_animation_exists(*(datum_index *)(act + 0x18), actor_dodge_table[i].action)) {
            best = value;
            best_direction = actor_dodge_table[i].direction;
            best_action = actor_dodge_table[i].action;
        }
    }
    if (best_action == -1) {
        return 0;
    }
    switch (best_direction) { // 0x40e250
    case 0: payload[0] = b; payload[1] = -a; break;
    case 1: payload[0] = -b; payload[1] = a; break;
    case 2:
    case 3: payload[0] = a; payload[1] = b; break;
    default: payload[0] = 0.0f; payload[1] = 0.0f; break;
    }
    queued = actor_queue_secondary_action(actor_index, best_action, (uint32_t *)payload);
    if (queued) {
        ai_communication_broadcast(0x2c, *(datum_index *)(act + 0x18), -1, -1, -1, -1, 0);
    }
    return queued;
}
