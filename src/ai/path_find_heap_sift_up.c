// path_find_heap_sift_up  (Ghidra: path_find_heap_sift_up, already named)
// address 0x43af70, size 150 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: types/ai.h path_find_context.heap (+0xd086, path_find_heap_entry {node,key})
//   and path_find_node.heap_index (+0x84 + node*0x34 + 0x30 = +0xb4), both confirmed by this
//   function's own arithmetic matching the header's cross-check.
// register convention: EAX -> context, DX -> index (the one-based heap slot to sift up).
//   // blam-cc: EAX -> context, EDX -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

// blam-cc: EAX -> context, EDX -> index
void path_find_heap_sift_up(path_find_context *context, int16_t index)
{
    int16_t node = context->heap[index].node;
    int16_t key = context->heap[index].key;
    int16_t parent_slot;
    int16_t parent_node;
    int16_t parent_key;

    while (1 < index) {
        parent_slot = index >> 1;
        parent_node = context->heap[parent_slot].node;
        parent_key = context->heap[parent_slot].key;
        if (parent_key <= key) {
            break;
        }
        context->heap[index].node = parent_node;
        context->heap[index].key = parent_key;
        context->nodes[parent_node].heap_index = index;
        index = parent_slot;
    }

    context->heap[index].node = node;
    context->heap[index].key = key;
    context->nodes[node].heap_index = index;
}

#if 0
// ---- original Ghidra decompilation (path_find_heap_sift_up @ 0x43af70) ----
void path_find_heap_sift_up(void)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  int in_EAX;
  short sVar5;
  short in_DX;

  sVar1 = *(short *)(in_EAX + 0xd088 + in_DX * 4);
  sVar2 = *(short *)(in_EAX + 0xd086 + in_DX * 4);
  while (1 < in_DX) {
    sVar5 = in_DX >> 1;
    sVar3 = *(short *)(in_EAX + 0xd088 + sVar5 * 4);
    sVar4 = *(short *)(in_EAX + 0xd086 + sVar5 * 4);
    if (sVar3 <= sVar1) break;
    *(short *)(in_EAX + 0xd086 + in_DX * 4) = sVar4;
    *(short *)(in_EAX + 0xd088 + in_DX * 4) = sVar3;
    *(short *)(sVar4 * 0x34 + 0xb4 + in_EAX) = in_DX;
    in_DX = sVar5;
  }
  *(short *)(in_EAX + 0xd086 + in_DX * 4) = sVar2;
  *(short *)(in_EAX + 0xd088 + in_DX * 4) = sVar1;
  *(short *)(sVar2 * 0x34 + 0xb4 + in_EAX) = in_DX;
  return;
}
#endif
