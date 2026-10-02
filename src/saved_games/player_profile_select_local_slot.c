// player_profile_select_local_slot  (Ghidra: FUN_00539cb0, renamed)
// address 0x539cb0, size 152 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md summary "Switches the active player profile to
// the given local slot, reloading it if its cached copy is stale." local_1ee0 sits 0x128 bytes
// above local_2008 in Ghidra's frame, matching saved_player_profile::last_campaign_level
// (offset 0x128); local_2008 is a full 0x1ffc-byte copy of profile_globals_block[slot]
// .profile despite its truncated Ghidra array declaration. The `uStack_c = 0x539cc0` line is
// the same harmless __chkstk-frame decompiler artifact seen elsewhere in this module (never
// read). Only slot 0 is ever reachable (param_1 < 1 guard, matching
// k_maximum_local_player_profiles).
// register convention: __cdecl; local_player_index is the recognized stack parameter.
// Phase 4 review: the tail call to player_profile_load (0x495970) passes local_player_index in
// AX and &profile in EDX plus the handle on the stack (objdump 0x539d2c..0x539d39), matching
// src/interface/player_profile_load.c; the first rewrite dropped both register arguments.
// campaign_level_find_index_for_path takes one stack argument everywhere (the 3-argument reading in
// game_checkpoint_write_stats_file was the fprintf arguments pushed early; fixed there). It
// copies and lowercases the name, so it maps a scenario name to a campaign level index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char unknown_00719779[]; // 0x00719779, UNSURE: current scenario/level name buffer
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8

extern int16_t campaign_level_find_index_for_path(char *scenario_name); // 0x4c8b90, not in this module; one stack argument, campaign level index or -1
extern void player_profile_write_data(int32_t handle, saved_player_profile *profile); // 0x53a950
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970, blam-cc: AX player_index, EDX source_profile

void player_profile_select_local_slot(int16_t local_player_index)
{
    int16_t current_level;
    int32_t handle;
    saved_player_profile profile;

    current_level = campaign_level_find_index_for_path(unknown_00719779);
    if (current_level == -1 || local_player_index < 0 || 1 <= local_player_index) {
        return;
    }

    handle = profile_globals_block[local_player_index].handle;
    if (handle == -1) {
        return;
    }

    profile = profile_globals_block[local_player_index].profile;
    if (profile.last_campaign_level != current_level) {
        profile.last_campaign_level = current_level;
        player_profile_write_data(handle, &profile);
    }
    player_profile_load(local_player_index, &profile, handle);
}

#if 0
Original Ghidra decompilation (0x539cb0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00539cb0(short param_1)

{
  int iVar1;
  short sVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 local_2008 [74];
  short local_1ee0;
  undefined4 uStack_c;

  uStack_c = 0x539cc0;
  sVar2 = FUN_004c8b90(&DAT_00719779);
  if (((sVar2 != -1) && (-1 < param_1)) && (param_1 < 1)) {
    iVar1 = (&DAT_00714dd4)[param_1 * 0x801];
    if (iVar1 != -1) {
      puVar4 = &DAT_00712dd8 + param_1 * 0x801;
      puVar5 = local_2008;
      for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar5 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar5 = puVar5 + 1;
      }
      if (local_1ee0 != sVar2) {
        local_1ee0 = sVar2;
        player_profile_write_data(iVar1,local_2008);
      }
      player_profile_load(iVar1);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
