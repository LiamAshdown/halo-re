// hs_thread_new  (Ghidra: hs_thread_new, already named)
// address 0x48a2f0, size 127 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: types/hs.h hs_thread (stack 0x10, stack_data 0x18, type 0x02, script_index 0x04,
//   flags 0x03, wake_tick 0x08) and hs_thread_type/hs_script_type; types/tags.h ScenarioScript
//   (script_type at 0x20).
// register convention: script index in EAX (in_EAX); thread type as the recognized stack
//   parameter (param_1).
//   // blam-cc: EAX -> script_index, stack -> type

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern datum_index datum_new(data_array *array); // blam-cc: EDX; memory module, 0x4d0480

extern data_array *hs_thread_data; // 0x0087a470
extern Scenario *global_scenario;  // 0x00746f8c

// Allocates a new hs_thread of the given `type`, associated with `script_index` (or -1 for none).
// A thread for a dormant (hs_script_type 1) script starts parked at wake_tick -2; every other
// thread starts runnable at wake_tick 0. Returns the new thread's handle, or k_datum_index_none
// if the thread table is full.
datum_index hs_thread_new(int32_t script_index, uint8_t type)
{
    datum_index handle;
    hs_thread *thread;
    ScenarioScript *scripts;

    handle = datum_new(hs_thread_data);
    if (handle != k_datum_index_none) {
        thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (handle & 0xffff) * 0x218);
        thread->stack = (hs_stack_frame *)&thread->stack_data;
        thread->stack->previous = 0;
        thread->stack->size = 0;
        thread->stack->syntax_node = k_datum_index_none;
        thread->type = type;
        thread->script_index = script_index;
        thread->flags = 0;

        scripts = (ScenarioScript *)global_scenario->scripts.pointer;
        if (script_index != -1 && scripts[script_index].script_type == _hs_script_dormant) {
            thread->wake_tick = -2;
            return handle;
        }
        thread->wake_tick = 0;
    }
    return handle;
}

#if 0
Original Ghidra decompilation (0x48a2f0):

void hs_thread_new(undefined1 param_1)

{
  int in_EAX;
  int iVar1;
  undefined8 uVar2;

  uVar2 = datum_new();
  if ((uint)uVar2 != 0xffffffff) {
    iVar1 = ((uint)uVar2 & 0xffff) * 0x218 + *(int *)((int)((ulonglong)uVar2 >> 0x20) + 0x34);
    *(undefined4 **)(iVar1 + 0x10) = (undefined4 *)(iVar1 + 0x18);
    *(undefined4 *)(iVar1 + 0x18) = 0;
    *(undefined2 *)(*(int *)(iVar1 + 0x10) + 0xc) = 0;
    *(undefined4 *)(*(int *)(iVar1 + 0x10) + 4) = 0xffffffff;
    *(undefined1 *)(iVar1 + 2) = param_1;
    *(int *)(iVar1 + 4) = in_EAX;
    *(undefined1 *)(iVar1 + 3) = 0;
    if ((in_EAX != -1) && (*(short *)(*(int *)(DAT_00746f8c + 0x4a0) + 0x20 + in_EAX * 0x5c) == 1))
    {
      *(undefined4 *)(iVar1 + 8) = 0xfffffffe;
      return;
    }
    *(undefined4 *)(iVar1 + 8) = 0;
  }
  return;
}
#endif
