// widget_draw_fullscreen_region  (Ghidra: FUN_004984c0, unnamed)
// address 0x4984c0, size 368 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/interface_functions.md "Draws all currently active full-screen UI widgets,
// an optional fade overlay, and the mouse cursor on top."; the non-split-screen sibling of
// widget_draw_split_screen_region (0x498330) -- same eligibility test over ui_root_widget[1],
// same dead controller-index clamp (proven there, reused here), but a fixed 640x480
// (0x280 x 0x1e0) destination rect and no clip rect (NULL) instead of a per-controller table.
// When the virtual keyboard is open, renders it (plus the cursor) instead.
// register convention: controller index in AX (in_AX), unresolved register read.
// blam-cc: AX -> controller_index
// UNSURE: ui_draw_filled_rectangle (0x449780) is called with no visible arguments; modeled as taking the
// computed fade color/alpha (local_10, EAX by this module's usual convention) and the fullscreen
// rect built just before it, neither confirmed independently.
// TYPES-GAP: DAT_00879f50 is not documented anywhere in types/interface.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern int32_t last_controller_index_00879f50; // 0x00879f50, TYPES-GAP, UNSURE name
extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern widget_instance *ui_root_widget[1]; // 0x00718f94
extern uint8_t ui_split_screen;            // 0x00718fc9
extern float ui_unknown_718fa8;            // 0x00718fa8

extern void widget_instance_render(widget_instance *widget, Rectangle2D *dest, int32_t offset_xy,
                                   uint32_t flag1, int32_t flag2); // 0x49a8c0, as defined in widget_instance_render.c
extern void interface_draw_cursor(void); // 0x497380
extern void virtual_keyboard_render(void); // 0x4a9510
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect); // 0x449780, solid rectangle fill
    // blam-cc: EAX -> packed_color, ECX -> rect (objdump call sites 0x494d28, 0x4973f9, 0x498617)

// Draws all currently active full-screen UI widgets (root widget slot 0, matching
// widget_draw_split_screen_region's own eligibility test), the mouse cursor on top of them if any
// were drawn, and, once a network-wait alpha is fading in the 0..1 range, a fullscreen fade quad
// over everything. If the virtual keyboard is open, draws it (plus the cursor) instead of any of
// that.
void widget_draw_fullscreen_region(int16_t controller_index)
{
    uint8_t drew_any = 0;
    int32_t clamped_controller;
    int32_t i;

    last_controller_index_00879f50 = (controller_index == (int16_t)0xffff) ? 0 : controller_index;

    if (virtual_keyboard.active != 0) {
        virtual_keyboard_render();
        interface_draw_cursor();
        return;
    }

    clamped_controller = 0; // dead clamp for controller_index >= 0, proven in widget_draw_split_screen_region.c

    for (i = 0; i < 1; i++) {
        widget_instance *widget = ui_root_widget[i];

        if (widget == (widget_instance *)0) {
            continue;
        }
        if (widget->render_always == 1 ||
            (widget->unknown_15 == 1 &&
             (widget->controller_index == clamped_controller || widget->controller_index == -1 ||
              ui_split_screen != 0)) ||
            (widget->unknown_15 != 1 &&
             ((widget->controller_index == -1 && i == 0) || widget->controller_index == clamped_controller))) {
            Rectangle2D dest = {0, 0, 0x1e0, 0x280};

            widget_instance_render(widget, &dest, 0, 1, 0); // objdump 0x498553: offset_xy built from two zero words
            drew_any = 1;
        }
    }

    if (drew_any != 0) {
        interface_draw_cursor();
    }
    if (0.0f <= ui_unknown_718fa8 && (ui_unknown_718fa8 < 1.0f) != (ui_unknown_718fa8 == 1.0f)) {
        Rectangle2D rect = {0, 0, 0x1e0, 0x280};
        int32_t fade_color;

        if (0.95f <= ui_unknown_718fa8) {
            ui_unknown_718fa8 = 1.0f;
        }
        fade_color = (int32_t)(ui_unknown_718fa8 * 255.0f + 0.5f); // ROUND()
        ui_draw_filled_rectangle((uint32_t)fade_color, &rect);
    }
}

#if 0
Original Ghidra decompilation (0x4984c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004984c0(void)

{
  ushort uVar1;
  int iVar2;
  bool bVar3;
  ushort in_AX;
  int iVar4;
  ushort uVar5;
  int local_10;
  undefined2 local_c;
  undefined2 local_a;
  undefined2 local_8;
  undefined2 local_6;
  undefined2 local_4;
  undefined2 local_2;

  _DAT_00879f50 = (in_AX == 0xffff) - 1 & in_AX;
  if (DAT_007193a8 == '\0') {
    bVar3 = false;
    if ((short)in_AX < 0) {
      uVar5 = 0;
    }
    else {
      uVar5 = (0 < (short)in_AX) - 1 & in_AX;
    }
    iVar4 = 0;
    do {
      iVar2 = (&DAT_00718f94)[iVar4];
      if (iVar2 == 0) goto LAB_00498580;
      if (*(char *)(iVar2 + 0x11) == '\x01') {
LAB_00498546:
        local_10 = 0;
        local_a = 0x280;
        local_c = 0x1e0;
        widget_instance_render(iVar2,&local_10,0,1,0);
        bVar3 = true;
      }
      else {
        uVar1 = *(ushort *)(iVar2 + 8);
        if (*(char *)(iVar2 + 0x15) == '\x01') {
          if (((uVar1 == uVar5) || (uVar1 == 0xffff)) ||
             ((uVar5 == 0xffff || (DAT_00718fc9 != '\0')))) goto LAB_00498546;
        }
        else if (((uVar1 == 0xffff) && (iVar4 == 0)) || (uVar1 == uVar5)) goto LAB_00498546;
      }
LAB_00498580:
      iVar4 = iVar4 + 1;
    } while (iVar4 < 1);
    if (bVar3) {
      interface_draw_cursor();
    }
    if ((0.0 <= _DAT_00718fa8) && (_DAT_00718fa8 < 1.0 != (_DAT_00718fa8 == 1.0))) {
      local_6 = 0;
      local_2 = 0x280;
      local_8 = 0;
      local_4 = 0x1e0;
      if (0.95 <= _DAT_00718fa8) {
        _DAT_00718fa8 = 1.0;
      }
      local_10 = (int)ROUND(_DAT_00718fa8 * 255.0);
      FUN_00449780();
      return;
    }
  }
  else {
    virtual_keyboard_render();
    interface_draw_cursor();
  }
  return;
}
#endif
