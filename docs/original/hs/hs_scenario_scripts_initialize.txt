// hs_scenario_scripts_initialize  (Ghidra: hs_scenario_scripts_initialize, already named)
// address 0x489ef0, size 576 bytes
// name confidence: 0.55 (out/phase4/hs_functions.md: "Resets the HS runtime for a newly loaded
//   scenario: reinitializes global variables to their default values and auto-starts all
//   non-static, non-stub scripts")
// rewrite confidence: 0.6
// evidence: types/tags.h ScenarioGlobal (type 0x20, initialization_expression_index 0x28) and
//   ScenarioScript (script_type 0x20, root_expression_index 0x28); types/hs.h hs_thread,
//   hs_stack_frame, hs_global, object_list_header::reference_count (whose increment here is
//   hs.h's own cited evidence for that field); src/memory's established datum_new/datum_delete/
//   datum_element_initialize signatures.
// register convention: none (void); cc unresolved by Ghidra but no stack parameters were
//   recognized either.
// UNSURE (substantial): hs_thread_push/hs_thread_evaluate/hs_global_get_value/
// hs_global_write_value/hs_thread_new are all called with zero or partial visible arguments;
// the bindings below are reconstructed from the surrounding writes (e.g. hs_thread_push's node
// argument is inferred to be `global->initialization_expression_index` because that is the only
// thing written into the thread immediately before the call) and from types/hs.h's prose, not
// observed directly. This should be revisited once those five functions are independently
// rewritten and their real signatures are known.

// FIXED (verified against 0x489fd3..0x48a0b1): the initializer thread pushes each global's
// initialization expression with the RUNTIME GLOBAL's value slot as the result address, not
// init_thread->result; and hs_global_get_value / hs_global_write_value take the packed
// hs_global_reference, not the datum index derived from it. Both were conflated before.
#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void data_delete_all(data_array *array); // blam-cc: ESI; memory module, 0x4d0580
extern datum_index datum_new(data_array *array); // blam-cc: EDX; memory module, 0x4d0480
extern void datum_delete(data_array *array, datum_index handle);
    // blam-cc: EAX -> array, EDX -> handle; memory module, 0x4d0510
extern void datum_element_initialize(data_array *array, void *element);
    // blam-cc: EDX -> array, ESI -> element; memory module, 0x4d06c0

extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address);
    // this module, 0x48a560; UNSURE, see header note
extern void hs_thread_evaluate_step(datum_index thread_handle); // this module, 0x48a370; UNSURE
extern int32_t hs_global_get_value(hs_global_reference reference); // this module, 0x48a720 (misnamed
    // hs_global_get_value_pointer in Ghidra; out/phase4/hs_types_notes.md renames it); UNSURE args
extern void hs_global_write_value(hs_global_reference reference);
    // blam-cc: EAX -> reference; this module, 0x48b030
extern datum_index hs_thread_new(int32_t script_index, uint8_t type); // this module, 0x48a2f0;
    // blam-cc: script_index UNSURE register, stack -> type

extern data_array *hs_thread_data;             // 0x0087a470
extern data_array *hs_globals_data;            // 0x0087a46c
extern data_array *object_list_header_data;    // 0x0087a464
extern uint8_t hs_runtime_active;              // 0x006b15e8
extern int16_t hs_current_thread_index;        // 0x006b15ea
extern datum_index global_scenario_index;      // 0x0069e8d4
extern Scenario *global_scenario;              // 0x00746f8c

// Resets the HS thread table and marks the runtime active, then (if a scenario is loaded)
// evaluates every scenario global's default-value expression via a transient type-1 thread
// (bumping the referenced object_list's holder count when a global's type is object_list), and
// finally auto-starts (hs_thread_new) every script whose type is not static (3) or stub (4).
void hs_scenario_scripts_initialize(void)
{
    hs_thread *init_thread;
    hs_stack_frame *frame;
    datum_index thread_handle;
    ScenarioGlobal *globals;
    ScenarioScript *scripts;
    int32_t i;
    int16_t slot;
    hs_global_reference reference;
    hs_global *global_slot;
    int32_t list_handle;
    object_list_header *list_header;

    hs_thread_data->valid = 1;
    data_delete_all(hs_thread_data);
    hs_runtime_active = 1;
    hs_current_thread_index = -1;

    thread_handle = datum_new(hs_thread_data);
    init_thread = 0;
    if (thread_handle != k_datum_index_none) {
        init_thread = (hs_thread *)((uint8_t *)hs_thread_data->data +
            (thread_handle & 0xffff) * 0x218);
        init_thread->stack = (hs_stack_frame *)&init_thread->stack_data;
        init_thread->stack->previous = 0;
        init_thread->stack->size = 0;
        init_thread->stack->syntax_node = k_datum_index_none;
        init_thread->type = 1; // transient global-initializer thread
        init_thread->script_index = -1;
        init_thread->flags = 0;
        init_thread->wake_tick = 0;
    }

    if (global_scenario_index == k_datum_index_none) {
        return;
    }

    if (init_thread != 0) {
        globals = (ScenarioGlobal *)global_scenario->globals.pointer;
        for (i = 0; i < (int32_t)global_scenario->globals.count; i++) {
            /* Two different encodings, which the earlier rewrite conflated. `reference` is the
               packed hs_global_reference the loop carries ([esp+0x14]); `slot` is the datum index
               it maps to -- builtin references index hs_globals_data directly, scenario ones sit
               after the 0x1eb builtins (0x489fd3..0x48a03b). */
            reference = (hs_global_reference)i;
            slot = (int16_t)(reference & k_hs_global_index_mask) +
                   ((reference & k_hs_global_builtin_bit) ? 0 : k_hs_builtin_global_count);

            if (-1 < slot && slot < hs_globals_data->maximum_count) {
                global_slot = (hs_global *)((uint8_t *)hs_globals_data->data +
                    hs_globals_data->size * slot);
                if (global_slot->identifier == 0) {
                    hs_globals_data->actual_count = hs_globals_data->actual_count + 1;
                    if (hs_globals_data->last_index <= slot) {
                        hs_globals_data->last_index = slot + 1;
                    }
                    datum_element_initialize(hs_globals_data, global_slot);
                    global_slot->identifier = (int16_t)0xaced;
                }
            }

            init_thread->script_index = -1;
            init_thread->stack->size = 0;
            /* 0x48a050 `lea eax,[edx+eax*8]` then 0x48a060 `lea ebx,[eax+4]`: the initializer
               writes straight into the runtime global's value slot, recomputed here from `slot`
               regardless of the guard above -- NOT into init_thread->result. */
            hs_thread_push(globals[i].initialization_expression_index, thread_handle,
                &((hs_global *)hs_globals_data->data)[slot & 0xffff].value);
            if ((init_thread->flags & 1) != 0) {
                hs_thread_evaluate_step(thread_handle);
                if (globals[i].type == 0x17) { // _hs_type_object_list
                    list_handle = hs_global_get_value(reference); // EAX = [esp+0x14], 0x48a087
                    if (list_handle != -1) {
                        list_header = (object_list_header *)((uint8_t *)object_list_header_data->data +
                            (list_handle & 0xffff) * 0x0c);
                        list_header->reference_count = list_header->reference_count + 1;
                    }
                }
            }
            hs_global_write_value(reference); // same packed reference, 0x48a0b1
        }
        datum_delete(hs_thread_data, thread_handle);
    }

    scripts = (ScenarioScript *)global_scenario->scripts.pointer;
    for (i = 0; i < (int32_t)global_scenario->scripts.count; i++) {
        if (scripts[i].script_type != 3 && scripts[i].script_type != 4) {
            hs_thread_new(i, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x489ef0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void hs_scenario_scripts_initialize(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  ushort uVar8;
  int iVar9;
  ushort uVar10;
  short *psVar11;
  short sVar12;

  *(undefined1 *)(DAT_0087a470 + 0x24) = 1;
  data_delete_all();
  iVar9 = DAT_0087a470;
  DAT_006b15e8 = 1;
  _DAT_006b15ea = 0xffff;
  uVar4 = datum_new();
  if (uVar4 != 0xffffffff) {
    iVar5 = (uVar4 & 0xffff) * 0x218 + *(int *)(iVar9 + 0x34);
    *(undefined4 **)(iVar5 + 0x10) = (undefined4 *)(iVar5 + 0x18);
    *(undefined4 *)(iVar5 + 0x18) = 0;
    *(undefined2 *)(*(int *)(iVar5 + 0x10) + 0xc) = 0;
    *(undefined4 *)(*(int *)(iVar5 + 0x10) + 4) = 0xffffffff;
    *(undefined1 *)(iVar5 + 2) = 1;
    *(undefined4 *)(iVar5 + 4) = 0xffffffff;
    *(undefined1 *)(iVar5 + 3) = 0;
    *(undefined4 *)(iVar5 + 8) = 0;
  }
  iVar5 = DAT_00746f8c;
  if (DAT_0069e8d4 != -1) {
    iVar6 = 0;
    iVar9 = (uVar4 & 0xffff) * 0x218 + *(int *)(iVar9 + 0x34);
    uVar10 = 0;
    if (0 < *(int *)(DAT_00746f8c + 0x4a8)) {
      do {
        iVar3 = DAT_0087a46c;
        iVar2 = *(int *)(DAT_00746f8c + 0x4ac);
        if ((uVar10 & 0x8000) == 0) {
          uVar8 = ((ushort)iVar6 & 0x7fff) + 0x1eb;
        }
        else {
          uVar8 = (ushort)iVar6 & 0x7fff;
        }
        if (((-1 < (short)uVar8) && ((short)uVar8 < *(short *)(DAT_0087a46c + 0x20))) &&
           (psVar11 = (short *)((int)*(short *)(DAT_0087a46c + 0x22) * (int)(short)uVar8 +
                               *(int *)(DAT_0087a46c + 0x34)), *psVar11 == 0)) {
          *(short *)(DAT_0087a46c + 0x30) = *(short *)(DAT_0087a46c + 0x30) + 1;
          if (*(short *)(iVar3 + 0x2e) <= (short)uVar8) {
            *(ushort *)(iVar3 + 0x2e) = uVar8 + 1;
          }
          FUN_004d06c0();
          *psVar11 = -0x5313;
        }
        *(undefined4 *)(iVar9 + 4) = 0xffffffff;
        *(undefined2 *)(*(int *)(iVar9 + 0x10) + 0xc) = 0;
        FUN_0048a560();
        if ((((*(byte *)(iVar9 + 3) & 1) != 0) &&
            (FUN_0048a370(uVar4), *(short *)(iVar6 * 0x5c + iVar2 + 0x20) == 0x17)) &&
           (uVar7 = hs_global_get_value_pointer(), uVar7 != 0xffffffff)) {
          psVar11 = (short *)(*(int *)(DAT_0087a464 + 0x34) + 4 + (uVar7 & 0xffff) * 0xc);
          *psVar11 = *psVar11 + 1;
        }
        hs_global_write_value();
        uVar10 = uVar10 + 1;
        iVar6 = (int)(short)uVar10;
      } while (iVar6 < *(int *)(iVar5 + 0x4a8));
    }
    datum_delete();
    sVar12 = 0;
    if (0 < *(int *)(iVar5 + 0x49c)) {
      iVar9 = 0;
      do {
        sVar1 = *(short *)(iVar9 * 0x5c + *(int *)(iVar5 + 0x4a0) + 0x20);
        if ((sVar1 != 3) && (sVar1 != 4)) {
          hs_thread_new(0);
        }
        sVar12 = sVar12 + 1;
        iVar9 = (int)sVar12;
      } while (iVar9 < *(int *)(iVar5 + 0x49c));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
