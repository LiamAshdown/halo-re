// path_find_heap_sift_down  (Ghidra: path_find_heap_sift_down, already named)
// address 0x43b010, size 213 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: types/ai.h path_find_context.heap_count (+0xd084) / heap (+0xd086) /
//   path_find_node.heap_index (+0xb4), same cross-check as path_find_heap_sift_up.c.
// register convention: EAX -> context, CX -> index (the one-based heap slot to sift down).
//   // blam-cc: EAX -> context, ECX -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

// blam-cc: EAX -> context, ECX -> index
void path_find_heap_sift_down(path_find_context *context, int16_t index)
{
    int16_t node = context->heap[index].node;
    int16_t key = context->heap[index].key;
    int16_t child_slot;
    int16_t best_slot;
    int16_t best_node;
    int16_t best_key;
    int16_t i;

    for (;;) {
        best_slot = index;
        best_node = node;
        best_key = key;
        child_slot = index * 2;

        for (i = 0; i < 2; i = i + 1) {
            if (context->heap_count <= child_slot) {
                break;
            }
            if (context->heap[child_slot].key < best_key) {
                best_node = context->heap[child_slot].node;
                best_slot = child_slot;
                best_key = context->heap[child_slot].key;
            }
            child_slot = child_slot + 1;
        }

        if (best_slot == index) {
            context->heap[index].key = key;
            context->heap[index].node = node;
            context->nodes[node].heap_index = index;
            return;
        }

        context->heap[index].key = best_key;
        context->heap[index].node = best_node;
        context->nodes[best_node].heap_index = index;
        index = best_slot;
    }
}

#if 0
// ---- original Ghidra decompilation (path_find_heap_sift_down @ 0x43b010) ----
void path_find_heap_sift_down(void)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  int in_EAX;
  short in_CX;
  short sVar5;
  int iVar6;
  short sVar7;
  short sVar8;
  short sVar9;

  sVar1 = *(short *)(in_EAX + 0xd086 + in_CX * 4);
  sVar2 = *(short *)(in_EAX + 0xd088 + in_CX * 4);
  do {
    sVar9 = 0;
    sVar5 = in_CX * 2;
    sVar4 = in_CX;
    sVar8 = sVar1;
    sVar7 = sVar2;
    do {
      if (*(short *)(in_EAX + 0xd084) <= sVar5) break;
      sVar3 = *(short *)(in_EAX + 0xd088 + sVar5 * 4);
      if (sVar3 < sVar7) {
        sVar8 = *(short *)(in_EAX + 0xd086 + sVar5 * 4);
        sVar4 = sVar5;
        sVar7 = sVar3;
      }
      sVar9 = sVar9 + 1;
      sVar5 = sVar5 + 1;
    } while (sVar9 < 2);
    iVar6 = (int)in_CX;
    if (sVar4 == in_CX) {
      *(short *)(in_EAX + 0xd088 + iVar6 * 4) = sVar2;
      *(short *)(in_EAX + 0xd086 + iVar6 * 4) = sVar1;
      *(short *)(sVar1 * 0x34 + 0xb4 + in_EAX) = in_CX;
      return;
    }
    *(short *)(in_EAX + 0xd088 + iVar6 * 4) = sVar7;
    *(short *)(in_EAX + 0xd086 + iVar6 * 4) = sVar8;
    *(short *)(sVar8 * 0x34 + 0xb4 + in_EAX) = in_CX;
    in_CX = sVar4;
  } while( true );
}
#endif
