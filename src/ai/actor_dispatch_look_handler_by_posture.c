// actor_dispatch_look_handler_by_posture  (Ghidra: actor_dispatch_look_handler_by_posture; really the perception range test)
// address 0x41bb30, size 720 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// REWRITTEN from objdump 0x41bb30..0x41bdfa (the old draft stopped at the 0x41be00 jump table, 133 bytes, with the
//   wrong arity). Grades whether the actor perceives `target` from `origin`:
//   - posture (BX) other than 0 or 1: 0;
//   - range = the actor definition's +0x150 (when positive; actor_get_actor_definition 0x40fa70) else the actor
//     tag's +0x18, times 0.4 / 0.6 / 0.8 / 1.0 by range_class (table 0x41be00); out of range: 0;
//   - a scale (1.0, or 0.3 / 0.7 for stance_a 0 / 1 unless the actor tag has flag bit 0) shrinks it; out of the
//     scaled range: 0;
//   - with check_facing and a non-swarm actor (+0x06), the target is taken into the actor's frame (+0x18c forward,
//     +0x198 left, +0x1a4 up): above 30 degrees or below -45 degrees it is unseen (range 0, scale 0), otherwise
//     0x41bed0 adjusts range and scale for its azimuth; without it the range becomes 0.7 x the scaled range;
//   - posture 0 inside the range: 3 within 6 units, else 2; otherwise 1 inside the scale (squared), else 0.
// blam-cc: EBX -> posture, stack -> actor_index, origin, target, stance_a, check_facing, range_class

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern double sqrt(double x);
extern double atan2(double y, double x);
extern double fabs(double x);

extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX
extern void unit_get_move_speed_for_range(datum_index actor_index, float param_a, float param_b, float param_dist,
    float *out_a, float *out_b); // 0x41bed0, EAX, stack

static const float k_perception_range_class_scale[4] = {0.4f, 0.6f, 0.8f, 1.0f}; // 0x672bc8, 0x672ca8, 0x672ca0, 0x672ac4

int16_t actor_dispatch_look_handler_by_posture(int16_t posture, uint32_t actor_index, void *origin, void *target,
    uint8_t stance_a, uint8_t check_facing, uint16_t range_class)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = (uint8_t *)tag_instances[((struct actor *)actor)->actor_definition_tag & 0xffff].data;
    uint8_t *definition;
    float *from = (float *)origin;
    float *to = (float *)target;
    float base;
    float range;
    float scale;
    float current;
    float dx, dy, dz;
    float distance_squared;

    if (posture != 0 && posture != 1) {
        return 0;
    }
    definition = (uint8_t *)actor_get_actor_definition(actor_index);
    base = ((Actor *)actor_tag)->max_vision_distance;
    if (*(float *)(definition + 0x150) > 0.0f) {
        base = *(float *)(definition + 0x150);
    }
    range = base * k_perception_range_class_scale[(int16_t)range_class & 3];
    dx = to[0] - from[0];
    dy = to[1] - from[1];
    dz = to[2] - from[2];
    distance_squared = dz * dz + dy * dy + dx * dx;
    if (!(range * range > distance_squared)) {
        return 0;
    }
    scale = 1.0f;
    if ((actor_tag[0] & 1) == 0) {
        if ((int8_t)stance_a == 0) {
            scale = 0.3f;
        } else if ((int8_t)stance_a == 1) {
            scale = 0.7f;
        }
    }
    current = scale * range;
    if (!(distance_squared < current * current)) {
        return 0;
    }

    if (actor[6] == 0 && check_facing != 0) {
        float forward = dz * ((struct actor *)actor)->facing_unknown_18c.k + dy * ((struct actor *)actor)->facing_unknown_18c.j + dx * ((struct actor *)actor)->facing_unknown_18c.i;
        float left = dz * *(float *)(actor + 0x1a0) + dy * *(float *)&((struct actor *)actor)->unknown_19c + dx * *(float *)(actor + 0x198);
        float up = dz * *(float *)(actor + 0x1ac) + dy * *(float *)&((struct actor *)actor)->unknown_1a8 + dx * *(float *)(actor + 0x1a4);
        float elevation = (float)atan2((double)up, sqrt((double)(left * left + forward * forward)));

        if (elevation > 0.5235988f || !(elevation > -0.78539819f)) {
            range = 0.0f;
            current = 0.0f;
        } else {
            float azimuth = (float)fabs(atan2((double)left, (double)forward));

            unit_get_move_speed_for_range(actor_index, range, scale, azimuth, &range, &scale);
            current = scale;
        }
    } else {
        range = current * 0.7f;
    }

    if (posture == 0 && range * range > distance_squared) {
        return distance_squared < 36.0f ? 3 : 2;
    }
    return distance_squared < current * current ? 1 : 0;
}

#if 0
Original Ghidra decompilation (0x41bb30):

undefined4 FUN_0041bb30(void)

{
  undefined4 uVar1;
  short unaff_BX;
  short in_stack_00000018;

  if ((unaff_BX != 0) && (unaff_BX != 1)) {
    return 0;
  }
  actor_get_actor_definition();
                    /* WARNING: Could not recover jumptable at 0x0041bba4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  uVar1 = (*(code *)(&PTR_LAB_0041be00)[in_stack_00000018])();
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
