// restart_map_dialog_choice_handler  (Ghidra: restart_map_dialog_choice_handler, already named)
// address 0x4a30e0, size 103 bytes, callers=0 in this build
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: matches the given name; functions.md: "Handles the two-choice restart-map
// confirmation dialog, restarting the current map unless the game has already ended." Reuses
// widget_close_all (already established) and chimera__console_out's established varargs
// signature. types/game.h names 0x0087aa10 game_engine_state_value, types/networking.h/game.h
// names 0x00719720 network_game_mode.
// register convention: cdecl, the one recognized stack parameter (widget -- the clicked button).
// UNSURE: this matches the shape of a ui_event_function callback but takes only one parameter
// here (event/out_handled are not read), and identifies "which choice was clicked" by comparing
// the widget pointer itself against its parent's first and second child -- i.e. widget IS the
// clicked button, not the dialog root.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int16_t network_game_mode;              // 0x00719720
extern game_engine_state game_engine_state_value; // 0x0087aa10

extern void game_engine_player_profile_cache_sync_all(int32_t unknown); // 0x466cb0, UNSURE args
extern void game_engine_reset_round_objects(void); // 0x468260, UNSURE
extern void game_engine_send_round_reset_message(void); // 0x4682c0, UNSURE
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50
extern void widget_close_all(void); // 0x498650

// If `widget` is the dialog's first choice button, closes all widgets and, if hosting and the
// game has not ended, restarts the current map (else logs a console message); returns 1 for
// either recognized choice button, 0 otherwise.
uint32_t restart_map_dialog_choice_handler(widget_instance *widget)
{
    widget_instance *first_choice = widget->parent->first_child->next_sibling;
    uint32_t handled = 0;

    if (widget == first_choice) {
        widget_close_all();
        if (network_game_mode == 2) {
            if (game_engine_state_value == 0) {
                game_engine_reset_round_objects();
                game_engine_send_round_reset_message();
                game_engine_player_profile_cache_sync_all(-1);
            } else {
                chimera__console_out((ColorARGB *)0, (char *)"Cannot restart the map when the game is over.");
            }
        }
        handled = 1;
    }
    if (widget == first_choice->next_sibling) {
        handled = 1;
    }
    return handled;
}

#if 0
Original Ghidra decompilation (0x4a30e0):

undefined4 restart_map_dialog_choice_handler(int param_1)

{
  int iVar1;
  undefined4 uVar2;

  iVar1 = *(int *)(*(int *)(*(int *)(param_1 + 0x30) + 0x34) + 0x2c);
  uVar2 = 0;
  if (param_1 == iVar1) {
    widget_close_all();
    if (DAT_00719720 == 2) {
      if (DAT_0087aa10 == 0) {
        FUN_00468260();
        FUN_004682c0();
        FUN_00466cb0(0xffffffff);
      }
      else {
        chimera__console_out("Cannot restart the map when the game is over.");
      }
    }
    uVar2 = 1;
  }
  if (param_1 == *(int *)(iVar1 + 0x2c)) {
    uVar2 = 1;
  }
  return uVar2;
}
#endif
