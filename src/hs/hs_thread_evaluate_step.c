// hs_thread_evaluate_step  (Ghidra: FUN_0048a370; named per out/phase4/hs_types_notes.md's own
// repeated references to "hs_thread_evaluate_step (0x48a370)")
// address 0x48a370, size 480 bytes
// name confidence: 0.5 (out/phase4/hs_functions.md: "Steps an HS thread's evaluation of its
//   current syntax node, resuming later if the per-tick time budget runs out, and finalizes it
//   on completion")
// rewrite confidence: 0.85
// evidence: types/hs.h hs_thread (type 0x02, flags 0x03, wake_tick 0x08, stack 0x10),
//   hs_syntax_node (index_union 0x02, flags 0x06, _hs_syntax_node_script_call_bit),
//   hs_function_definition (evaluate at +0xc, signature `void (*)(int16_t index, datum_index
//   thread, char first)`), types/tags.h ScenarioScript (script_type 0x20, root_expression_index
//   0x24), hs_script_type (_hs_script_startup=0, _hs_script_dormant=1).
// register convention: none (void); thread index is the recognized stack parameter (param_1).
// FIXED (re-derived instruction by instruction from 0x48a370..0x48a54f):
//   * Both hs_thread_push calls take a 4-byte result slot carved out of the CURRENT frame --
//     `scratch = (uint8 *)frame + 0x0e + frame->size; frame->size += 4` (0x48a3d7..0x48a3e9 and
//     0x48a4bb..0x48a4cb) -- not &thread->result. thread->result at +0x14 is what
//     hs_evaluate_expression @0x48a250 passes from outside; this function never touches it.
//   * The script-call branch resolves the CALLED script from node->index_union
//     (`imul eax,eax,0x5c` + Scenario::scripts.pointer, 0x48a49b), not the thread's own script,
//     and on the finishing pass returns `*scratch` (`mov eax,[ebx]`, 0x48a4df) rather than 0.
//   * hs_current_thread_index is reset to -1 on exactly ONE exit path: a startup or dormant
//     script that has finished (0x48a52b). Every other return -- including the command-thread
//     delete at 0x48a541 -- leaves it pointing at this thread.
// UNSURE (preserved): the auto-push at the top dereferences the script pointer even when the
// thread type is not _hs_thread_script, where that pointer is still NULL. The decompile does not
// gate this path by thread type either; every caller in this module happens to have pushed a
// frame already for non-script threads, so the fault (if it is one) may be unreachable.
// UNSURE: the `first` byte is spilled as a byte at [esp+0x14] and re-loaded as a DWORD
// (`mov ecx,[esp+0x14]`, 0x48a48c) before being pushed as the evaluate handler's third argument,
// so the upper three bytes of that argument are stack garbage. Modeled as a plain char.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address);
    // this module, 0x48a560; UNSURE, see header note
extern void hs_thread_return(int32_t value, uint32_t thread_index); // this module, 0x48a640
extern void datum_delete(data_array *array, datum_index handle); // blam-cc: EAX -> array,
    // EDX -> handle; memory module, 0x4d0510

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern Scenario *global_scenario;       // 0x00746f8c
extern int16_t hs_current_thread_index; // 0x006b15ea
extern uint8_t hs_runtime_active;       // 0x006b15e8

// hs_game_time_globals: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)
extern hs_game_time_globals *game_time; // 0x006f1d6c

// Steps `thread_index` through as many syntax nodes as its per-tick time budget allows (see the
// game_time check below), dispatching each node's function evaluate handler (or, for a
// script-call node, pushing/finalizing the called script's own thread) until either its
// evaluation stack empties (finished) or it must pause and resume later. On finishing: a
// startup/dormant script thread is parked forever (wake_tick -1); a command thread (type 2) is
// deleted outright; any other thread (including a continuous/static script) is simply left as is
// for the next tick.
void hs_thread_evaluate_step(uint32_t thread_index)
{
    hs_thread *thread;
    ScenarioScript *script;
    ScenarioScript *called_script;
    uint8_t *scratch;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    uint8_t saved_flags;
    char first;
    hs_function_definition *definition;

    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    script = 0;
    hs_current_thread_index = (int16_t)thread_index;

    if (thread->type == _hs_thread_script) {
        script = &((ScenarioScript *)global_scenario->scripts.pointer)[thread->script_index];
    }

    thread->wake_tick = 0;
    if ((void *)thread->stack == (void *)&thread->stack_data) {
        /* Carve the root frame's 4-byte result slot: scratch starts at frame + 0x0e, and the
           frame's size is bumped past it. 0x48a3cc..0x48a3f4. Note `script` is dereferenced here
           unconditionally -- a non-script thread that reaches this branch with script == NULL
           would fault; retail relies on hs_thread_new having already pushed a frame for those. */
        frame = thread->stack;
        frame->size = 0;
        scratch = (uint8_t *)frame + 0x0e + frame->size;
        frame->size = frame->size + 4;
        hs_thread_push(script->root_expression_index, thread_index, scratch);
    }

    while ((void *)thread->stack != (void *)&thread->stack_data) {
        if (thread->wake_tick < 0 ||
            (game_time->initialized != 0 &&
             (game_time->budget_flag_1 != 0 || game_time->budget_flag_2 != 0) &&
             game_time->current_tick < thread->wake_tick) ||
            hs_runtime_active == 0) {
            break;
        }

        frame = thread->stack;
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (frame->syntax_node & 0xffff) * 0x14);
        saved_flags = thread->flags;
        frame->size = 0;
        thread->flags = thread->flags & 0xfe;
        first = saved_flags & 1;

        if ((node->flags & _hs_syntax_node_script_call_bit) == 0) {
            definition = hs_function_definitions[node->index_union];
            ((void (*)(int16_t, uint32_t, char))definition->evaluate)(node->index_union,
                thread_index, first);
        } else {
            /* A script-call node. 0x48a49b..0x48a4e9: resolve the CALLED script, carve a 4-byte
               result slot out of the current frame, then either start the call (first time
               through: push the called script's root expression, writing its result into that
               slot) or finish it (second time: hand the slot's contents back to the parent). */
            called_script = &((ScenarioScript *)global_scenario->scripts.pointer)[node->index_union];
            frame = thread->stack;
            scratch = (uint8_t *)frame + 0x0e + frame->size;
            frame->size = frame->size + 4;
            if (first != 0) {
                hs_thread_push(called_script->root_expression_index, thread_index, scratch);
            } else {
                hs_thread_return(*(int32_t *)scratch, thread_index);
            }
        }
    }

    if ((void *)thread->stack == (void *)&thread->stack_data) {
        if (thread->type == _hs_thread_script) {
            if (script->script_type == _hs_script_startup ||
                script->script_type == _hs_script_dormant) {
                /* A one-shot script that has run: park it forever and clear the running-thread
                   marker. This is the ONLY exit that resets hs_current_thread_index (0x48a52b);
                   every other path leaves it pointing at this thread. */
                thread->wake_tick = -1;
                hs_current_thread_index = -1;
                return;
            }
        } else if (thread->type == _hs_thread_command) {
            datum_delete(hs_thread_data, thread_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x48a370):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0048a370(uint param_1)

{
  byte bVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int local_8;
  uint local_4;

  iVar3 = DAT_0087a470;
  iVar4 = (param_1 & 0xffff) * 0x218;
  iVar5 = *(int *)(DAT_0087a470 + 0x34) + iVar4;
  local_8 = 0;
  _DAT_006b15ea = (undefined2)param_1;
  if (*(char *)(*(int *)(DAT_0087a470 + 0x34) + 2 + iVar4) == '\0') {
    local_8 = *(int *)(iVar5 + 4) * 0x5c + *(int *)(DAT_00746f8c + 0x4a0);
  }
  *(undefined4 *)(iVar5 + 8) = 0;
  if (*(int *)(iVar5 + 0x10) == iVar5 + 0x18) {
    *(undefined2 *)(*(int *)(iVar5 + 0x10) + 0xc) = 0;
    iVar3 = *(int *)(*(int *)(iVar3 + 0x34) + 0x10 + iVar4);
    *(short *)(iVar3 + 0xc) = *(short *)(iVar3 + 0xc) + 4;
    FUN_0048a560();
  }
  if (*(int *)(iVar5 + 0x10) != iVar5 + 0x18) {
    do {
      if ((*(int *)(iVar5 + 8) < 0) ||
         (((*DAT_006f1d6c != '\0' &&
           (((DAT_006f1d6c[1] != '\0' || (DAT_006f1d6c[2] != '\0')) &&
            (*(int *)(DAT_006f1d6c + 0xc) < *(int *)(iVar5 + 8))))) || (DAT_006b15e8 == '\0'))))
      break;
      iVar3 = *(int *)(DAT_0087a474 + 0x34) +
              (*(uint *)(*(int *)(iVar5 + 0x10) + 4) & 0xffff) * 0x14;
      bVar1 = *(byte *)(iVar5 + 3);
      *(undefined2 *)(*(int *)(iVar5 + 0x10) + 0xc) = 0;
      *(byte *)(iVar5 + 3) = *(byte *)(iVar5 + 3) & 0xfe;
      sVar2 = *(short *)(iVar3 + 2);
      local_4 = CONCAT31(local_4._1_3_,bVar1) & 0xffffff01;
      if ((*(byte *)(iVar3 + 6) & 2) == 0) {
        (**(code **)((&PTR_DAT_00688b58)[sVar2] + 0xc))((int)sVar2,param_1,local_4);
      }
      else {
        iVar3 = *(int *)(*(int *)(DAT_0087a470 + 0x34) + 0x10 + iVar4);
        *(short *)(iVar3 + 0xc) = *(short *)(iVar3 + 0xc) + 4;
        if ((bVar1 & 1) == 0) {
          FUN_0048a640();
        }
        else {
          FUN_0048a560();
        }
      }
    } while (*(int *)(iVar5 + 0x10) != iVar5 + 0x18);
  }
  if (*(int *)(iVar5 + 0x10) == iVar5 + 0x18) {
    if (*(char *)(iVar5 + 2) == '\0') {
      if ((*(short *)(local_8 + 0x20) == 0) || (*(short *)(local_8 + 0x20) == 1)) {
        *(undefined4 *)(iVar5 + 8) = 0xffffffff;
        _DAT_006b15ea = 0xffff;
        return;
      }
    }
    else if (*(char *)(iVar5 + 2) == '\x02') {
      datum_delete();
    }
  }
  _DAT_006b15ea = 0xffff;
  return;
}
#endif
