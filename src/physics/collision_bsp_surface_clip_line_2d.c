// collision_bsp_surface_clip_line_2d  (Ghidra: FUN_005017f0, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x5017f0, size 393 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: out/phase4/physics_types_notes.md section 2 (collision_bsp_boundary_clip) derives
//   this exact struct from this function: {enter, exit} each {t, edge_index, surface_index},
//   seeded to -FLT_MAX/+FLT_MAX and a don't-care sentinel for the two index fields, and read
//   back as "exit.t < enter.t means no overlap" -- the final comparison here. Classic
//   Cyrus-Beck line clip against a convex polygon: track the largest "entering" t and the
//   smallest "exiting" t across every edge.
// register convention: in_ECX -> clip (collision_bsp_boundary_clip *, output). param_1..param_4
//   are Ghidra-recognized stack parameters (bsp, surface_index, origin, direction).
//   // blam-cc: ECX -> clip, stack -> bsp, surface_index, origin, direction
// UNSURE: Ghidra types the loop's edge-index variable and the boundary_clip index fields as
//   float because they are read/written through float-typed pointers (the edge table has no
//   float fields at those offsets); every value that ever flows through them is a small integer
//   index, so this rewrite carries them as int32_t directly rather than round-tripping through
//   float bit patterns, which is behaviorally identical for every value they can hold. The two
//   sentinel index values Ghidra shows as -NAN are never read except alongside their sentinel
//   t value (which signals "no hit yet"), so they are set to -1 here rather than reproducing an
//   exact NaN bit pattern that no caller can observe.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "fn_physics.h"

// blam-cc: ECX -> clip, stack -> bsp, surface_index, origin, direction
uint32_t collision_bsp_surface_clip_line_2d(collision_bsp_boundary_clip *clip,
                                             ModelCollisionGeometryBSP *bsp,
                                             int32_t surface_index, real_point2d *origin,
                                             real_vector2d *direction)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
    // reinterpreted as float[4] per vertex (x, y, z, first_edge); only [0..1] are ever indexed
    float *vertex_floats = (float *)bsp->vertices.pointer;
    int32_t start_edge = (int32_t)surfaces[surface_index].first_edge;
    int32_t edge_index = start_edge;

    clip->enter.t = -3.4028235e+38f;
    clip->enter.edge_index = -1;
    clip->enter.surface_index = -1;
    clip->exit.t = 3.4028235e+38f;
    clip->exit.edge_index = -1;
    clip->exit.surface_index = -1;

    do {
        ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
        int owns_right_side = ((int32_t)edge->right_surface == surface_index);
        int32_t far_surface =
            owns_right_side ? (int32_t)edge->left_surface : (int32_t)edge->right_surface;
        float start_i = vertex_floats[edge->start_vertex * 4 + 0];
        float start_j = vertex_floats[edge->start_vertex * 4 + 1];
        float edge_i = vertex_floats[edge->end_vertex * 4 + 0] - start_i;
        float edge_j = vertex_floats[edge->end_vertex * 4 + 1] - start_j;
        float denom = edge_j * direction->i - edge_i * direction->j;
        float numer = (origin->y - start_j) * edge_i - (origin->x - start_i) * edge_j;

        if (denom == 0.0f) {
            if ((numer < 0.0f) != owns_right_side) {
                clip->enter.t = 3.4028235e+38f;
                clip->enter.edge_index = edge_index;
                clip->enter.surface_index = far_surface;
                clip->exit.t = -3.4028235e+38f;
                clip->exit.edge_index = edge_index;
                clip->exit.surface_index = far_surface;
            }
        } else {
            float t = numer / denom;
            if ((denom < 0.0f) == owns_right_side) {
                if (t < clip->exit.t) {
                    clip->exit.t = t;
                    clip->exit.edge_index = edge_index;
                    clip->exit.surface_index = far_surface;
                }
            } else if (clip->enter.t < t) {
                clip->enter.t = t;
                clip->enter.edge_index = edge_index;
                clip->enter.surface_index = far_surface;
            }
        }
        edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
    } while (edge_index != start_edge);

    return (clip->exit.t < clip->enter.t) ? 1u : 0u;
}

#if 0
Original Ghidra decompilation (0x5017f0):

undefined4 FUN_005017f0(int param_1,int param_2,float *param_3,float *param_4)

{
  int iVar1;
  float fVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  int iVar7;
  float *pfVar8;
  float *in_ECX;
  int iVar9;
  float fVar10;
  bool bVar11;

  fVar2 = *(float *)(*(int *)(param_1 + 0x40) + 4 + param_2 * 0xc);
  *in_ECX = -3.4028235e+38;
  in_ECX[1] = -NAN;
  in_ECX[2] = -NAN;
  in_ECX[3] = 3.4028235e+38;
  in_ECX[4] = -NAN;
  in_ECX[5] = -NAN;
  fVar10 = fVar2;
  do {
    iVar3 = *(int *)(param_1 + 0x58);
    iVar1 = *(int *)(param_1 + 0x4c) + (int)fVar10 * 0x18;
    bVar11 = *(int *)(iVar1 + 0x14) == param_2;
    iVar9 = *(int *)(iVar1 + 4) * 0x10;
    iVar7 = *(int *)(*(int *)(param_1 + 0x4c) + (int)fVar10 * 0x18) * 0x10;
    fVar4 = *(float *)(iVar9 + iVar3) - *(float *)(iVar7 + iVar3);
    pfVar8 = (float *)(iVar7 + iVar3);
    fVar6 = *(float *)(iVar9 + 4 + iVar3) - pfVar8[1];
    fVar5 = fVar6 * *param_4 - fVar4 * param_4[1];
    fVar4 = (param_3[1] - pfVar8[1]) * fVar4 - (*param_3 - *pfVar8) * fVar6;
    if (fVar5 == 0.0) {
      if (fVar4 < 0.0 != bVar11) {
        *in_ECX = 3.4028235e+38;
        in_ECX[1] = fVar10;
        in_ECX[2] = *(float *)(iVar1 + 0x10 + (uint)!bVar11 * 4);
        in_ECX[3] = -3.4028235e+38;
        in_ECX[4] = fVar10;
        in_ECX[5] = *(float *)(iVar1 + 0x10 + (uint)!bVar11 * 4);
      }
    }
    else {
      fVar4 = fVar4 / fVar5;
      if (fVar5 < 0.0 == bVar11) {
        if (fVar4 < in_ECX[3]) {
          in_ECX[3] = fVar4;
          in_ECX[4] = fVar10;
          in_ECX[5] = *(float *)(iVar1 + 0x10 + (uint)!bVar11 * 4);
        }
      }
      else if (*in_ECX < fVar4) {
        *in_ECX = fVar4;
        in_ECX[1] = fVar10;
        in_ECX[2] = *(float *)(iVar1 + 0x10 + (uint)!bVar11 * 4);
      }
    }
    fVar10 = *(float *)(iVar1 + 8 + (uint)bVar11 * 4);
  } while (fVar10 != fVar2);
  if (in_ECX[3] < *in_ECX) {
    return 1;
  }
  return 0;
}
#endif
