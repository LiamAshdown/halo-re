// hs_get_parameter_indices  (Ghidra: hs_get_parameter_indices, already named)
// address 0x484fb0, size 161 bytes
// name confidence: 0.9   rewrite confidence: 0.75
// evidence: CEA-PDB string match ("the %s call requires %d arguments."); walks a call node's
// argument list (node->data.first_child, then each child's next_node) into an output array,
// erroring if the count doesn't exactly match required_count.
// register convention: function_name and required_count are recognized directly by Ghidra;
// node_index (in_ECX) and out_indices (unaff_EBX) are unrecognized, which by the blam-cc
// convention are the second and fourth register slots, ECX and EBX.

// FIXED (verified against the retail bytes): an evaluated call node's first child is the function-name
//   node; the original starts at its next_node ([first_child*0x14 + 8]), so arguments begin at the second
//   child. The draft started at the name node itself (scripts ran with misaligned arguments).
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>

extern data_array *hs_syntax_data;      // 0x0087a474
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc

// blam-cc: node index in ECX, out_indices array in EBX
// Collects call node `node_index`'s argument-node indices into out_indices[0..required_count),
// reporting an error if it does not have exactly required_count arguments.
char hs_get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index, datum_index *out_indices)
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index child;
    int16_t count;
    char success;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    child = ((hs_syntax_node *)((uint8_t *)nodes->data + (node->data.first_child & 0xffff) * nodes->size))->next_node;
    success = 1;
    for (count = 0; (child != k_datum_index_none) && (count < required_count); count = count + 1) {
        out_indices[count] = child;
        child = ((hs_syntax_node *)((uint8_t *)nodes->data + (child & 0xffff) * nodes->size))->next_node;
    }
    if ((count != required_count) || (child != k_datum_index_none)) {
        sprintf(hs_compile_error_buffer, "the %s call requires %d arguments.", function_name, (int)required_count);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = node->source_offset;
        success = 0;
    }
    return success;
}

#if 0
Original Ghidra decompilation (0x484fb0):

undefined4 hs_get_parameter_indices(undefined4 param_1,short param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  uint in_ECX;
  short sVar4;
  int unaff_EBX;
  int iVar5;

  iVar2 = DAT_0087a474;
  iVar5 = (in_ECX & 0xffff) * 0x14;
  uVar1 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 +
                   (*(uint *)(*(int *)(DAT_0087a474 + 0x34) + 0x10 + iVar5) & 0xffff) * 0x14);
  uVar3 = 1;
  for (sVar4 = 0; (uVar1 != 0xffffffff && (sVar4 < param_2)); sVar4 = sVar4 + 1) {
    *(uint *)(unaff_EBX + sVar4 * 4) = uVar1;
    uVar1 = *(uint *)(*(int *)(iVar2 + 0x34) + 8 + (uVar1 & 0xffff) * 0x14);
  }
  if ((sVar4 != param_2) || (uVar1 != 0xffffffff)) {
    _sprintf(&DAT_006b14dc,"the %s call requires %d arguments.",param_1,(int)param_2);
    DAT_006b14d4 = &DAT_006b14dc;
    DAT_006b14d8 = *(undefined4 *)(*(int *)(DAT_0087a474 + 0x34) + 0xc + iVar5);
    uVar3 = 0;
  }
  return uVar3;
}
#endif
