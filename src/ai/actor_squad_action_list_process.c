// actor_squad_action_list_process  (Ghidra: actor_squad_action_list_process, already named)
// address 0x406e30, size 246 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (checked against objdump 0x406e30..0x406f25)
// evidence: types/tags.h Scenario.command_lists/ScenarioCommandList.commands (same layout as
//   the other squad-action functions in this session); calls actor_squad_action_is_complete,
//   actor_squad_action_reset_entry (actor_squad_action_reset_entry) and actor_squad_action_execute in
//   sequence, exactly matching the phase-4 summary "drives a squad's scripted action list
//   for this actor to completion, calling the per-entry check/reset/execute functions in
//   sequence".
// register convention: already a full stack-parameter signature in the decompilation.
//   param_1/param_2 are the actor_index/check_object_index forwarded to the callees,
//   param_3 the command_list_index, param_4 the state record, param_5 the aim_state record,
//   param_6 an output success/continuation byte.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario; // 0x00746f8c

extern char actor_squad_action_execute(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state); // 0x405520, this session
extern uint8_t actor_squad_action_is_complete(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state); // 0x4066d0, this session
extern void actor_squad_action_reset_entry(uint32_t actor_index, uint32_t check_object_index, uint8_t *state, int16_t command_list_index, uint8_t *aim_state, uint8_t *next_action_index_out); // 0x406c50, this session

// Drives the actor's current squad action list (command_list_index) forward: while the
// current entry is not complete, stops; otherwise resets it, advances to the next entry (or
// marks the list finished if that runs past the end), and executes the new entry, repeating
// until the entry signals it wants to stop being re-driven this tick (state[4] bit 4) or the
// list finishes.
void actor_squad_action_list_process(uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state, uint8_t *aim_state, uint8_t *out)
{
    ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];
    uint8_t have_current_entry;
    uint8_t next_action_index;

    if ((state[4] & 2) != 0) {
        goto finish;
    }

    have_current_entry = state[0] < (int32_t)list->commands.count;
    state[1] = 0;
    do {
        if (have_current_entry != 0 && actor_squad_action_is_complete(aim_state, actor_index, check_object_index, command_list_index, state) == 0) {
            break;
        }

        next_action_index = (state[0] == 0xff) ? 0 : (uint8_t)(state[0] + 1);

        if (have_current_entry != 0) {
            actor_squad_action_reset_entry(actor_index, check_object_index, state, command_list_index, aim_state, &next_action_index);
        }
        if (next_action_index >= (int32_t)list->commands.count) {
            state[4] |= 2;
            break;
        }
        state[0] = next_action_index;
        have_current_entry = actor_squad_action_execute(aim_state, actor_index, check_object_index, command_list_index, state);
    } while ((state[4] & 4) == 0);

finish:
    if ((state[4] & 2) == 0) {
        *out = 0;
    }
}

#if 0
Original Ghidra decompilation (0x406e30):

void actor_squad_action_list_process
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,byte *param_4,
               undefined4 param_5,undefined1 *param_6)

{
  char cVar1;
  char cVar2;
  byte local_5;
  int local_4;

  local_4 = (short)param_3 * 0x60 + *(int *)(global_scenario + 0x43c);
  if ((param_4[4] & 2) == 0) {
    cVar2 = (int)(uint)*param_4 < *(int *)(local_4 + 0x30);
    param_4[1] = 0;
    do {
      if ((cVar2 != '\0') &&
         (cVar1 = actor_squad_action_is_complete(param_1,param_3,param_4), cVar1 == '\0')) break;
      if (*param_4 == 0xffffffff) {
        local_5 = 0;
      }
      else {
        local_5 = *param_4 + 1;
      }
      if (cVar2 != '\0') {
        FUN_00406c50(param_3,param_5,&local_5);
      }
      if (*(int *)(local_4 + 0x30) <= (int)(uint)local_5) {
        param_4[4] = param_4[4] | 2;
        break;
      }
      *param_4 = local_5;
      cVar2 = actor_squad_action_execute(param_1,param_2,param_3,param_4);
    } while ((param_4[4] & 4) == 0);
  }
  if ((param_4[4] & 2) == 0) {
    *param_6 = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
