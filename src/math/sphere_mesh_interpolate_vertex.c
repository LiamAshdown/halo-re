// sphere_mesh_interpolate_vertex  (Ghidra: sphere_mesh_interpolate_vertex, already named)
// address 0x4ca9a0, size 154 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: types/math.h sphere_mesh section; out/phase4/math_functions.md. Body writes a new
//   point at points[new_index] as a lerp of points[vertex_lo]/points[vertex_hi] by t =
//   position/total, then renormalizes it onto the unit sphere via
//   vector3d_normalize_with_length @0x401990.
// register convention: this leaf function's incoming registers (in_AX/in_CX/unaff_EDI in the
//   Ghidra output) are inferred from how sphere_mesh_get_edge_point @0x4ca8c0 calls it (that
//   call site shows only the 3 stack args explicitly; position/total/mesh are carried in
//   registers left over from the caller's own loop, which Ghidra does not print). Mapped here
//   as EAX=position, ECX=total, EDI=mesh, in register-priority order, then the 3 stack args
//   unchanged from the decompile.
//   // blam-cc: EAX -> position, ECX -> total, EDI -> mesh, stack -> (new_index, vertex_lo, vertex_hi)
// UNSURE: the EAX/ECX/EDI register assignment is inferred from cross-referencing the caller,
//   not confirmed from disassembly; the data-flow (t = position/total, lerp, normalize) is
//   taken directly from the decompile and is solid.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990

// Computes a new sphere-mesh vertex by lerping between two existing vertices and re-projecting
// the result onto the unit sphere.
void sphere_mesh_interpolate_vertex(int16_t position, int16_t total, sphere_mesh *mesh,
                                     int16_t new_index, int16_t vertex_lo, int16_t vertex_hi)
{
    real t;
    real one_minus_t;
    real_point3d *lo;
    real_point3d *hi;
    real_point3d *out;

    t = (real)position / (real)total;
    one_minus_t = 1.0f - t;
    hi = &mesh->points[vertex_hi];
    lo = &mesh->points[vertex_lo];
    out = &mesh->points[new_index];

    out->x = t * hi->x + one_minus_t * lo->x;
    out->y = one_minus_t * lo->y + t * hi->y;
    out->z = one_minus_t * lo->z + t * hi->z;
    vector3d_normalize_with_length((real_vector3d *)out);
}

#if 0
Original Ghidra decompilation (0x4ca9a0):

void sphere_mesh_interpolate_vertex(short param_1,short param_2,short param_3)

{
  float *pfVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  int iVar7;
  float fVar8;
  float fVar9;
  short in_AX;
  int iVar10;
  short in_CX;
  int unaff_EDI;
  
  fVar8 = (float)(int)in_AX / (float)(int)in_CX;
  iVar7 = *(int *)(unaff_EDI + 4);
  pfVar1 = (float *)(iVar7 + param_3 * 0xc);
  pfVar2 = (float *)(iVar7 + param_2 * 0xc);
  iVar10 = param_1 * 0xc;
  fVar9 = 1.0 - fVar8;
  fVar3 = pfVar1[2];
  fVar4 = pfVar2[2];
  fVar5 = pfVar1[1];
  fVar6 = pfVar2[1];
  *(float *)(iVar10 + iVar7) = fVar8 * *pfVar1 + fVar9 * *pfVar2;
  *(float *)(iVar10 + 4 + iVar7) = fVar9 * fVar6 + fVar8 * fVar5;
  *(float *)(iVar10 + 8 + iVar7) = fVar9 * fVar4 + fVar8 * fVar3;
  vector3d_normalize_with_length();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
