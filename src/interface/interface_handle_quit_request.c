// interface_handle_quit_request  (Ghidra: interface_handle_quit_request, already named)
// address 0x499170, size 125 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: matches the given name exactly; a force-quit flag short-circuits straight to
// ExitProcess, otherwise the function arms a "confirm quit" prompt -- one shape for split-screen
// (a small record at 0x00719754/0x0071973c this module does not otherwise name) and one shape
// for single-player (an error-dialog-like record at 0x00718fac..0x00718fb1, matching the same
// four-field pattern display_error and interface_tick's error_string_index/DAT_00718fae/
// DAT_00718fb0/DAT_00718fb1 use).
// register convention: no register-passed arguments.
// UNSURE: 0x00718fac is declared by types/interface.h as ui_player_help_string[3] (int16_t[3]),
// but types_notes.md's own "Unresolved offsets" section already flags that stride as
// unconfirmed; the four separate 1/2-byte fields written here (ac=int16, ae=int16, b0=byte,
// b1=byte) match interface_tick's display_error call shape instead, so they are named as a
// distinct record rather than forced through ui_player_help_string.
// TYPES-GAP: 0x00719754 and 0x0071973c are not documented anywhere in types/interface.h; named
// as anonymous externs with UNSURE field guesses (a pending-prompt string index and a
// controller/flag word).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t ui_force_quit;   // 0x00718fca
extern uint8_t ui_split_screen; // 0x00718fc9
extern int16_t network_join_error_code; // 0x00718fa4, reset to -1

// UNSURE: split-screen "confirm quit" prompt record, not documented elsewhere in this module.
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, low 16 bits of the DAT_00719754 dword
extern uint8_t network_join_error_reason; // 0x0071973c, byte stores only
extern uint8_t split_screen_quit_prompt_armed;  // 0x00719757, byte 3 of the DAT_00719754 dword

// UNSURE: see file header -- conflicts with types/interface.h's ui_player_help_string[3] at the
// same address; named separately rather than forced through that array.
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae;   // 0x00718fae
extern uint8_t quit_confirm_error_modal;        // 0x00718fb0
extern uint8_t quit_confirm_error_is_error;     // 0x00718fb1

extern void keystone_library_unload(void); // 0x542cf0, UNSURE module

// Either force-quits the process immediately (unloading the keystone/DRM library first) when
// ui_force_quit is set, or arms a "are you sure you want to quit" confirmation prompt: the
// split-screen shaped one when ui_split_screen is set, otherwise the single-player error-dialog
// shaped one (only if neither is already armed).
void interface_handle_quit_request(void)
{
    if (ui_force_quit != 0) {
        keystone_library_unload();
        ExitProcess(0xffffec7a); // does not return
    }
    if (ui_split_screen == 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = 0x23;
        }
        split_screen_quit_prompt_string = 0xffff;
        network_join_error_reason = 0;
        split_screen_quit_prompt_armed = 1;
        ui_force_quit = 0;
        return;
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x23;
        quit_confirm_error_unknown_ae = 0;
        quit_confirm_error_modal = 0;
        quit_confirm_error_is_error = 0;
    }
    ui_force_quit = 0;
}

#if 0
Original Ghidra decompilation (0x499170):

void interface_handle_quit_request(void)

{
  if (DAT_00718fca != '\0') {
    keystone_library_unload();
                    /* WARNING: Subroutine does not return */
    ExitProcess(0xffffec7a);
  }
  if (DAT_00718fc9 == '\0') {
    if (DAT_00718fa4 == -1) {
      DAT_00718fa4 = 0x23;
    }
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719754._3_1_ = 1;
    DAT_00718fca = '\0';
    return;
  }
  if (DAT_00718fac == -1) {
    DAT_00718fac = 0x23;
    DAT_00718fae = 0;
    DAT_00718fb0 = 0;
    DAT_00718fb1 = 0;
  }
  DAT_00718fca = '\0';
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
