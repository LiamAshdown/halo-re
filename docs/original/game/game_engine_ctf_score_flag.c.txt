// game_engine_ctf_score_flag  (Ghidra: FUN_0046e080; named per its summary)
// address 0x46e080, size 213 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Processes a flag being scored for a team: validates
//   eligibility, updates per-team flag bookkeeping, and completes the capture once all required
//   flags (in multi-flag mode) or the single flag (in neutral mode) are in"); types/tags.h
//   ScenarioNetgameFlags::usage_id (+0x12); ctf_globals::team_flag_id/flag_id_mask/unknown_44
//   (this batch's corrected per-team captured-flags-mask reading); game_engine_ctf_is_flag_
//   eligible_for_capture / game_engine_ctf_on_flag_captured / game_engine_ctf_pick_random_flag
//   (this batch).
// register convention: team/player index in param_1 (stack); scenario netgame-flag index in
//   in_EAX.
//   // blam-cc: stack -> team, EAX -> scenario_flag_index
// UNSURE: game_engine_ctf_is_flag_eligible_for_capture's own (team, flag_id) arguments are
//   elided at this call site; modeled as this function's own team/usage_id, which are the only
//   matching values in scope.
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c
extern ctf_globals ctf_globals_live; // 0x006b1290
extern uint32_t ctf_team_captured_flags_mask[]; // 0x006b12d4, this batch
extern game_variant game_engine_variant; // 0x006f1c88 (ctf_option_7c aliased 0x006f1d04)
extern int32_t ctf_neutral_flag_id; // 0x006b1314

extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void game_engine_ctf_on_flag_captured(uint32_t flag_index); // 0x46dde0, this batch
extern uint8_t game_engine_ctf_is_flag_eligible_for_capture(uint32_t team, int32_t flag_id); // 0x46df30, this batch
extern int32_t game_engine_ctf_pick_random_flag(int32_t exclude_flag_index); // 0x46dfe0, this batch

// blam-cc: stack -> team, EAX -> scenario_flag_index
void game_engine_ctf_score_flag(uint32_t team, int32_t scenario_flag_index)
{
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
    int16_t usage_id = flags[scenario_flag_index].usage_id;
    uint32_t idx = team & 0xffff;

    if (game_engine_ctf_is_flag_eligible_for_capture(team, usage_id) == 0) {
        return;
    }

    game_engine_queue_multiplayer_sound(0x1a, team, 1); // 0x46e0bc..0x46e0cb: EDI is the first argument (a player handle)
    if (ctf_globals_live.team_flag_id[idx] == -1) {
        ctf_globals_live.team_flag_id[idx] = usage_id;
    }

    if (game_engine_variant.engine.race.race_type == 2) {
        game_engine_ctf_on_flag_captured(team);
        ctf_neutral_flag_id = game_engine_ctf_pick_random_flag(ctf_neutral_flag_id);
        return;
    }
    if (ctf_team_captured_flags_mask[idx] == ctf_globals_live.flag_id_mask) {
        game_engine_ctf_on_flag_captured(team);
        return;
    }
    ctf_team_captured_flags_mask[idx] |= 1u << (usage_id & 0x1f);
}

#if 0
Original Ghidra decompilation (0x46e080), from tools/pack.py 0x46e080:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046e080(uint param_1)

{
  undefined2 uVar1;
  char cVar2;
  int in_EAX;
  int iVar3;
  uint uVar4;

  iVar3 = in_EAX * 0x94 + *(int *)(global_scenario + 0x37c);
  uVar1 = *(undefined2 *)(iVar3 + 0x12);
  cVar2 = FUN_0046df30();
  if (cVar2 != '\0') {
    uVar4 = param_1 & 0xffff;
    game_engine_queue_multiplayer_sound(1);
    if ((&DAT_006b1294)[uVar4] == -1) {
      (&DAT_006b1294)[uVar4] = (int)*(short *)(iVar3 + 0x12);
    }
    if (_DAT_006f1d04 == 2) {
      game_engine_ctf_on_flag_captured(param_1);
      DAT_006b1314 = game_engine_ctf_pick_random_flag(DAT_006b1314);
      return;
    }
    if ((&DAT_006b12d4)[uVar4] == DAT_006b1290) {
      game_engine_ctf_on_flag_captured(param_1);
      return;
    }
    (&DAT_006b12d4)[uVar4] = 1 << ((byte)uVar1 & 0x1f) | (&DAT_006b12d4)[uVar4];
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
