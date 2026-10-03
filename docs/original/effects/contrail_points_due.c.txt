// contrail_points_due  (Ghidra: FUN_0044cf80; named per out/phase4/effects_types_notes.md, which
// refers to this address by this name directly: "generation_timer ... reloaded with
// 1 / Contrail.point_generation_rate (+0x04, scale flag bit 0)")
// address 0x44cf80, size 155 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/tags.h Contrail.point_generation_rate (0x04) and ContrailScaleFlags bit 0;
// types/effects.h contrail.generation_timer (0x20).
// register convention: contrail handle in EAX (in_EAX, a datum_index, not a pointer -- the
// function computes contrail_data->data + (handle & 0xffff) * sizeof(contrail) itself), elapsed
// time on the stack (Ghidra's param_1).
//   // blam-cc: EAX -> contrail_handle, stack -> elapsed_time
// UNSURE: Ghidra folds the return into CONCAT22(garbage, count) where the high 16 bits are the
// upper half of a stray float bit pattern left in EDX by the loop and the low 16 bits are the
// real point count; every caller in this batch assigns the result to a 16-bit local, discarding
// the high half, so it is dropped here rather than reproduced.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *contrail_data;   // 0x0087abec
extern tag_instance *tag_instances; // 0x0087bc14

// Advances contrail->generation_timer by `elapsed_time` and returns how many new points are due,
// reloading the timer with 1 / point_generation_rate (optionally scaled by contrail->scale) for
// each one consumed.
int16_t contrail_points_due(datum_index contrail_handle, real elapsed_time)
{
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
    real rate = tag->point_generation_rate;
    int16_t count = 0;

    if ((tag->scale_flags & 1) != 0) {
        rate = rate * self->scale;
    }

    while (self->generation_timer <= elapsed_time) {
        real timer = self->generation_timer;
        self->generation_timer = 1.0f / rate;
        count = count + 1;
        elapsed_time = elapsed_time - timer;
    }
    self->generation_timer = self->generation_timer - elapsed_time;

    return count;
}

#if 0
Original Ghidra decompilation (0x44cf80):

/* WARNING: Removing unreachable block (ram,0x0044d005) */

undefined4 FUN_0044cf80(float param_1)

{
  float fVar1;
  float fVar2;
  uint in_EAX;
  int iVar3;
  short sVar4;
  
  iVar3 = (in_EAX & 0xffff) * 0x44 + *(int *)(DAT_0087abec + 0x34);
  fVar2 = *(float *)((*(uint *)(iVar3 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  fVar1 = *(float *)((int)fVar2 + 4);
  sVar4 = 0;
  if ((*(byte *)((int)fVar2 + 2) & 1) != 0) {
    fVar1 = fVar1 * *(float *)(iVar3 + 0x10);
  }
  while (*(float *)(iVar3 + 0x20) <= param_1) {
    fVar2 = *(float *)(iVar3 + 0x20);
    *(float *)(iVar3 + 0x20) = 1.0 / fVar1;
    sVar4 = sVar4 + 1;
    param_1 = param_1 - fVar2;
    fVar2 = 1.0 / fVar1;
  }
  *(float *)(iVar3 + 0x20) = *(float *)(iVar3 + 0x20) - param_1;
  return CONCAT22((short)((uint)fVar2 >> 0x10),sVar4);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
