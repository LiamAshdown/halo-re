// widget_memory_pool_initialize  (Ghidra: widget_memory_pool_initialize, already named)
// address 0x4979b0, size 181 bytes
// name confidence: 0.8   rewrite confidence: 0.65
// evidence: string "widget_memory_pool" (the heap's debug name, types/interface.h's
// widget_memory_pool_name); allocates the 0x20000 byte GlobalAlloc arena and re-derives the
// heap header, matching the "Layouts the binary itself states" table's widget_instance/heap
// evidence, then zeroes every ui_* global from ui_root_widget through widget_memory_pool_valid.
// register convention: no register-passed arguments.
// UNSURE: ends with `*blocks[0] = &blocks[0]`, storing the block array's own address into its
// first slot -- types/interface.h's "Unresolved offsets" section already flags this as
// contradicting memory.h's own "NULL means free" comment on heap::blocks; preserved verbatim.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern heap *widget_memory_pool; // 0x006926c4, 0x20000 byte GlobalAlloc arena
extern void *widget_memory_pool_name; // 0x0068e690, the literal "widget_memory_pool"
extern uint8_t widget_memory_pool_valid; // 0x00718fc2

extern widget_instance *ui_root_widget[1];         // 0x00718f94
extern widget_history_node *ui_widget_history[3];  // 0x00718f98
extern int16_t ui_unknown_718fa4;                  // 0x00718fa4
extern float ui_unknown_718fa8;                    // 0x00718fa8
extern int16_t quit_confirm_error_string_index;      // 0x00718fac, first word of the pending message record (types/interface.h)
extern ui_pending_error ui_pending_error_alternate; // 0x00718fb2
extern ui_pending_error ui_pending_errors[4];       // 0x00718fb6

extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes);

// Allocates the widget system's 0x20000 byte heap arena, re-derives its heap header (clearing
// and restoring base/size/unknown_00/maximum_blocks around a full header wipe, then
// self-referencing blocks[0]), and clears every UI global from ui_root_widget through
// widget_memory_pool_valid before setting the latter from whether the allocation succeeded.
void widget_memory_pool_initialize(void)
{
    void *allocation;
    uint8_t *base;
    int32_t size;
    uint32_t unknown_00;
    int32_t maximum_blocks;
    heap_block **blocks;
    int32_t i;
    uint8_t *clear_cursor;

    allocation = GlobalAlloc(0, 0x20000);
    if (allocation != (void *)0) {
        widget_memory_pool->base = (uint8_t *)allocation;
        widget_memory_pool->size = 0x20000;
    }

    base = widget_memory_pool->base;
    size = widget_memory_pool->size;
    unknown_00 = widget_memory_pool->unknown_00;
    maximum_blocks = widget_memory_pool->maximum_blocks;

    blocks = widget_memory_pool->blocks;
    for (i = 0; i < maximum_blocks; i++) {
        blocks[i] = (heap_block *)0;
    }
    for (clear_cursor = (uint8_t *)widget_memory_pool, i = 0; i < 0x34; i++) {
        clear_cursor[i] = 0;
    }

    widget_memory_pool->base = base;
    widget_memory_pool->size = size;
    widget_memory_pool->unknown_00 = unknown_00;
    widget_memory_pool->maximum_blocks = maximum_blocks;
    widget_memory_pool->blocks[0] = (heap_block *)&widget_memory_pool->blocks[0]; // see UNSURE

    for (clear_cursor = (uint8_t *)&ui_root_widget[0], i = 0; i < 0x34; i++) {
        clear_cursor[i] = 0;
    }
    ui_unknown_718fa4 = -1;
    ui_pending_error_alternate.error_string_index = -1;
    quit_confirm_error_string_index = -1;
    ui_pending_errors[0].error_string_index = -1;
    ui_unknown_718fa8 = -1.0f;
    widget_memory_pool_valid = (allocation != (void *)0);
}

#if 0
Original Ghidra decompilation (0x4979b0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void widget_memory_pool_initialize(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined *puVar4;
  HGLOBAL pvVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;

  pvVar5 = GlobalAlloc(0,0x20000);
  puVar4 = PTR_PTR_006926c4;
  if (pvVar5 != (HGLOBAL)0x0) {
    *(HGLOBAL *)(PTR_PTR_006926c4 + 4) = pvVar5;
    *(undefined4 *)(puVar4 + 8) = 0x20000;
  }
  uVar1 = *(undefined4 *)(puVar4 + 4);
  uVar2 = *(undefined4 *)(puVar4 + 8);
  uVar3 = *(undefined4 *)puVar4;
  puVar9 = (undefined4 *)(puVar4 + 0x34);
  iVar7 = *(int *)(puVar4 + 0xc);
  puVar8 = puVar9;
  for (iVar6 = iVar7; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  puVar8 = (undefined4 *)puVar4;
  for (iVar6 = 0xd; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  *(undefined4 *)(puVar4 + 4) = uVar1;
  *(undefined4 *)(puVar4 + 8) = uVar2;
  *(undefined4 *)puVar4 = uVar3;
  *(int *)(puVar4 + 0xc) = iVar7;
  *puVar9 = puVar9;
  puVar9 = &DAT_00718f94;
  for (iVar7 = 0xd; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar9 = 0;
    puVar9 = puVar9 + 1;
  }
  DAT_00718fa4 = 0xffff;
  DAT_00718fb2 = 0xffff;
  DAT_00718fac = 0xffff;
  DAT_00718fb6 = 0xffff;
  _DAT_00718fa8 = 0xbf800000;
  DAT_00718fc2 = pvVar5 != (HGLOBAL)0x0;
  return;
}
#endif
