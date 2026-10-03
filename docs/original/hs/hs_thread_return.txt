// hs_thread_return  (Ghidra: FUN_0048a640; named per out/phase4/hs_types_notes.md's own repeated
// references to "hs_thread_return (0x48a640) writes through **(void ***)(*thread->stack + 8)")
// address 0x48a640, size 212 bytes
// name confidence: 0.85  rewrite confidence: 0.9
// evidence: types/hs.h hs_syntax_node (index_union 0x02, type 0x04, flags 0x06,
//   _hs_syntax_node_script_call_bit), hs_function_definition (return_type at +0, the first
//   field), types/tags.h ScenarioScript (return_type at 0x22), hs_thread/hs_stack_frame (stack
//   0x10, result_address +8); hs_thread_pop_frame's identical inline pop at the end.
// register convention: value to return in EAX (in_EAX); thread index in ECX (in_ECX) -- NOTE this
//   is ECX, not EDX like hs_thread_push, despite both being "the current thread".
//   // blam-cc: EAX -> value, ECX -> thread_index
// FIXED (verified against 0x48a6f3..0x48a712): the finished value is written through the PARENT
// frame's result_address, not the current frame's --
//     mov ecx,[edi+0x10]   ; ecx = thread->stack        (the frame being finished)
//     mov edx,[ecx]        ; edx = frame->previous
//     mov ecx,[edx+8]      ; ecx = previous->result_address
//     mov [ecx],eax
// which is the other half of hs_thread_push's design: push stores "where this child's result
// goes" on the frame it is suspending, so a parent with several children rewrites that one slot
// before each child. The current frame's own +8 is never written when it is created, so the
// earlier `*frame->result_address = value` read an uninitialized pointer.
// VERIFIED: hs_type_conversion_procedures entries are called unconditionally (`push eax` then
// `call dword [ecx*4+0x68bc10]` at 0x48a6e8), with no NULL test; object_lookup_table_get is a plain
// `call` at 0x48a6d6 with the value already in EAX. The EAX in/out convention is confirmed by
// hs_coerce_value @0x48ad10, which is this same block compiled as a standalone function.
// UNSURE: callers pass `value` via a bare call with no visible EAX write, so which value each
// caller intends is still inferred from that caller's own local state (documented per call site).

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t object_lookup_table_get(int32_t value); // UNSURE args; module unknown, 0x4f73c0

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern Scenario *global_scenario; // 0x00746f8c
extern int32_t (*hs_type_conversion_procedures[k_hs_type_count][k_hs_type_count])(int32_t value);
    // 0x0068bc10, indexed [destination_type][source_type]; UNSURE signature

// Finishes evaluating the current frame's syntax node with the given `value`: converts it from
// the node's actual return type (a script's declared return type, or a function definition's) to
// the type its parent context expects (unless they already match, the source is "passthrough",
// or the expected type is in the object-name family 0x2b..0x30, which this function does not
// convert into), writes the (possibly converted) value through the current frame's
// result_address, and pops the frame.
void hs_thread_return(int32_t value, uint32_t thread_index)
{
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    hs_type_t actual_type;
    hs_type_t expected_type;
    ScenarioScript *scripts;

    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread->stack;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (frame->syntax_node & 0xffff) * 0x14);

    if ((node->flags & _hs_syntax_node_script_call_bit) == 0) {
        actual_type = hs_function_definitions[node->index_union]->return_type;
    } else {
        scripts = (ScenarioScript *)global_scenario->scripts.pointer;
        actual_type = scripts[node->index_union].return_type;
    }

    expected_type = node->type;
    if (actual_type != expected_type && actual_type != _hs_type_passthrough &&
        (expected_type < 0x2b || 0x30 < expected_type)) {
        if (expected_type < 0x25 || 0x2a < expected_type) {
            // no NULL test in retail -- see the VERIFIED note above
            value = hs_type_conversion_procedures[expected_type][actual_type](value);
        } else if (0x2a < actual_type && actual_type < 0x31) {
            value = object_lookup_table_get(value);
        }
    }

    *(int32_t *)frame->previous->result_address = value;
    thread->stack = frame->previous;
}

#if 0
Original Ghidra decompilation (0x48a640):

void FUN_0048a640(void)

{
  int iVar1;
  short sVar2;
  int iVar3;
  undefined4 in_EAX;
  uint in_ECX;
  short sVar4;
  int iVar5;

  iVar5 = (in_ECX & 0xffff) * 0x218;
  iVar3 = *(int *)(DAT_0087a470 + 0x34);
  iVar1 = *(int *)(DAT_0087a474 + 0x34) +
          (*(uint *)(*(int *)(iVar3 + 0x10 + iVar5) + 4) & 0xffff) * 0x14;
  if ((*(byte *)(iVar1 + 6) & 2) == 0) {
    sVar4 = *(short *)(&PTR_DAT_00688b58)[*(short *)(iVar1 + 2)];
  }
  else {
    sVar4 = *(short *)(*(short *)(iVar1 + 2) * 0x5c + 0x22 + *(int *)(DAT_00746f8c + 0x4a0));
  }
  sVar2 = *(short *)(iVar1 + 4);
  if (((sVar4 != sVar2) && (sVar4 != 3)) && ((sVar2 < 0x2b || (0x30 < sVar2)))) {
    if ((sVar2 < 0x25) || (0x2a < sVar2)) {
      in_EAX = (**(code **)(&DAT_0068bc10 + (sVar2 * 0x31 + (int)sVar4) * 4))();
    }
    else if ((0x2a < sVar4) && (sVar4 < 0x31)) {
      in_EAX = FUN_004f73c0();
    }
  }
  iVar1 = DAT_0087a470;
  **(undefined4 **)(**(int **)(iVar3 + iVar5 + 0x10) + 8) = in_EAX;
  *(undefined4 *)(*(int *)(iVar1 + 0x34) + iVar5 + 0x10) =
       **(undefined4 **)(*(int *)(iVar1 + 0x34) + 0x10 + iVar5);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
