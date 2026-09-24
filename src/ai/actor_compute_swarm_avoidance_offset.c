// actor_compute_swarm_avoidance_offset  (Ghidra: actor_compute_swarm_avoidance_offset, renamed)
// address 0x425c70, size 755 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: phase-4 summary "Computes a per-swarm-member avoidance/spacing offset vector,
// either mirroring the member's own velocity or invoking a steering helper, clamped to a
// maximum radius." No static callers are recorded by Ghidra (out/functions.json: callers=0).
// types/ai.h swarm.unit_index[16]/component_index[16] (0x18/0x58), swarm_component.
// unknown_14 (0x14, "swarm_add_component sets -1"); actor.swarm_index (0x28), actor.facing
// (0x174). Calls vector2d_normalize_with_length (0x4018e0, math module) and FUN_004beb30
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

extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *object_data;          // 0x008603b0
extern data_array *prop_data;            // 0x008802c0
extern const real_vector2d *global_forward2d_pointer; // 0x006966e8, UNSURE name (mirrors
                                                       // global_forward3d_pointer @0x696718)

extern double sqrt(double x); // FSQRT
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern uint8_t FUN_004beb30(float duration, float target, float *curve, int32_t a, int32_t b,
                            int32_t c, int32_t d, float *in_value, float *out_value); // 0x4beb30, UNSURE signature

// blam-cc: stack -> actor_index, unit_index, radius, out_offset
// For the swarm member matching `unit_index`: if its swarm_component has a still-valid
// "target" reference (unknown_14) with certain flag bits set, mirrors that target's forward
// direction (falling back to the global forward vector when degenerate) scaled by its own
// stored 2D direction/scale into `out_offset`, and clears one of those flag bits. Otherwise
// runs a steering curve (FUN_004beb30) against the actor's own facing direction (falling back
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
                uint16_t flags = *(uint16_t *)((uint8_t *)component + 2); // UNSURE offset
                datum_index target = component->unknown_14;

                if ((flags & 1) == 0 || target == (datum_index)k_datum_index_none) {
                    if ((flags & 8) != 0 && (flags & 0x10) != 0) {
                        uint8_t byte21 = *((uint8_t *)component + 0x21); // UNSURE offset
                        if ((byte21 & 4) != 0 && (byte21 & 0x10) != 0) {
                            real_vector2d dir;
                            dir.i = unit_object->forward.i;
                            dir.j = unit_object->forward.j;
                            if (vector2d_normalize_with_length(&dir) == 0.0f) {
                                dir.i = *(float *)((uint8_t *)unit_object + 0x80);
                                dir.j = *(float *)((uint8_t *)unit_object + 0x84);
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
                    prop *target_prop = &((prop *)prop_data->data)[target & 0xffff];
                    float curve[1] = { 0.7f }; // 0x3f4ccccd
                    float clamp_radius = radius;
                    float curve_out;

                    if (clamp_radius <= 0.12f) {
                        clamp_radius = 0.12f;
                    }
                    if (FUN_004beb30(clamp_radius, 1.0f, curve, 0, 0, 0, 0, &radius, &curve_out) != 0) {
                        real_vector2d dir;
                        float z;

                        dir.i = self->facing.i;
                        dir.j = self->facing.j;
                        z = self->facing.k;
                        if (vector2d_normalize_with_length(&dir) == 0.0f) {
                            dir.i = global_forward2d_pointer->i;
                            dir.j = global_forward2d_pointer->j;
                            z = *(float *)((const uint8_t *)global_forward2d_pointer + 8);
                        }

                        if (target_prop->unknown_130 == 0 && radius > 0.075f) { // UNSURE offset
                            radius = 0.075f;
                        }

                        {
                            float x = dir.i * curve_out;
                            float y = dir.j * curve_out;
                            float sum_sq = radius * radius + x * x + y * y;

                            out_offset[2] = radius;
                            out_offset[0] = x;
                            out_offset[1] = y;

                            if (clamp_radius * clamp_radius < sum_sq) {
                                float k = clamp_radius / (float)sqrt((double)sum_sq);
                                out_offset[0] = x * k;
                                out_offset[1] = y * k;
                                out_offset[2] = k * radius;
                            }
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
