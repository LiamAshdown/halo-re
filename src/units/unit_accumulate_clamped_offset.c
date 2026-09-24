// unit_accumulate_clamped_offset  (Ghidra: FUN_00570400; renamed from the phase2 proposal)
// address 0x570400, size 89 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.6
// evidence: types/units.h unit_data.animation_blend_weight (0x2e8, "unit_update decays it
//   toward 0 each tick").
// register convention: unit object index in EAX (in_EAX); the new value in a stack parameter.
//   // blam-cc: EAX -> object_index, stack -> new_value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

// Applies a rate-limited (max 0.3 change per call) update to unit_data.animation_blend_weight.
void unit_accumulate_clamped_offset(uint32_t object_index, float new_value)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    float delta = new_value - unit->animation_blend_weight;

    if (delta < -0.3f) {
        unit->animation_blend_weight -= 0.3f;
        return;
    }
    if (delta > 0.3f) {
        delta = 0.3f;
    }
    unit->animation_blend_weight += delta;
}

#if 0
Original Ghidra decompilation (0x570400):

void FUN_00570400(float param_1)

{
  int iVar1;
  uint in_EAX;
  float *pfVar2;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  param_1 = param_1 - *(float *)(iVar1 + 0x2e8);
  pfVar2 = (float *)(iVar1 + 0x2e8);
  if (param_1 < -0.3) {
    *pfVar2 = *pfVar2 + -0.3;
    return;
  }
  if (0.3 < param_1) {
    param_1 = 0.3;
  }
  *pfVar2 = param_1 + *pfVar2;
  return;
}
#endif
