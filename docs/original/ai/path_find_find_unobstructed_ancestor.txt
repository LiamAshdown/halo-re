// path_find_find_unobstructed_ancestor  (Ghidra: path_find_find_unobstructed_ancestor, renamed)
// address 0x43a220, size 236 bytes
// name confidence: 0.35  rewrite confidence: 0.35
// evidence: types/ai.h path_find_context.nodes (+0x84, stride 0x34, confirmed by the module
//   header's own cross-check), path_find_node.parent (+0x02), path_find_node.position
//   (+0x0c), path_find_context.start_position (+0x14). Calls
//   path_find_hash_lookup_vertex @0x43b2b0 (this rewrite, not yet written when this file was
//   authored -- see its own file) and path_find_trace_bsp_boundary (0x43d9b0, this rewrite, BSP boundary
//   trace).
// register convention: ECX -> context (in_ECX); stack -> vertex_id-or-similar (param_1,
//   forwarded to path_find_hash_lookup_vertex and path_find_trace_bsp_boundary), out_used_start (param_2),
//   out_position (param_3).
//   // blam-cc: ECX -> context, stack -> vertex_id, out_used_start, out_position
//
// UNSURE: path_find_trace_bsp_boundary's real parameter count/order is finalized in its own file
// (path_find_trace_bsp_boundary.c in this rewrite); the call here is declared with exactly
// the three operands Ghidra shows at this call site (context.unknown_48, a byte from the
// request block at context+4, and this function's own param_1), which may not line up with
// that function's canonical signature -- see this module's established convention for that
// situation (e.g. encounter_squad_spawn_actor.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t path_find_hash_lookup_vertex(path_find_context *context, uint32_t vertex_id); // 0x43b2b0
extern uint8_t path_find_trace_bsp_boundary(void *context, uint8_t ignore_permission, real_point3d *point_a,
    int32_t start_edge, real_point3d *point_b, int32_t exclude_vertex, path_find_boundary_crossing *out_result); // 0x43d9b0, stack

// blam-cc: ECX -> context, EAX -> vertex_id, stack -> point, out_used_start, out_position
// Walks from the point's node (vertex_id) back towards the start until the straight line from the point to a
// node's parent crosses no boundary (0x43d9b0 against the structure bsp at context +0x64, permission byte +4).
uint8_t path_find_find_unobstructed_ancestor(path_find_context *context, uint32_t vertex_id, real_point3d *point,
                                             uint8_t *out_used_start, real_point3d *out_position)
{
    int16_t node_index;
    path_find_node *node;

    node_index = path_find_hash_lookup_vertex(context, vertex_id);
    if (node_index == -1) {
        return 0;
    }

    node = &context->nodes[node_index];
    while (node->parent != -1) {
        path_find_node *parent = &context->nodes[node->parent];
        path_find_boundary_crossing crossing;

        // 0x43a260: stack bsp, permission, the point, its vertex, the parent's position and vertex, &crossing
        if (path_find_trace_bsp_boundary((void *)context->structure_bsp, *((uint8_t *)context + 4), point,
                (int32_t)vertex_id, &parent->position, (int32_t)parent->vertex_id, &crossing) != 0) {
            break;
        }
        node = parent;
    }

    if (node->parent == -1) {
        *out_used_start = 1;
        *out_position = context->start_position;
        return 1;
    }

    *out_used_start = 0;
    *out_position = node->position;
    return 1;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043a220 @ 0x43a220) ----
uint FUN_0043a220(undefined4 param_1,undefined1 *param_2,undefined4 *param_3)

{
  short sVar1;
  undefined4 uVar2;
  char cVar3;
  uint uVar4;
  int in_ECX;
  int iVar5;

  uVar4 = FUN_0043b2b0();
  if ((short)uVar4 == -1) {
    return uVar4 & 0xffffff00;
  }
  iVar5 = (short)uVar4 * 0x34 + 0x84 + in_ECX;
  sVar1 = *(short *)(iVar5 + 2);
  while ((sVar1 != -1 &&
         (cVar3 = FUN_0043d9b0(*(undefined4 *)(in_ECX + 100),*(undefined1 *)(in_ECX + 4),param_1),
         cVar3 == '\0'))) {
    iVar5 = *(short *)(iVar5 + 2) * 0x34;
    sVar1 = *(short *)(iVar5 + 0x86 + in_ECX);
    iVar5 = iVar5 + 0x84 + in_ECX;
  }
  if (*(short *)(iVar5 + 2) == -1) {
    *param_2 = 1;
    *param_3 = *(undefined4 *)(in_ECX + 0x14);
    param_3[1] = *(undefined4 *)(in_ECX + 0x18);
    uVar2 = *(undefined4 *)(in_ECX + 0x1c);
    param_3[2] = uVar2;
    return CONCAT31((int3)((uint)uVar2 >> 8),1);
  }
  *param_2 = 0;
  *param_3 = *(undefined4 *)(iVar5 + 0xc);
  uVar2 = *(undefined4 *)(iVar5 + 0x10);
  param_3[1] = uVar2;
  param_3[2] = *(undefined4 *)(iVar5 + 0x14);
  return CONCAT31((int3)((uint)uVar2 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
