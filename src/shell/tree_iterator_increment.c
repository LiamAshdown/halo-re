// tree_iterator_increment  (already named; task-provided)
// address 0x57c5e0, size 87 bytes
// name confidence: 0.5 (already carries this name; the textbook MSVC 7.1 `_Tree::_Inc`
//   in-order-successor walk: if the node has a right child, the successor is that child's
//   leftmost descendant; otherwise it is the nearest ancestor for which the node lies in the
//   left subtree)
// rewrite confidence: 0.6 (standard library code, confirmed against the decompilation)
// evidence: types/shell.h hwreq_map_node (left 0x00, parent 0x04, right 0x08, is_nil 0x2d).
// register convention: EDX = hwreq_map_node **iterator (in_EDX; the iterator is advanced
//   in place).
// blam-cc: tree_iterator_increment(hwreq_map_node **iterator /*EDX*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void tree_iterator_increment(hwreq_map_node **iterator)
{
    hwreq_map_node *node = *iterator;

    if (node->is_nil != 0) {
        return;
    }

    {
        hwreq_map_node *right = (hwreq_map_node *)node->right;
        if (right->is_nil == 0) {
            hwreq_map_node *cursor = (hwreq_map_node *)right->left;
            while (cursor->is_nil == 0) {
                right = cursor;
                cursor = (hwreq_map_node *)cursor->left;
            }
            *iterator = right;
            return;
        }
    }

    {
        hwreq_map_node *parent = (hwreq_map_node *)node->parent;
        while (parent->is_nil == 0 && node == (hwreq_map_node *)parent->right) {
            node = parent;
            parent = (hwreq_map_node *)parent->parent;
        }
        *iterator = parent;
    }
}

#if 0
Original Ghidra decompilation (0x57c5e0):

void tree_iterator_increment(void)

{
  char cVar1;
  int iVar2;
  int *piVar3;
  int *piVar4;
  int *in_EDX;

  iVar2 = *in_EDX;
  if (*(char *)(iVar2 + 0x2d) == '\0') {
    piVar3 = *(int **)(iVar2 + 8);
    if (*(char *)((int)piVar3 + 0x2d) == '\0') {
      cVar1 = *(char *)(*piVar3 + 0x2d);
      piVar4 = (int *)*piVar3;
      while (cVar1 == '\0') {
        cVar1 = *(char *)(*piVar4 + 0x2d);
        piVar3 = piVar4;
        piVar4 = (int *)*piVar4;
      }
      *in_EDX = (int)piVar3;
      return;
    }
    iVar2 = *(int *)(iVar2 + 4);
    cVar1 = *(char *)(iVar2 + 0x2d);
    while ((cVar1 == '\0' && (*in_EDX == *(int *)(iVar2 + 8)))) {
      *in_EDX = iVar2;
      iVar2 = *(int *)(iVar2 + 4);
      cVar1 = *(char *)(iVar2 + 0x2d);
    }
    *in_EDX = iVar2;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
