// widget_close_all  (Ghidra: widget_close_all, already named)
// address 0x498650, size 88 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: matches the given name; closes the single root widget, frees its go-back history
// list, resets ui_pause_depth, and (when armed) clears a 0x290 byte controls-related block.
// register convention: no register-passed arguments.
// UNSURE: DAT_006953e8/00712542/00712544..00712548's field layout is not documented anywhere in
// this module; declared as raw externs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern widget_instance *ui_root_widget[1];        // 0x00718f94
extern widget_history_node *ui_widget_history[3]; // 0x00718f98
extern int16_t ui_pause_depth; // 0x00718fa6
extern int32_t controls_capture_row; // 0x006953e8, UNSURE
extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE
extern uint8_t controls_input_capture_buffer[0x290]; // 0x00712544, UNSURE: 0xa0 dwords zeroed

extern void widget_close(widget_instance *widget); // 0x497c00
extern void widget_pool_list_free_all(widget_history_node **head); // 0x4994b0

// Closes the (single) root widget, frees its go-back history list, resets ui_pause_depth to 0,
// and, when controls_capture_row is armed, clears a 0x290 byte controls-related block.
void widget_close_all(void)
{
    int32_t i;

    if (ui_root_widget[0] != (widget_instance *)0) {
        widget_close(ui_root_widget[0]);
    }
    if (ui_widget_history[0] != (widget_history_node *)0) {
        widget_pool_list_free_all(&ui_widget_history[0]);
    }
    ui_pause_depth = 0;
    if (controls_capture_row != -1) {
        controls_input_capture_flags = controls_input_capture_flags & 0xf7;
        for (i = 0; i < 0x290; i++) {
            controls_input_capture_buffer[i] = 0;
        }
        controls_capture_row = -1;
    }
}

#if 0
Original Ghidra decompilation (0x498650):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void widget_close_all(void)

{
  int iVar1;
  undefined4 *puVar2;

  if (DAT_00718f94 != 0) {
    widget_close(DAT_00718f94);
  }
  if (DAT_00718f98 != 0) {
    FUN_004994b0();
  }
  _DAT_00718fa6 = 0;
  if (DAT_006953e8 != -1) {
    DAT_00712542 = DAT_00712542 & 0xf7;
    puVar2 = &DAT_00712544;
    for (iVar1 = 0xa0; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    DAT_006953e8 = -1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
