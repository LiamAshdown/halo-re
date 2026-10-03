// actor_mode_alert_process  (not a Ghidra function; AI "alert" mode)
// address 0x4010e0, size 501 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: actor_mode_definitions[2] ("alert") +0x0c slot.
// objdump 0x4010e0..0x4012d4, the mode data at actor +0x9c: +0x9c move position count, +0xa2 current and +0xa4 next
//   position index, +0x9e wait ticks, +0xa6 arrived, +0xa8 the current position record (0x50 bytes):
//   - with positions and no next one chosen: once the actor is at the current one (moving, +0x4a8: within
//     max(actor_compute_accuracy_scale, 0.5) of +0xa8) and done waiting and not arrived and its unit's +0x2a3 is not
//     0x1c, the next index comes from actor_select_move_position(count, current, &+0xa0);
//   - an actor that may move (+0x4c) and is not frozen (+0x13) takes a chosen next position: when it is a valid move
//     position of the actor's squad (encounter +0x34, squad +0x3a; squad +0xc4 count, +0xc8 records of 0x50) its
//     record is copied, the wait becomes random(+0x14, +0x18) seconds and arrival is flagged, and the movement is
//     set (actor_movement_set_destination_move_position); otherwise (or when that fails) the position is taken with
//     no wait. Always returns 0.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;

#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX, ECX
extern float actor_compute_accuracy_scale(datum_index actor_index); // 0x429620, EAX
extern int32_t actor_select_move_position(uint32_t actor_index, int16_t select_mode, int32_t position_index,
    uint8_t *direction_flag); // 0x4014c0, EAX, stack
extern real random_real_range(real min, real max); // 0x401050
extern uint8_t actor_movement_set_destination_move_position(datum_index actor_index, int16_t move_position_index); // 0x417750, EDI, stack
#include <string.h>

uint8_t actor_mode_alert_process(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    int16_t count = W(0x9c);

    if (count != 0 && W(0xa4) == -1) {
        int16_t current = W(0xa2);
        int ready = 1;

        if (current != -1 && B(0x4a8) != 0) {
            float distance_squared = vector3d_distance_squared((real_point3d *)(actor + 0xa8), (real_point3d *)(actor + 0x12c));
            float radius = actor_compute_accuracy_scale(actor_index);

            if (!(radius > 0.5f)) {
                radius = 0.5f;
            }
            if (distance_squared > radius * radius) {
                ready = 0;
            }
        }
        if (ready && !(W(0x9e) > 0) && B(0xa6) == 0) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[D(0x18) & 0xffff].data;

            if (unit[0x2a3] != 0x1c) {
                W(0xa4) = (int16_t)actor_select_move_position(actor_index, count, current, actor + 0xa0);
            }
        }
    }

    if (B(0x4c) == 0 || B(0x13) != 0 || W(0xa4) == -1) {
        return 0;
    }
    if (D(0x34) != 0xffffffff) {
        uint8_t *encounter = (uint8_t *)global_scenario->encounters.pointer + (D(0x34) & 0xffff) * 0xb0;
        uint8_t *squad = *(uint8_t **)(encounter + 0x84) + W(0x3a) * 0xe8;
        int16_t next = W(0xa4);

        if (next >= 0 && next < *(int32_t *)(squad + 0xc4)) {
            uint8_t *position = *(uint8_t **)(squad + 0xc8) + next * 0x50;
            float wait = random_real_range(*(float *)(position + 0x14), *(float *)(position + 0x18)) * 30.0f;

            W(0xa2) = W(0xa4);
            W(0xa4) = -1;
            memcpy(actor + 0xa8, position, 0x50);
            W(0x9e) = (int16_t)(int32_t)wait;
            B(0xa6) = 1;
            if (actor_movement_set_destination_move_position(actor_index, W(0xa2))) {
                return 0;
            }
        }
    }
    W(0xa2) = W(0xa4);
    W(0xa4) = -1;
    W(0x9e) = 0;
    B(0xa6) = 0;
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
