// path_find_push_start_node  (Ghidra: path_find_push_start_node, already named)
// address 0x43a760, size 321 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/ai.h path_find_context.start_position(+0x14, rejects z below -1000.0,
//   confirmed by this function)/start_vertex_id(+0x20)/have_goal(+0x4c)/best_cost(+0x6c)/
//   unknown_70/best_position(+0x74)/best_node(+0x68)/node_count(+0x80)/nodes(+0x84)/
//   vertex_hash(+0xe08a); path_find_node.parent/unknown_04/vertex_id/position/cost/
//   unknown_1c(FLT_MAX)/unknown_20/unknown_24/distance/key/waypoint. Calls
//   path_find_heap_push @0x43b0f0 (this rewrite).
// register convention: ESI -> context (the only `unaff_` register Ghidra's decompile
//   shows).
//   // blam-cc: ESI -> context
//
// UNSURE: `__ftol()` is called here with no visible floating-point operand (Ghidra's
// `extraout_ST0`), matching this module's established "hidden FPU input" situation (see
// e.g. actor_recompute_grenade_eligibility.c); the real value is not reconstructed and is
// stood in for with 0.0. It ends up in both path_find_node.key (truncated to int16) and
// path_find_node.distance (as the untruncated float), which -- since key is the heap
// ordering field -- suggests it is meant to be some rounded distance/cost estimate for the
// start node, not confirmed further here. path_find_heap_push is called with context
// forwarded from a different register (EAX there vs ESI here); called explicitly with this
// function's own `context` since the value is known here regardless of the register
// mismatch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern int32_t __ftol(double x); // FISTP-based float-to-int truncation
extern void path_find_heap_push(path_find_context *context, int16_t node, int16_t key); // 0x43b0f0

// blam-cc: ESI -> context
uint8_t path_find_push_start_node(path_find_context *context)
{
    float value;      // UNSURE: hidden FPU operand, see header
    int32_t key;
    int16_t node_index;
    path_find_node *node;

    if ((context->start_vertex_id == 0xffffffff) || (context->start_position.z <= -1000.0f)) {
        return 0;
    }

    if (context->have_goal == 0) {
        value = 0.0f;
        key = 0;
    } else {
        value = 0.0f; // UNSURE, see header
        key = __ftol((double)value);
        if (0x7ffe < key) {
            return 0;
        }
    }

    node_index = context->node_count;
    node = &context->nodes[node_index];
    context->node_count = node_index + 1;

    node->parent = -1;
    node->unknown_04 = -1;
    node->vertex_id = context->start_vertex_id;
    node->position = context->start_position;
    node->distance = value;
    node->cost = 0.0f;
    node->unknown_20 = 0.0f;
    node->unknown_1c = 3.4028235e+38f;
    node->unknown_24 = 0.0f;
    node->key = (int16_t)key;
    node->waypoint = 0;

    if (context->have_goal != 0) {
        context->best_cost = value;
        context->unknown_70 = value;
        context->best_position.x = context->start_position.x;
        context->best_position.y = context->start_position.y;
        context->best_node = node_index;
        context->best_position.z = context->start_position.z;
    }

    context->vertex_hash[(node->vertex_id & 0x1ff) * 8] = node_index;
    path_find_heap_push(context, node_index, (int16_t)key);
    return 1;
}

#if 0
// ---- original Ghidra decompilation (path_find_push_start_node @ 0x43a760) ----
undefined4 path_find_push_start_node(void)

{
  int iVar1;
  short sVar2;
  int unaff_ESI;
  float10 extraout_ST0;
  float10 fVar3;
  int local_4;

  if ((*(int *)(unaff_ESI + 0x20) != -1) && (-1000.0 < *(float *)(unaff_ESI + 0x1c))) {
    if (*(char *)(unaff_ESI + 0x4c) == '\0') {
      fVar3 = (float10)0.0;
      local_4 = 0;
    }
    else {
      local_4 = __ftol();
      fVar3 = extraout_ST0;
      if (0x7ffe < local_4) {
        return 0;
      }
    }
    sVar2 = *(short *)(unaff_ESI + 0x80);
    iVar1 = sVar2 * 0x34 + 0x84 + unaff_ESI;
    *(short *)(unaff_ESI + 0x80) = sVar2 + 1;
    *(undefined2 *)(iVar1 + 2) = 0xffff;
    *(undefined4 *)(iVar1 + 4) = 0xffffffff;
    *(undefined4 *)(iVar1 + 8) = *(undefined4 *)(unaff_ESI + 0x20);
    *(undefined4 *)(iVar1 + 0xc) = *(undefined4 *)(unaff_ESI + 0x14);
    *(undefined4 *)(iVar1 + 0x10) = *(undefined4 *)(unaff_ESI + 0x18);
    *(undefined4 *)(iVar1 + 0x14) = *(undefined4 *)(unaff_ESI + 0x1c);
    *(float *)(iVar1 + 0x28) = (float)fVar3;
    *(undefined4 *)(iVar1 + 0x18) = 0;
    *(undefined4 *)(iVar1 + 0x20) = 0;
    *(undefined4 *)(iVar1 + 0x1c) = 0x7f7fffff;
    *(undefined4 *)(iVar1 + 0x24) = 0;
    *(short *)(iVar1 + 0x2c) = (short)local_4;
    *(undefined2 *)(iVar1 + 0x2e) = 0;
    if (*(char *)(unaff_ESI + 0x4c) != '\0') {
      *(float *)(unaff_ESI + 0x6c) = (float)fVar3;
      *(float *)(unaff_ESI + 0x70) = (float)fVar3;
      *(undefined4 *)(unaff_ESI + 0x74) = *(undefined4 *)(unaff_ESI + 0x14);
      *(undefined4 *)(unaff_ESI + 0x78) = *(undefined4 *)(unaff_ESI + 0x18);
      *(short *)(unaff_ESI + 0x68) = sVar2;
      *(undefined4 *)(unaff_ESI + 0x7c) = *(undefined4 *)(unaff_ESI + 0x1c);
    }
    *(short *)((*(uint *)(iVar1 + 8) & 0x1ff) * 0x10 + 0xe08a + unaff_ESI) = sVar2;
    path_find_heap_push(sVar2,local_4);
    return 1;
  }
  return 0;
}
#endif
