// actor_mode_alert_tick  (not a Ghidra function; AI "alert" mode)
// address 0x4012e0, size 299 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: actor_mode_definitions[2] ("alert") +0x10 slot.
// objdump 0x4012e0..0x40140a: with a current move position (+0xa2) and not frozen (+0x13): when moving under
//   actor control (+0x4a8 without +0x484) only once within 0.5 of it (distance squared < 0.25); the wait (+0x9e)
//   counts down, and on arrival (+0xa6) the position's animation (+0xc4: scenario +0x448 entries of 0x3c, the name
//   first, graph +0x2c or the unit's own +0x44) plays (unit_start_user_animation, interpolated) and arrival clears.
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

extern uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name,
    uint8_t interpolate); // 0x5702a0, stack unit, EDI graph, EAX name, stack interpolate

void actor_mode_alert_tick(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    if (B(0x13) != 0 || W(0xa2) == -1) {
        return;
    }
    if (B(0x4a8) != 0 && B(0x484) == 0) {
        float dx = F(0xa8) - F(0x12c);
        float dy = F(0xac) - F(0x130);
        float dz = F(0xb0) - F(0x134);

        if (!(dz * dz + dy * dy + dx * dx < 0.25f)) {
            return;
        }
    }
    if (W(0x9e) > 0) {
        W(0x9e) = (int16_t)(W(0x9e) - 1);
    }
    if (B(0xa6) == 0) {
        return;
    }
    if (W(0xc4) != -1) {
        uint8_t *animation = (uint8_t *)global_scenario->ai_animation_references.pointer + W(0xc4) * 0x3c;
        datum_index graph = *(datum_index *)(animation + 0x2c);

        if (graph == k_datum_index_none) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[D(0x18) & 0xffff].data;

            graph = *(datum_index *)((uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data + 0x44);
        }
        unit_start_user_animation(D(0x18), graph, (const char *)animation, 1);
    }
    B(0xa6) = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
