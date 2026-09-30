// game_engine_ctf_assign_flag_ids  (Ghidra: FUN_0046d800; named per its summary)
// address 0x46d800, size 133 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Assigns each type-3 scenario starting location (used
//   for CTF-style flag stands) a unique slot id in the range 0-31, resolving any duplicates");
//   types/tags.h ScenarioNetgameFlags (type +0x10, usage_id +0x12), referenced directly in
//   game_types_notes.md's own evidence for this function.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "fn_game.h"

extern Scenario *global_scenario; // 0x00746f8c

// For every type-3 (CTF flag stand) ScenarioNetgameFlags entry whose usage_id is already in
// 0..31, claims that id in a 32-bit used-mask; if it collides with an already-claimed id,
// reassigns it to the lowest still-free id instead.
void game_engine_ctf_assign_flag_ids(void)
{
    int32_t flag_count = (int32_t)global_scenario->netgame_flags.count;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
    uint32_t used_mask = 0;
    int32_t i;

    for (i = 0; i < flag_count; i++) {
        int16_t usage_id;
        if (flags[i].type != 3) {
            continue;
        }
        usage_id = flags[i].usage_id;
        if (usage_id < 0 || usage_id >= 0x20) {
            continue;
        }
        {
            uint32_t bit = 1u << (usage_id & 0x1f);
            if ((used_mask & bit) == 0) {
                used_mask |= bit;
            } else {
                int32_t free_id;
                for (free_id = 0; free_id < 0x20; free_id++) {
                    if ((used_mask & (1u << (free_id & 0x1f))) == 0) {
                        used_mask |= (1u << (free_id & 0x1f));
                        break;
                    }
                }
                flags[i].usage_id = (int16_t)free_id;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x46d800), from tools/pack.py 0x46d800:

void FUN_0046d800(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  short sVar6;
  uint uVar7;

  iVar2 = global_scenario;
  uVar7 = 0;
  if (0 < *(int *)(global_scenario + 0x378)) {
    iVar3 = 0;
    sVar6 = 0;
    do {
      iVar3 = iVar3 * 0x94 + *(int *)(iVar2 + 0x37c);
      if (((*(short *)(iVar3 + 0x10) == 3) && (sVar1 = *(short *)(iVar3 + 0x12), -1 < sVar1)) &&
         (sVar1 < 0x20)) {
        uVar5 = 1 << ((byte)sVar1 & 0x1f);
        if ((uVar7 & uVar5) == 0) {
          uVar7 = uVar7 | uVar5;
        }
        else {
          iVar4 = 0;
          do {
            if ((uVar7 & 1 << ((byte)iVar4 & 0x1f)) == 0) {
              uVar7 = uVar7 | uVar5;
              *(short *)(iVar3 + 0x12) = (short)iVar4;
              goto LAB_0046d872;
            }
            iVar4 = iVar4 + 1;
          } while (iVar4 < 0x20);
          *(short *)(iVar3 + 0x12) = (short)iVar4;
        }
      }
LAB_0046d872:
      sVar6 = sVar6 + 1;
      iVar3 = (int)sVar6;
    } while (iVar3 < *(int *)(iVar2 + 0x378));
  }
  return;
}
#endif
