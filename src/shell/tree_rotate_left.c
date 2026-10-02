// tree_rotate_left  (already named; task-provided)
// address 0x57cb10, size 81 bytes
// name confidence: 0.5 (already carries this name; the textbook red-black tree left-rotation
//   around node x: y = x->right; x->right = y->left; ...; y->left = x; x->parent = y)
// rewrite confidence: 0.55 (standard library code; every field access matches
//   types/shell.h hwreq_map_node exactly)
// evidence: types/shell.h hwreq_map_node (left 0x00, parent 0x04, right 0x08, is_nil 0x2d),
//   msvc_std_map (head 0x04). `tree` here is the map object; `tree->head->parent` is the root.
// register convention: ECX = x (the node to rotate around), stack argument = tree
//   (msvc_std_map *, used only to reach head->parent, the root pointer).
// blam-cc: tree_rotate_left(hwreq_map_node *x /*ECX*/, msvc_std_map *tree /*stack*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void tree_rotate_left(hwreq_map_node *x, msvc_std_map *tree)
{
    hwreq_map_node *y = (hwreq_map_node *)x->right;
    hwreq_map_node *head = (hwreq_map_node *)tree->head;

    x->right = y->left;
    if (((hwreq_map_node *)y->left)->is_nil == 0) {
        ((hwreq_map_node *)y->left)->parent = (uint32_t)x;
    }
    y->parent = x->parent;

    if (x == (hwreq_map_node *)head->parent) {
        head->parent = (uint32_t)y;
    } else if (x == (hwreq_map_node *)((hwreq_map_node *)x->parent)->left) {
        ((hwreq_map_node *)x->parent)->left = (uint32_t)y;
    } else {
        ((hwreq_map_node *)x->parent)->right = (uint32_t)y;
    }

    y->left = (uint32_t)x;
    x->parent = (uint32_t)y;
}

#if 0
Original Ghidra decompilation (0x57cb10):

void tree_rotate_left(int param_1)

{
  int *piVar1;
  int *piVar2;
  int in_ECX;

  piVar1 = *(int **)(in_ECX + 8);
  *(int *)(in_ECX + 8) = *piVar1;
  if (*(char *)(*piVar1 + 0x2d) == '\0') {
    *(int *)(*piVar1 + 4) = in_ECX;
  }
  piVar1[1] = *(int *)(in_ECX + 4);
  if (in_ECX == *(int *)(*(int *)(param_1 + 4) + 4)) {
    *(int **)(*(int *)(param_1 + 4) + 4) = piVar1;
    *piVar1 = in_ECX;
    *(int **)(in_ECX + 4) = piVar1;
    return;
  }
  piVar2 = *(int **)(in_ECX + 4);
  if (in_ECX == *piVar2) {
    *piVar2 = (int)piVar1;
    *piVar1 = in_ECX;
    *(int **)(in_ECX + 4) = piVar1;
    return;
  }
  piVar2[2] = (int)piVar1;
  *piVar1 = in_ECX;
  *(int **)(in_ECX + 4) = piVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
