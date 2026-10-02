// hs_object_set_health_fraction  (Ghidra: FUN_00488600)
// address 0x488600, size 98 bytes
// name confidence: 0.4 (out/phase4/hs_functions.md: "Sets an object field to a fraction
//   (0.0-1.0, clamped) of a corresponding maximum field, consistent with a health or shield
//   percentage setter")
// rewrite confidence: 0.65
// evidence: out/phase4/hs_types_notes.md object field offsets, 0xdc maximum_health / 0xe4
//   current_health.
// register convention: object index in EAX (in_EAX); fraction as the recognized stack parameter
//   (param_1).
//   // blam-cc: EAX -> object_index, stack -> fraction

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0, stride 0x0c, object data pointer at +0x08

// hs_object_record: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// Sets an object's current_health to `fraction` (clamped to [0, 1]) of its maximum_health. A
// negative fraction is treated as 0 via `maximum_health * 0.0` rather than a literal 0, matching
// the decompiled multiply exactly (so a non-finite maximum_health still propagates as NaN).
void hs_object_set_health_fraction(datum_index object_index, float fraction)
{
    hs_object_record *object;

    if (object_index != k_datum_index_none) {
        object = *(hs_object_record **)((uint8_t *)object_data->data +
            (object_index & 0xffff) * 0x0c + 8);
        if (fraction < 0.0f) {
            object->current_health = object->maximum_health * 0.0f;
            return;
        }
        if (1.0f < fraction) {
            fraction = 1.0f;
        }
        object->current_health = fraction * object->maximum_health;
    }
}

#if 0
Original Ghidra decompilation (0x488600):

void FUN_00488600(float param_1)

{
  int iVar1;
  uint in_EAX;

  if (in_EAX != 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (param_1 < 0.0) {
      *(float *)(iVar1 + 0xe4) = *(float *)(iVar1 + 0xdc) * 0.0;
      return;
    }
    if (1.0 < param_1) {
      param_1 = 1.0;
    }
    *(float *)(iVar1 + 0xe4) = param_1 * *(float *)(iVar1 + 0xdc);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
