// ai_reference_parse  (Ghidra: ai_reference_parse; named for this rewrite)
// address 0x432320, size 254 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: calls scenario_find_encounter_index_by_name / encounter_definition_find_squad_
// index_by_name / encounter_definition_find_platoon_index_by_name (0x432200/0x432260/
// 0x4322c0, this batch) to parse a scripted "encounter[/squad-or-platoon]" reference string
// into the packed value the rest of this address range (ai_reference_expand @0x432420,
// ai_reference_starting_location_iterator_new @0x4324f0, actor_iterator_by_squad_new
// @0x432650, ...) all decode the same way: bits 0..15 the encounter index, bit 30 set for a
// platoon (0x4000 << 0x10 == 0x40000000) or bit 31 set for a squad (0xffff8000 << 0x10, i.e.
// the low byte of the sub-index sign-extended into the high word). Confirmed against
// objdump (bin/halo.exe 0x432320..0x43241d): EAX and ECX are genuine register parameters
// that Ghidra's own decompile of this function never names (only the output pointer, a real
// stack argument, appears as param_1).
// register convention: EAX -> reference_string, ECX -> scenario (confirmed by objdump: `mov
// edi,eax` / `mov ebx,ecx` at function entry, then `mov esi,ebx` before every encounter
// lookup); stack -> out_packed_reference.
//   // blam-cc: EAX -> reference_string, ECX -> scenario, stack -> out_packed_reference

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern int32_t scenario_find_encounter_index_by_name(Scenario *scenario, char *name); // 0x432200, this batch
extern int32_t encounter_definition_find_squad_index_by_name(ScenarioEncounter *encounter_definition, char *name); // 0x432260, this batch
extern int32_t encounter_definition_find_platoon_index_by_name(ScenarioEncounter *encounter_definition, char *name); // 0x4322c0, this batch

// blam-cc: EAX -> reference_string, ECX -> scenario, stack -> out_packed_reference
// Parses a scripted "encounter", "encounter/squad" or "encounter/platoon" reference string
// (or "none") into a packed reference value: bits 0..15 the encounter index, and either
// 0xffff8000 (squad) or 0x4000 (platoon) shifted left 16 and ORed with the sub-index in its
// low byte, or all bits set (0xffffffff) for "none" or an unresolved name. Returns whether
// the reference resolved to something other than "none"/unresolved.
uint8_t ai_reference_parse(char *reference_string, Scenario *scenario, uint32_t *out_packed_reference)
{
    uint32_t packed = 0xffffffff;
    char *slash;

    if (_stricmp(reference_string, "none") == 0) {
        *out_packed_reference = 0xffffffff;
        return 1;
    }

    slash = strrchr(reference_string, '/');
    if (slash == 0) {
        int32_t encounter_index = scenario_find_encounter_index_by_name(scenario, reference_string);
        if (encounter_index != -1) {
            packed = (uint32_t)encounter_index & 0xffff;
        }
    } else {
        int32_t name_length = (int32_t)(slash - reference_string);
        if (name_length < 0x20) {
            char encounter_name[32];
            int32_t encounter_index;

            strncpy(encounter_name, reference_string, name_length);
            encounter_name[name_length] = '\0';

            encounter_index = scenario_find_encounter_index_by_name(scenario, encounter_name);
            if (encounter_index != -1) {
                ScenarioEncounter *encounter_definition =
                    &((ScenarioEncounter *)scenario->encounters.pointer)[encounter_index];
                int32_t squad_index = encounter_definition_find_squad_index_by_name(encounter_definition, slash + 1);
                uint32_t high;

                if (squad_index != -1) {
                    high = ((uint32_t)squad_index & 0xff) | 0xffff8000;
                } else {
                    int32_t platoon_index =
                        encounter_definition_find_platoon_index_by_name(encounter_definition, slash + 1);
                    if (platoon_index == -1) {
                        goto done;
                    }
                    high = ((uint32_t)platoon_index & 0xff) | 0x4000;
                }
                packed = (high << 0x10) | ((uint32_t)encounter_index & 0xffff);
            }
        }
    }

done:
    *out_packed_reference = packed;
    return packed != 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x432320):

bool FUN_00432320(uint *param_1)

{
  char *in_EAX;
  int iVar1;
  char *pcVar2;
  uint uVar3;
  uint uVar4;
  size_t _Count;
  uint local_24;
  char local_20 [32];

  local_24 = 0xffffffff;
  iVar1 = __stricmp(in_EAX,"none");
  if (iVar1 == 0) {
    *param_1 = 0xffffffff;
    return true;
  }
  pcVar2 = _strrchr(in_EAX,0x2f);
  if (pcVar2 == (char *)0x0) {
    uVar3 = FUN_00432200();
    if (uVar3 != 0xffffffff) {
      local_24 = uVar3 & 0xffff;
    }
  }
  else {
    _Count = (int)pcVar2 - (int)in_EAX;
    if ((int)_Count < 0x20) {
      _strncpy(local_20,in_EAX,_Count);
      local_20[_Count] = '\0';
      uVar3 = FUN_00432200(local_20);
      if (uVar3 != 0xffffffff) {
        uVar4 = FUN_00432260(pcVar2 + 1);
        if (uVar4 == 0xffffffff) {
          uVar4 = FUN_004322c0(pcVar2 + 1);
          if (uVar4 == 0xffffffff) goto LAB_00432406;
          uVar4 = uVar4 & 0xff | 0x4000;
        }
        else {
          uVar4 = uVar4 & 0xff | 0xffff8000;
        }
        local_24 = uVar4 << 0x10 | uVar3 & 0xffff;
      }
    }
  }
LAB_00432406:
  *param_1 = local_24;
  return local_24 != 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
