// tree_destroy_subtree  (already named; task-provided)
// address 0x57cce0, size 60 bytes
// name confidence: 0.5 (already carries this name; postorder-destroys a subtree: recurses on
//   the right child, destructs and frees the current node, then continues iteratively down the
//   left spine -- the standard MSVC 7.1 `_Erase` tree-teardown shape that avoids one recursive
//   call per node)
// rewrite confidence: 0.55 (standard library code; confirmed against the decompilation)
// evidence: types/shell.h hwreq_map_node (left 0x00, right 0x08, is_nil 0x2d).
// register convention: the node to destroy arrives however the caller set it up (Ghidra
//   recognizes it as a genuine stack parameter here, unlike most of this batch).
// blam-cc: ECX -> map_self, stack -> node
// FIXED (register inputs, objdump): ECX carries a second, unchanged-across-recursion pointer
//   (read at 0x57ccec, `mov ebx,ecx`, then reloaded into ECX for the recursive self-call at
//   0x57ccf6/0x57ccf8). The 0x57c32d call site loads it via `mov ecx,esi; call 0x57cce0` where
//   esi is the tree/map container -- this is __thiscall's ECX = `this` (the owning hwreq_map),
//   not dereferenced inside this function but threaded through to every recursive call. Exact
//   type unknown; kept as an opaque pointer.
// UNSURE: hwreq_map_node_key_destruct (0x57cde0, this pass) is declared as taking the node
//   directly even though Ghidra's own decompile shows the call with zero visible arguments;
//   this matches every other "opaque zero-arg call" resolved elsewhere in this pass by trusting
//   the live register rather than Ghidra's rendering.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


// blam-cc: ECX -> map_self, stack -> node
void tree_destroy_subtree(void *map_self, hwreq_map_node *node)
{
    while (node->is_nil == 0) {
        hwreq_map_node *left = (hwreq_map_node *)node->left;
        tree_destroy_subtree(map_self, (hwreq_map_node *)node->right);
        hwreq_map_node_key_destruct(node);
        free(node);
        node = left;
    }
}

#if 0
Original Ghidra decompilation (0x57cce0):

void tree_destroy_subtree(int *param_1)

{
  char cVar1;
  int *piVar2;

  cVar1 = *(char *)((int)param_1 + 0x2d);
  while (cVar1 == '\0') {
    tree_destroy_subtree(param_1[2]);
    piVar2 = (int *)*param_1;
    FUN_0057cde0();
    _free(param_1);
    param_1 = piVar2;
    cVar1 = *(char *)((int)piVar2 + 0x2d);
  }
  return;
}
#endif
