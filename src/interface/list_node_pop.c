// list_node_pop  (Ghidra: FUN_00499460; named by types/interface.h's own widget_history_node
// note)
// address 0x499460, size 68 bytes
// name confidence: 0.6   rewrite confidence: 0.45
// evidence: types/interface.h documents this address as list_node_pop, the mirror of
// list_node_prepend @0x499430: pops the head node's definition/list_definition/selection/controller_index into
// a caller output, advances the list head to the popped node's next, then unlinks the popped
// node's own heap block.
// register convention: output record (3 dwords) in ECX (in_ECX), list head address in EDX
// (in_EDX), both unresolved register reads recovered only by field-shape.
// blam-cc: ECX -> out, EDX -> head
// UNSURE: heap_unlink_block's call here shows no visible arguments and the extraout_ECX result
// it leaves behind is modeled as widget_memory_pool, matching every other inlined
// heap_unlink_block call site in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern heap *widget_memory_pool; // 0x006926c4
extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0

// blam-cc: ECX -> out, EDX -> head
// Pops the head widget_history_node from `*head` into `*out` (definition/list_definition/
// selection/controller_index only, not the link), advances `*head`, and frees the popped node's own heap block.
void list_node_pop(widget_history_node *out, widget_history_node **head)
{
    widget_history_node *node = *head;
    heap_block *block;
    uint32_t size;

    out->definition = node->definition;
    out->list_definition = node->list_definition;
    out->selection = node->selection;
    out->controller_index = node->controller_index;
    *head = node->next;

    block = (heap_block *)((uint8_t *)node - 0x10);
    size = block->size;
    heap_unlink_block(block, widget_memory_pool);
    widget_memory_pool->bytes_allocated = widget_memory_pool->bytes_allocated - (int32_t)(size & 0x7fffffff);
    widget_memory_pool->allocation_count = widget_memory_pool->allocation_count - 1;
}

#if 0
Original Ghidra decompilation (0x499460):

void FUN_00499460(void)

{
  undefined4 *puVar1;
  uint uVar2;
  undefined4 *in_ECX;
  int extraout_ECX;
  int *in_EDX;

  puVar1 = (undefined4 *)*in_EDX;
  *in_ECX = *puVar1;
  in_ECX[1] = puVar1[1];
  in_ECX[2] = puVar1[2];
  *in_EDX = puVar1[3];
  uVar2 = puVar1[-4];
  heap_unlink_block();
  *(uint *)(extraout_ECX + 0x14) = *(int *)(extraout_ECX + 0x14) - (uVar2 & 0x7fffffff);
  *(int *)(extraout_ECX + 0x1c) = *(int *)(extraout_ECX + 0x1c) + -1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
