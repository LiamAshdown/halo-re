// path_find_heights_are_close  (Ghidra: path_find_heights_are_close, renamed)
// address 0x43d910, size 160 bytes
// name confidence: 0.45   rewrite confidence: 0.75 (orphan pass 4 review: re-derived from objdump
//   0x43d910..0x43d9af while reconciling the two calls into decal_plane_solve_third_axis 0x44d860;
//   the draft had lost the EAX / EDX inputs and the point both planes are evaluated at)
// evidence: phase-4 summary "checks whether two navmesh cluster locations sit at nearly the
//   same height, used to reject path steps needing a large vertical jump". objdump: EAX is the
//   ScenarioStructureBSP (its +0xb4 is the collision_bsp pointer, a ModelCollisionGeometryBSP:
//   planes pointer +0x10, 0x10 stride; surfaces pointer +0x40, 0xc stride, plane index at +0 with
//   bit 31 the flip bit, masked off here), EDX the 2D point, ECX and the stack dword two surface
//   indices. For each surface the point is lifted onto the surface's plane with
//   decal_plane_solve_third_axis (AL = 1, SI = 2 -- solve z --, EBX = &plane, EDI = point); the
//   surfaces are "close" when the two heights differ by less than 0.05 (double at 0x00672b28,
//   `fabs; fcomp; test ah,5; jp`: a NaN difference is not close).
// register convention: EAX -> structure_bsp, EDX -> point, ECX -> surface_a, stack -> surface_b.
//   // blam-cc: EAX -> structure_bsp, EDX -> point, ECX -> surface_a, stack -> surface_b

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h> // uintptr_t: tag block pointers are 32-bit fields

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double fabs(double x); // ABS
extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
    const real_plane3d *plane, const real_point2d *known);
    // 0x44d860, src/math; blam-cc: stack out, AL component_sign, SI dominant_axis, EBX plane, EDI known

// Whether `point` lies at nearly the same height on the planes of surface_a and surface_b of the
// structure BSP's collision geometry.
uint8_t path_find_heights_are_close(ScenarioStructureBSP *structure_bsp, real_point2d *point, int32_t surface_a,
    int32_t surface_b)
{
    ModelCollisionGeometryBSP *collision_bsp;
    ModelCollisionGeometryBSPSurface *surfaces;
    real_plane3d *planes;
    real_point3d position_a, position_b;

    if (surface_a == -1 || surface_b == -1) {
        return 0;
    }
    collision_bsp = (ModelCollisionGeometryBSP *)(uintptr_t)((struct ScenarioStructureBSP *)structure_bsp)->collision_bsp.pointer;
    surfaces = (ModelCollisionGeometryBSPSurface *)(uintptr_t)collision_bsp->surfaces.pointer;
    planes = (real_plane3d *)(uintptr_t)collision_bsp->planes.pointer;

    decal_plane_solve_third_axis(&position_a, 1, 2, &planes[surfaces[surface_a].plane & 0x7fffffff], point);
    decal_plane_solve_third_axis(&position_b, 1, 2, &planes[surfaces[surface_b].plane & 0x7fffffff], point);
    return (uint8_t)(fabs((double)(position_a.z - position_b.z)) < 0.05000000074505806);
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d910 @ 0x43d910) ----
uint FUN_0043d910(int param_1)

{
  float fVar1;
  uint in_EAX;
  uint uVar2;
  undefined2 extraout_var;
  uint3 uVar3;
  int in_ECX;
  undefined1 local_18 [8];
  float local_10;
  undefined1 local_c [8];
  float local_4;

  uVar2 = in_EAX & 0xffffff00;
  if ((in_ECX != -1) && (param_1 != -1)) {
    FUN_0044d860(local_18);
    FUN_0044d860(local_c);
    fVar1 = ABS(local_10 - local_4);
    uVar3 = (uint3)(CONCAT22(extraout_var,
                             (ushort)(fVar1 < 0.05) << 8 | (ushort)NAN(fVar1) << 10 |
                             (ushort)(fVar1 == 0.05) << 0xe) >> 8);
    if (fVar1 < 0.05) {
      return CONCAT31(uVar3,1);
    }
    uVar2 = (uint)uVar3 << 8;
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
