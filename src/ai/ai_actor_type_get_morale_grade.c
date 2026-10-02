// ai_actor_type_get_morale_grade  (Ghidra: ai_actor_type_get_morale_grade; named for this rewrite)
// address 0x434ed0, size 75 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: indexes Scenario.command_lists (TagReflexive at +0x438, matching ai.h's own
// comment) by actor type to get a ScenarioCommandList, validates a caller-supplied command
// index against that list's commands (TagReflexive at ScenarioCommandList+0x30, matching
// ScenarioCommand's own established 0x20-byte size), and returns a grade derived from bit 4
// of a second byte in the caller's own command-reference record (+4): 3 when clear, 2 when
// set, or 1 if the index/list is invalid. Matches the phase-4 summary.
// register convention: Ghidra resolved neither parameter.
//   // blam-cc: AX -> actor_type_index, EDX -> command_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: AX -> actor_type_index, EDX -> command_reference
// command_reference[0] is the command index within the actor type's command list;
// command_reference[4]'s bit 4 selects the grade (clear -> 3, set -> 2).
uint32_t ai_actor_type_get_morale_grade(int16_t actor_type_index, uint8_t *command_reference)
{
    ScenarioCommandList *command_list =
        &((ScenarioCommandList *)global_scenario->command_lists.pointer)[actor_type_index];

    if ((uint32_t)command_reference[0] < (uint32_t)command_list->commands.count &&
        (ScenarioCommand *)command_list->commands.pointer + command_reference[0] != 0) {
        return ((uint8_t)(~command_reference[4]) & 0x10 | 0x20) >> 4;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x434ed0):

uint FUN_00434ed0(void)

{
  short in_AX;
  int iVar1;
  byte *in_EDX;

  iVar1 = in_AX * 0x60 + *(int *)(global_scenario + 0x43c);
  if (((int)(uint)*in_EDX < *(int *)(iVar1 + 0x30)) &&
     ((uint)*in_EDX * 0x20 + *(int *)(iVar1 + 0x34) != 0)) {
    return ((byte)~in_EDX[4] & 0x10 | 0x20) >> 4;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
