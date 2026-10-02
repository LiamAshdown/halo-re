// glow_particle_advance_time
// address 0x4fd650, size 469 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fd650 |
//   lightning_segment_advance_time | glow_particle_advance_time")
// rewrite confidence: 0.35
// evidence: types/objects.h glow (definition_tag 0x224, total_length 0x234), glow_particle
//   (flags 0x54 bit 0, t 0x28); object (function_out_values 0x134, function_valid_flags 0x123);
//   global 0x008603b0 object_data; this module's glow_particle_reposition 0x4fde40.
// register convention: Ghidra shows a clean `float param_1` (the raw rate to add/subtract) plus
//   three unresolved implicit inputs -- `in_EAX` (object index, read the same way
//   glow_particle_compute_position.c's in_ECX is), `in_EDX` (the glow entry, +0x224 definition
//   tag matches every sibling function in this group) and `unaff_ESI` (the particle, +0x28/+0x54
//   match every other particle field access in this group).
// blam-cc: EAX -> object_index, EDX -> entry, ESI -> particle, stack -> rate
// UNSURE: the loop-mode value at Glow tag+0x22 (0 = clamp-and-flip direction, 1 = pin at the
//   endpoint after one wrap, else = free wrap) is preserved as a raw offset; not matched to
//   GlowNormalParticleDistribution_t by name.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *object_data;     // 0x008603b0

extern void glow_particle_reposition(glow *entry, uint8_t *particle, float phase_rate); // 0x4fde40

void glow_particle_advance_time(uint32_t object_index /*EAX*/, glow *entry /*EDX*/,
                                 uint8_t *particle /*ESI*/, float rate /*stack*/)
    // blam-cc: EAX -> object_index, EDX -> entry, ESI -> particle, stack -> rate
{
    uint8_t *tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;
    int16_t attachment = *(int16_t *)(tag + 0x80);
    int16_t loop_mode = *(int16_t *)(tag + 0x22);
    uint32_t flags = *(uint32_t *)(particle + 0x54);
    float *t = (float *)(particle + 0x28);

    if (attachment != -1) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        float driver = *(float *)((uint8_t *)obj + 0x134 + attachment * 4);

        if (((1 << (attachment & 0x1f)) & *((uint8_t *)obj + 0x123)) == 0) {
            driver = 0.0f;
        }
        *(float *)(particle + 0x1c) =
            (*(float *)(tag + 0x88) - *(float *)(tag + 0x84)) *
            ((*(float *)(tag + 0x90) - *(float *)(tag + 0x8c)) * driver + *(float *)(tag + 0x8c)) +
            *(float *)(tag + 0x84);
    }

    if ((flags & 1) == 0) {
        // Advancing forward.
        rate = rate + *t;
        *t = rate;

        if (loop_mode == 0) {
            if (entry->total_length < rate) {
                do {
                    *t = *t - entry->total_length;
                } while (entry->total_length < *t);
                *(uint32_t *)(particle + 0x54) = flags | 1; // flip direction (bounce)
                *t = entry->total_length - *t;
                glow_particle_reposition(entry, particle, rate);
                return;
            }
        } else if (loop_mode == 1 && entry->total_length < rate) {
            do {
                *t = *t - entry->total_length;
            } while (entry->total_length < *t);
            glow_particle_reposition(entry, particle, rate);
            return;
        }
    } else {
        // Advancing backward.
        rate = *t - rate;
        *t = rate;

        if (loop_mode == 0) {
            if (rate < 0.0f) {
                float wrapped;
                do {
                    wrapped = entry->total_length + *t;
                    *t = wrapped;
                } while (*t < 0.0f);
                *(uint32_t *)(particle + 0x54) = flags & ~1U; // flip direction back to forward
                *t = entry->total_length - wrapped;
            }
        } else if (loop_mode == 1 && rate < 0.0f) {
            do {
                *t = entry->total_length + *t;
            } while (*t < 0.0f);
            glow_particle_reposition(entry, particle, rate);
            return;
        }
    }

    glow_particle_reposition(entry, particle, rate);
}

#if 0
Original Ghidra decompilation (0x4fd650):

void FUN_004fd650(float param_1)

{
  float fVar1;
  float fVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  uint in_EAX;
  int in_EDX;
  int unaff_ESI;

  iVar4 = *(int *)((*(uint *)(in_EDX + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar3 = *(short *)(iVar4 + 0x80);
  if (sVar3 != -1) {
    iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    fVar1 = *(float *)(iVar5 + 0x134 + sVar3 * 4);
    if (((byte)(1 << ((byte)sVar3 & 0x1f)) & *(byte *)(iVar5 + 0x123)) == 0) {
      fVar1 = 0.0;
    }
    *(float *)(unaff_ESI + 0x1c) =
         (*(float *)(iVar4 + 0x88) - *(float *)(iVar4 + 0x84)) *
         ((*(float *)(iVar4 + 0x90) - *(float *)(iVar4 + 0x8c)) * fVar1 + *(float *)(iVar4 + 0x8c))
         + *(float *)(iVar4 + 0x84);
  }
  uVar6 = *(uint *)(unaff_ESI + 0x54);
  if ((uVar6 & 1) == 0) {
    param_1 = param_1 + *(float *)(unaff_ESI + 0x28);
    *(float *)(unaff_ESI + 0x28) = param_1;
    if (*(short *)(iVar4 + 0x22) == 0) {
      if (*(float *)(in_EDX + 0x234) < param_1) {
        if (*(float *)(in_EDX + 0x234) < param_1) {
          do {
            *(float *)(unaff_ESI + 0x28) = *(float *)(unaff_ESI + 0x28) - *(float *)(in_EDX + 0x234)
            ;
          } while (*(float *)(in_EDX + 0x234) < *(float *)(unaff_ESI + 0x28));
        }
        fVar1 = *(float *)(in_EDX + 0x234);
        *(uint *)(unaff_ESI + 0x54) = uVar6 | 1;
        *(float *)(unaff_ESI + 0x28) = fVar1 - *(float *)(unaff_ESI + 0x28);
        FUN_004fde40();
        return;
      }
    }
    else if ((*(short *)(iVar4 + 0x22) == 1) && (*(float *)(in_EDX + 0x234) < param_1)) {
      do {
        *(float *)(unaff_ESI + 0x28) = *(float *)(unaff_ESI + 0x28) - *(float *)(in_EDX + 0x234);
      } while (*(float *)(in_EDX + 0x234) < *(float *)(unaff_ESI + 0x28));
      FUN_004fde40();
      return;
    }
  }
  else {
    param_1 = *(float *)(unaff_ESI + 0x28) - param_1;
    *(float *)(unaff_ESI + 0x28) = param_1;
    if (*(short *)(iVar4 + 0x22) == 0) {
      if (param_1 < 0.0) {
        do {
          fVar1 = *(float *)(in_EDX + 0x234) + *(float *)(unaff_ESI + 0x28);
          *(float *)(unaff_ESI + 0x28) = fVar1;
        } while (*(float *)(unaff_ESI + 0x28) < 0.0);
        fVar2 = *(float *)(in_EDX + 0x234);
        *(uint *)(unaff_ESI + 0x54) = uVar6 & 0xfffffffe;
        *(float *)(unaff_ESI + 0x28) = fVar2 - fVar1;
      }
    }
    else if ((*(short *)(iVar4 + 0x22) == 1) && (param_1 < 0.0)) {
      do {
        *(float *)(unaff_ESI + 0x28) = *(float *)(in_EDX + 0x234) + *(float *)(unaff_ESI + 0x28);
      } while (*(float *)(unaff_ESI + 0x28) < 0.0);
      FUN_004fde40();
      return;
    }
  }
  FUN_004fde40();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
