// ui_free_profile_list  (Ghidra: FUN_0049df70, renamed)
// renamed from FUN_0049df70 in the naming pass
// address 0x49df70, size 74 bytes, callers=0 in this build
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: functions.md: "Releases the player-profile list's allocated array and resets its
// entry count as part of widget teardown." types/interface.h documents 0x4a7b20 as
// ui_list_free_all, called at the end here to also clear the three shared UI list groups.
// register convention: cdecl, the one recognized stack parameter (widget).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_memory.h"
#include "fn_interface.h"

extern heap *widget_memory_pool; // 0x006926c4


// Frees the widget's allocated profile-slot-id array (if any), clears its item count, and frees
// the three shared UI list groups.
uint32_t ui_free_profile_list(widget_instance *widget)
{
    if (widget->list_items != (void *)0) {
        heap_block *block = (heap_block *)((uint8_t *)widget->list_items - 0x10);
        uint32_t size = block->size;

        heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated =
            widget_memory_pool->bytes_allocated - (int32_t)(size & 0x7fffffff);
        widget_memory_pool->allocation_count = widget_memory_pool->allocation_count - 1;
        widget->list_items = (void *)0;
    }
    widget->item_count = 0;
    ui_list_free_all();
    return 1;
}

#if 0
Original Ghidra decompilation (0x49df70):

undefined4 FUN_0049df70(int param_1)

{
  uint uVar1;
  int extraout_ECX;

  if (*(int *)(param_1 + 0x44) != 0) {
    uVar1 = *(uint *)(*(int *)(param_1 + 0x44) + -0x10);
    heap_unlink_block();
    *(uint *)(extraout_ECX + 0x14) = *(int *)(extraout_ECX + 0x14) - (uVar1 & 0x7fffffff);
    *(int *)(extraout_ECX + 0x1c) = *(int *)(extraout_ECX + 0x1c) + -1;
    *(undefined4 *)(param_1 + 0x44) = 0;
  }
  *(undefined2 *)(param_1 + 0x48) = 0;
  FUN_004a7b20();
  return 1;
}
#endif
