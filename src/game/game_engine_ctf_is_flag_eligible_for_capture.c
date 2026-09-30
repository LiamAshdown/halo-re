// game_engine_ctf_is_flag_eligible_for_capture  (Ghidra: FUN_0046df30; named per its summary)
// address 0x46df30, size 175 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Determines whether a given flag id is the one
//   currently eligible to be captured for a particular team/slot, accounting for the active CTF
//   sub-mode"); ctf_globals::flag_id_mask (0x006b1290) and ::unknown_44 as a per-team 32-bit
//   captured-flags mask (0x006b12d4, corrected in game_engine_ctf_on_flag_captured.c, this
//   batch); ctf_globals::team_flag_id[16] (0x006b1294); ctf_neutral_flag_id (0x006b1314);
//   game_variant::score_limit/ctf_option_7c aliased 0x006f1ce0/0x006f1d04; player::unknown_c6
//   (already UNSURE elsewhere in this batch).
// register convention: team index in in_ECX; flag id in unaff_EDI.
//   // blam-cc: ECX -> team, EDI -> flag_id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern ctf_globals ctf_globals_live;            // 0x006b1290
extern uint32_t ctf_team_captured_flags_mask[]; // 0x006b12d4, this batch (ctf_globals::unknown_44)
extern data_array *player_data;                 // 0x0087a480
extern game_variant game_engine_variant;        // 0x006f1c88 (score_limit/ctf_option_7c aliased
                                                 // 0x006f1ce0/0x006f1d04)
extern int32_t ctf_neutral_flag_id;             // 0x006b1314

// blam-cc: ECX -> team, EDI -> flag_id
// True when `flag_id` is the one `team` may currently capture: out of range (>=32) always
// fails; the team's own capture-count (player::unknown_c6, indexed by team here) must be below
// the score limit; in single-flag mode (ctf_option_7c==2) only the neutral flag qualifies; with
// every flag already captured by this team, only its currently assigned team_flag_id qualifies;
// otherwise `flag_id` must still be uncaptured by this team, and in "assign lowest" mode
// (ctf_option_7c==0) additionally must be the LOWEST such uncaptured id.
uint8_t game_engine_ctf_is_flag_eligible_for_capture(uint32_t team, int32_t flag_id)
{
    uint32_t team_idx = team & 0xffff;
    uint32_t uncaptured_mask = ~ctf_team_captured_flags_mask[team_idx] & ctf_globals_live.flag_id_mask;
    player *p;

    if (flag_id > 0x1f) {
        return 0;
    }

    p = (player *)((uint8_t *)player_data->data + team_idx * sizeof(player));
    if (*(int16_t *)((uint8_t *)p + 0xc6) >= game_engine_variant.score_limit) {
        return 0;
    }

    if (game_engine_variant.ctf_option_7c == 2) {
        return ctf_neutral_flag_id == flag_id;
    }
    if (ctf_team_captured_flags_mask[team_idx] == ctf_globals_live.flag_id_mask) {
        return flag_id == ctf_globals_live.team_flag_id[team_idx];
    }
    if ((uncaptured_mask & (1u << (flag_id & 0x1f))) != 0) {
        if (game_engine_variant.ctf_option_7c == 0) {
            int32_t i;
            for (i = 0; i != flag_id; i++) {
                if ((uncaptured_mask & (1u << (i & 0x1f))) != 0) {
                    return 0;
                }
                if (i > 0x1f) {
                    return 1;
                }
            }
        }
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x46df30), from tools/pack.py 0x46df30:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_0046df30(void)

{
  uint in_ECX;
  uint uVar1;
  int iVar2;
  uint uVar3;
  int unaff_EDI;

  uVar1 = in_ECX & 0xffff;
  uVar3 = ~(&DAT_006b12d4)[uVar1] & DAT_006b1290;
  if (0x1f < unaff_EDI) {
    return false;
  }
  if (*(short *)(uVar1 * 0x200 + 0xc6 + *(int *)(DAT_0087a480 + 0x34)) < DAT_006f1ce0) {
    if (_DAT_006f1d04 == 2) {
      return DAT_006b1314 == unaff_EDI;
    }
    if ((&DAT_006b12d4)[uVar1] == DAT_006b1290) {
      return unaff_EDI == (&DAT_006b1294)[uVar1];
    }
    if ((uVar3 & 1 << ((byte)unaff_EDI & 0x1f)) != 0) {
      if (_DAT_006f1d04 == 0) {
        iVar2 = 0;
        while (iVar2 != unaff_EDI) {
          if ((uVar3 & 1 << ((byte)iVar2 & 0x1f)) != 0) {
            return false;
          }
          iVar2 = iVar2 + 1;
          if (0x1f < iVar2) {
            return true;
          }
        }
      }
      return true;
    }
  }
  return false;
}
#endif
