// object_function_get_value  (Ghidra: object_function_get_value, already named)
// address 0x4f6e70, size 70 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Returns a cached 'object function' output value and its validity
//   flag for the given function index")
// rewrite confidence: 0.75
// evidence: types/objects.h object (function_out_values 0x134, function_valid_flags 0x123,
//   object_function_input enum); global 0x008603b0 object_data.
// register convention: object index in EAX, function selector in CX, out-value pointer in EDX,
//   returns the validity flag in AL. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f6e7f cmp cx,0xffff and 0x4f6e92/0x4f6eb2 both write only AL before ret.
//   // blam-cc: EAX -> object_index, CX -> selector, EDX -> out_value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

uint8_t object_function_get_value(uint32_t object_index, int16_t selector, float *out_value)
    // blam-cc: EAX -> object_index, CX -> selector, EDX -> out_value
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (selector == -1) {
        *out_value = 1.0f;
        return 1;
    }

    *out_value = obj->function_out_values[selector];
    return (obj->function_valid_flags & (1 << (selector & 0x1f))) != 0;
}

#if 0
Original Ghidra decompilation (0x4f6e70):

undefined4 object_function_get_value(void)

{
  int iVar1;
  uint in_EAX;
  undefined3 uVar2;
  short in_CX;
  undefined4 *in_EDX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = (undefined3)((uint)iVar1 >> 8);
  if (in_CX == -1) {
    *in_EDX = 0x3f800000;
    return CONCAT31(uVar2,1);
  }
  *in_EDX = *(undefined4 *)(iVar1 + 0x134 + in_CX * 4);
  return CONCAT31(uVar2,((byte)(1 << ((byte)in_CX & 0x1f)) & *(byte *)(iVar1 + 0x123)) != 0);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
