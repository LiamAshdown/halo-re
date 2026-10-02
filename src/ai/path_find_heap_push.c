// path_find_heap_push  (Ghidra: path_find_heap_push, already named)
// address 0x43b0f0, size 63 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: types/ai.h path_find_context.heap_count (+0xd084) / heap (+0xd086);
// k_path_find_maximum_heap (0x400). Calls path_find_heap_sift_up @0x43af70 (this rewrite).
// register convention: EAX -> context; stack -> node, key.
//   // blam-cc: EAX -> context, stack -> node, key

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void path_find_heap_sift_up(path_find_context *context, int16_t index); // 0x43af70

// blam-cc: EAX -> context, stack -> node, key
void path_find_heap_push(path_find_context *context, int16_t node, int16_t key)
{
    int16_t index = context->heap_count;

    if (index < k_path_find_maximum_heap) {
        context->heap_count = index + 1;
        context->heap[index].node = node;
        context->heap[index].key = key;
        path_find_heap_sift_up(context, index);
    }
}

#if 0
// ---- original Ghidra decompilation (path_find_heap_push @ 0x43b0f0) ----
void path_find_heap_push(undefined2 param_1,undefined2 param_2)

{
  short sVar1;
  int in_EAX;

  sVar1 = *(short *)(in_EAX + 0xd084);
  if (sVar1 < 0x400) {
    *(short *)(in_EAX + 0xd084) = sVar1 + 1;
    *(undefined2 *)(in_EAX + 0xd086 + sVar1 * 4) = param_1;
    *(undefined2 *)(in_EAX + 0xd088 + sVar1 * 4) = param_2;
    path_find_heap_sift_up();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
