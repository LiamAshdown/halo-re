// hs_evaluate_random  (Ghidra: hs_evaluate_random, already named)
// address 0x488c60, size 508 bytes
// name confidence: 0.55 (out/phase4/hs_functions.md: "Implements the HS 'random' special form:
//   randomly selects and evaluates one not-yet-chosen child expression per invocation, avoiding
//   repeats until the set is exhausted")
// rewrite confidence: 0.8
// evidence: types/hs.h hs_thread (stack at 0x10), hs_stack_frame (size at 0x0c, scratch at
//   0x0e), hs_random_state (child_count 0x00, chosen 0x02, exactly the three scratch bumps here:
//   2 + 4 + 4), hs_syntax_node (next_node at 0x08, data.first_child at 0x10), and the hs_thread_push
//   mechanics documented against this address's own raw bytes.
// register convention: thread pointer/handle in EAX (param_1, per hs_evaluate handler convention
//   `void (*)(int16_t function_index, datum_index thread, char first)`); thread index as the
//   recognized stack parameter (param_2, redundant with param_1 -- both select the same thread
//   record; kept as decompiled rather than merged); `first` (whether this is the special form's
//   first evaluation, triggering setup) as param_3.
// UNSURE: hs_thread_push (0x48a560) and hs_thread_return (0x48a640) are called with zero visible
// arguments; both are rewritten by this module later (0x48a560, 0x48a640) and their signatures
// there should be reconciled with the guesses used here (thread, chosen child node, and the
// current frame's own result_address forwarded through). The dead "count down to zero" loop
// before the push (originally `if (0 < sVar10) { do { uVar9--; } while (uVar9 != 0); }`) has no
// observable effect and is kept for fidelity.

// FIXED (verified against the retail bytes): the third scratch slot (4 bytes, allocated at
// 0x488cd0 and pointed at by [esp+0x14]) is the chosen child's result slot. hs_thread_push takes
// it as EBX at 0x488e0f -- not frame->result_address -- and the terminal hs_thread_return hands
// back its contents (`mov ecx,[esp+4] / mov eax,[ecx]`, 0x488e46), not 0.
// NOTE: that slot sits immediately after the chosen bitmap's first word, and the bitmap clear on
// the first call zeroes ((child_count + 31) / 32) words -- so a `random` form with more than 32
// alternatives clears the result slot as part of the bitmap. k_hs_maximum_random_children is 0x40
// only in the sense that the bit arithmetic can address that many.
// FIXED (verified against the retail bytes): an evaluated call node's first child is the function-name
//   node; the original starts at its next_node ([first_child*0x14 + 8]), so arguments begin at the second
//   child. The draft started at the name node itself (scripts ran with misaligned arguments).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address);
    // this module, 0x48a560; blam-cc: EBX -> result_address (documented in types/hs.h), the
    // other two registers UNSURE
extern void hs_thread_return(int32_t value, uint32_t thread_index); // this module, 0x48a640; UNSURE args

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern random_seed random_seed_global; // 0x00719cd0

// Evaluate handler for the 'random' special form. On the first call (`first` set), counts the
// children of the syntax node being evaluated and clears the "already chosen" bitmap. Each call
// (first or not) advances the shared LCG seed, then scans for an unchosen child starting from a
// seed-derived index, wrapping modulo the child count; the first unchosen child found is marked
// chosen and pushed as a new stack frame to be evaluated. If every child has already been chosen,
// calls hs_thread_return instead (the form is exhausted).
void hs_evaluate_random(hs_thread *thread, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_random_state *state;
    hs_syntax_node *node;
    datum_index child;
    int16_t child_count;
    uint32_t *chosen_words;
    int32_t *child_value;
    uint32_t word_count;
    uint32_t i;
    int16_t scan;
    uint32_t candidate;
    int16_t chosen_index;
    datum_index chosen_child;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread_record->stack;

    state = (hs_random_state *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;
    frame = thread_record->stack;
    chosen_words = (uint32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread_record->stack;
    child_value = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & 0xffff) * 0x14);
        child = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & 0xffff) * 0x14))->next_node;
        state->child_count = 0;
        while (child != k_datum_index_none) {
            node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (child & 0xffff) * 0x14);
            child = node->next_node;
            state->child_count = state->child_count + 1;
        }
        word_count = (uint32_t)(state->child_count + 0x1f) >> 5 & 0x3fffffff;
        for (i = 0; i < word_count; i++) {
            chosen_words[i] = 0;
        }
    }

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    scan = 0;
    child_count = state->child_count;
    chosen_index = child_count;
    if (0 < child_count) {
        do {
            candidate = (uint32_t)(scan + (int16_t)(((random_seed_global >> 0x10) *
                (uint32_t)child_count) >> 0x10)) % (uint32_t)child_count;
            chosen_index = (int16_t)candidate;
            if ((chosen_words[(int16_t)candidate >> 5] & (1 << (candidate & 0x1f))) == 0) {
                if (0 < chosen_index) {
                    uint32_t dead = candidate & 0xffff;
                    do {
                        dead = dead - 1;
                    } while (dead != 0);
                }
                node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                    (frame->syntax_node & 0xffff) * 0x14);
                chosen_child = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & 0xffff) * 0x14))->next_node;
                for (i = 0; i < (uint32_t)chosen_index; i++) {
                    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                        (chosen_child & 0xffff) * 0x14);
                    chosen_child = node->next_node;
                }
                /* 0x488e0f: `mov ebx,[esp+0x14]` -- scratch slot 3, the 4 bytes that
                   follow the chosen bitmap's first word. */
                hs_thread_push(chosen_child, thread_index, child_value);
                chosen_words[(int16_t)candidate >> 5] |= 1 << (candidate & 0x1f);
                break;
            }
            scan = scan + 1;
        } while (scan < child_count);
    }

    if (scan == state->child_count) {
        hs_thread_return(*child_value, thread_index); // `mov eax,[ecx]` at 0x488e4a, not 0
    }
}

#if 0
Original Ghidra decompilation (0x488c60):

void hs_evaluate_random(undefined4 param_1,uint param_2,char param_3)

{
  int *piVar1;
  short *psVar2;
  undefined4 *puVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  short sVar10;
  undefined4 *puVar11;

  iVar4 = DAT_0087a470;
  iVar6 = (param_2 & 0xffff) * 0x218;
  piVar1 = (int *)(*(int *)(DAT_0087a470 + 0x34) + 0x10 + iVar6);
  iVar8 = *piVar1;
  psVar2 = (short *)(*(short *)(iVar8 + 0xc) + 0xe + iVar8);
  *(short *)(iVar8 + 0xc) = *(short *)(iVar8 + 0xc) + 2;
  iVar8 = *(int *)(*(int *)(iVar4 + 0x34) + 0x10 + iVar6);
  puVar3 = (undefined4 *)(*(short *)(iVar8 + 0xc) + 0xe + iVar8);
  *(short *)(iVar8 + 0xc) = *(short *)(iVar8 + 0xc) + 4;
  iVar8 = *(int *)(*(int *)(iVar4 + 0x34) + 0x10 + iVar6);
  *(short *)(iVar8 + 0xc) = *(short *)(iVar8 + 0xc) + 4;
  iVar8 = DAT_0087a474;
  if (param_3 != '\0') {
    uVar7 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 +
                     (*(uint *)(*(int *)(DAT_0087a474 + 0x34) + 0x10 +
                               (*(uint *)(*piVar1 + 4) & 0xffff) * 0x14) & 0xffff) * 0x14);
    *psVar2 = 0;
    while (uVar7 != 0xffffffff) {
      uVar7 = *(uint *)(*(int *)(iVar8 + 0x34) + 8 + (uVar7 & 0xffff) * 0x14);
      *psVar2 = *psVar2 + 1;
    }
    puVar11 = puVar3;
    for (uVar7 = *psVar2 + 0x1f >> 5 & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
      *puVar11 = 0;
      puVar11 = puVar11 + 1;
    }
    for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
      *(undefined1 *)puVar11 = 0;
      puVar11 = (undefined4 *)((int)puVar11 + 1);
    }
  }
  DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
  sVar5 = 0;
  if (0 < *psVar2) {
    do {
      uVar7 = ((int)sVar5 + (int)(short)((DAT_00719cd0 >> 0x10) * (int)*psVar2 >> 0x10)) %
              (int)*psVar2;
      sVar10 = (short)uVar7;
      if ((puVar3[(int)sVar10 >> 5] & 1 << ((byte)uVar7 & 0x1f)) == 0) {
        if (0 < sVar10) {
          uVar9 = uVar7 & 0xffff;
          do {
            uVar9 = uVar9 - 1;
          } while (uVar9 != 0);
        }
        FUN_0048a560();
        puVar3[(int)sVar10 >> 5] = puVar3[(int)sVar10 >> 5] | 1 << ((byte)uVar7 & 0x1f);
        break;
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < *psVar2);
  }
  if (sVar5 == *psVar2) {
    FUN_0048a640();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
