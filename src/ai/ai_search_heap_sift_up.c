// ai_search_heap_sift_up  (Ghidra: ai_search_heap_sift_up, already named)
// address 0x43b450, size 119 bytes
// name confidence: 0.55  rewrite confidence: 0.85 (verified against objdump)
// evidence: types/ai.h ai_search_context.heap (+0x1432, plain node-index array, zero-based)
//   and ai_search_node.cost (+0x20); the `(index+2)*0x28` arithmetic Ghidra shows algebraically
//   simplifies to `0x30 + index*0x28 + 0x20`, i.e. `nodes[index].cost` off the context's own
//   +0x30 node array, both already established in the header.
// register convention: ECX -> index (the zero-based heap slot to sift up), EDX -> context.
//   // blam-cc: EDX -> context, ECX -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: EDX -> context, ECX -> index
void ai_search_heap_sift_up(ai_search_context *context, int16_t index)
{
    int16_t parent;
    int16_t node;
    int16_t parent_node;

    while (0 < index) {
        parent = (index - 1) >> 1;
        node = context->heap[index];
        parent_node = context->heap[parent];
        if (context->nodes[parent_node].cost <= context->nodes[node].cost) {
            break;
        }
        context->heap[index] = parent_node;
        context->heap[parent] = node;
        index = parent;
    }
}

#if 0
// ---- original Ghidra decompilation (ai_search_heap_sift_up @ 0x43b450) ----
void ai_search_heap_sift_up(void)

{
  short sVar1;
  short sVar2;
  int in_ECX;
  int in_EDX;
  int iVar3;

  sVar2 = (short)in_ECX;
  while( true ) {
    if (sVar2 < 1) {
      return;
    }
    iVar3 = (int)(short)in_ECX;
    in_ECX = iVar3 + -1 >> 1;
    sVar2 = (short)in_ECX;
    sVar1 = *(short *)(in_EDX + 0x1432 + sVar2 * 2);
    if (*(float *)(in_EDX + (sVar1 + 2) * 0x28) <=
        *(float *)(in_EDX + (*(short *)(in_EDX + 0x1432 + iVar3 * 2) + 2) * 0x28)) break;
    *(undefined2 *)(in_EDX + 0x1432 + sVar2 * 2) = *(undefined2 *)(in_EDX + 0x1432 + iVar3 * 2);
    *(short *)(in_EDX + 0x1432 + iVar3 * 2) = sVar1;
  }
  return;
}
#endif
