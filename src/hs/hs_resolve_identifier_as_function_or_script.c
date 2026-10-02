// hs_resolve_identifier_as_function_or_script  (Ghidra: hs_resolve_identifier_as_function_or_script, already named)
// address 0x486680, size 132 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: `node` is a call syntax node and node->data.first_child is the identifier
// (function-name) pseudo-node at the head of the call list, matching the same first_child/
// function-name relationship hs_compile_postprocess resolves; the first time an identifier
// node is seen (type != _hs_type_function_name) it is looked up as a builtin function, then as
// a user script if that fails, and the outcome is cached both on the call node's index_union
// and back onto the identifier node itself so repeat visits take the fast cached path.
// register convention: the node index is unrecognized by Ghidra (in_EAX); by the blam-cc
// convention this is the first register slot, EAX.
// UNSURE: the name text passed to hs_find_function_by_name is inferred (hs_compiled_source +
// identifier_node->source_offset, the same text hs_script_find_by_name is explicitly shown
// using) -- Ghidra's decompiled call itself shows zero visible arguments.

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t hs_find_function_by_name(char *name); // 0x00483520, this batch
extern int16_t hs_script_find_by_name(char *name); // 0x004833a0, this batch

extern data_array *hs_syntax_data; // 0x0087a474
extern char *hs_compiled_source;   // 0x006b14c0

// blam-cc: call node index in EAX
// Resolves the identifier at the head of call node `node_index` to either a builtin function
// index or a user-script index, caching the result on both the call node and the identifier
// node so a second visit skips the name lookup entirely.
void hs_resolve_identifier_as_function_or_script(datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    hs_syntax_node *identifier_node;
    int16_t function_index;
    int16_t script_index;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    identifier_node = (hs_syntax_node *)((uint8_t *)nodes->data + (node->data.first_child & 0xffff) * nodes->size);
    if (identifier_node->type != _hs_type_function_name) {
        function_index = hs_find_function_by_name(hs_compiled_source + identifier_node->source_offset);
        node->index_union = function_index;
        identifier_node->type = _hs_type_function_name;
        if (node->index_union == -1) {
            script_index = hs_script_find_by_name(hs_compiled_source + identifier_node->source_offset);
            node->index_union = script_index;
            if (script_index != -1) {
                node->flags = node->flags | _hs_syntax_node_script_call_bit;
            }
        }
        identifier_node->index_union = node->index_union;
        return;
    }
    node->index_union = identifier_node->index_union;
}

#if 0
Original Ghidra decompilation (0x486680):

void hs_resolve_identifier_as_function_or_script(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined2 uVar4;
  short sVar5;
  uint in_EAX;
  uint uVar6;

  iVar3 = *(int *)(DAT_0087a474 + 0x34);
  iVar1 = iVar3 + (in_EAX & 0xffff) * 0x14;
  uVar6 = *(uint *)(iVar3 + 0x10 + (in_EAX & 0xffff) * 0x14) & 0xffff;
  iVar2 = iVar3 + uVar6 * 0x14;
  if (*(short *)(iVar3 + 4 + uVar6 * 0x14) != 2) {
    uVar4 = hs_find_function_by_name();
    *(undefined2 *)(iVar1 + 2) = uVar4;
    *(undefined2 *)(iVar2 + 4) = 2;
    if (*(short *)(iVar1 + 2) == -1) {
      sVar5 = hs_script_find_by_name(*(int *)(iVar2 + 0xc) + DAT_006b14c0);
      *(short *)(iVar1 + 2) = sVar5;
      if (sVar5 != -1) {
        *(byte *)(iVar1 + 6) = *(byte *)(iVar1 + 6) | 2;
      }
    }
    *(undefined2 *)(iVar2 + 2) = *(undefined2 *)(iVar1 + 2);
    return;
  }
  *(undefined2 *)(iVar1 + 2) = *(undefined2 *)(iVar2 + 2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
