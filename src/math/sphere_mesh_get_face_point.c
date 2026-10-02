// sphere_mesh_get_face_point  (Ghidra: sphere_mesh_get_face_point, already named)
// address 0x4ca7c0, size 248 bytes
// name confidence: 0.6   rewrite confidence: 0.4
// evidence: types/math.h sphere_mesh_face_cache section (index = (n+1)*row + col, confirmed by
//   *in_ECX being dereferenced as subdivisions and the (n+1)*row+col arithmetic). A triangular
//   face is parameterized by (row, col) with 0<=col<=row<=subdivisions; row 0 is the apex
//   vertex, row==subdivisions is the base edge between vertex_a and vertex_b. The four
//   candidate cases match the four ways a barycentric grid point can be resolved:
//     col==0            -> on the apex-to-vertex_a edge, at depth row
//     row==subdivisions -> on the vertex_a-to-vertex_b edge, at depth col
//     col==row           -> on the apex-to-vertex_b edge, at depth row
//     otherwise          -> interior point, allocated fresh and lerped between the two edge
//                           points at the same row (the apex-a and apex-b edges), weighted by
//                           col/row, via sphere_mesh_interpolate_vertex @0x4ca9a0
// register convention (reconstructed, see UNSURE below): mesh in ECX (confirmed: *in_ECX is
//   dereferenced as subdivisions), next_point_index in EBX (confirmed: *unaff_EBX is
//   incremented as the point-creation bump counter), apex vertex in ESI (inferred, see below),
//   then the 6 stack params exactly as sphere_mesh_build_face's call site shows them
//   (vertex_a, vertex_b, row, col, edge_cache, face_cache).
//   // blam-cc: ECX -> mesh, EBX -> next_point_index, ESI -> apex, stack -> (vertex_a, vertex_b, row, col, edge_cache, face_cache)
// UNSURE: sphere_mesh_build_face's own param_3 (the face's third/apex vertex) is never read
//   anywhere in build_face's body, and this function's body never receives an explicit third
//   vertex argument either, yet three geometrically distinct edges are resolved from it. The
//   only consistent reading is that build_face's apex parameter is forwarded to this function
//   through a register neither function's body ever names (assigned ESI here for lack of a
//   confirmed slot); the leading `(undefined2)((uint)in_EAX >> 0x10)` at function entry is
//   32-bit return-register plumbing (preserving whatever was in EAX's high half across an
//   early return of a 16-bit value) and not a real input, so it is dropped here. The branch
//   vertex pairings (apex/vertex_a, vertex_a/vertex_b, apex/vertex_b) are inferred from which
//   grid corner each condition covers, not read directly off registers; the cache indexing,
//   the four-way branch structure, and the interior lerp weights (col, row) match the decompile
//   exactly.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t sphere_mesh_get_edge_point(int16_t vertex_a, int16_t vertex_b, sphere_mesh *mesh,
                                           int16_t position, int16_t *next_point_index,
                                           sphere_mesh_edge_cache *edge_cache); // 0x4ca8c0
extern void sphere_mesh_interpolate_vertex(int16_t position, int16_t total, sphere_mesh *mesh,
                                            int16_t new_index, int16_t vertex_lo, int16_t vertex_hi); // 0x4ca9a0

// Resolves (with caching) the point index for a given barycentric position inside a subdivided
// octahedron face.
int16_t sphere_mesh_get_face_point(sphere_mesh *mesh, int16_t *next_point_index, int16_t apex,
                                    int16_t vertex_a, int16_t vertex_b, int16_t row, int16_t col,
                                    sphere_mesh_edge_cache *edge_cache,
                                    sphere_mesh_face_cache *face_cache)
{
    int32_t cache_index;
    int16_t *cached;
    int16_t new_index;
    int16_t left;
    int16_t right;

    cache_index = (mesh->subdivisions + 1) * row + col;
    cached = &face_cache->point_index[cache_index];
    if (*cached == -1) {
        if (col == 0) {
            *cached = sphere_mesh_get_edge_point(apex, vertex_a, mesh, row, next_point_index, edge_cache);
        } else if (row == mesh->subdivisions) {
            *cached = sphere_mesh_get_edge_point(vertex_a, vertex_b, mesh, col, next_point_index, edge_cache);
        } else if (col == row) {
            *cached = sphere_mesh_get_edge_point(apex, vertex_b, mesh, row, next_point_index, edge_cache);
        } else {
            new_index = *next_point_index;
            *next_point_index = new_index + 1;
            left = sphere_mesh_get_edge_point(apex, vertex_a, mesh, row, next_point_index, edge_cache);
            right = sphere_mesh_get_edge_point(apex, vertex_b, mesh, row, next_point_index, edge_cache);
            *cached = new_index;
            sphere_mesh_interpolate_vertex(col, row, mesh, new_index, left, right);
        }
    }
    return *cached;
}

#if 0
Original Ghidra decompilation (0x4ca7c0):

undefined4
sphere_mesh_get_face_point
          (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
          undefined4 param_5,int param_6)

{
  short *psVar1;
  short sVar2;
  short sVar3;
  undefined4 in_EAX;
  undefined2 uVar6;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined2 extraout_var;
  short *in_ECX;
  int iVar7;
  short *unaff_EBX;
  
  uVar6 = (undefined2)((uint)in_EAX >> 0x10);
  sVar3 = (short)param_3;
  sVar2 = (short)param_4;
  iVar7 = (int)(short)((*in_ECX + 1) * sVar3 + sVar2);
  psVar1 = (short *)(param_6 + iVar7 * 2);
  if (*(short *)(param_6 + iVar7 * 2) == -1) {
    if (sVar2 == 0) {
      uVar4 = sphere_mesh_get_edge_point(param_3);
      *psVar1 = (short)uVar4;
      return uVar4;
    }
    if (sVar3 == *in_ECX) {
      uVar4 = sphere_mesh_get_edge_point(param_4);
      *psVar1 = (short)uVar4;
      return uVar4;
    }
    if (sVar2 == sVar3) {
      uVar4 = sphere_mesh_get_edge_point(param_3);
      *psVar1 = (short)uVar4;
      return uVar4;
    }
    sVar2 = *unaff_EBX;
    *unaff_EBX = sVar2 + 1;
    uVar4 = sphere_mesh_get_edge_point(param_3);
    uVar5 = sphere_mesh_get_edge_point(param_3);
    *psVar1 = sVar2;
    sphere_mesh_interpolate_vertex(sVar2,uVar4,uVar5);
    uVar6 = extraout_var;
  }
  return CONCAT22(uVar6,*psVar1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
