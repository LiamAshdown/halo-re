// console_process_key_events  (Ghidra: FUN_004c65c0; renamed per out/phase4/main_types_notes.md,
//   which documents this address as "the console's queued key events for the frame")
// address 0x4c65c0, size 376 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: types/main.h console_globals (terminal.key_event_count/key_events, history ring,
// history_count/history_newest_index/history_browse_index); types/interface.h terminal_console,
// ui_key_event, text_edit_state; types/input.h input_key (_input_key_tab/_input_key_enter/
// _input_key_numpad_enter/_input_key_up/_input_key_down) and input_abstraction_globals
// (system_key_states[0] is the grave/console key, per its own comment "grave, escape and print");
// src/input/input_get_mouse_button_state.c documents this function (0x4c65c0, mislabeled
// "interface" there) as the one caller of that helper at 0x4c65fe; src/interface/chat_close.c
// names 0x006b3858 chat_dialog_open; src/interface/widget_text_edit_reset_length.c is FUN_0044c5b0.
// The up/down history browse arithmetic (case 0x4d falls into 0x4e) was decoded from Ghidra's
// bit-mask idioms with the help of the field comments already in main.h
// (history_browse_index: "+2 then -1 on up, -1 on down, clamped to count-1").
// register convention: __cdecl, no arguments; returns console_globals_data.active.
// phase 4 review (disassembly 0x4c65c0..0x4c6743 incl. the key jump table 0x4c6744/0x4c675c (6 paste, 0x1e tab, 0x38/0x66 enter, 0x4d/0x4e history): no drift.
// UNSURE: key code 6 (paste) has no name in types/input.h; the header notes F1..F10 are 1..10,
// so 6 would ordinarily be F6, but nothing here contradicts main_types_notes.md's reading of it
// as a dedicated paste shortcut, so it is left as a literal.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern console_globals console_globals_data;         // 0x006b7020
extern uint8_t chat_dialog_open;                 // 0x006b3858
extern input_abstraction_globals input_globals;  // 0x00710328 (system_key_states at +0x14a8)

extern uint8_t input_get_mouse_button_state(int16_t button_index); // 0x490e00
extern void console_toggle(void);                        // this module, 0x4c6530
extern void console_deactivate(void);                     // this module, 0x4c64b0
extern uint32_t console_paste_clipboard_text(void);        // this module, 0x4c6570
extern void console_autocomplete_command(void);            // this module, 0x4c6bc0
extern char console_process_command(char *command_line, uint32_t context_flags); // this module, 0x4c6a80, blam-cc: EDI -> command_line, stack -> context_flags
extern void widget_text_edit_reset_length(text_edit_state *state); // 0x44c5b0, blam-cc: ESI -> state

// Per-frame console input: toggles the console on the grave key, and while open, consumes every
// buffered key event of the frame (paste, tab-completion, Enter/close, and command history
// recall with Up/Down), submitting the input line and recording it once Enter is pressed.
// Returns whether the console ends the frame open.
uint8_t console_process_key_events(void)
{
    int16_t i;
    int16_t key_code;
    int16_t browse_index;
    int16_t history_index;

    if (console_globals_data.enabled != 0 && chat_dialog_open == 0) {
        if (input_globals.system_key_states[0] == 1) { // grave key just pressed
            console_toggle();
            return console_globals_data.active;
        }
        if (console_globals_data.active != 0) {
            if (input_get_mouse_button_state(2) == 1) { // middle mouse button
                console_paste_clipboard_text();
            }
            for (i = 0; i < console_globals_data.terminal.key_event_count; i++) {
                key_code = console_globals_data.terminal.key_events[i].key_code;
                switch (key_code) {
                case 6: // UNSURE: no name in types/input.h; treated as a dedicated paste key
                    console_paste_clipboard_text();
                    break;

                case _input_key_tab:
                    console_autocomplete_command();
                    break;

                case _input_key_enter:
                case _input_key_numpad_enter:
                    if (console_globals_data.terminal.input[0] == 0) {
                        console_deactivate();
                    } else {
                        console_process_command(console_globals_data.terminal.input, 0); // EDI -> input
                        console_globals_data.terminal.input[0] = 0;
                        console_globals_data.terminal.edit.cursor = 0;
                        console_globals_data.terminal.edit.selection_anchor = -1;
                    }
                    break;

                case _input_key_up:
                    console_globals_data.history_browse_index = console_globals_data.history_browse_index + 2;
                    // fall through
                case _input_key_down:
                    browse_index = console_globals_data.history_browse_index - 1;
                    if (browse_index < 1) {
                        browse_index = 0; // clamp to a floor of 0 (max(0, browse_index - 1))
                    }
                    if (browse_index > console_globals_data.history_count - 1) {
                        browse_index = console_globals_data.history_count - 1;
                    }
                    console_globals_data.history_browse_index = browse_index;
                    if (browse_index != -1) {
                        history_index = (int16_t)((console_globals_data.history_newest_index - browse_index + 8) & 7);
                        strcpy(console_globals_data.terminal.input, console_globals_data.history[history_index]);
                        widget_text_edit_reset_length(&console_globals_data.terminal.edit); // ESI -> &edit
                    }
                    break;
                }
            }
        }
    }
    return console_globals_data.active;
}

#if 0
Original Ghidra decompilation (0x4c65c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char FUN_004c65c0(void)

{
  char cVar1;
  short sVar2;
  uint uVar3;
  char *pcVar4;
  ushort uVar5;
  char *pcVar6;
  short sVar7;

  if ((DAT_006b7021 != '\0') && (DAT_006b3858 == '\0')) {
    if (DAT_007127d0 == '\x01') {
      console_toggle();
      return DAT_006b7020;
    }
    if (DAT_006b7020 != '\0') {
      cVar1 = FUN_00490e00(2);
      if (cVar1 == '\x01') {
        console_paste_clipboard_text();
      }
      sVar7 = 0;
      if (0 < DAT_006b7024) {
        do {
          switch((&DAT_006b7028)[sVar7 * 2]) {
          case 6:
            console_paste_clipboard_text();
            break;
          case 0x1e:
            console_autocomplete_command();
            break;
          case 0x38:
          case 0x66:
            if (DAT_006b70d8 == '\0') {
              console_deactivate();
            }
            else {
              console_process_command(0);
              DAT_006b70d8 = '\0';
              _DAT_006b71de = 0;
              _DAT_006b71e0 = 0xffff;
            }
            break;
          case 0x4d:
            DAT_006b79e0._0_2_ = (short)DAT_006b79e0 + 2;
          case 0x4e:
            uVar5 = ((short)((short)DAT_006b79e0 - 1U) < 1) - 1 & (short)DAT_006b79e0 - 1U;
            uVar3 = (int)DAT_006b79dc - 1;
            if ((int)(short)uVar5 <= (int)uVar3) {
              uVar3 = (uint)uVar5;
            }
            sVar2 = (short)uVar3;
            DAT_006b79e0 = CONCAT22(DAT_006b79e0._2_2_,sVar2);
            if (sVar2 != -1) {
              uVar3 = ((int)DAT_006b79de - (int)sVar2) + 8U & 0x80000007;
              if ((int)uVar3 < 0) {
                uVar3 = (uVar3 - 1 | 0xfffffff8) + 1;
              }
              pcVar4 = &DAT_006b71e4 + uVar3 * 0xff;
              pcVar6 = &DAT_006b70d8;
              do {
                cVar1 = *pcVar4;
                pcVar4 = pcVar4 + 1;
                *pcVar6 = cVar1;
                pcVar6 = pcVar6 + 1;
              } while (cVar1 != '\0');
              FUN_0044c5b0();
            }
          }
          sVar7 = sVar7 + 1;
        } while (sVar7 < DAT_006b7024);
      }
    }
  }
  return DAT_006b7020;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
