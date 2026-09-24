// collision_bsp_surface_test_point_side_2d  (Ghidra: FUN_005014a0, still unnamed there; name
// from out/phase2/results/physics_00.json)
// address 0x5014a0, size 252 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: same ModelCollisionGeometryBSP surfaces/edges/vertices layout as
//   collision_bsp_surface_get_vertices (0x501400); types/math.h projection_axis_pair /
//   k_projection_axes at 0x0065c29c, indexed axis*2+sign exactly as documented there. The sole
//   caller, FUN_0055ab30 (units module, 0x55ab30), assigns the result to a `char` and only ever
//   tests it against zero (`cVar2 = FUN_005014a0(...); if (cVar2 != '\0') ...`).
// register convention: in_EAX -> bsp (ModelCollisionGeometryBSP *), unaff_EDI -> point
//   (real_point2d *, already projected by the caller). param_1/param_2/param_3 are
//   Ghidra-recognized stack parameters (surface_index, axis, sign).
//   // blam-cc: EAX -> bsp, EDI -> point, stack -> surface_index, axis, sign
// UNSURE: Ghidra's return value is a raw x87-flags-and-pointer-residue pack (an FNSTSW/SAHF
//   style idiom: CONCAT of the high 16 bits of a vertex pointer with condition-code bits for
//   "cross < 0", "cross is NaN" and "cross == 0", then shifted and OR'd with a 0/1 tail byte).
//   The only real caller reads the result as a `char`, i.e. only the low byte, which is exactly
//   0 when the loop returns early (a strictly positive cross product found) and 1 when the loop
//   runs out of edges without one; that low byte is what this rewrite reproduces. The upper 24
//   bits of Ghidra's return value are compiler/register residue, not meaningful output, and are
//   not reproduced.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)

// blam-cc: EAX -> bsp, EDI -> point, stack -> surface_index, axis, sign
uint8_t collision_bsp_surface_test_point_side_2d(ModelCollisionGeometryBSP *bsp,
                                                  real_point2d *point, int32_t surface_index,
                                                  int16_t axis, uint8_t sign)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
    // reinterpreted as float[4] per vertex (x, y, z, first_edge); only [0..2] are ever indexed
    float *vertex_floats = (float *)bsp->vertices.pointer;
    int32_t start_edge = (int32_t)surfaces[surface_index].first_edge;
    int32_t edge_index = start_edge;
    projection_axis_pair proj = k_projection_axes[axis * 2 + sign];

    do {
        ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
        int owns_right_side = ((int32_t)edge->right_surface == surface_index);
        uint32_t near_vertex = owns_right_side ? edge->end_vertex : edge->start_vertex;
        uint32_t far_vertex = owns_right_side ? edge->start_vertex : edge->end_vertex;
        float *near = vertex_floats + near_vertex * 4;
        float *far = vertex_floats + far_vertex * 4;

        float cross = (point->x - near[proj.i]) * (point->y - far[proj.j]) -
                      (point->y - near[proj.j]) * (point->x - far[proj.i]);

        if (cross >= 0.0f && cross != 0.0f) {
            return 0;
        }
        edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
    } while (edge_index != start_edge);

    return 1;
}

#if 0
Original Ghidra decompilation (0x5014a0):

int FUN_005014a0(int param_1,short param_2,byte param_3)

{
  int iVar1;
  float fVar2;
  int in_EAX;
  int iVar3;
  uint uVar4;
  int iVar5;
  uint3 uVar6;
  int iVar7;
  int iVar8;
  float *unaff_EDI;
  bool bVar9;

  iVar1 = *(int *)(*(int *)(in_EAX + 0x40) + 4 + param_1 * 0xc);
  iVar3 = ((uint)param_3 + param_2 * 2) * 4;
  iVar8 = iVar1;
  do {
    iVar8 = *(int *)(in_EAX + 0x4c) + iVar8 * 0x18;
    bVar9 = *(int *)(iVar8 + 0x14) == param_1;
    uVar4 = (uint)bVar9;
    iVar5 = *(int *)(iVar8 + uVar4 * 4) * 0x10 + *(int *)(in_EAX + 0x58);
    iVar7 = *(int *)(iVar8 + (uint)!bVar9 * 4) * 0x10 + *(int *)(in_EAX + 0x58);
    fVar2 = (*unaff_EDI - *(float *)(iVar5 + *(short *)(&DAT_0065c29c + iVar3) * 4)) *
            (unaff_EDI[1] - *(float *)(*(short *)(&DAT_0065c29e + iVar3) * 4 + iVar7)) -
            (unaff_EDI[1] - *(float *)(*(short *)(&DAT_0065c29e + iVar3) * 4 + iVar5)) *
            (*unaff_EDI - *(float *)(iVar7 + *(short *)(&DAT_0065c29c + iVar3) * 4));
    uVar6 = (uint3)(CONCAT22((short)((uint)iVar5 >> 0x10),
                             (ushort)(fVar2 < 0.0) << 8 | (ushort)NAN(fVar2) << 10 |
                             (ushort)(fVar2 == 0.0) << 0xe) >> 8);
    if (fVar2 >= 0.0 && (fVar2 == 0.0) == 0) {
      return (uint)uVar6 << 8;
    }
    iVar8 = *(int *)(iVar8 + 8 + uVar4 * 4);
  } while (iVar8 != iVar1);
  return CONCAT31(uVar6,1);
}
#endif
