// object_set_scale_and_refresh_nodes  (Ghidra: FUN_004f96a0; renamed, Blam-style, not
// previously named)
// address 0x4f96a0, size 70 bytes
// name confidence: 0.25 (matches functions.md's summary: "Stores a value on an object's
//   attachment node and, if certain status flags are unset, invokes a follow-up handler")
// rewrite confidence: 0.95 (zero recorded callers)
// evidence: types/objects.h object (scale 0x0b0, type 0x0b4, _object_mask_no_node_functions ==
//   0xfe0); global 0x008603b0 object_data; callee object_copy_default_node_transforms
//   (0x4f6b70, this batch).
// register convention: object index in EAX, new scale as the sole stack parameter. Consistent
//   with Ghidra's own "FUN_004f96a0(undefined4 param_1)" plus "in_EAX".
//   // blam-cc: EAX -> object_index, stack -> scale
// UNSURE: object_copy_default_node_transforms is called here with no visible second argument;
//   its requested_count parameter is guessed as 0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); // 0x4f6b70, this batch, UNSURE: see file header

// FIXED (objdump 0x4f96dc): a second stack argument (the script's tick count) is tail-passed in DX to 0x4f6b70;
//   the draft passed 0.
void object_set_scale_and_refresh_nodes(uint32_t object_index, float scale, int16_t ticks) // blam-cc: EAX -> object_index, stack -> scale, ticks
{
    if (object_index != k_datum_index_none) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        obj->scale = scale;
        if (((1u << (obj->type & 0x1f)) & _object_mask_no_node_functions) == 0) {
            object_copy_default_node_transforms(object_index, ticks);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f96a0):

void FUN_004f96a0(undefined4 param_1)

{
  int iVar1;
  uint in_EAX;

  if ((in_EAX != 0xffffffff) &&
     (iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc),
     *(undefined4 *)(iVar1 + 0xb0) = param_1, (1 << (*(byte *)(iVar1 + 0xb4) & 0x1f) & 0xfe0U) == 0)
     ) {
    FUN_004f6b70();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
