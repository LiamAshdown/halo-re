// tree_find_max  (already named; task-provided)
// address 0x57cd20, size 29 bytes
// name confidence: 0.5 (already carries this name; walks `right` children until `is_nil`, the
//   mirror image of tree_find_min.c)
// rewrite confidence: 0.6 (trivial, standard library code)
// evidence: types/shell.h hwreq_map_node (right 0x08, is_nil 0x2d).
// register convention: EAX = the starting node itself (in_EAX; unlike tree_find_min.c, this
//   function is NOT handed a pointer-to-pointer -- it dereferences `in_EAX + 8` directly).
// blam-cc: tree_find_max(hwreq_map_node *node /*EAX*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

hwreq_map_node *tree_find_max(hwreq_map_node *node)
{
    hwreq_map_node *cursor = (hwreq_map_node *)node->right;
    while (cursor->is_nil == 0) {
        cursor = (hwreq_map_node *)cursor->right;
    }
    return cursor;
}

#if 0
Original Ghidra decompilation (0x57cd20):

void tree_find_max(void)

{
  char cVar1;
  int iVar2;
  int in_EAX;

  iVar2 = *(int *)(in_EAX + 8);
  cVar1 = *(char *)(iVar2 + 0x2d);
  while (cVar1 == '\0') {
    iVar2 = *(int *)(iVar2 + 8);
    cVar1 = *(char *)(iVar2 + 0x2d);
  }
  return;
}
#endif
