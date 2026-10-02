// game_engine_validate_scenario_placements_noop  (Ghidra: FUN_00463810; renamed per its summary)
// address 0x463810, size 156 bytes
// name confidence: 0.2   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Low-confidence: repeatedly walks scenario
// placement-list counts with no clear side effect in the decompilation; may be a validation/
// assert routine or a decompiler artifact"); types/tags.h Scenario::netgame_flags (+0x378) and
// the field at Scenario+900 (0x384, Scenario::netgame_equipment.count); this batch's
// game_engine_scan_netgame_flags_noop (0x4637c0).
// UNSURE: every counting loop here has no observable effect (matches nothing, writes nothing);
// transcribed exactly as decompiled rather than invented into something meaningful.

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario; // 0x00746f8c

extern void game_engine_scan_netgame_flags_noop(int16_t needle); // 0x4637c0, this batch

void game_engine_validate_scenario_placements_noop(void)
{
    int32_t netgame_flags_count;
    int32_t netgame_equipment_count;
    int16_t i;
    int32_t pass;

    game_engine_scan_netgame_flags_noop(0); // UNSURE: needle elided by Ghidra

    netgame_flags_count = (int32_t)global_scenario->netgame_flags.count;
    i = 0;
    if (0 < netgame_flags_count) {
        do {
            i = i + 1;
        } while (i < netgame_flags_count);
    }

    game_engine_scan_netgame_flags_noop(0); // UNSURE: needle elided by Ghidra

    netgame_equipment_count = (int32_t)global_scenario->netgame_equipment.count;
    for (pass = 0; pass < 5; pass++) {
        i = 0;
        if (0 < netgame_equipment_count) {
            do {
                i = i + 1;
            } while (i < netgame_equipment_count);
        }
    }
}

#if 0
Original Ghidra decompilation (0x463810), from tools/pack.py 0x463810:

void FUN_00463810(void)

{
  int iVar1;
  short sVar2;

  FUN_004637c0();
  iVar1 = global_scenario;
  sVar2 = 0;
  if (0 < *(int *)(global_scenario + 0x378)) {
    do {
      sVar2 = sVar2 + 1;
    } while ((int)sVar2 < *(int *)(global_scenario + 0x378));
  }
  FUN_004637c0();
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 900)) {
    do {
      sVar2 = sVar2 + 1;
    } while ((int)sVar2 < *(int *)(iVar1 + 900));
  }
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 900)) {
    do {
      sVar2 = sVar2 + 1;
    } while ((int)sVar2 < *(int *)(iVar1 + 900));
  }
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 900)) {
    do {
      sVar2 = sVar2 + 1;
    } while ((int)sVar2 < *(int *)(iVar1 + 900));
  }
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 900)) {
    do {
      sVar2 = sVar2 + 1;
    } while ((int)sVar2 < *(int *)(iVar1 + 900));
  }
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 900)) {
    do {
      sVar2 = sVar2 + 1;
    } while ((int)sVar2 < *(int *)(iVar1 + 900));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
