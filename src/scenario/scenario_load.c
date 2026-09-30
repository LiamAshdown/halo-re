// scenario_load  (Ghidra: already named scenario_load)
// address 0x53e6a0, size 215 bytes
// name confidence: 0.85   rewrite confidence: 0.75
// evidence: out/phase2/results/scenario_00.json ("Loads the map's cache file, resolves the
// scenario and matg globals tag data pointers, and switches in the initial structure bsp");
// calls cache_file_load, derives global_scenario from the returned tag id via tag_instances,
// looks up "globals\\globals" ('matg', 0x6d617467) with tag_lookup for global_globals, and
// calls scenario_structure_bsp_switch(0) to bring in the initial structure bsp. On success it
// then stamps -1 into every netgame_equipment element's runtime item handle
// (unknown_ffffffff, +0x10), matching types/scenario.h's note on that field.
// register convention: raw disassembly (0x53e6a0-0x53e780) shows EAX never set before
// `call 0x442290` (cache_file_load) -- both of scenario_load's own two call sites
// (0x4c9621, 0x4de7d8) load EAX from a stack path buffer right before calling it and then test
// AL on return, so EAX is scenario_load's own path parameter, forwarded straight through to
// cache_file_load, and the bool result comes back in AL.
//   // blam-cc: EAX -> path (forwarded unchanged to cache_file_load); return in AL

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "scenario.h"
#include "fn_cache.h"
#include <string.h>


extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, cache module, blam-cc: EDI -> group
extern uint8_t scenario_structure_bsp_switch(int16_t structure_bsp_index); // 0x53eeb0, this module, out of this batch's range

extern tag_instance *tag_instances;    // 0x0087bc14
extern datum_index global_scenario_index; // 0x0069e8d4
extern Scenario *global_scenario;      // 0x00746f8c
extern Globals *global_globals;   // 0x00746fa0
extern char k_empty_string[1];         // 0x0065512c

// blam-cc: EAX -> path
// Loads the map cache file at `path`. On success, resolves global_scenario from the returned
// scenario tag id, looks up the "globals\\globals" ('matg') tag for global_globals, and
// switches in structure bsp 0 via scenario_structure_bsp_switch. If that switch succeeds, every
// element of the scenario's netgame_equipment block has its runtime item handle
// (unknown_ffffffff, +0x10) reset to -1. Returns nonzero (in AL) on success.
// On a cache_file_load failure, the original walks the (always-empty) k_empty_string buffer
// looking for embedded newlines to print error lines; nothing in that buffer ever matches, so
// it always falls straight through and returns 0. Preserved as-is.
uint8_t scenario_load(char *path)
{
    char *scan;
    char *next_newline;
    int32_t i;
    uint8_t result;

    global_scenario_index = cache_file_load(path);
    if (global_scenario_index == (datum_index)0xffffffff) {
        result = 0;
        scan = k_empty_string;
        do {
            next_newline = strchr(scan, '\n');
            result = 0;
            if (next_newline == (char *)0) {
                break;
            }
            scan = next_newline + 1;
            *next_newline = '\n';
            result = 0;
        } while (scan != (char *)0);
        return result;
    }

    global_scenario = (Scenario *)tag_instances[global_scenario_index & 0xffff].data;
    if ((int32_t)global_scenario->structure_bsps.count <= 0) {
        return 0;
    }

    global_globals = (Globals *)tag_instances[
        tag_lookup(0x6d617467 /* 'matg' */, "globals\\globals") & 0xffff].data;

    if (scenario_structure_bsp_switch(0) == 0) {
        return 0;
    }

    if (0 < (int32_t)global_scenario->netgame_equipment.count) {
        ScenarioNetgameEquipment *equipment =
            (ScenarioNetgameEquipment *)global_scenario->netgame_equipment.pointer;
        for (i = 0; i < (int32_t)global_scenario->netgame_equipment.count; i++) {
            equipment[i].unknown_ffffffff = 0xffffffff;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x53e6a0):

uint __cdecl scenario_load(void)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  undefined1 *puVar4;
  undefined1 *puVar5;
  int iVar6;

  DAT_0069e8d4 = cache_file_load();
  if (DAT_0069e8d4 == 0xffffffff) {
    puVar4 = &DAT_0065512c;
    do {
      puVar5 = (undefined1 *)FUN_006257e0(puVar4,10);
      uVar1 = 0;
      if (puVar5 == (undefined1 *)0x0) break;
      puVar4 = puVar5 + 1;
      *puVar5 = 10;
      uVar1 = 0;
    } while (puVar4 != (undefined1 *)0x0);
  }
  else {
    uVar1 = *(uint *)((DAT_0069e8d4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    global_scenario = uVar1;
    if (0 < *(int *)(uVar1 + 0x5a4)) {
      uVar1 = tag_lookup("globals\\globals");
      DAT_00746fa0 = *(undefined4 *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      uVar2 = scenario_structure_bsp_switch();
      uVar1 = global_scenario;
      if ((char)uVar2 != '\0') {
        iVar3 = 0;
        if (0 < *(int *)(global_scenario + 900)) {
          iVar6 = 0;
          do {
            *(undefined4 *)(*(int *)(uVar1 + 0x388) + 0x10 + iVar6) = 0xffffffff;
            iVar3 = iVar3 + 1;
            iVar6 = iVar6 + 0x90;
          } while (iVar3 < *(int *)(uVar1 + 900));
        }
        return CONCAT31((int3)((uint)iVar3 >> 8),1);
      }
      return uVar2 & 0xffffff00;
    }
  }
  return uVar1 & 0xffffff00;
}

Raw disassembly (0x53e6a0-0x53e6a3), confirming path forwards through EAX with no local setup:

  53e6a0: push   ebx
  53e6a1: xor    bl,bl
  53e6a3: call   0x442290                ; cache_file_load(EAX=path, forwarded from scenario_load's own EAX)
#endif
