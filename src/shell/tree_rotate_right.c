// tree_rotate_right  (already named; task-provided)
// address 0x57cb90, size 85 bytes
// name confidence: 0.5 (already carries this name; the mirror image of tree_rotate_left.c --
//   textbook red-black tree right-rotation around node x)
// rewrite confidence: 0.55 (standard library code, mirrors tree_rotate_left.c exactly)
// evidence: types/shell.h hwreq_map_node, msvc_std_map. Same field roles as tree_rotate_left.c.
// register convention: ECX = x, stack argument = tree (msvc_std_map *).
// blam-cc: tree_rotate_right(hwreq_map_node *x /*ECX*/, msvc_std_map *tree /*stack*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"

void tree_rotate_right(hwreq_map_node *x, msvc_std_map *tree)
{
    hwreq_map_node *y = (hwreq_map_node *)x->left;
    hwreq_map_node *head = (hwreq_map_node *)tree->head;

    x->left = y->right;
    if (((hwreq_map_node *)y->right)->is_nil == 0) {
        ((hwreq_map_node *)y->right)->parent = (uint32_t)x;
    }
    y->parent = x->parent;

    if (x == (hwreq_map_node *)head->parent) {
        head->parent = (uint32_t)y;
    } else if (x == (hwreq_map_node *)((hwreq_map_node *)x->parent)->right) {
        ((hwreq_map_node *)x->parent)->right = (uint32_t)y;
    } else {
        ((hwreq_map_node *)x->parent)->left = (uint32_t)y;
    }

    y->right = (uint32_t)x;
    x->parent = (uint32_t)y;
}

#if 0
Original Ghidra decompilation (0x57cb90):

void tree_rotate_right(int param_1)

{
  int iVar1;
  int *piVar2;
  int *in_ECX;

  iVar1 = *in_ECX;
  *in_ECX = *(int *)(iVar1 + 8);
  if (*(char *)(*(int *)(iVar1 + 8) + 0x2d) == '\0') {
    *(int **)(*(int *)(iVar1 + 8) + 4) = in_ECX;
  }
  *(int *)(iVar1 + 4) = in_ECX[1];
  if (in_ECX == *(int **)(*(int *)(param_1 + 4) + 4)) {
    *(int *)(*(int *)(param_1 + 4) + 4) = iVar1;
    *(int **)(iVar1 + 8) = in_ECX;
    in_ECX[1] = iVar1;
    return;
  }
  piVar2 = (int *)in_ECX[1];
  if (in_ECX == (int *)piVar2[2]) {
    piVar2[2] = iVar1;
    *(int **)(iVar1 + 8) = in_ECX;
    in_ECX[1] = iVar1;
    return;
  }
  *piVar2 = iVar1;
  *(int **)(iVar1 + 8) = in_ECX;
  in_ECX[1] = iVar1;
  return;
}
#endif
