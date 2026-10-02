// weather_instance_adjust_count  (Ghidra: FUN_00457fc0, still unnamed there; named directly by
//   types/effects.h: "weather_instance_adjust_count 0x457fc0 grows or shrinks the list toward
//   the target and weather_instance_update 0x458420 walks it")
// address 0x457fc0, size 174 bytes
// name confidence: 0.6   rewrite confidence: 0.35 (see UNSURE)
// evidence: types/effects.h weather_instance_type (particle_count +0x08, first_particle +0x0c),
//   weather_particle.next_particle (+0x50); weather_instance_update 0x458420's call site.
// register convention: particle type index in AX (in_AX); weather instance index and a target
//   count are Ghidra's own recognized stack parameter (param_1) plus a second float Ghidra never
//   resolves into a named parameter at all (consumed directly off the FPU stack by the leading
//   __ftol()), matching how weather_instance_update 0x458420 computes and pushes exactly one
//   more float right before this call.
//   // blam-cc: in_AX -> type_index, stack -> (instance_index, target)
// UNSURE: weather_instance_update's own visible expression for this second argument is
//   `(1 - fade_out) * fade_in * instance.intensity`, with no visible multiplication by
//   weather_instance_type.target_count at all -- yet this function never reads target_count
//   either, and types/effects.h documents intensity as scaling "the per type target count",
//   which only makes sense if target_count is folded in somewhere. Modeled here as the caller
//   multiplying by target_count as well (a plausible reading of the struct comment, not
//   something visible in either function's own decompiled body).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern weather_instance weather_instances[1]; // 0x006b0ae4
extern data_array *weather_particle_data;     // 0x0087abcc

extern datum_index weather_particle_new(int16_t instance_index, int16_t type_index); // 0x458070,
                                    // this module
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module

// Grows or shrinks one weather instance particle type slot's live particle count toward `target`
// (see file header for how the caller computes it): creates particles (stopping early if the
// pool is exhausted) while short, or deletes particles from the head of the list while over.
void weather_instance_adjust_count(int16_t instance_index, int16_t type_index, real target_value)
{
    weather_instance_type *slot = &weather_instances[instance_index].types[type_index];
    int32_t target = (int32_t)target_value;

    if (target < 0) {
        target = 0;
    }

    while (slot->particle_count < target) {
        if (weather_particle_new(instance_index, type_index) == (datum_index)0xffffffff) {
            break;
        }
    }

    while (target < slot->particle_count) {
        weather_particle *p =
            &((weather_particle *)weather_particle_data->data)[(uint16_t)slot->first_particle];
        datum_index next = p->next_particle;

        datum_delete(weather_particle_data, slot->first_particle);
        slot->particle_count -= 1;
        slot->first_particle = next;
    }
}

#if 0
Original Ghidra decompilation (0x457fc0):

void FUN_00457fc0(undefined4 param_1)

{
  short *psVar1;
  short sVar2;
  undefined4 uVar3;
  int iVar4;
  short in_AX;
  uint uVar5;
  int iVar6;

  iVar4 = in_AX * 0x10 + (short)param_1 * 0x9c;
  uVar5 = __ftol();
  uVar5 = ((int)uVar5 < 0) - 1 & uVar5;
  sVar2 = *(short *)(&DAT_006b0b00 + iVar4 + 8);
  while (((int)sVar2 < (int)uVar5 && (iVar6 = FUN_00458070(param_1), iVar6 != -1))) {
    sVar2 = *(short *)(&DAT_006b0b00 + iVar4 + 8);
  }
  sVar2 = *(short *)(&DAT_006b0b00 + iVar4 + 8);
  iVar6 = DAT_0087abcc;
  while ((int)uVar5 < (int)sVar2) {
    uVar3 = *(undefined4 *)
             ((*(uint *)(&DAT_006b0b00 + iVar4 + 0xc) & 0xffff) * 0x54 + 0x50 +
             *(int *)(iVar6 + 0x34));
    iVar6 = datum_delete();
    psVar1 = (short *)(&DAT_006b0b00 + iVar4 + 8);
    *psVar1 = *psVar1 + -1;
    *(undefined4 *)(&DAT_006b0b00 + iVar4 + 0xc) = uVar3;
    sVar2 = *(short *)(&DAT_006b0b00 + iVar4 + 8);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
