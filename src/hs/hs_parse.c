// hs_parse  (Ghidra: hs_parse, already named)
// address 0x486420, size 83 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: writes the expected type into an unparsed node then dispatches to
// hs_parse_primitive or hs_parse_nonprimitive depending on the primitive flag, matching
// hs.h's description of hs_syntax_node::type.
// register convention: __cdecl, both parameters recognized directly by Ghidra.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


extern data_array *hs_syntax_data; // 0x0087a474

// Recursively parses/type-checks a single syntax node against `expected_type`, dispatching to
// the primitive or nonprimitive parser the first time it is visited (node->type == 0); returns
// 1 without reparsing if the node was already typed.
char hs_parse(datum_index node_index, hs_type_t expected_type)
{
    data_array *nodes;
    hs_syntax_node *node;
    char result;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    result = 1;
    if (node->type == 0) {
        node->type = expected_type;
        if ((node->flags & _hs_syntax_node_primitive_bit) != 0) {
            node->index_union = expected_type;
            result = hs_parse_primitive(node_index);
            return result;
        }
        result = hs_parse_nonprimitive(node_index);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x486420):

undefined2 hs_parse(uint param_1,undefined2 param_2)

{
  int iVar1;
  undefined2 uVar2;
  int iVar3;
  int iVar4;

  iVar1 = DAT_0087a474;
  iVar4 = (param_1 & 0xffff) * 0x14;
  iVar3 = *(int *)(DAT_0087a474 + 0x34) + iVar4;
  uVar2 = 1;
  if (*(short *)(iVar3 + 4) == 0) {
    *(undefined2 *)(iVar3 + 4) = param_2;
    if ((*(byte *)(*(int *)(iVar1 + 0x34) + 6 + iVar4) & 1) != 0) {
      *(undefined2 *)(iVar3 + 2) = param_2;
      uVar2 = hs_parse_primitive();
      return uVar2;
    }
    uVar2 = hs_parse_nonprimitive(param_1);
  }
  return uVar2;
}
#endif
