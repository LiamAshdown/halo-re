// hs_tokenize  (Ghidra: hs_tokenize, already named)
// address 0x486120, size 130 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: CEA-PDB string match ("i couldn't allocate a syntax node."); allocates one
// hs_syntax_node datum, seeds its index_union/next_node/flags/type, and dispatches to
// hs_tokenize_primitive or hs_tokenize_nonprimitive based on whether the next character is '('.
// register convention: __cdecl, cursor is the recognized single stack parameter.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_memory.h"


extern data_array *hs_syntax_data; // 0x0087a474
extern char *hs_compile_error;     // 0x006b14d4

// Allocates one syntax-node datum for the next token/expression at *cursor and dispatches to
// the primitive or nonprimitive tokenizer depending on whether it starts with '('.
datum_index hs_tokenize(char **cursor)
{
    data_array *nodes;
    datum_index index;
    hs_syntax_node *node;

    nodes = hs_syntax_data;
    index = datum_new(nodes);
    if (index == k_datum_index_none) {
        hs_compile_error = "i couldn't allocate a syntax node.";
        return k_datum_index_none;
    }
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (index & 0xffff) * nodes->size);
    node->index_union = (int16_t)0xffff;
    node->next_node = k_datum_index_none;
    node->flags = 0;
    node->type = 0;
    node->flags = (uint16_t)(**cursor != '(');
    if ((node->flags & _hs_syntax_node_primitive_bit) != 0) {
        hs_tokenize_primitive(cursor, index);
        return index;
    }
    hs_tokenize_nonprimitive(index, cursor);
    return index;
}

#if 0
Original Ghidra decompilation (0x486120):

uint hs_tokenize(undefined4 *param_1)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;

  iVar1 = DAT_0087a474;
  uVar2 = datum_new();
  if (uVar2 == 0xffffffff) {
    DAT_006b14d4 = "i couldn\'t allocate a syntax node.";
    return 0xffffffff;
  }
  iVar4 = (uVar2 & 0xffff) * 0x14;
  iVar3 = *(int *)(iVar1 + 0x34) + iVar4;
  *(undefined2 *)(iVar3 + 2) = 0xffff;
  *(undefined4 *)(iVar3 + 8) = 0xffffffff;
  *(undefined2 *)(iVar3 + 6) = 0;
  *(undefined2 *)(iVar3 + 4) = 0;
  *(ushort *)(iVar3 + 6) = (ushort)(*(char *)*param_1 != '(');
  if ((*(byte *)(*(int *)(iVar1 + 0x34) + 6 + iVar4) & 1) != 0) {
    hs_tokenize_primitive();
    return uVar2;
  }
  hs_tokenize_nonprimitive();
  return uVar2;
}
#endif
