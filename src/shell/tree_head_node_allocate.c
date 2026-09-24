// tree_head_node_allocate  (already named; task-provided)
// address 0x57cbf0, size 55 bytes
// name confidence: 0.5 (already carries this name; allocates a bare hwreq_map_node and zeroes
//   left/parent/right, matching how msvc_std_map's head sentinel is constructed before the map
//   is populated)
// rewrite confidence: 0.5 (allocation and the left/parent/right zeroing are confirmed against
//   the decompilation; the color/is_nil values this function itself writes -- color=1,
//   is_nil=0 -- are the OPPOSITE of what a head sentinel should end up with per
//   types/shell.h's own note ("head node has is_nil = 1 and color = 1"), so either the caller
//   (hwreq_parser_construct 0x579ef0, not this pass) overwrites is_nil afterward, or this
//   function is a generic "allocate a zeroed node" helper reused for the head case, not a
//   head-specific constructor; preserved exactly as decompiled rather than guessed at)
// evidence: types/shell.h hwreq_map_node (left 0x00, parent 0x04, right 0x08, color 0x2c,
//   is_nil 0x2d).
// register convention: none; takes no parameters (Ghidra's own signature).
// UNSURE: see rewrite confidence note above -- the is_nil=0 write is preserved verbatim even
//   though it looks backwards for a sentinel.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void *operator_new(uint32_t size); // 0x6277da

hwreq_map_node *tree_head_node_allocate(void)
{
    hwreq_map_node *node = (hwreq_map_node *)operator_new(sizeof(hwreq_map_node));

    if (node != 0) {
        node->left = 0;
        node->parent = 0;
        node->right = 0;
        node->color = 1;
        node->is_nil = 0;
    }
    return node;
}

#if 0
Original Ghidra decompilation (0x57cbf0):

void tree_head_node_allocate(void)

{
  undefined4 *puVar1;

  puVar1 = operator_new(0x30);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = 0;
  }
  if (puVar1 + 1 != (undefined4 *)0x0) {
    puVar1[1] = 0;
  }
  if (puVar1 + 2 != (undefined4 *)0x0) {
    puVar1[2] = 0;
  }
  *(undefined1 *)(puVar1 + 0xb) = 1;
  *(undefined1 *)((int)puVar1 + 0x2d) = 0;
  return;
}
#endif
