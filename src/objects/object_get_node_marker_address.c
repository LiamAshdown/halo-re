// object_get_node_marker_address
// address 0x4f6000, size 41 bytes
// name confidence: 0.85 (types/objects.h names and cites this exact address as
//   "object_get_node_marker_address" in the object.nodes field comment: "computes
//   obj + *(int16 *)(obj+0x1f2) + i*0x34")
// rewrite confidence: 0.8
// evidence: types/objects.h object_block_reference (offset at 0x02) and object.nodes (0x1f0,
//   node_count*0x34 bytes, real_matrix4x3 elements); global 0x008603b0 object_data.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"

extern data_array *object_data; // 0x008603b0

// FIXED (register inputs, objdump): the original never reads ECX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: EAX -> object_index, stack -> node_index
real_matrix4x3 *object_get_node_marker_address(uint32_t object_index, int16_t node_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    return (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + node_index * 0x34);
}

#if 0
Original Ghidra decompilation (0x4f6000):

int FUN_004f6000(short param_1)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  return param_1 * 0x34 + *(short *)(iVar1 + 0x1f2) + iVar1;
}
#endif
