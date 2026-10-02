// game_engine_scan_netgame_flags_noop  (Ghidra: FUN_004637c0; renamed per its summary)
// address 0x4637c0, size 76 bytes
// name confidence: 0.2   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Low-confidence: appears to search a scenario
// placement list for a matching short identifier, but the decompiled loop body has no
// observable effect, suggesting lost side effects (for example an element index or match flag
// this decompilation failed to capture)"); types/tags.h Scenario::netgame_flags (+0x378),
// ScenarioNetgameFlags::type (+0x10).
// register convention: a value to match in DI (unaff_DI).
//   // blam-cc: unaff_DI -> needle
// UNSURE: transcribed exactly as decompiled, including the inner for-loop that only advances a
// counter with no other effect. This is almost certainly missing real behavior (see summary
// above), but nothing in this batch's evidence recovers what that behavior was.

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: unaff_DI -> needle
void game_engine_scan_netgame_flags_noop(int16_t needle)
{
    int32_t count = (int32_t)global_scenario->netgame_flags.count;
    int32_t i;

    if (0 < count) {
        int16_t next = 1;
        for (i = 0; i < count; ) {
            int16_t j = next;
            if (needle == ((ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer)[i].type) {
                for (; j < count; j++) {
                    // UNSURE: no observable effect, see file header
                }
            }
            i = next;
            next = next + 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4637c0), from tools/pack.py 0x4637c0:

void FUN_004637c0(void)

{
  int iVar1;
  short sVar2;
  int iVar3;
  short sVar4;
  short unaff_DI;

  iVar1 = *(int *)(global_scenario + 0x378);
  if (0 < iVar1) {
    iVar3 = 0;
    sVar4 = 1;
    do {
      sVar2 = sVar4;
      if (unaff_DI == *(short *)(iVar3 * 0x94 + 0x10 + *(int *)(global_scenario + 0x37c))) {
        for (; sVar2 < iVar1; sVar2 = sVar2 + 1) {
        }
      }
      iVar3 = (int)sVar4;
      sVar4 = sVar4 + 1;
    } while (iVar3 < iVar1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
