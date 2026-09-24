// console_draw_overlay  (Ghidra: console_draw_overlay, already named)
// address 0x496730, size 835 bytes
// name confidence: 0.7   rewrite confidence: 0.45
// evidence: out/phase4/interface_functions.md "Renders the developer console overlay: the
// current input line with a text caret, plus the list of recent messages."; types/interface.h
// terminal_console ("console_draw_overlay terminates prompt at 0xb3 and input at 0x1b3") and
// console_message (text/color/age layout); Globals::interface_bitmaps -> GlobalsInterfaceBitmaps
// -> font_terminal.tag_id and the shared hud_text_draw_* globals, both already named by
// src/game/hud_draw_world_relative_text.c.
// register convention: none (void).
// UNSURE: the prompt+input concatenation and the message-text draw are reproduced here as
// strcpy/strcat and a direct field read; Ghidra shows both as manual byte/dword copy loops over
// raw pointer arithmetic (see the #if 0 block) that reduce to exactly those library calls.
// UNSURE: `local_124 = DAT_006e4744;` in the message loop writes a stack slot (local_124) that
// is never read again in this function (the message text is drawn from the console_message
// record directly, not through the local_120 buffer the input line used) -- a dead leftover of
// register/stack-slot reuse, dropped here as a true no-op.
// UNSURE: the exact meaning of the two background-box constants stored for a command-echo
// message (background_mode=3, 0x006e474a/474e) is not resolved beyond "boxed highlight for
// echoed commands"; kept as the literal values the disassembly stores.

// Phase-4 review: 0x006e4738 is the float alpha of the text color; it is copied, not converted to
// int (objdump 0x49685d, 0x496a11).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern Globals *global_globals;           // 0x00746fa0
extern tag_instance *tag_instances;       // 0x0087bc14
extern uint8_t terminal_initialized;       // 0x006b2efc
extern terminal_console *console_active;   // 0x006b2f0c
extern uint8_t console_caret_visible;      // 0x006b2f10
extern uint8_t console_show_messages;      // 0x0068e670
extern datum_index console_message_head;   // 0x006b2f04
extern data_array *terminal_messages;      // 0x006b2f00, "terminal output"

extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734
extern int16_t hud_text_draw_column;          // 0x006e4736
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730, always zeroed here
extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c
extern float hud_text_draw_color_a;           // 0x006e4738, alpha (ColorARGB order); float bits, stored with mov
extern float hud_text_draw_color_r;           // 0x006e473c
extern float hud_text_draw_color_g;           // 0x006e4740
extern float hud_text_draw_color_b;           // 0x006e4744
extern int16_t hud_text_draw_background_mode; // 0x006e4748
extern uint32_t hud_text_draw_box_field_474a; // 0x006e474a, UNSURE: background box geometry
extern uint32_t hud_text_draw_box_field_474e; // 0x006e474e, UNSURE: background box geometry

extern void chimera__draw_8_bit_text(int32_t x, int32_t y, const char *text); // 0x5148b0

// Draws the developer console overlay. When the terminal has been initialized: if a console is
// active, builds "prompt + input" into a scratch line, splices in a caret glyph (0x7f) at the
// cursor position when the caret is currently visible, and draws it through the globals
// font_terminal font; then, if message display is enabled, draws each live console_message
// (newest first) climbing up the screen one line-height at a time, fading each one's alpha by
// its age and boxing command-echo messages, until running out of vertical room.
void console_draw_overlay(void)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    int32_t font_terminal_id;
    Font *font;
    int16_t line_height;
    char line[288];
    int32_t cursor;
    datum_index message_handle;
    console_message *message;
    float fade;
    int16_t y;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    font_terminal_id = *(int32_t *)&interface_bitmaps->font_terminal.tag_id;

    if (terminal_initialized == 0) {
        return;
    }

    font = (Font *)tag_instances[(uint16_t)font_terminal_id].data;
    line_height = font->ascending_height + font->descending_height + font->leading_height;

    if (console_active != (terminal_console *)0) {
        console_active->prompt[0x1f] = '\0';
        console_active->input[0xff] = '\0';
        strcpy(line, console_active->prompt);
        strcat(line, console_active->input);

        hud_text_draw_color_a = console_active->color.alpha; // objdump 0x49685d: dword copy, no conversion
        hud_text_draw_color_r = console_active->color.red;
        hud_text_draw_color_g = console_active->color.green;
        hud_text_draw_color_b = console_active->color.blue;
        hud_text_draw_color_or_flags = 0xffff;
        hud_text_draw_column = 0;
        hud_text_draw_unknown_4730 = 0;

        if (console_caret_visible != 0) {
            cursor = console_active->edit.cursor + (int16_t)strlen(console_active->prompt);
            if (line[cursor] == '\0') {
                line[cursor + 1] = '\0';
            }
            line[cursor] = '\x7f';
        }

        hud_text_draw_font_tag_id = font_terminal_id;
        chimera__draw_8_bit_text(0, 0, line);
    }

    if (console_show_messages != 0) {
        y = 0x1e0 - line_height;
        message_handle = console_message_head;
        while (message_handle != (datum_index)0xffffffff && y != line_height && y - line_height >= 0) {
            message = (console_message *)((char *)terminal_messages->data +
                                           (uint16_t)message_handle * sizeof(console_message));
            hud_text_draw_color_r = message->color.red;
            hud_text_draw_color_g = message->color.green;
            hud_text_draw_color_b = message->color.blue;
            fade = 4.0f - (float)message->age * 0.033333335f;
            if (fade < 0.0f) {
                fade = 0.0f;
            } else if (fade > 1.0f) {
                fade = 1.0f;
            }
            hud_text_draw_color_a = fade * message->color.alpha; // objdump 0x496a11: float store, no conversion
            y = y - line_height;
            if (message->is_command_echo != 0) {
                hud_text_draw_background_mode = 3;
                hud_text_draw_box_field_474a = 0x014000a0;
                hud_text_draw_box_field_474e = 0x000001d6;
            }
            hud_text_draw_color_or_flags = 0xffff;
            hud_text_draw_column = 0;
            hud_text_draw_unknown_4730 = 0;
            hud_text_draw_font_tag_id = font_terminal_id;
            chimera__draw_8_bit_text(0, 0, message->text);
            hud_text_draw_background_mode = 0;
            message_handle = message->next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x496730):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void console_draw_overlay(void)

{
  char cVar1;
  uint uVar2;
  float fVar3;
  char *pcVar4;
  char *pcVar5;
  char *pcVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  short sVar10;
  char *pcVar11;
  short sVar12;
  char *pcVar13;
  undefined4 local_124;
  char local_120 [288];

  iVar7 = DAT_006b2f0c;
  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar9 = 0;
  }
  else {
    iVar9 = *(int *)(DAT_00746fa0 + 0x144);
  }
  uVar2 = *(uint *)(iVar9 + 0x1c);
  if (DAT_006b2efc != '\0') {
    iVar9 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar10 = *(short *)(iVar9 + 8) + *(short *)(iVar9 + 6) + *(short *)(iVar9 + 4);
    if (DAT_006b2f0c != 0) {
      pcVar6 = (char *)(DAT_006b2f0c + 0x94);
      local_120[0] = '\0';
      *(undefined1 *)(DAT_006b2f0c + 0xb3) = 0;
      pcVar4 = pcVar6;
      do {
        cVar1 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar1 != '\0');
      pcVar13 = (char *)((int)&local_124 + 3);
      do {
        pcVar11 = pcVar13 + 1;
        pcVar13 = pcVar13 + 1;
      } while (*pcVar11 != '\0');
      pcVar11 = pcVar6;
      for (uVar8 = (uint)((int)pcVar4 - (int)pcVar6) >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
        *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
        pcVar11 = pcVar11 + 4;
        pcVar13 = pcVar13 + 4;
      }
      pcVar5 = local_120;
      for (uVar8 = (int)pcVar4 - (int)pcVar6 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
        *pcVar13 = *pcVar11;
        pcVar11 = pcVar11 + 1;
        pcVar13 = pcVar13 + 1;
      }
      *(undefined1 *)(iVar7 + 0x1b3) = 0;
      do {
        cVar1 = *pcVar5;
        pcVar5 = pcVar5 + 1;
      } while (cVar1 != '\0');
      sVar12 = (short)pcVar5 - ((short)local_120 + 1);
      pcVar6 = (char *)(iVar7 + 0xb4);
      iVar9 = (int)sVar12 - (int)pcVar6;
      do {
        cVar1 = *pcVar6;
        (local_120 + iVar9)[(int)pcVar6] = cVar1;
        pcVar6 = pcVar6 + 1;
      } while (cVar1 != '\0');
      DAT_006e4738 = *(float *)(iVar7 + 0x84);
      DAT_006e473c = *(undefined4 *)(iVar7 + 0x88);
      DAT_006e4740 = *(undefined4 *)(iVar7 + 0x8c);
      DAT_006e4744 = *(undefined4 *)(iVar7 + 0x90);
      DAT_006e4734._0_2_ = 0xffff;
      DAT_006e4734._2_2_ = 0;
      _DAT_006e4730 = 0;
      if (DAT_006b2f10 != '\0') {
        iVar7 = (int)(short)(*(short *)(iVar7 + 0x1ba) + sVar12);
        if (local_120[iVar7] == '\0') {
          local_120[iVar7 + 1] = '\0';
        }
        local_120[iVar7] = '\x7f';
      }
      DAT_006e472c = uVar2;
      chimera__draw_8_bit_text(0,0,local_120);
    }
    if (DAT_0068e670 != '\0') {
      sVar12 = 0x1e0 - sVar10;
      uVar8 = DAT_006b2f04;
      while ((uVar8 != 0xffffffff && ((int)sVar12 != (int)sVar10 && -1 < (int)sVar12 - (int)sVar10))
            ) {
        iVar7 = (uVar8 & 0xffff) * 0x124 + *(int *)(DAT_006b2f00 + 0x34);
        DAT_006e473c = *(undefined4 *)(iVar7 + 0x114);
        DAT_006e4740 = *(undefined4 *)(iVar7 + 0x118);
        DAT_006e4744 = *(undefined4 *)(iVar7 + 0x11c);
        fVar3 = 4.0 - (float)*(int *)(iVar7 + 0x120) * 0.033333335;
        if (0.0 <= fVar3) {
          if (1.0 < fVar3) {
            fVar3 = 1.0;
          }
        }
        else {
          fVar3 = 0.0;
        }
        DAT_006e4738 = fVar3 * *(float *)(iVar7 + 0x110);
        sVar12 = sVar12 - sVar10;
        if (*(char *)(iVar7 + 0xc) != '\0') {
          DAT_006e4748 = 3;
          _DAT_006e474a = 0x14000a0;
          _DAT_006e474e = 0x1d6;
        }
        DAT_006e4734._0_2_ = 0xffff;
        DAT_006e4734._2_2_ = 0;
        _DAT_006e4730 = 0;
        DAT_006e472c = uVar2;
        local_124 = DAT_006e4744;
        chimera__draw_8_bit_text(0,0,iVar7 + 0xd);
        DAT_006e4748 = 0;
        uVar8 = *(uint *)(iVar7 + 8);
      }
    }
  }
  return;
}
#endif
