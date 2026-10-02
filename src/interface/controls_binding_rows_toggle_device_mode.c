// controls_binding_rows_toggle_device_mode  (Ghidra: FUN_004b53a0, named in phase 4)
// address 0x4b53a0, size 243 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b53a0..0x4b5492 in the phase-4 review. The first
// rewrite read the second sensitivity spinner one child level too deep and named
// 0x006953ec a device count; it is controls_selected_device (written by
// controls_binding_row_handle_input: the device spinner selection, +1 above 0).
//   ESI is the controls menu widget; its second child (row_a) and the child after it
// (row_b) are two alternative panels. With the mode byte clear, or a keyboard / mouse device
// (controls_selected_device below 2), row_a is shown (state 1, hidden 0) and focused and
// row_b hidden. Otherwise, for a real profile (selected_saved_item low nibble 0), row_b is
// shown and focused, row_a hidden, and the first spinner (first child of widget type 2) of
// the first grandchild group and of the group after it get selection (byte - 1) from the
// per-device bytes at 0x007157d4 and 0x007157d8 (working profile +0x954 / +0x958, indexed by
// the device), 0 when the byte is 0 or above 10. The mode byte is stored at 0x00719445 on
// every path. The spinner search does not stop at NULL before the write (kept from the
// binary).
// register convention: ESI widget; one stack byte.
//   // blam-cc: widget -> ESI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t controls_selected_device;        // 0x006953ec
extern int32_t selected_saved_item;             // 0x00714e7c
extern uint8_t controls_menu_list_mode;         // 0x00719445 (see controls_apply_preset.c)
extern uint8_t controls_device_sensitivity_a[]; // 0x007157d4, indexed by device; UNSURE name
extern uint8_t controls_device_sensitivity_b[]; // 0x007157d8, indexed by device; UNSURE name

static widget_instance *controls_find_spinner(widget_instance *child)
{
    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

static void controls_spinner_set_from_byte(widget_instance *spinner, uint8_t value)
{
    if (value == 0 || value > 10) {
        spinner->selection_index = 0;
    } else {
        spinner->selection_index = (int16_t)(value - 1);
    }
}

// blam-cc: widget -> ESI
void controls_binding_rows_toggle_device_mode(widget_instance *widget, uint8_t mode)
{
    widget_instance *row_a = widget->first_child->next_sibling;
    widget_instance *row_b = row_a->next_sibling;

    if (mode == 0 || controls_selected_device < 2) {
        row_b->state = 0;
        row_b->hidden = 1;
        widget->focused_child = row_a;
        row_a->hidden = 0;
        row_a->state = 1;
    } else if ((selected_saved_item & 0xf) == 0) {
        int32_t device = controls_selected_device;
        widget_instance *group;

        row_b->state = 1;
        row_b->hidden = 0;
        widget->focused_child = row_b;
        row_a->state = 0;
        row_a->hidden = 1;

        group = row_b->first_child->first_child;
        controls_spinner_set_from_byte(controls_find_spinner(group->first_child), controls_device_sensitivity_a[device]);
        group = group->next_sibling;
        controls_spinner_set_from_byte(controls_find_spinner(group->first_child), controls_device_sensitivity_b[device]);
    }
    controls_menu_list_mode = mode;
}

#if 0
Original Ghidra decompilation (0x4b53a0):

void FUN_004b53a0(char param_1)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int unaff_ESI;

  iVar4 = DAT_006953ec;
  iVar2 = *(int *)(*(int *)(unaff_ESI + 0x34) + 0x2c);
  iVar3 = *(int *)(iVar2 + 0x2c);
  if ((param_1 == '\0') || (DAT_006953ec < 2)) {
    *(undefined1 *)(iVar3 + 0x10) = 0;
    *(undefined1 *)(iVar3 + 0x12) = 1;
    *(int *)(unaff_ESI + 0x38) = iVar2;
    *(undefined1 *)(iVar2 + 0x12) = 0;
    *(undefined1 *)(iVar2 + 0x10) = 1;
    DAT_00719445 = param_1;
    return;
  }
  if ((DAT_00714e7c & 0xf) == 0) {
    *(undefined1 *)(iVar3 + 0x10) = 1;
    *(undefined1 *)(iVar3 + 0x12) = 0;
    *(int *)(unaff_ESI + 0x38) = iVar3;
    *(undefined1 *)(iVar2 + 0x10) = 0;
    *(undefined1 *)(iVar2 + 0x12) = 1;
    iVar2 = *(int *)(*(int *)(iVar3 + 0x34) + 0x34);
    for (iVar3 = *(int *)(iVar2 + 0x34); (iVar3 != 0 && (*(short *)(iVar3 + 0xe) != 2));
        iVar3 = *(int *)(iVar3 + 0x2c)) {
    }
    bVar1 = *(byte *)(iVar4 + 0x7157d4);
    if ((bVar1 == 0) || (10 < bVar1)) {
      *(undefined2 *)(iVar3 + 0x40) = 0;
    }
    else {
      *(ushort *)(iVar3 + 0x40) = bVar1 - 1;
    }
    for (iVar2 = *(int *)(*(int *)(iVar2 + 0x2c) + 0x34);
        (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2)); iVar2 = *(int *)(iVar2 + 0x2c)) {
    }
    bVar1 = *(byte *)(iVar4 + 0x7157d8);
    if ((bVar1 == 0) || (10 < bVar1)) {
      *(undefined2 *)(iVar2 + 0x40) = 0;
      DAT_00719445 = param_1;
      return;
    }
    *(ushort *)(iVar2 + 0x40) = bVar1 - 1;
  }
  DAT_00719445 = param_1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
