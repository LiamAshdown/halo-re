// main_switch_structure_bsp_and_notify  (Ghidra: FUN_004c9b60; still unnamed -> renamed)
// address 0x4c9b60, size 99 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/main_functions.md summary ("Clears pending HUD notification flags each
// frame and, if a queued HUD message id is set, fetches and displays that message"), combined
// with out/phase4/main_types_notes.md's field table: "0x054 switch_structure_bsp_index: WORD;
// hs 0x474c33 writes, FUN_004c9b60 0x4c9b60 reads it (dword load, si used) and resets -1" --
// this function both performs the queued structure BSP switch and dispatches a queued HUD
// message. hud_get_message_string and chimera__hud_message reuse
// src/interface/hud_display_loading_message.c and src/interface/chimera__hud_message.c's
// established signatures.
// register convention: cdecl, no parameters.
// phase 4 review (disassembly 0x4c9b60..0x4c9bc2): the bsp switch takes SI =
// switch_structure_bsp_index, and the HUD message goes to local player 0 only when
// player_globals->local_players[0] is set, -1 otherwise (the phase 3 file dropped both).
// UNSURE: 0x0071941c (declared `viewport_globals` in src/game/game_engine_post_rasterize_post_
// game.c and `HUDGlobals *hud_globals_tag_data` in src/interface/chimera__motion_sensor_
// update.c) and its +0x3da field (a message id, per the int16_t read/-1 sentinel and the
// following hud_get_message_string(message_id) call), and the 0x006b3a40 byte-clear loop's
// stride-0x8c/4-entries shape (offset +0x82 of each), have no further documentation anywhere;
// declared here as opaque raw pointers rather than modeled fields.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "main.h"
#include "fn_scenario.h"
#include <wchar.h>

extern HUDGlobals *hud_globals_tag_data;   // 0x0071941c, foreign (the hud globals tag)
extern uint8_t *hud_messaging;             // 0x006b3a40, foreign (interface module), TYPES-GAP
extern main_globals main_globals_data;     // 0x00719700


    // blam-cc: SI -> structure_bsp_index (0x4c9b61 mov esi,[0x719754])
extern player_globals *local_player_globals; // 0x0087a478, foreign (game module)
extern uint16_t *hud_get_message_string(int32_t message_index); // 0x4aa3f0, foreign (interface module)
    // blam-cc: EDX -> message_index (0x4c9bae movsx edx,dx)
extern void chimera__hud_message(int16_t local_player_index, const wchar_t *text); // 0x4ae180, foreign (interface module)
    // blam-cc: AX -> local_player_index, stack -> text

// Performs the queued structure BSP switch, resets switch_structure_bsp_index, clears a small
// per-message "active" byte in each of 4 stride-0x8c hud_messaging entries, and if a HUD message
// id is queued (hud_globals_tag_data->loading_end_text != -1), fetches and displays it (for local player 0 when one exists).
void main_switch_structure_bsp_and_notify(void)
{
    int16_t message_id;
    uint8_t *entry;
    int32_t i;

    scenario_structure_bsp_switch(main_globals_data.switch_structure_bsp_index);
    main_globals_data.switch_structure_bsp_index = -1;

    message_id = (int16_t)hud_globals_tag_data->loading_end_text;

    entry = hud_messaging + 0x82;
    for (i = 4; i != 0; i--) {
        *entry = 0;
        entry = entry + 0x8c;
    }

    if (message_id != -1) {
        int16_t local_player_index = -1;
        uint16_t *text;

        if (local_player_globals->local_players[0] != (datum_index)-1) {
            local_player_index = 0;
        }
        text = hud_get_message_string(message_id);
        chimera__hud_message(local_player_index, (const wchar_t *)text);
    }
}

#if 0
Original Ghidra decompilation (0x4c9b60):

void FUN_004c9b60(void)

{
  short sVar1;
  undefined1 *puVar2;
  undefined4 uVar3;
  int iVar4;

  scenario_structure_bsp_switch();
  DAT_00719754._0_2_ = 0xffff;
  sVar1 = *(short *)(DAT_0071941c + 0x3da);
  puVar2 = (undefined1 *)(DAT_006b3a40 + 0x82);
  iVar4 = 4;
  do {
    *puVar2 = 0;
    puVar2 = puVar2 + 0x8c;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  if (sVar1 != -1) {
    uVar3 = hud_get_message_string();
    chimera__hud_message(uVar3);
  }
  return;
}
#endif
