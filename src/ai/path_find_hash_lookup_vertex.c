// path_find_hash_lookup_vertex  (Ghidra: path_find_hash_lookup_vertex, renamed)
// address 0x43b2b0, size 57 bytes
// name confidence: 0.45  rewrite confidence: 0.55
// evidence: types/ai.h path_find_context.vertex_hash (+0xe08a, 512 buckets of 8, confirmed
//   by the module header's own cross-check) / path_find_node.vertex_id (+0x8c = +0x84+0x08).
//   k_path_find_hash_buckets (0x200) / k_path_find_hash_bucket_size (8).
// register convention: EDX -> context, ESI -> vertex_id (both `in_`/`unaff_`-prefixed
//   registers Ghidra could not attribute to named parameters).
//   // blam-cc: EDX -> context, ESI -> vertex_id
//
// UNSURE: Ghidra decompiled this as a `void` function with no return statement, but every
// call site (path_find_find_unobstructed_ancestor.c and others) reads its result as a
// int16_t node index or -1. The last value computed on both exit paths is `sVar1` (the slot
// just read from the hash bucket), which is exactly the {node index, or -1 for "empty
// slot"} this function is documented to look up -- so this is almost certainly a case of
// Ghidra failing to recognize that the value is left in AX as the real return, not a
// genuinely void function. Rewritten to return that value explicitly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// VERIFIED against disassembly 0x43b2b0..0x43b2e8 (2026-09-30): probe order, wrap mask, node stride 0x34 and offsets
//   (hash 0xe08a, vertex_id 0x8c) match; the difftest hang is a random table with no empty slot.
// blam-cc: EDX -> context, ESI -> vertex_id
int16_t path_find_hash_lookup_vertex(path_find_context *context, uint32_t vertex_id)
{
    uint32_t slot = (vertex_id & 0x1ff) << 3;
    int16_t node;

    for (;;) {
        node = context->vertex_hash[slot];
        slot = (slot + 1) & 0xfff;
        if (node == -1) {
            return node;
        }
        if (context->nodes[node].vertex_id == vertex_id) {
            return node;
        }
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043b2b0 @ 0x43b2b0) ----
void FUN_0043b2b0(void)

{
  short sVar1;
  uint uVar2;
  int in_EDX;
  uint unaff_ESI;

  uVar2 = (unaff_ESI & 0x1ff) << 3;
  do {
    sVar1 = *(short *)(in_EDX + 0xe08a + (short)uVar2 * 2);
    uVar2 = uVar2 + 1 & 0xfff;
    if (sVar1 == -1) {
      return;
    }
  } while (*(uint *)(sVar1 * 0x34 + 0x8c + in_EDX) != unaff_ESI);
  return;
}
#endif
