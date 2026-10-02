// tree_iterator_decrement  (already named; task-provided)
// address 0x57cd40, size 93 bytes
// name confidence: 0.5 (already carries this name; the textbook MSVC 7.1 `_Tree::_Dec`
//   in-order-predecessor walk, the mirror image of tree_iterator_increment.c, plus the
//   `_Tree::end()` special case: stepping back from the head sentinel lands on the tree's
//   maximum via `node->right`)
// rewrite confidence: 0.55 (standard library code, confirmed against the decompilation)
// evidence: types/shell.h hwreq_map_node (left 0x00, parent 0x04, right 0x08, is_nil 0x2d).
// register convention: EDX = hwreq_map_node **iterator (in_EDX).
// blam-cc: tree_iterator_decrement(hwreq_map_node **iterator /*EDX*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void tree_iterator_decrement(hwreq_map_node **iterator)
{
    hwreq_map_node *node = *iterator;

    if (node->is_nil != 0) {
        // *iterator was the head sentinel (end()); step to the tree's maximum (head->right).
        *iterator = (hwreq_map_node *)node->right;
        return;
    }

    {
        hwreq_map_node *left = (hwreq_map_node *)node->left;
        if (left->is_nil == 0) {
            hwreq_map_node *prev = left;
            hwreq_map_node *cursor = (hwreq_map_node *)left->right;
            while (cursor->is_nil == 0) {
                prev = cursor;
                cursor = (hwreq_map_node *)cursor->right;
            }
            *iterator = prev;
            return;
        }
    }

    {
        hwreq_map_node *parent = (hwreq_map_node *)node->parent;
        if (parent->is_nil == 0) {
            while (*iterator == (hwreq_map_node *)parent->left) {
                *iterator = parent;
                parent = (hwreq_map_node *)parent->parent;
                if (parent->is_nil != 0) {
                    break;
                }
            }
            if (parent->is_nil == 0) {
                *iterator = parent;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x57cd40):

void tree_iterator_decrement(void)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int *in_EDX;

  piVar4 = (int *)*in_EDX;
  if (*(char *)((int)piVar4 + 0x2d) != '\0') {
    *in_EDX = piVar4[2];
    return;
  }
  iVar2 = *piVar4;
  if (*(char *)(iVar2 + 0x2d) == '\0') {
    cVar1 = *(char *)(*(int *)(iVar2 + 8) + 0x2d);
    iVar3 = *(int *)(iVar2 + 8);
    while (cVar1 == '\0') {
      cVar1 = *(char *)(*(int *)(iVar3 + 8) + 0x2d);
      iVar2 = iVar3;
      iVar3 = *(int *)(iVar3 + 8);
    }
    *in_EDX = iVar2;
    return;
  }
  piVar4 = (int *)piVar4[1];
  if (*(char *)((int)piVar4 + 0x2d) == '\0') {
    do {
      if (*in_EDX != *piVar4) break;
      *in_EDX = (int)piVar4;
      piVar4 = (int *)piVar4[1];
    } while (*(char *)((int)piVar4 + 0x2d) == '\0');
    if (*(char *)((int)piVar4 + 0x2d) == '\0') {
      *in_EDX = (int)piVar4;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
