// collision_bsp_surface_get_vertices  (Ghidra: FUN_00501400, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x501400, size 112 bytes
// name confidence: 0.45   rewrite confidence: 0.65
// evidence: types/tags.h ModelCollisionGeometryBSP.surfaces (+0x40), .edges (+0x4c) and
//   .vertices (+0x58) match unaff_EDI+0x40/0x4c/0x58 exactly; ModelCollisionGeometryBSPSurface
//   .first_edge and ModelCollisionGeometryBSPEdge {start_vertex, end_vertex, forward_edge,
//   reverse_edge, left_surface, right_surface} match every field this function reads.
// register convention: unaff_EDI -> bsp (ModelCollisionGeometryBSP *; never reloaded from the
//   stack, so Ghidra could not attach it to a numbered parameter). param_1/param_2 are
//   Ghidra-recognized stack parameters.
//   // blam-cc: EDI -> bsp, stack -> surface_index, out_vertices
// UNSURE: this function is declared void and never has an explicit `return count;` statement,
// but its sole caller in this batch (physics_shape_add_surface_proxy, 0x503c50) reads the
// vertex count straight out of EAX after the call (`uVar4 = FUN_00501400(...)`). The loop
// counter (sVar4 here) is exactly the value that ends up in EAX/AX at the end of this function's
// real assembly, so the true calling convention treats it as an implicit return value even
// though the C source never says `return`; this rewrite makes that explicit.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EDI -> bsp, stack -> surface_index, out_vertices
int16_t collision_bsp_surface_get_vertices(ModelCollisionGeometryBSP *bsp, int32_t surface_index,
                                            real_point3d *out_vertices)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
    ModelCollisionGeometryBSPVertex *vertices =
        (ModelCollisionGeometryBSPVertex *)bsp->vertices.pointer;
    int32_t start_edge = (int32_t)surfaces[surface_index].first_edge;
    int32_t edge_index = start_edge;
    int16_t vertex_count = 0;

    do {
        ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
        // the surface owns the edge's "far" side when it is the edge's right_surface; walk the
        // matching vertex/next-edge pair for whichever side that is
        int owns_right_side = ((int32_t)edge->right_surface == surface_index);
        uint32_t vertex_index = owns_right_side ? edge->end_vertex : edge->start_vertex;

        out_vertices[vertex_count].x = vertices[vertex_index].point.x;
        out_vertices[vertex_count].y = vertices[vertex_index].point.y;
        out_vertices[vertex_count].z = vertices[vertex_index].point.z;

        edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
        vertex_count = vertex_count + 1;
    } while (edge_index != start_edge);

    return vertex_count;
}

#if 0
Original Ghidra decompilation (0x501400):

void FUN_00501400(int param_1,int param_2)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  short sVar4;
  int iVar5;
  uint uVar6;
  undefined4 *puVar7;
  int unaff_EDI;

  iVar3 = *(int *)(*(int *)(unaff_EDI + 0x40) + 4 + param_1 * 0xc);
  sVar4 = 0;
  iVar5 = iVar3;
  do {
    iVar1 = *(int *)(unaff_EDI + 0x4c) + iVar5 * 0x18;
    uVar6 = (uint)(*(int *)(*(int *)(unaff_EDI + 0x4c) + 0x14 + iVar5 * 0x18) == param_1);
    puVar7 = (undefined4 *)(*(int *)(iVar1 + uVar6 * 4) * 0x10 + *(int *)(unaff_EDI + 0x58));
    puVar2 = (undefined4 *)(param_2 + sVar4 * 0xc);
    *puVar2 = *puVar7;
    puVar2[1] = puVar7[1];
    puVar2[2] = puVar7[2];
    iVar5 = *(int *)(iVar1 + 8 + uVar6 * 4);
    sVar4 = sVar4 + 1;
  } while (iVar5 != iVar3);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
