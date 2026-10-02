// tree_erase_range  (already named; task-provided)
// address 0x57c310, size 122 bytes
// name confidence: 0.5 (already carries this name; standard MSVC 7.1
//   `_Tree::erase(iterator first, iterator last)`, with a whole-tree fast path when
//   [first, last) spans the entire map)
// rewrite confidence: 0.4 (the whole-tree fast path is confirmed against the decompilation;
//   the general per-node loop delegates to an opaque single-node erase/rebalance helper,
//   FUN_0057c820, not in this pass's address list)
// evidence: types/shell.h hwreq_map_node, msvc_std_map. Sole caller in this pass is
//   hwreq_map_destruct.c (0x579fe0), which always calls erase(begin(), end()), taking the fast
//   path every time.
// register convention: stack arguments = out (hwreq_map_node **, receives the iterator the
//   standard erase(first,last) would return), first (hwreq_map_node *), last
//   (hwreq_map_node *); ESI = tree (msvc_std_map *, unaff_ESI).
// blam-cc: tree_erase_range(hwreq_map_node **out /*stack*/, hwreq_map_node *first /*stack*/,
//   hwreq_map_node *last /*stack*/, msvc_std_map *tree /*ESI*/)
// UNSURE: FUN_0057c820 (single-node erase-and-rebalance) is an opaque extern, not this pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void tree_destroy_subtree(hwreq_map_node *node); // 0x57cce0, same pass
extern void tree_iterator_increment(hwreq_map_node **iterator); // 0x57c5e0, same pass
extern void tree_erase_one(msvc_std_map *tree, hwreq_map_node **erase_holder, hwreq_map_node *node); // 0x57c820, UNSURE: signature guessed, not this pass

hwreq_map_node **tree_erase_range(hwreq_map_node **out, hwreq_map_node *first, hwreq_map_node *last, msvc_std_map *tree)
{
    hwreq_map_node *head = (hwreq_map_node *)tree->head;

    if (first == (hwreq_map_node *)head->left && last == head) {
        // Erasing the whole tree: destroy it and reset to the empty state.
        tree_destroy_subtree((hwreq_map_node *)head->parent);
        head->parent = (uint32_t)head;
        tree->size = 0;
        head->left = (uint32_t)head;
        head->right = (uint32_t)head;
        *out = (hwreq_map_node *)head->left;
        return out;
    }

    while (first != last) {
        hwreq_map_node *current = first;
        tree_iterator_increment(&first);
        tree_erase_one(tree, out, current); // UNSURE: passed &out per FUN_0057c820(unaff_ESI, &param_1, piVar2)
    }
    *out = first;
    return out;
}

#if 0
Original Ghidra decompilation (0x57c310):

undefined4 * tree_erase_range(undefined4 *param_1,int *param_2,int *param_3)

{
  int *piVar1;
  int *piVar2;
  undefined4 *puVar3;
  int *piVar4;
  int unaff_ESI;

  piVar4 = param_3;
  puVar3 = param_1;
  piVar1 = *(int **)(unaff_ESI + 4);
  piVar2 = param_2;
  if ((param_2 == (int *)*piVar1) && (param_3 == piVar1)) {
    tree_destroy_subtree(piVar1[1]);
    *(int *)(*(int *)(unaff_ESI + 4) + 4) = *(int *)(unaff_ESI + 4);
    *(undefined4 *)(unaff_ESI + 8) = 0;
    *(undefined4 *)*(undefined4 *)(unaff_ESI + 4) = *(undefined4 *)(unaff_ESI + 4);
    *(int *)(*(int *)(unaff_ESI + 4) + 8) = *(int *)(unaff_ESI + 4);
    *puVar3 = **(undefined4 **)(unaff_ESI + 4);
    return puVar3;
  }
  while (piVar2 != piVar4) {
    param_2 = piVar2;
    tree_iterator_increment();
    FUN_0057c820(unaff_ESI,&param_1,piVar2);
    piVar2 = param_2;
  }
  *puVar3 = piVar2;
  return puVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
