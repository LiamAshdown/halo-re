// collision_bsp_surface_test_point_2d  (Ghidra: FUN_00502600, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x502600, size 299 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: identical edge-loop cross-product algorithm and return convention (low byte 1 when
//   the loop completes without a strictly-positive cross, 0 otherwise) as
//   collision_bsp_surface_test_point_side_2d (0x5014a0), plus the same breakable-surface gate
//   as breakable_surface_apply_damage/collect_geometry. The sole two callers (FUN_00502460 in
//   this batch, and one outside it) only ever read the result as a `char`, i.e. only the low
//   byte, which this rewrite reproduces directly as a 0/1 boolean ("is the point inside the
//   surface's projected boundary").
// register convention: in_EAX -> bsp (ModelCollisionGeometryBSP *). param_1..param_6 are
//   Ghidra-recognized stack parameters.
//   // blam-cc: EAX -> bsp, stack -> breakable_surface_count, breakable_surfaces, surface_index,
//   //           axis, sign, point
// UNSURE: see collision_bsp_surface_test_point_side_2d.c for the upper-24-bit residue this
//   rewrite does not reproduce.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)

// blam-cc: EAX -> bsp, stack -> breakable_surface_count, breakable_surfaces, surface_index,
//          axis, sign, point
uint8_t collision_bsp_surface_test_point_2d(ModelCollisionGeometryBSP *bsp,
                                             int16_t breakable_surface_count,
                                             uint32_t *breakable_surfaces, int32_t surface_index,
                                             int16_t axis, uint8_t sign, real_point2d *point)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPSurface *surface = &surfaces[surface_index];
    uint8_t surface_breakable_index = (uint8_t)surface->breakable_surface;

    if ((surface->flags & 0x08) == 0 ||
        breakable_surface_count <= (int16_t)(uint16_t)surface_breakable_index ||
        (breakable_surfaces[surface_breakable_index >> 5] &
         (1u << (surface_breakable_index & 0x1f))) != 0) {
        ModelCollisionGeometryBSPEdge *edges =
            (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
        float *vertex_floats = (float *)bsp->vertices.pointer;
        int32_t start_edge = (int32_t)surface->first_edge;
        int32_t edge_index = start_edge;
        projection_axis_pair proj = k_projection_axes[axis * 2 + sign];

        for (;;) {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            int owns_right_side = ((int32_t)edge->right_surface == surface_index);
            uint32_t near_vertex = owns_right_side ? edge->end_vertex : edge->start_vertex;
            uint32_t far_vertex = owns_right_side ? edge->start_vertex : edge->end_vertex;
            float near_i = vertex_floats[near_vertex * 4 + proj.i];
            float near_j = vertex_floats[near_vertex * 4 + proj.j];
            float far_i = vertex_floats[far_vertex * 4 + proj.i];
            float far_j = vertex_floats[far_vertex * 4 + proj.j];
            float cross = (point->x - near_i) * (far_j - near_j) - (point->y - near_j) * (far_i - near_i);

            if (cross >= 0.0f && cross != 0.0f) {
                return 0;
            }
            edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
            if (edge_index == start_edge) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x502600):

uint FUN_00502600(short param_1,int param_2,int param_3,short param_4,byte param_5,float *param_6)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  int in_EAX;
  int iVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  bool bVar9;

  uVar6 = *(int *)(in_EAX + 0x40) + param_3 * 0xc;
  if ((((*(byte *)(uVar6 + 8) & 8) == 0) || (param_1 <= (short)(ushort)*(byte *)(uVar6 + 9))) ||
     ((*(uint *)(param_2 + (uint)(*(byte *)(uVar6 + 9) >> 5) * 4) &
      1 << (*(byte *)(uVar6 + 9) & 0x1f)) != 0)) {
    piVar1 = (int *)(uVar6 + 4);
    iVar4 = ((uint)param_5 + param_4 * 2) * 4;
    iVar8 = *piVar1;
    while( true ) {
      iVar8 = *(int *)(in_EAX + 0x4c) + iVar8 * 0x18;
      bVar9 = *(int *)(iVar8 + 0x14) == param_3;
      uVar7 = (uint)bVar9;
      iVar5 = *(int *)(iVar8 + uVar7 * 4) * 0x10 + *(int *)(in_EAX + 0x58);
      fVar2 = *(float *)(*(short *)(&DAT_0065c29c + iVar4) * 4 + iVar5);
      fVar3 = *(float *)(iVar5 + *(short *)(&DAT_0065c29e + iVar4) * 4);
      iVar5 = *(int *)(iVar8 + (uint)!bVar9 * 4) * 0x10 + *(int *)(in_EAX + 0x58);
      fVar2 = (*param_6 - fVar2) *
              (*(float *)(iVar5 + *(short *)(&DAT_0065c29e + iVar4) * 4) - fVar3) -
              (param_6[1] - fVar3) *
              (*(float *)(*(short *)(&DAT_0065c29c + iVar4) * 4 + iVar5) - fVar2);
      uVar6 = CONCAT22((short)((uint)param_6 >> 0x10),
                       (ushort)(fVar2 < 0.0) << 8 | (ushort)NAN(fVar2) << 10 |
                       (ushort)(fVar2 == 0.0) << 0xe);
      if (fVar2 >= 0.0 && (fVar2 == 0.0) == 0) break;
      iVar8 = *(int *)(iVar8 + 8 + uVar7 * 4);
      if (iVar8 == *piVar1) {
        return CONCAT31((int3)(uVar6 >> 8),1);
      }
    }
  }
  return uVar6 & 0xffffff00;
}
#endif
