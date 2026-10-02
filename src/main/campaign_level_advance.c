// campaign_level_advance  (Ghidra: campaign_level_advance, already named)
// address 0x4c9bd0, size 153 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: matches the given name exactly. campaign_level_paths[10] (0x00696574) is main.h's
// own documented global ("levels a10 .. d40 scenario paths"); campaign_level_find_index_for_path
// (FUN_004c8b90) is this module's own function; player_profile_mark_level_visited_and_select
// (FUN_00539d50) reuses src/saved_games/player_profile_mark_level_visited_and_select.c's
// established signature. 0x00719754/0x0071973c/0x00719757 are main_globals.switch_structure_bsp_
// index/save_map/return_to_main_menu (see src/main/main_queue_map_change.c's correction, not the
// interface module's less-informed guess for the same overlapping addresses).
// register convention: cdecl, no parameters.
// phase 4 review (disassembly 0x4c9bd0..0x4c9c73 (0x4c9c60 is its tail, 0x4c9c6b its credits branch): no drift.

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern main_globals main_globals_data;       // 0x00719700
extern char *campaign_level_paths[k_main_campaign_level_count]; // 0x00696574
extern int16_t local_player_count;           // 0x006894b8, foreign (saved_games module)

extern int campaign_level_find_index_for_path(char *path); // 0x4c8b90, this module
extern void player_profile_mark_level_visited_and_select(int16_t local_player_index); // 0x539d50, foreign (saved_games module)
extern void credits_load_directly_for_endgame(void); // 0x4c8d40, this module
extern void main_queue_map_change(char *map_name);   // 0x4c8740, this module

// Determines the next single-player campaign level after finishing the current one (by finding
// the current scenario's campaign index and advancing by one), marks it visited for every local
// player, and either queues it to load, rolls the end credits (once past the last level), or --
// in the unreachable-in-practice fallback -- returns to the main menu.
void campaign_level_advance(void)
{
    int16_t next_index;
    int16_t i;

    main_globals_data.return_to_main_menu = 1;
    main_globals_data.won_map = 0;

    next_index = (int16_t)(campaign_level_find_index_for_path(main_globals_data.scenario_path) + 1);
    if (next_index > 9) {
        next_index = -1;
    }

    for (i = 0; i < local_player_count; i++) {
        player_profile_mark_level_visited_and_select(i);
    }

    if (next_index == -1) {
        credits_load_directly_for_endgame();
        return;
    }
    if (next_index >= 0 && next_index < 10) {
        main_queue_map_change(campaign_level_paths[next_index]);
        main_globals_data.restore_checkpoint_on_load = 0;
        return;
    }

    // Unreachable given the clamp above; preserved verbatim.
    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.save_map = 0;
    main_globals_data.return_to_main_menu = 1;
}

#if 0
Original Ghidra decompilation (0x4c9bd0):

void __cdecl campaign_level_advance(void)

{
  short sVar1;
  int iVar2;

  DAT_00719754._3_1_ = 1;
  DAT_0071974e = 0;
  sVar1 = FUN_004c8b90(&DAT_00719779);
  sVar1 = sVar1 + 1;
  if (9 < sVar1) {
    sVar1 = -1;
  }
  iVar2 = 0;
  if (0 < DAT_006894b8) {
    do {
      FUN_00539d50(iVar2);
      iVar2 = iVar2 + 1;
    } while ((short)iVar2 < DAT_006894b8);
  }
  if (sVar1 == -1) {
    credits_load_directly_for_endgame();
    return;
  }
  if ((-1 < sVar1) && (sVar1 < 10)) {
    main_queue_map_change();
    DAT_00719778 = 0;
    return;
  }
  DAT_00719754._0_2_ = 0xffff;
  DAT_0071973c = 0;
  DAT_00719754._3_1_ = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
