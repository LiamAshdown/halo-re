// ai_search_heap_sift_down  (Ghidra: ai_search_heap_sift_down, already named)
// address 0x43b4d0, size 202 bytes
// name confidence: 0.55  rewrite confidence: 0.55
// evidence: types/ai.h ai_search_context (the heap count at +0x1430, cross-checked in the
//   module header against the same byte read as dword index 0x50c) / heap (+0x1432) /
//   ai_search_node.cost (+0x20), same offset arithmetic as ai_search_heap_sift_up.c.
// register convention: ECX -> context, DX -> index (the zero-based heap slot to sift down).
//   // blam-cc: ECX -> context, EDX -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: ECX -> context, EDX -> index
void ai_search_heap_sift_down(ai_search_context *context, int16_t index)
{
    int16_t count = *(int16_t *)((uint8_t *)context + 0x1430);
    int16_t left, right, smallest;

    if (index < count) {
        for (;;) {
            left = index * 2 + 1;
            right = index * 2 + 2;
            smallest = index;

            if ((left < count) &&
                (context->nodes[context->heap[left]].cost < context->nodes[context->heap[index]].cost)) {
                smallest = left;
            }
            if ((right < count) &&
                (context->nodes[context->heap[right]].cost < context->nodes[context->heap[smallest]].cost)) {
                smallest = right;
            }
            if (smallest == index) {
                break;
            }

            {
                int16_t tmp = context->heap[smallest];
                context->heap[smallest] = context->heap[index];
                context->heap[index] = tmp;
            }
            index = smallest;
        }
    }
}

#if 0
// ---- original Ghidra decompilation (ai_search_heap_sift_down @ 0x43b4d0) ----
void ai_search_heap_sift_down(void)

{
  short sVar1;
  short sVar2;
  undefined2 uVar3;
  int in_ECX;
  short in_DX;
  short sVar4;

  if (in_DX < *(short *)(in_ECX + 0x1430)) {
    while( true ) {
      sVar1 = in_DX * 2 + 1;
      sVar2 = in_DX * 2 + 2;
      sVar4 = in_DX;
      if ((sVar1 < *(short *)(in_ECX + 0x1430)) &&
         (*(float *)(in_ECX + (*(short *)(in_ECX + 0x1432 + sVar1 * 2) + 2) * 0x28) <
          *(float *)(in_ECX + (*(short *)(in_ECX + 0x1432 + in_DX * 2) + 2) * 0x28))) {
        sVar4 = sVar1;
      }
      if ((sVar2 < *(short *)(in_ECX + 0x1430)) &&
         (*(float *)(in_ECX + (*(short *)(in_ECX + 0x1432 + sVar2 * 2) + 2) * 0x28) <
          *(float *)(in_ECX + (*(short *)(in_ECX + 0x1432 + sVar4 * 2) + 2) * 0x28))) {
        sVar4 = sVar2;
      }
      if (sVar4 == in_DX) break;
      uVar3 = *(undefined2 *)(in_ECX + 0x1432 + sVar4 * 2);
      *(undefined2 *)(in_ECX + 0x1432 + sVar4 * 2) = *(undefined2 *)(in_ECX + 0x1432 + in_DX * 2);
      *(undefined2 *)(in_ECX + 0x1432 + in_DX * 2) = uVar3;
      in_DX = sVar4;
    }
  }
  return;
}
#endif
