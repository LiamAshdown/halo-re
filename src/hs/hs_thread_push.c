// hs_thread_push  (Ghidra: FUN_0048a560; named per out/phase4/hs_types_notes.md's "frame layout
// from hs_thread_push (0x48a560): new = frame + 0x10 + frame->size; new->previous = frame;
// new->size = 0; new->syntax_node = node; and the outgoing frame gets frame->result_address =
// EBX")
// address 0x48a560, size 212 bytes
// name confidence: 0.85  rewrite confidence: 0.9
// evidence: types/hs.h hs_stack_frame (previous 0x00, syntax_node 0x04, result_address 0x08,
//   size 0x0c), hs_thread (flags 0x03, stack 0x10), hs_syntax_node (flags 0x06,
//   _hs_syntax_node_primitive_bit, _hs_syntax_node_global_bit).
// register convention: node index in EAX (in_EAX); thread index in EDX (in_EDX); result address
//   in EBX (unaff_EBX).
//   // blam-cc: EAX -> node, EDX -> thread_index, EBX -> result_address
// RESOLVED (was UNSURE): both callees' register arguments were read straight off the retail
// bytes at 0x48a5a1..0x48a5f9.
//   hs_global_get_value @0x48a720  EAX = the raw hs_global_reference (`movzx eax, [esi+0x10]`).
//   hs_coerce_value     @0x48ad10  EAX = value, DX = SOURCE type, CX = DESTINATION type.
// The earlier rewrite passed node->type for both types, which made the coercion a no-op; that was
// wrong in both branches. What retail actually does:
//   * global node: the source type is the GLOBAL's declared type, looked up before the call --
//     bit 15 set means hs_global_definitions[ref & 0x7fff]->type (word at +4, 0x48a5b3), clear
//     means global_scenario->globals.pointer[ref & 0x7fff].type (stride 0x5c, +0x20, 0x48a5c0).
//     The destination type is node->type.
//   * plain constant: the source type is node->index_union (`mov dx,[esi+2]`, 0x48a5f1) -- for a
//     primitive that field holds the constant's own type -- and the destination is node->type.
// VERIFIED: the frame push really does write result_address into the OUTGOING (parent) frame
// rather than the new one. hs_thread_return @0x48a640 confirms the design: it stores the finished
// value through `thread->stack->previous->result_address` (0x48a6f3..0x48a701), so the parent
// frame carries "where this child's result goes" and hs_thread_push rewrites it before each
// child.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern int32_t hs_global_get_value(hs_global_reference reference);
    // blam-cc: EAX -> reference; this module, 0x48a720
extern int32_t hs_coerce_value(int32_t value, hs_type_t dest_type, hs_type_t source_type);
    // blam-cc: EAX -> value, CX -> dest_type, DX -> source_type; this module, 0x48ad10

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern Scenario *global_scenario;  // 0x00746f8c

// Evaluates `node` as the next step for `thread_index`: if it is a non-primitive expression,
// pushes a new stack frame to evaluate its children (recording `result_address` in the outgoing
// frame first, per the docs above). If it is a primitive, resolves its value immediately (via
// the global-variable path when the node has the global bit set, otherwise directly) and stores
// it straight into `*result_address` without pushing a frame.
void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address)
{
    hs_syntax_node *syntax_node;
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_stack_frame *new_frame;
    int32_t value;
    hs_global_reference reference;
    uint16_t index;
    hs_type_t source_type;

    syntax_node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & 0xffff) * 0x14);
    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);

    if ((syntax_node->flags & _hs_syntax_node_primitive_bit) == 0) {
        thread->stack->result_address = result_address;
        frame = thread->stack;
        new_frame = (hs_stack_frame *)((uint8_t *)frame + 0x10 + frame->size);
        new_frame->previous = frame;
        thread->stack = new_frame;
        new_frame->size = 0;
        thread->flags = thread->flags | 1;
        new_frame->syntax_node = node;
        return;
    }

    if ((syntax_node->flags & _hs_syntax_node_global_bit) != 0) {
        reference = (hs_global_reference)syntax_node->data.global_reference;
        index = reference & k_hs_global_index_mask;
        if ((reference & k_hs_global_builtin_bit) != 0) {
            source_type = hs_global_definitions[index]->type;
        } else {
            source_type = ((ScenarioGlobal *)global_scenario->globals.pointer)[index].type;
        }
        value = hs_global_get_value(reference);
        value = hs_coerce_value(value, syntax_node->type, source_type);
        *(int32_t *)result_address = value;
        return;
    }
    value = hs_coerce_value(syntax_node->data.long_value, syntax_node->type,
                            (hs_type_t)syntax_node->index_union);
    *(int32_t *)result_address = value;
}

#if 0
Original Ghidra decompilation (0x48a560):

void FUN_0048a560(void)

{
  int *piVar1;
  byte bVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  undefined4 uVar5;
  uint in_EDX;
  int iVar6;
  undefined4 *unaff_EBX;

  iVar3 = DAT_0087a470;
  iVar6 = (in_EDX & 0xffff) * 0x218;
  bVar2 = *(byte *)(*(int *)(DAT_0087a474 + 0x34) + (in_EAX & 0xffff) * 0x14 + 6);
  iVar4 = *(int *)(DAT_0087a470 + 0x34) + iVar6;
  if ((bVar2 & 1) == 0) {
    *(undefined4 **)(*(int *)(iVar4 + 0x10) + 8) = unaff_EBX;
    iVar6 = *(int *)(iVar3 + 0x34) + iVar6;
    iVar3 = *(int *)(iVar6 + 0x10);
    piVar1 = (int *)(*(short *)(iVar3 + 0xc) + 0x10 + iVar3);
    *piVar1 = iVar3;
    *(int **)(iVar6 + 0x10) = piVar1;
    *(undefined2 *)(piVar1 + 3) = 0;
    *(byte *)(iVar4 + 3) = *(byte *)(iVar4 + 3) | 1;
    *(uint *)(*(int *)(iVar4 + 0x10) + 4) = in_EAX;
    return;
  }
  if ((bVar2 & 4) != 0) {
    hs_global_get_value_pointer();
    uVar5 = FUN_0048ad10();
    *unaff_EBX = uVar5;
    return;
  }
  uVar5 = FUN_0048ad10();
  *unaff_EBX = uVar5;
  return;
}
#endif
