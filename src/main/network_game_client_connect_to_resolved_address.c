// network_game_client_connect_to_resolved_address  (Ghidra:
// network_game_client_connect_to_resolved_address, already named)
// address 0x4c8660, size 224 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/main_types_notes.md pins main_globals_data.connect_address/connect_password/
// connect_pending (0x00719a7a/0x00719a9a/0x00719a79) and the interface module's loading-screen
// globals (0x0068e680/4/8, 0x00718f8c/90, 0x006b2f28/68), reused from
// src/networking/network_join_request_resolve_host.c under the same names, plus
// ui_unknown_718fa4 (0x00718fa4), reused from src/interface/interface_handle_quit_request.c.
// 0x00719754/0x0071973c/0x00719757 are, per main.h's own byte-width-confirmed field table,
// actually main_globals.switch_structure_bsp_index/save_map/return_to_main_menu (confirmed by
// main_menu_return_and_reset 0x4c8a60, which clears the same return_to_main_menu byte once it
// services the request) -- NOT the interface module's own less-informed "split_screen_quit_
// prompt_*" guess for the same addresses. Confirmed against objdump -d -M intel bin/halo.exe at
// 0x4c8660..0x4c873f: `mov eax,0x719a7a; call 0x4dc790` at 0x4c86e5/0x4c86ea is
// network_game_client_connect_to_address with EAX = &main_globals_data.connect_address (its
// established EAX -> address_string convention, per
// src/networking/network_join_request_resolve_host.c's file header), and the one stack argument
// pushed just before it (`lea edx,[esp+0x8]; push edx`) is the wide password buffer built in
// place on the stack just above.
// register convention: no register-passed arguments; called with main_globals_data.connect_address /
// phase 4 review (disassembly 0x4c8660..0x4c873f: the password widening loop and failure path match; 0x006b2f28 / 0x006b2f68 are WORD stores into progress_screen_text / progress_screen_subtext (the phase 3 file wrote dwords).
// connect_password already staged by this module's other connect functions.
// UNSURE: the widen-to-uint16 loop building the password buffer clamps its length to 9 code
// units (8 chars + terminator, matching k_main_connect_password_length) when the source string
// is longer, mirroring the strncpy(...,8) bound the two staging functions already enforce; kept
// exactly as decompiled.

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <string.h>

extern main_globals main_globals_data; // 0x00719700

// Foreign interface-module "loading screen" globals; see file header evidence.
extern int32_t interface_loading_screen_address_a;   // 0x0068e680
extern int32_t interface_loading_screen_address_b;   // 0x0068e684
extern int32_t interface_loading_screen_ui_state;    // 0x00718f8c
extern int32_t interface_loading_screen_progress;    // 0x00718f90
extern uint16_t progress_screen_text[0x20];    // 0x006b2f28, foreign (types/interface.h); WORD stores only
extern uint16_t progress_screen_subtext[0x20]; // 0x006b2f68, foreign (types/interface.h); WORD stores only
extern int32_t interface_loading_screen_request_id;  // 0x0068e688

extern int16_t network_join_error_code; // 0x00718fa4, foreign (interface module), -1 when none pending

extern void network_dispatch_initialize(void); // 0x4414c0, foreign (networking module)
extern void widget_close_all(void);            // 0x498650, foreign (interface module)
extern void main_menu_music_stop(void);        // 0x4c8b40, this module
extern uint32_t network_game_client_connect_to_address(char *address_string,
                                                        uint16_t *target_string); // 0x4dc790, foreign

// blam-cc: EAX -> connect_address (the function's own address, matching the
// network_game_client_connect_to_address convention)
// Resets the interface's loading-screen state, widens main_globals_data.connect_password to a
// uint16_t string (clamped to 8 code units), and attempts the connect. On failure, arms the
// "can't connect" prompt exactly as the two staging functions do; on success, closes the UI and
// stops the main menu music. Either way, clears the staged connect state before returning.
void network_game_client_connect_to_resolved_address(void)
{
    uint16_t wide_password[10];
    size_t length;
    size_t i;

    interface_loading_screen_address_a = -1;
    interface_loading_screen_address_b = -1;
    interface_loading_screen_ui_state = 0;
    interface_loading_screen_progress = 0;
    progress_screen_text[0] = 0;    // WORD store
    progress_screen_subtext[0] = 0; // WORD store
    interface_loading_screen_request_id = -1;
    network_dispatch_initialize();

    length = strlen(main_globals_data.connect_password);
    if (length > 8) {
        length = 8;
    }
    wide_password[length] = 0;
    for (i = length; i > 0; i--) {
        wide_password[i - 1] = (uint8_t)main_globals_data.connect_password[i - 1];
    }

    if (network_game_client_connect_to_address(main_globals_data.connect_address, wide_password) == 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = 0x35;
        }
        main_globals_data.switch_structure_bsp_index = -1;
        main_globals_data.save_map = 0;
        main_globals_data.return_to_main_menu = 1;
    } else {
        widget_close_all();
        main_menu_music_stop();
    }
    main_globals_data.connect_address[0] = 0;
    main_globals_data.connect_password[0] = 0;
    main_globals_data.connect_pending = 0;
}

#if 0
Original Ghidra decompilation (0x4c8660):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl network_game_client_connect_to_resolved_address(void)

{
  char *pcVar1;
  char cVar2;
  char *pcVar3;
  ushort local_14 [10];

  DAT_0068e680 = 0xffffffff;
  DAT_0068e684 = 0xffffffff;
  DAT_00718f8c = 0;
  DAT_00718f90 = 0;
  _DAT_006b2f28 = 0;
  _DAT_006b2f68 = 0;
  DAT_0068e688 = 0xffffffff;
  FUN_004414c0();
  pcVar1 = &DAT_00719a9a;
  do {
    pcVar3 = pcVar1;
    pcVar1 = pcVar3 + 1;
  } while (*pcVar3 != '\0');
  pcVar3 = pcVar3 + -0x719a9a;
  if (0x12 < (int)pcVar3 * 2 + 2U) {
    pcVar3 = (char *)0x8;
  }
  if ((int)pcVar3 * 2 + 2U < 0x13) {
    local_14[(int)pcVar3] = 0;
    pcVar1 = pcVar3 + -1;
    while (-1 < (int)pcVar1) {
      local_14[(int)(pcVar1 + -1 + 1)] = (ushort)(byte)pcVar1[0x719a9a];
      pcVar1 = pcVar1 + -1;
    }
  }
  cVar2 = network_game_client_connect_to_address(local_14);
  if (cVar2 == '\0') {
    if (DAT_00718fa4 == -1) {
      DAT_00718fa4 = 0x35;
    }
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719754._3_1_ = 1;
  }
  else {
    widget_close_all();
    main_menu_music_stop();
  }
  DAT_00719a7a = 0;
  DAT_00719a9a = 0;
  DAT_00719a79 = 0;
  return;
}
#endif
