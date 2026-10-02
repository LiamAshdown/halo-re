// particles_update  (Ghidra: FUN_00455b60, still unnamed there; named from its own summary in
//   out/phase4/effects_functions.md: "Per-tick update for every active particle: ages, animates,
//   or deletes each one", matching types/effects.h's own citation "particles_update 0x455b60
//   (last_update_tick, age)")
// address 0x455b60, size 272 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (VERIFIED against objdump 0x455b60..0x455c7d)
// evidence: types/effects.h particle.last_update_tick (+0x10, "a particle more than 0x10 ticks
//   stale is deleted instead of updated"), age (+0x14), lifespan (+0x18); types/tags.h
//   Particle.final_sequence_count.
// register convention: __cdecl, delta_time on the stack.
// UNSURE: the tail of the original decompile inlines datum_next 0x4d0630's own body instead of
//   calling it, exactly as src/effects/contrail_update.c documents for the same pattern; this
//   rewrite calls datum_next directly since it is semantically identical.

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
extern int32_t render_frame_index; // 0x007c3100, UNSURE: foreign module (render globals)

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module
extern uint8_t particle_advance_animation(datum_index particle_handle, real delta_time); // 0x4560c0,
                                    // this module
extern uint8_t particle_update_motion(datum_index particle_handle, real delta_time); // 0x4561a0,
                                    // this module, UNSURE signature -- see that file
extern void particle_impact(datum_index particle_handle); // 0x456550, this module

// Per-tick driver: ages every live particle, advances its animation and motion while it still
// has time (or life) left, triggers its impact response once its lifespan (and any final
// sequence) has run out, and deletes particles that have gone stale (not touched for 16 ticks).
void particles_update(real delta_time)
{
    datum_index particle_index = datum_next(-1, particle_data);

    while (particle_index != k_datum_index_none) {
        particle *self = &((particle *)particle_data->data)[(uint16_t)particle_index];
        real age_before = self->age;
        Particle *tag = (Particle *)tag_instances[(uint16_t)self->definition_index].data;

        if (render_frame_index - self->last_update_tick < 0x10) {
            self->age = delta_time + self->age;

            if (self->age < self->lifespan || age_before == 0.0f || tag->final_sequence_count != 0) {
                if (particle_advance_animation(particle_index, delta_time) != 0) {
                    particle_update_motion(particle_index, delta_time);
                }
            } else {
                particle_impact(particle_index);
            }
        } else {
            datum_delete(particle_data, particle_index);
        }

        particle_index = datum_next((int16_t)particle_index, particle_data);
    }
}

#if 0
Original Ghidra decompilation (0x455b60):

void FUN_00455b60(float param_1)

{
  float fVar1;
  float fVar2;
  char cVar3;
  uint uVar4;
  short *psVar5;
  short sVar6;
  int iVar7;
  int iVar8;
  int iVar9;

  iVar9 = DAT_0087abd0;
  uVar4 = datum_next();
  do {
    do {
      if (uVar4 == 0xffffffff) {
        return;
      }
      iVar7 = (uVar4 & 0xffff) * 0x70;
      fVar1 = *(float *)(iVar7 + 0x14 + *(int *)(iVar9 + 0x34));
      iVar7 = iVar7 + *(int *)(iVar9 + 0x34);
      iVar8 = *(int *)((*(uint *)(iVar7 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (DAT_007c3100 - *(int *)(iVar7 + 0x10) < 0x10) {
        fVar2 = param_1 + *(float *)(iVar7 + 0x14);
        *(float *)(iVar7 + 0x14) = fVar2;
        if (((fVar2 < *(float *)(iVar7 + 0x18)) || (fVar1 == 0.0)) ||
           (*(short *)(iVar8 + 0x9e) != 0)) {
          cVar3 = FUN_004560c0(param_1);
          iVar9 = DAT_0087abd0;
          if (cVar3 != '\0') {
            FUN_004561a0(uVar4,param_1);
            iVar9 = DAT_0087abd0;
          }
        }
        else {
          FUN_00456550();
          iVar9 = DAT_0087abd0;
        }
      }
      else {
        datum_delete();
      }
      iVar8 = uVar4 + 1;
      uVar4 = 0xffffffff;
      sVar6 = (short)iVar8;
    } while ((sVar6 < 0) || (*(short *)(iVar9 + 0x2e) <= sVar6));
    psVar5 = (short *)((int)sVar6 * (int)*(short *)(iVar9 + 0x22) + *(int *)(iVar9 + 0x34));
    do {
      if (*psVar5 != 0) {
        uVar4 = (int)*psVar5 << 0x10 | (int)(short)iVar8;
        break;
      }
      iVar8 = iVar8 + 1;
      psVar5 = (short *)((int)psVar5 + (int)*(short *)(iVar9 + 0x22));
    } while ((short)iVar8 < *(short *)(iVar9 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
