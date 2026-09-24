// widget_draw_split_screen_region  (Ghidra: FUN_00498330, unnamed;
// out/phase2/results/interface_01.json names it widget_draw_split_screen_region, conf=0.35)
// address 0x498330, size 379 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: phase-2 evidence; builds a destination rect from the caller's own viewport
// (unaff_ESI, a Rectangle2D) zeroed to origin, picks one entry out of a local split-screen
// viewport-rect table by controller index, and renders the root widget into it via
// widget_instance_render, but the do/while loop bound (`iVar3 < 1`) means it only ever executes
// once, for slot 0 -- there is exactly one root widget on retail PC (types/interface.h:
// "ui_root_widget[1]").
// register convention: source viewport Rectangle2D in ESI (unaff_ESI), controller index in CX
// (in_CX), both unresolved register reads.
// blam-cc: ESI -> viewport, CX -> controller_index
// UNSURE: `(0 < (short)in_CX) - 1 & in_CX` (operator precedence: `((0<in_CX)-1) & in_CX`)
// evaluates to exactly 0 for every in_CX >= 0, and the in_CX < 0 branch sets it to 0 directly
// too -- proven algebraically, not a guess -- so the clamped controller index used throughout
// this function is unconditionally 0 regardless of the caller's in_CX. Written as the literal
// constant 0 rather than reproducing the (dead) clamp expression.
// TYPES-GAP/UNSURE: Ghidra's own frame analysis for this function is corrupted (it reports a
// 163834-dword phantom local, clearly a mis-decoded stack probe), so its `local_N` names could
// not be trusted; the 0x48-byte table below and the final indexing arithmetic were instead read
// directly from disassembly (0x498330..0x498499). That disassembly shows the table is read back
// as a 4-byte value (`mov ecx,[esp+edx*4+8]`, pushed straight into widget_instance_
// render's 3rd argument, which is the packed offset_xy), at byte offset `4*(controller + 4*local_player_globals->local_player_count)`
// from the table's own base -- which, for controller==0 (always true here, see the clamp note
// above) and local_player_count==0, is offset 0, a slot this function never explicitly initializes (it
// only ever writes offsets 0xc and up). Reproduced literally, uninitialized slot included, rather
// than "corrected" into a plausible-looking rect table; this path is unreachable in the
// non-split-screen retail configuration this whole codebase targets (types/interface.h:
// "ui_root_widget[1]"), so it was not runtime-verified.
// UNSURE: widget_instance_render's signature (0x49a8c0) is inferred purely from this call site
// and cross-checked when that function was itself rewritten in this session.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern widget_instance *ui_root_widget[1]; // 0x00718f94
extern uint8_t ui_split_screen;            // 0x00718fc9
extern player_globals *local_player_globals; // 0x0087a478 (types/game.h)

extern void widget_instance_render(widget_instance *widget, Rectangle2D *dest, int32_t offset_xy,
                                   uint32_t flag1, int32_t flag2); // 0x49a8c0, as defined in widget_instance_render.c

// Draws the current root widget clipped to one split-screen viewport region. Only ever runs for
// controller slot 0 (see header note); the destination rect is the caller's viewport shifted to
// the origin.
void widget_draw_split_screen_region(Rectangle2D *viewport, int16_t controller_index)
{
    // Raw table, laid out exactly as the disassembly writes it (byte offsets from F, the stack
    // position right after `push ebx`; matching `[esp+N]` at 0x498330..0x4983e1). Sized to 0x4c
    // to cover the highest offset written (0x48, a 2-byte write) -- see the UNSURE note above;
    // offsets 0x00..0x0b are never written by this function (bytes 0..3 are where the real
    // function's `push ebx` physically lands one frame up, per that same note).
    uint8_t table[0x4c] = {0};
    int32_t clamped_controller;
    int32_t i;

    *(int16_t *)(table + 0x22) = 0xf0;
    *(int16_t *)(table + 0x32) = 0xf0;
    *(int16_t *)(table + 0x34) = 0x140;
    *(int16_t *)(table + 0x36) = 0xf0;
    *(int16_t *)(table + 0x40) = 0x140;
    *(int16_t *)(table + 0x46) = 0xf0;
    *(int16_t *)(table + 0x48) = 0x140;
    *(int16_t *)(table + 0x4a) = 0xf0;

    if (virtual_keyboard.active != 0) {
        return;
    }

    clamped_controller = 0; // see UNSURE note in file header

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
            Rectangle2D dest;
            int32_t byte_offset = 4 * (clamped_controller + 4 * local_player_globals->local_player_count);
            int32_t offset_xy = *(int32_t *)(table + byte_offset); // packed {x, y} words, pushed as argument 3 (objdump 0x49848d)

            dest.top = 0;
            dest.left = 0;
            dest.bottom = viewport->bottom - viewport->top;
            dest.right = viewport->right - viewport->left;
            widget_instance_render(widget, &dest, offset_xy, 1, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x498330):

void FUN_00498330(void)

{
  ushort uVar1;
  int iVar2;
  ushort in_CX;
  int iVar3;
  short *unaff_ESI;
  ushort uVar4;
  undefined4 auStackY_a0050 [163834];
  undefined2 local_48;
  undefined2 local_46;
  short local_44;
  short local_42;
  undefined2 local_40;
  undefined2 local_3e;
  undefined2 local_3c;
  undefined2 local_3a;
  undefined2 local_38;
  undefined2 local_36;
  undefined2 local_34;
  undefined2 local_32;
  undefined2 local_30;
  undefined2 local_2e;
  undefined2 local_2c;
  undefined2 local_2a;
  undefined2 local_28;
  undefined2 local_26;
  undefined2 local_24;
  undefined2 local_22;
  undefined2 local_20;
  undefined2 local_1e;
  undefined2 local_1c;
  undefined2 local_1a;
  undefined2 local_18;
  undefined2 local_16;
  undefined2 local_14;
  undefined2 local_12;
  undefined2 local_10;
  undefined2 local_e;
  undefined2 local_c;
  undefined2 local_a;
  undefined2 local_8;
  undefined2 local_6;
  undefined2 local_4;
  undefined2 local_2;

  local_2a = 0xf0;
  local_1a = 0xf0;
  local_16 = 0xf0;
  local_6 = 0xf0;
  local_2 = 0xf0;
  local_40 = 0;
  local_3e = 0;
  local_3c = 0;
  local_3a = 0;
  local_38 = 0;
  local_36 = 0;
  local_34 = 0;
  local_32 = 0;
  local_30 = 0;
  local_2e = 0;
  local_2c = 0;
  local_28 = 0;
  local_26 = 0;
  local_24 = 0;
  local_22 = 0;
  local_20 = 0;
  local_1e = 0;
  local_1c = 0;
  local_18 = 0x140;
  local_14 = 0;
  local_12 = 0;
  local_10 = 0;
  local_e = 0;
  local_c = 0x140;
  local_a = 0;
  local_8 = 0;
  local_4 = 0x140;
  if (DAT_007193a8 == '\0') {
    if ((short)in_CX < 0) {
      uVar4 = 0;
    }
    else {
      uVar4 = (0 < (short)in_CX) - 1 & in_CX;
    }
    iVar3 = 0;
    do {
      iVar2 = (&DAT_00718f94)[iVar3];
      if (iVar2 == 0) goto LAB_004984a2;
      if (*(char *)(iVar2 + 0x11) == '\x01') {
LAB_00498459:
        local_42 = unaff_ESI[3] - unaff_ESI[1];
        local_44 = unaff_ESI[2] - *unaff_ESI;
        local_46 = 0;
        local_48 = 0;
        widget_instance_render
                  (iVar2,&local_48,
                   *(undefined4 *)
                    (&stack0xffffffb0 + ((int)(short)uVar4 + *(short *)(DAT_0087a478 + 0xc) * 4) * 4
                    ),1,0);
      }
      else {
        uVar1 = *(ushort *)(iVar2 + 8);
        if (*(char *)(iVar2 + 0x15) == '\x01') {
          if (((uVar1 == uVar4) || (uVar1 == 0xffff)) ||
             ((uVar4 == 0xffff || (DAT_00718fc9 != '\0')))) goto LAB_00498459;
        }
        else if (((uVar1 == 0xffff) && (iVar3 == 0)) || (uVar1 == uVar4)) goto LAB_00498459;
      }
LAB_004984a2:
      iVar3 = iVar3 + 1;
    } while (iVar3 < 1);
  }
  return;
}
#endif
