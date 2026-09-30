// sphere_mesh_get_edge_point  (Ghidra: sphere_mesh_get_edge_point, already named)
// address 0x4ca8c0, size 211 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: types/math.h sphere_mesh_edge_cache section ("indexes it as hi + lo*8 after
//   sorting the two base vertex indices"). Boundary cases: position==0 returns vertex_a
//   unchanged, position==subdivisions returns vertex_b unchanged; otherwise the point at that
//   fractional position along the edge is created on first use (subdividing the whole edge into
//   subdivisions-1 interior points via sphere_mesh_interpolate_vertex @0x4ca9a0) and cached in
//   edge_cache[lo][hi] so the adjacent face does not duplicate it.
// register convention: EAX=vertex_a, ECX=vertex_b, EDX=mesh (Ghidra's in_EAX/in_ECX/in_EDX),
//   then the 3 stack params (position, next_point_index, edge_cache) as declared.
//   // blam-cc: EAX -> vertex_a, ECX -> vertex_b, EDX -> mesh, stack -> (position, next_point_index, edge_cache)
// UNSURE: sphere_mesh_get_face_point's calls into this function show only one explicit stack
//   argument (position); vertex_a/vertex_b/mesh/next_point_index/edge_cache are inferred to be
//   carried through in registers left over from the caller (see sphere_mesh_get_face_point's
//   header for the full chain reasoning). The arithmetic and control flow below match the
//   decompile exactly.

#include "tags.h"
#include "math.h"
#include "fn_math.h"


// Resolves (with caching) the shared point index for a position along an edge between two base
// vertices, so adjacent faces don't duplicate vertices.
int16_t sphere_mesh_get_edge_point(int16_t vertex_a, int16_t vertex_b, sphere_mesh *mesh,
                                    int16_t position, int16_t *next_point_index,
                                    sphere_mesh_edge_cache *edge_cache)
{
    int16_t hi;
    int16_t lo;
    int16_t *cached;
    int16_t step;
    int16_t new_index;

    hi = vertex_a;
    lo = vertex_b;
    if (vertex_a <= vertex_b) {
        hi = vertex_b;
        lo = vertex_a;
    }
    if (position == 0) {
        return vertex_a;
    }
    if (position == mesh->subdivisions) {
        return vertex_b;
    }
    cached = &edge_cache->point_index[lo][hi];
    if (*cached == -1) {
        *cached = *next_point_index;
        if (1 < mesh->subdivisions) {
            for (step = 1; step < mesh->subdivisions; step++) {
                new_index = *next_point_index;
                *next_point_index = new_index + 1;
                sphere_mesh_interpolate_vertex(step, mesh->subdivisions, mesh, new_index, lo, hi);
            }
        }
    }
    if (vertex_b < vertex_a) {
        return (int16_t)((uint16_t)(*cached + (mesh->subdivisions - position)) - 1);
    }
    return (int16_t)((uint16_t)(*cached + position) - 1);
}

#if 0
Original Ghidra decompilation (0x4ca8c0):

uint sphere_mesh_get_edge_point(short param_1,short *param_2,int param_3)

{
  int iVar1;
  short *psVar2;
  short sVar3;
  uint in_EAX;
  uint in_ECX;
  short *in_EDX;
  uint uVar4;
  short sVar5;
  uint local_8;
  
  uVar4 = in_EAX;
  local_8 = in_ECX;
  if ((short)in_EAX <= (short)in_ECX) {
    uVar4 = in_ECX;
    local_8 = in_EAX;
  }
  if (param_1 != 0) {
    if (param_1 == *in_EDX) {
      return in_ECX & 0xffff;
    }
    iVar1 = (int)(short)uVar4 + (short)local_8 * 8;
    psVar2 = (short *)(param_3 + iVar1 * 2);
    if (*(short *)(param_3 + iVar1 * 2) == -1) {
      sVar5 = 1;
      *psVar2 = *param_2;
      if (1 < *in_EDX) {
        do {
          sVar3 = *param_2;
          *param_2 = sVar3 + 1;
          sphere_mesh_interpolate_vertex(sVar3,local_8,uVar4);
          sVar5 = sVar5 + 1;
        } while (sVar5 < *in_EDX);
      }
    }
    if ((short)in_ECX < (short)in_EAX) {
      return (ushort)(*psVar2 + (*in_EDX - param_1)) - 1;
    }
    in_EAX = (ushort)(*psVar2 + param_1) - 1;
  }
  return in_EAX;
}
#endif
