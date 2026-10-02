// actor_compute_swarm_avoidance_offset  (Ghidra: actor_compute_swarm_avoidance_offset, renamed)
// address 0x425c70, size 755 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN: leap-solve branch decoded from 0x425d26 (register arguments, leap direction, half-gravity cap, radius clamp); flag branch verified)
// evidence: phase-4 summary "Computes a per-swarm-member avoidance/spacing offset vector,
// either mirroring the member's own velocity or invoking a steering helper, clamped to a
// maximum radius." No static callers are recorded by Ghidra (out/functions.json: callers=0).
// types/ai.h swarm.unit_index[16]/component_index[16] (0x18/0x58), swarm_component.
// unknown_14 (0x14, "swarm_add_component sets -1"); actor.swarm_index (0x28), actor.facing
// (0x174). Calls vector2d_normalize_with_length (0x4018e0, math module) and projectile_solve_ballistic_arc
// (0x4beb30, outside this rewrite's range, UNSURE signature -- a curve/steering evaluator
// judging by its nine arguments, all of which Ghidra did resolve).
// UNSURE: swarm_component+0x02 (a flags word), +0x21 (a flag byte within the unnamed
// unknown_18[40] run) and +0x28/+0x2c (a stored 2D direction and scale, also within that
// run) have no individual names in types/ai.h; accessed as raw offsets. prop.unknown_130
// (checked against '\0' here) likewise has no established meaning beyond its raw offset.
// register convention: Ghidra already resolved all four parameters as ordinary parameters.
//   // blam-cc: stack -> actor_index, unit_index, radius, out_offset (not independently
//   re-verified with objdump, given this function has no live callers).

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
extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *object_data;          // 0x008603b0
extern data_array *prop_data;            // 0x008802c0
extern const real_vector2d *global_forward2d_pointer; // 0x006966e8, UNSURE name (mirrors
                                                       // global_forward3d_pointer @0x696718)

extern double sqrt(double x); // FSQRT
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern uint8_t projectile_solve_ballistic_arc(real_point3d *target, real_point3d *origin,
    real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc,
    real_vector3d *out_direction, real *max_speed_override, real *out_speed,
    real *out_time_of_flight, real *out_range, real *out_half_gravity_term,
    real *out_horizontal_speed); // 0x4beb30, EAX, ECX, ESI, EDI, stack
extern const real_vector3d *global_forward3d_pointer; // 0x00696718

// blam-cc: stack -> actor_index, unit_index, radius, out_offset
// For the swarm member matching `unit_index`: if its swarm_component has a still-valid
// "target" reference (unknown_14) with certain flag bits set, mirrors that target's forward
// direction (falling back to the global forward vector when degenerate) scaled by its own
// stored 2D direction/scale into `out_offset`, and clears one of those flag bits. Otherwise
// runs a steering curve (projectile_solve_ballistic_arc) against the actor's own facing direction (falling back
// the same way) to produce a spacing offset, clamped to `radius`.
void actor_compute_swarm_avoidance_offset(datum_index actor_index, datum_index unit_index, float radius, float *out_offset)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm_index == (datum_index)k_datum_index_none) {
        return;
    }

    {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;

        for (i = 0; i < s->component_count; i++) {
            if (s->unit_index[i] == unit_index) {
                object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
                swarm_component *component = &((swarm_component *)swarm_component_data->data)[s->component_index[i] & 0xffff];
                uint16_t flags = *(uint16_t *)&((struct swarm_component *)component)->flags; // UNSURE offset
                datum_index target = component->leap_target_index;

                if ((flags & 1) == 0 || target == (datum_index)k_datum_index_none) {
                    if ((flags & 8) != 0 && (flags & 0x10) != 0) {
                        uint8_t byte21 = *((uint8_t *)component + 0x21); // UNSURE offset
                        if ((byte21 & 4) != 0 && (byte21 & 0x10) != 0) {
                            real_vector2d dir;
                            dir.i = unit_object->forward.i;
                            dir.j = unit_object->forward.j;
                            if (vector2d_normalize_with_length(&dir) == 0.0f) {
                                dir.i = ((struct object *)unit_object)->up.i;
                                dir.j = ((struct object *)unit_object)->up.j;
                                if (vector2d_normalize_with_length(&dir) == 0.0f) {
                                    dir.i = global_forward2d_pointer->i;
                                    dir.j = global_forward2d_pointer->j;
                                }
                            }
                            {
                                float scale = *((float *)((uint8_t *)component + 0x28)); // UNSURE offset
                                float z = *((float *)((uint8_t *)component + 0x2c));      // UNSURE offset
                                out_offset[0] = dir.i * scale;
                                out_offset[1] = dir.j * scale;
                                out_offset[2] = z;
                            }
                        }
                        *((uint8_t *)component + 2) &= 0xef;
                    }
                } else {
                    // REWRITTEN (0x425d26..0x425e9a): a leap solve from the component (+0x4) to the
                    // target prop's +0xc8 point, speed limit = max(radius, 0.12), max time 0.7; the
                    // offset is the leap direction (2D-normalized; actor facing +0x174, then the
                    // global forward as fallbacks) times the horizontal speed, with the half-gravity
                    // term (capped at 0.075 unless prop +0x130) as z, clamped to length radius.
                    // The old C called the solver without its register arguments (EAX target,
                    // ECX origin, ESI direction, EDI 0) and always steered along the facing.
                    const uint8_t *target_prop = (const uint8_t *)prop_data->data + (target & 0xffff) * 0x138;
                    real max_time = 0.7f; // 0x3f4ccccd
                    real half_gravity;
                    real horizontal_speed;
                    real_vector3d leap;

                    if (!(radius > 0.12f)) {
                        radius = 0.12f;
                    }
                    if (projectile_solve_ballistic_arc((real_point3d *)(target_prop + 0xc8),
                            (real_point3d *)((uint8_t *)component + 0x4), radius, 1.0f, &max_time, 0,
                            &leap, 0, 0, 0, 0, &half_gravity, &horizontal_speed)) {
                        float x, y, sum_sq;

                        if (vector2d_normalize_with_length((real_vector2d *)&leap) == 0.0f) {
                            leap = *(real_vector3d *)&((struct actor *)self)->facing.i;
                            if (vector2d_normalize_with_length((real_vector2d *)&leap) == 0.0f) {
                                leap = *global_forward3d_pointer;
                            }
                        }
                        if (target_prop[0x130] == 0 && !(half_gravity <= 0.075f)) {
                            half_gravity = 0.075f;
                        }
                        x = leap.i * horizontal_speed;
                        y = leap.j * horizontal_speed;
                        out_offset[2] = half_gravity;
                        out_offset[0] = x;
                        out_offset[1] = y;
                        sum_sq = y * y + x * x + half_gravity * half_gravity;
                        if (!(sum_sq <= radius * radius)) {
                            float k = radius / (float)sqrt((double)sum_sq);
                            out_offset[0] = x * k;
                            out_offset[1] = y * k;
                            out_offset[2] = half_gravity * k;
                        }
                    }
                }
            } // matches types/ai.h's swarm.unit_index[] uniqueness assumption; the original
              // loop has no early exit here and keeps scanning (harmlessly, since no other
              // slot can match).
        }
    }
}

#if 0
Original Ghidra decompilation (0x425c70):

void FUN_00425c70(float param_1,uint param_2,float param_3,float *param_4)

{
  float fVar1;
  float fVar2;
  ushort uVar3;
  int iVar4;
  uint uVar5;
  float fVar6;
  char cVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  float local_24;
  int local_20;
  undefined4 local_1c;
  int local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  undefined4 local_4;

  local_18 = ((uint)param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(uint *)(local_18 + 0x28) != 0xffffffff) {
    iVar8 = (*(uint *)(local_18 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    local_20 = 0;
    if (0 < *(short *)(iVar8 + 2)) {
      do {
        if (*(uint *)(iVar8 + 0x18 + (short)local_20 * 4) == param_2) {
          iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
          iVar9 = (*(uint *)(iVar8 + 0x58 + (short)local_20 * 4) & 0xffff) * 0x40 +
                  *(int *)(DAT_00880358 + 0x34);
          uVar3 = *(ushort *)(iVar9 + 2);
          if (((uVar3 & 1) == 0) || (uVar5 = *(uint *)(iVar9 + 0x14), uVar5 == 0xffffffff)) {
            if (((uVar3 & 8) != 0) && ((uVar3 & 0x10) != 0)) {
              if (((*(byte *)(iVar9 + 0x21) & 4) != 0) && ((*(byte *)(iVar9 + 0x21) & 0x10) != 0)) {
                local_14 = *(float *)(iVar4 + 0x74);
                local_10 = *(float *)(iVar4 + 0x78);
                fVar10 = (float10)vector2d_normalize_with_length();
                if ((float10)0.0 == fVar10) {
                  local_10 = *(float *)(iVar4 + 0x84);
                  local_14 = *(float *)(iVar4 + 0x80);
                  fVar10 = (float10)vector2d_normalize_with_length();
                  if ((float10)0.0 == fVar10) {
                    local_14 = *(float *)PTR_DAT_006966e8;
                    local_10 = *(float *)(PTR_DAT_006966e8 + 4);
                  }
                }
                fVar1 = *(float *)(iVar9 + 0x2c);
                fVar2 = *(float *)(iVar9 + 0x28);
                *param_4 = local_14 * *(float *)(iVar9 + 0x28);
                param_4[1] = local_10 * fVar2;
                param_4[2] = fVar1;
              }
              *(byte *)(iVar9 + 2) = *(byte *)(iVar9 + 2) & 0xef;
            }
          }
          else {
            iVar4 = *(int *)(DAT_008802c0 + 0x34);
            local_1c = 0x3f4ccccd;
            if (param_3 <= 0.12) {
              param_3 = 0.12;
            }
            cVar7 = FUN_004beb30(param_3,0x3f800000,&local_1c,0,0,0,0,&param_1,&local_24);
            if (cVar7 != '\0') {
              fVar10 = (float10)vector2d_normalize_with_length();
              if ((float10)0.0 == fVar10) {
                local_c = *(float *)(local_18 + 0x174);
                local_8 = *(float *)(local_18 + 0x178);
                local_4 = *(undefined4 *)(local_18 + 0x17c);
                fVar10 = (float10)vector2d_normalize_with_length();
                if ((float10)0.0 == fVar10) {
                  local_c = *(float *)PTR_DAT_00696718;
                  local_8 = *(float *)(PTR_DAT_00696718 + 4);
                  local_4 = *(undefined4 *)(PTR_DAT_00696718 + 8);
                }
              }
              if ((*(char *)((uVar5 & 0xffff) * 0x138 + iVar4 + 0x130) == '\0') && (0.075 < param_1)
                 ) {
                param_1 = 0.075;
              }
              fVar1 = local_c * local_24;
              param_4[2] = param_1;
              *param_4 = fVar1;
              fVar6 = local_8 * local_24;
              param_4[1] = fVar6;
              fVar2 = param_1 * param_1 + fVar1 * fVar1 + fVar6 * fVar6;
              if (param_3 * param_3 < fVar2) {
                fVar2 = param_3 / SQRT(fVar2);
                *param_4 = fVar1 * fVar2;
                param_4[1] = fVar6 * fVar2;
                param_4[2] = fVar2 * param_1;
              }
            }
          }
        }
        local_20 = local_20 + 1;
      } while ((short)local_20 < *(short *)(iVar8 + 2));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
