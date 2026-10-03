// particle_advance_animation  (Ghidra: FUN_004560c0, still unnamed there; named directly by
//   types/effects.h: "particle_advance_animation 0x4560c0 (animation_timer against
//   inverse_animation_period)")
// address 0x4560c0, size 223 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (VERIFIED against objdump 0x4560c0..0x45619e)
// evidence: types/effects.h particle.animation_timer (+0x1c, "-1.0 at create so the first
//   update forces a frame advance") and inverse_animation_period (+0x20); ParticleFlags bit
//   layout comment in types/tags.h (bit 1 animation_stops_at_rest, bit 3
//   animate_once_per_frame -- there is no named C enum for this bitfield, so the raw masks are
//   used with a comment).
// register convention: particle handle in EDI (unaff_EDI); elapsed time as the recognized stack
//   parameter (param_1).
//   // blam-cc: unaff_EDI -> particle_handle, stack -> delta_time

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *particle_data;   // 0x0087abd0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t particle_advance_frame(datum_index particle_handle); // 0x456000, this module

// Consumes `delta_time` seconds of a particle's animation clock, calling particle_advance_frame
// once per whole animation period until the remaining time runs out. A tag flagged
// animate_once_per_frame instead advances exactly one frame per nonzero call, and a tag flagged
// animation_stops_at_rest with the particle currently at rest does not animate at all.
uint8_t particle_advance_animation(datum_index particle_handle, real delta_time)
{
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)tag_instances[(uint16_t)self->definition_index].data;
    uint8_t has_frame = 1;

    if ((tag->flags & 0x2) != 0 /* ParticleFlags::animation_stops_at_rest */ &&
        (self->flags & _particle_at_rest_bit) != 0) {
        return has_frame;
    }

    if ((tag->flags & 0x8) != 0 /* ParticleFlags::animate_once_per_frame */) {
        if (delta_time != 0.0f) {
            return particle_advance_frame(particle_handle);
        }
        return has_frame;
    }

    if (self->animation_timer == -1.0f) {
        has_frame = particle_advance_frame(particle_handle);
        self->animation_timer = 0.0f;
    }

    if (delta_time > 0.0f) {
        while (has_frame != 0) {
            real remaining = self->inverse_animation_period - self->animation_timer;

            if (remaining > delta_time) {
                self->animation_timer += delta_time;
                break;
            }

            has_frame = particle_advance_frame(particle_handle);
            delta_time -= remaining;
            if (delta_time <= 0.0f) {
                return has_frame;
            }
        }
    }

    return has_frame;
}

#if 0
Original Ghidra decompilation (0x4560c0):

undefined4 FUN_004560c0(float param_1)

{
  float fVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  char cVar5;
  uint uVar6;
  int iVar7;
  uint unaff_EDI;

  iVar7 = (unaff_EDI & 0xffff) * 0x70 + *(int *)(DAT_0087abd0 + 0x34);
  uVar3 = **(uint **)((*(uint *)(iVar7 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar6 = 1;
  cVar5 = '\x01';
  if (((uVar3 & 2) == 0) || ((*(byte *)(iVar7 + 2) & 2) == 0)) {
    if ((uVar3 & 8) == 0) {
      if (*(int *)(iVar7 + 0x1c) == -0x40800000) {
        uVar3 = FUN_00456000();
        uVar6 = uVar3 & 0xff;
        *(undefined4 *)(iVar7 + 0x1c) = 0;
      }
      cVar5 = (char)uVar6;
      uVar3 = CONCAT22((short)(uVar3 >> 0x10),
                       (ushort)(param_1 < 0.0) << 8 | (ushort)NAN(param_1) << 10 |
                       (ushort)(param_1 == 0.0) << 0xe);
      if (param_1 < 0.0 == 0 && (param_1 == 0.0) == 0) {
        while (cVar5 = (char)uVar6, cVar5 != '\0') {
          fVar1 = *(float *)(iVar7 + 0x20) - *(float *)(iVar7 + 0x1c);
          uVar3 = CONCAT22((short)(uVar3 >> 0x10),
                           (ushort)(fVar1 < param_1) << 8 |
                           (ushort)(NAN(fVar1) || NAN(param_1)) << 10 |
                           (ushort)(fVar1 == param_1) << 0xe);
          if (fVar1 < param_1 == (fVar1 == param_1)) {
            *(float *)(iVar7 + 0x1c) = param_1 + *(float *)(iVar7 + 0x1c);
            break;
          }
          uVar4 = FUN_00456000();
          param_1 = param_1 - fVar1;
          uVar6 = uVar4 & 0xff;
          uVar3 = CONCAT22((short)(uVar4 >> 0x10),
                           (ushort)(param_1 < 0.0) << 8 | (ushort)NAN(param_1) << 10 |
                           (ushort)(param_1 == 0.0) << 0xe);
          if (param_1 < 0.0 != 0 || (param_1 == 0.0) != 0) {
            return CONCAT31((int3)(uVar3 >> 8),(char)uVar4);
          }
        }
      }
    }
    else {
      uVar3 = CONCAT22((short)(uVar3 >> 0x10),
                       (ushort)(param_1 < 0.0) << 8 | (ushort)NAN(param_1) << 10 |
                       (ushort)(param_1 == 0.0) << 0xe);
      if (param_1 != 0.0) {
        uVar2 = FUN_00456000();
        return uVar2;
      }
    }
  }
  return CONCAT31((int3)(uVar3 >> 8),cVar5);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
