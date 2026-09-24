// ui_controls_options_free_list  (Ghidra: FUN_004a0a30, renamed)
// renamed from FUN_004a0a30 in the naming pass
// address 0x4a0a30, size 68 bytes, callers=0 in this build
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: functions.md: "Releases a widget's allocated child array and performs generic
// teardown, mirroring ui_free_profile_list for the controls/options widget context." Byte-for-byte
// the same heap-unlink pattern as FUN_0049df70.c (ui_free_profile_list), minus the item_count
// reset (this widget type does not use it the same way).
// register convention: cdecl, the one recognized stack parameter (widget).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern heap *widget_memory_pool; // 0x006926c4
extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0
extern void ui_list_free_all(void); // 0x4a7b20

uint32_t ui_controls_options_free_list(widget_instance *widget)
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
    ui_list_free_all();
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a0a30):

undefined4 FUN_004a0a30(int param_1)

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
  FUN_004a7b20();
  return 1;
}
#endif
