// tree_find_min  (already named; task-provided)
// address 0x57cb70, size 28 bytes
// name confidence: 0.5 (already carries this name; walks `left` children until `is_nil`,
//   the textbook red-black tree minimum-node search)
// rewrite confidence: 0.6 (trivial, standard library code)
// evidence: types/shell.h hwreq_map_node (left 0x00, is_nil 0x2d).
// register convention: EAX = a pointer to the starting node's `left` field (in_EAX; the
//   decompilation dereferences `*in_EAX` first, so the true starting node is `*(hwreq_map_node **)in_EAX`).
// blam-cc: EAX -> subtree_root_left_field
// FIXED (register inputs, objdump): EAX (read at 0x57cb70, `mov ecx,[eax]`) was already a C
//   parameter but had no machine-checked "blam-cc" line in the parser's "REG -> name" format
//   (the old note was a prose function signature); reworded so the checker recognizes it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

// blam-cc: EAX -> subtree_root_left_field
hwreq_map_node *tree_find_min(hwreq_map_node **subtree_root_left_field)
{
    hwreq_map_node *node = *subtree_root_left_field;
    while (node->is_nil == 0) {
        node = (hwreq_map_node *)node->left;
    }
    return node;
}

#if 0
Original Ghidra decompilation (0x57cb70):

void tree_find_min(void)

{
  char cVar1;
  int *piVar2;
  int *in_EAX;

  piVar2 = (int *)*in_EAX;
  cVar1 = *(char *)((int)piVar2 + 0x2d);
  while (cVar1 == '\0') {
    piVar2 = (int *)*piVar2;
    cVar1 = *(char *)((int)piVar2 + 0x2d);
  }
  return;
}
#endif
