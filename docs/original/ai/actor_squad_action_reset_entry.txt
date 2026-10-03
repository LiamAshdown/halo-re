// actor_squad_action_reset_entry  (Ghidra: actor_squad_action_reset_entry, renamed)
// address 0x406c50, size 312 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (checked against objdump 0x406c50..0x406d83)
// evidence: types/ai.h actor.unit_index; types/tags.h Scenario.command_lists/
//   ScenarioCommandList.commands/ScenarioCommand (same layout as actor_squad_action_execute.c
//   and actor_squad_action_is_complete.c); phase-4 summary "resets/cleans up per-entry state
//   for the actor's current squad action-list item before moving to the next one". Parameter
//   roles match the other two squad-action functions in this session (actor index in EAX,
//   check_object_index in ECX, state in EBX); command_list_index, aim_state and an output
//   byte (used only by atom type 0x14, a scripted jump) are the recognized stack parameters.
//   // blam-cc: EAX -> actor_index, ECX -> check_object_index, EBX -> state,
//   //          stack -> command_list_index, aim_state, next_action_index_out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern Scenario *global_scenario; // 0x00746f8c

extern void actor_clear_vocalization(uint32_t actor_index);   // 0x414560
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, this module,
                                                                 // blam-cc: EDX -> actor_index
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

void actor_squad_action_reset_entry(uint32_t actor_index, uint32_t check_object_index, uint8_t *state, int16_t command_list_index, uint8_t *aim_state, uint8_t *next_action_index_out)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];
    uint32_t current_action_index = state[0];

    if (current_action_index >= (uint32_t)list->commands.count) {
        return;
    }

    {
        ScenarioCommand *entry = (ScenarioCommand *)((uint8_t *)list->commands.pointer + current_action_index * sizeof(ScenarioCommand));

        switch (entry->atom_type) {
        case 1:
        case 2:
            if (check_object_index == a->unit_index) {
                actor_movement_action_stop(actor_index);
            }
            if (aim_state != 0) {
                aim_state[4] = 0;
                aim_state[0x18] = 0;
            }
            break;
        case 3:
        case 0x16:
            state[5] &= 0xfe;
            state[8] = 0xff;
            state[9] = 0xff;
            return;
        case 4:
        case 0x17:
        case 0x18:
        case 0x19:
            if (check_object_index == a->unit_index) {
                actor_clear_vocalization(actor_index);
                return;
            }
            break;
        case 7:
            if (aim_state != 0) {
                aim_state[0x36] = 0;
                return;
            }
            break;
        case 10:
        case 0xb:
            state[5] &= 0xfb;
            state[8] = 0;
            state[9] = 0;
            return;
        case 0xd: {
            uint32_t *obj = (uint32_t *)object_try_and_get(check_object_index, 1); // 0x406d52: ECX = the unit
            if (obj != 0) {
                uint32_t *flags = (uint32_t *)((uint8_t *)obj + 0x4cc);
                *flags &= 0xfffffff3;
                return;
            }
            break;
        }
        case 0x14:
            if (entry->atom_modifier == 1) {
                uint8_t flags = state[4];
                state[4] = flags & 0xf7;
                if ((~(flags >> 3) & 1) == 0) {
                    state[4] = flags & 0xe7;
                    return;
                }
                state[4] = (flags & 0xf7) | 0x10;
            }
            if ((int16_t)entry->command != (int32_t)current_action_index && state[1] < 10) {
                *next_action_index_out = *((uint8_t *)entry + 0x16);
                state[1] = state[1] + 1;
                return;
            }
            break;
        default:
            break;
        }
    }
}

#if 0
Original Ghidra decompilation (0x406c50):

void FUN_00406c50(short param_1,int param_2,undefined1 *param_3)

{
  byte bVar1;
  uint in_EAX;
  int in_ECX;
  uint uVar2;
  byte *unaff_EBX;
  undefined2 *puVar3;
  int iVar4;
  int iVar5;

  uVar2 = (uint)*unaff_EBX;
  iVar4 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar5 = param_1 * 0x60 + *(int *)(global_scenario + 0x43c);
  if ((int)uVar2 < *(int *)(iVar5 + 0x30)) {
    puVar3 = (undefined2 *)(uVar2 * 0x20 + *(int *)(iVar5 + 0x34));
    switch(*puVar3) {
    case 1:
    case 2:
      if (in_ECX == *(int *)(iVar4 + 0x18)) {
        actor_movement_action_stop();
      }
      if (param_2 != 0) {
        *(undefined1 *)(param_2 + 4) = 0;
        *(undefined1 *)(param_2 + 0x18) = 0;
      }
      break;
    case 3:
    case 0x16:
      unaff_EBX[5] = unaff_EBX[5] & 0xfe;
      unaff_EBX[8] = 0xff;
      unaff_EBX[9] = 0xff;
      return;
    case 4:
    case 0x17:
    case 0x18:
    case 0x19:
      if (in_ECX == *(int *)(iVar4 + 0x18)) {
        actor_clear_vocalization();
        return;
      }
      break;
    case 7:
      if (param_2 != 0) {
        *(undefined1 *)(param_2 + 0x36) = 0;
        return;
      }
      break;
    case 10:
    case 0xb:
      unaff_EBX[5] = unaff_EBX[5] & 0xfb;
      unaff_EBX[8] = 0;
      unaff_EBX[9] = 0;
      return;
    case 0xd:
      iVar4 = object_try_and_get(1);
      if (iVar4 != 0) {
        *(uint *)(iVar4 + 0x4cc) = *(uint *)(iVar4 + 0x4cc) & 0xfffffff3;
        return;
      }
      break;
    case 0x14:
      if (puVar3[1] == 1) {
        bVar1 = unaff_EBX[4];
        unaff_EBX[4] = bVar1 & 0xf7;
        if ((~(bVar1 >> 3) & 1) == 0) {
          unaff_EBX[4] = bVar1 & 0xe7;
          return;
        }
        unaff_EBX[4] = bVar1 & 0xf7 | 0x10;
      }
      if (((int)(short)puVar3[0xb] != uVar2) && (unaff_EBX[1] < 10)) {
        *param_3 = *(undefined1 *)(puVar3 + 0xb);
        unaff_EBX[1] = unaff_EBX[1] + 1;
        return;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
