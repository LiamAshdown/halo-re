// player_profile_mark_level_visited_and_select  (Ghidra: FUN_00539d50, renamed)
// address 0x539d50, size 162 bytes
// name confidence: 0.35   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md summary "Switches the active player profile
// like player_profile_select, but also flags the current scenario/map as visited in the
// profile before saving." abStack_1eea sits 0x11e bytes above local_2008 in Ghidra's frame,
// matching saved_player_profile::campaign_progress (offset 0x11e, 10 bytes, "per level, bit n
// set = finished on difficulty n" per types/saved_games.h); the difficulty byte comes from
// main_game_globals+0xe, matching game_state_build_header's own read of the same field.
// register convention: __cdecl; local_player_index is the recognized stack parameter.
// Phase 4 review: the tail call to player_profile_load (0x495970) passes local_player_index in
// AX and &profile in EDX plus the handle on the stack (objdump 0x539dd8..0x539de3), matching
// src/interface/player_profile_load.c; the first rewrite dropped both register arguments.
// UNSURE: unlike the sibling player_profile_select_local_slot (0x539cb0), this function does
// not check current_level != -1 before indexing campaign_progress[current_level] -- reproduced
// literally; if campaign_level_find_index_for_path can return -1 here this indexes one byte before the array
// (identical to the original binary's own behaviour, not a rewrite bug).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"
#include "fn_interface.h"

extern char unknown_00719779[]; // 0x00719779, UNSURE: current scenario/level name buffer
extern game_main_globals *main_game_globals; // 0x006b0b80
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8

extern int16_t campaign_level_find_index_for_path(char *scenario_name); // 0x4c8b90, not in this module; one stack argument, campaign level index or -1
extern void player_profile_write_data(int32_t handle, saved_player_profile *profile); // 0x53a950


void player_profile_mark_level_visited_and_select(int16_t local_player_index)
{
    int16_t current_level;
    int16_t difficulty;
    int32_t handle;
    saved_player_profile profile;

    current_level = campaign_level_find_index_for_path(unknown_00719779);
    difficulty = main_game_globals->difficulty;

    if (local_player_index < 0 || 1 <= local_player_index) {
        return;
    }
    handle = profile_globals_block[local_player_index].handle;
    if (handle == -1) {
        return;
    }

    profile = profile_globals_block[local_player_index].profile;
    profile.campaign_progress[current_level] |= (uint8_t)(1 << (difficulty & 0x1f));
    player_profile_write_data(handle, &profile);
    player_profile_load(local_player_index, &profile, handle);
}

#if 0
Original Ghidra decompilation (0x539d50):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00539d50(short param_1)

{
  undefined2 uVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 local_2008 [71];
  byte abStack_1eea [7902];
  undefined4 uStack_c;

  uStack_c = 0x539d60;
  sVar3 = FUN_004c8b90(&DAT_00719779);
  uVar1 = *(undefined2 *)(DAT_006b0b80 + 0xe);
  if ((-1 < param_1) && (param_1 < 1)) {
    iVar2 = (&DAT_00714dd4)[param_1 * 0x801];
    if (iVar2 != -1) {
      puVar5 = &DAT_00712dd8 + param_1 * 0x801;
      puVar6 = local_2008;
      for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      abStack_1eea[sVar3] = abStack_1eea[sVar3] | '\x01' << ((byte)uVar1 & 0x1f);
      player_profile_write_data(iVar2,local_2008);
      player_profile_load(iVar2);
    }
  }
  return;
}
#endif
