// credits_load_directly_for_endgame  (Ghidra: credits_load_directly_for_endgame, already named)
// address 0x4c8d40, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: matches the given name exactly. current_profile_index (0x00714dd4) and
// profile_globals_block (0x00712dd8) reuse src/interface/FUN_0049ce00.c and FUN_0049cc80.c's
// names; player_profile_write_data's signature reuses src/saved_games/player_profile_write_data.c.
// The profile flags field (0x00712ef4 = 0x00712dd8 + 0x11c) is
// saved_player_profile.flags (types/saved_games.h); bit 0x0004 is
// _saved_player_profile_end_credits_reached_bit, which unlocks console context 0x20
// (FUN_004c69c0's console_command_context_flags _console_context_unknown_20). Confirmed against
// objdump -d -M intel bin/halo.exe at 0x4c8d40..0x4c8d96: the profile flags update is a BYTE
// `or`, matching bit 0x0004 living in the field's low byte; the tag group for
// "ui\\shell\\main_menu\\main_menu" is 'DeLa' (mov edi,0x44654c61), the same widget-definition
// group src/interface/chimera__load_ui_widget.c's own tag_lookup call uses.
// register convention: cdecl, no parameters.
// reconciled: R22 saved_player_profile_flags gains _saved_player_profile_end_credits_reached_bit (0x0004); the literal 4 now uses it

// phase 4 review (disassembly 0x4c8d40..0x4c8d9a): 0x00714dd4 is profile_globals_block[0].handle
// and 0x00719230 is a DWORD store (the phase 3 file wrote one byte).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "saved_games.h"
#include "main.h"

extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8,
    // foreign (saved_games); slot 0 handle at 0x00714dd4 (+0x1ffc)
extern int32_t hud_text_message_cycle_state_00719230; // 0x00719230, foreign, TYPES-GAP (interface module); DWORD store

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, foreign (cache module)
    // blam-cc: EDI -> group, stack -> path
extern void player_profile_write_data(int32_t handle, saved_player_profile *profile); // 0x53a950, foreign (saved_games module)
extern void main_menu_return_and_reset(void); // 0x4c8a60, this module
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, foreign (interface module)

// Marks the current profile as having reached the end credits, saves it if a profile is
// currently selected, returns to the main menu, and immediately shows the end-game credits
// screen widget (with the main menu's own widget as its history entry, so backing out returns
// there) and marks the HUD text message cycle as active.
void credits_load_directly_for_endgame(void)
{
    datum_index main_menu_tag;

    profile_globals_block[0].profile.flags = profile_globals_block[0].profile.flags | _saved_player_profile_end_credits_reached_bit; // byte OR at 0x00712ef4
    if (profile_globals_block[0].handle != -1) {
        player_profile_write_data(profile_globals_block[0].handle, &profile_globals_block[0].profile);
    }
    main_menu_return_and_reset();
    main_menu_tag = tag_lookup(0x44654c61 /* 'DeLa' */, "ui\\shell\\main_menu\\main_menu");
    chimera__load_ui_widget("ui\\shell\\main_menu\\credits_screen", (datum_index)-1,
                             (widget_instance *)0, (uint16_t)-1, main_menu_tag, (datum_index)-1,
                             -1);
    hud_text_message_cycle_state_00719230 = 1;
}

#if 0
Original Ghidra decompilation (0x4c8d40):

void __cdecl credits_load_directly_for_endgame(void)

{
  undefined4 uVar1;

  DAT_00712ef4 = DAT_00712ef4 | 4;
  if (DAT_00714dd4 != -1) {
    player_profile_write_data(DAT_00714dd4,&DAT_00712dd8);
  }
  main_menu_return_and_reset();
  uVar1 = tag_lookup("ui\\shell\\main_menu\\main_menu");
  chimera__load_ui_widget
            ("ui\\shell\\main_menu\\credits_screen",0xffffffff,0,0xffffffff,uVar1,0xffffffff,
             0xffffffff);
  DAT_00719230 = 1;
  return;
}
#endif
