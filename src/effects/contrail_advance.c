// contrail_advance  (Ghidra: FUN_0044ca60; named per out/phase4/effects_types_notes.md, which
// refers to this address as contrail_advance directly: "contrail_advance 0x44ca60 sets
// scale_function_index" / "contrail_advance adds accumulated_delta_time")
// address 0x44ca60, size 101 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: types/effects.h contrail.flags (_contrail_emitting_bit), object_index (detach to -1)
// and accumulated_delta_time (0x28).
// register convention: contrail handle in EDI (unaff_EDI); detach flag and delta time are the
// two stack arguments Ghidra already recognises (param_1, param_2).
//   // blam-cc: EDI -> contrail_handle, stack -> (detach, delta_time)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern data_array *contrail_data; // 0x0087abec

extern int16_t contrail_points_due(datum_index contrail_handle, real elapsed_time); // 0x44cf80,
    // this module; blam-cc: EAX -> contrail_handle, stack -> elapsed_time
extern void contrail_generate_points(datum_index contrail_handle, int16_t point_count,
    uint8_t force); // 0x44d020, this module;
    // blam-cc: EAX -> contrail_handle, stack -> (point_count, force)

// Advances one contrail by `delta_time` seconds: generates any points now due, optionally
// detaches it from its owning object, and accumulates the elapsed time for contrail_update to
// consume.
void contrail_advance(datum_index contrail_handle, uint8_t detach, real delta_time)
{
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];

    if ((self->flags & _contrail_emitting_bit) != 0) {
        int16_t due = contrail_points_due(contrail_handle, delta_time);
        contrail_generate_points(contrail_handle, due < 1 ? 1 : due, 0);
    }

    if (detach != 0) {
        self->object_index = k_datum_index_none;
    }

    self->accumulated_delta_time = self->accumulated_delta_time + delta_time;
}

#if 0
Original Ghidra decompilation (0x44ca60):

void FUN_0044ca60(char param_1,float param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint unaff_EDI;
  
  iVar2 = (unaff_EDI & 0xffff) * 0x44;
  iVar3 = iVar2 + *(int *)(DAT_0087abec + 0x34);
  if ((*(byte *)(iVar2 + 2 + *(int *)(DAT_0087abec + 0x34)) & 1) != 0) {
    sVar1 = FUN_0044cf80(param_2);
    if (sVar1 < 1) {
      iVar2 = 1;
    }
    else {
      iVar2 = (int)sVar1;
    }
    contrail_generate_points(iVar2,0);
  }
  if (param_1 != '\0') {
    *(undefined4 *)(iVar3 + 8) = 0xffffffff;
  }
  *(float *)(iVar3 + 0x28) = param_2 + *(float *)(iVar3 + 0x28);
  return;
}
#endif
