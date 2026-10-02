// widget_pool_list_free_all  (Ghidra: FUN_004994b0; named by types/interface.h's own
// widget_history_node note)
// address 0x4994b0, size 129 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: types/interface.h documents this address as widget_pool_list_free_all; walks a
// widget_history_node singly-linked list via ->next and frees each node's own heap block with
// the same inline unlink sequence widget_close and list_node_pop use.
// register convention: list head address in EDI (unaff_EDI), unresolved register read.
// blam-cc: EDI -> head

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

// blam-cc: EDI -> head
// Frees every node of a widget_history_node list, unlinking each node's own heap block as it
// goes (the same manual sequence widget_close uses for the widget it closes).
void widget_pool_list_free_all(widget_history_node **head)
{
    heap *pool = widget_memory_pool;
    widget_history_node *node = *head;

    while (node != (widget_history_node *)0) {
        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
        uint32_t size = block->size;
        int32_t slot = block->slot;

        *head = node->next;

        if (block->previous != (heap_block *)0) {
            block->previous->next = block->next;
        }
        if (block->next != (heap_block *)0) {
            block->next->previous = block->previous;
        }
        if (block == pool->first_block) {
            pool->first_block = block->next;
        }
        if (block == pool->last_block) {
            pool->last_block = block->previous;
        }
        pool->blocks[slot] = (heap_block *)0;
        pool->next_free_slot = (pool->first_block != (heap_block *)0) ? slot : 0;
        pool->bytes_allocated = pool->bytes_allocated - (int32_t)(size & 0x7fffffff);
        pool->allocation_count = pool->allocation_count - 1;

        node = *head;
    }
}

#if 0
Original Ghidra decompilation (0x4994b0):

void FUN_004994b0(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  undefined *puVar4;
  uint *puVar5;
  int *unaff_EDI;

  puVar4 = PTR_PTR_006926c4;
  iVar1 = *unaff_EDI;
  while (iVar1 != 0) {
    iVar1 = *unaff_EDI;
    puVar5 = (uint *)(iVar1 + -0x10);
    *unaff_EDI = *(int *)(iVar1 + 0xc);
    uVar2 = *puVar5;
    uVar3 = *(uint *)(iVar1 + -0xc);
    if (*(int *)(iVar1 + -8) != 0) {
      *(undefined4 *)(*(int *)(iVar1 + -8) + 0xc) = *(undefined4 *)(iVar1 + -4);
    }
    if (*(int *)(iVar1 + -4) != 0) {
      *(undefined4 *)(*(int *)(iVar1 + -4) + 8) = *(undefined4 *)(iVar1 + -8);
    }
    if (puVar5 == *(uint **)(puVar4 + 0x2c)) {
      *(undefined4 *)(puVar4 + 0x2c) = *(undefined4 *)(iVar1 + -4);
    }
    if (puVar5 == *(uint **)(puVar4 + 0x30)) {
      *(undefined4 *)(puVar4 + 0x30) = *(undefined4 *)(iVar1 + -8);
    }
    *(undefined4 *)(puVar4 + uVar3 * 4 + 0x34) = 0;
    *(uint *)(puVar4 + 0x10) = -(uint)(*(int *)(puVar4 + 0x2c) != 0) & uVar3;
    *(uint *)(puVar4 + 0x14) = *(int *)(puVar4 + 0x14) - (uVar2 & 0x7fffffff);
    *(int *)(puVar4 + 0x1c) = *(int *)(puVar4 + 0x1c) + -1;
    iVar1 = *unaff_EDI;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
