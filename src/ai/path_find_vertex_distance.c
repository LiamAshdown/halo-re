// path_find_vertex_distance  (Ghidra: path_find_vertex_distance, renamed)
// address 0x43b130, size 137 bytes (0x43b130..0x43b1b8)
// name confidence: 0.35  rewrite confidence: 0.75 (orphan pass 4 review: re-derived from objdump
//   0x43b130..0x43b1b9 while reconciling its call into decal_plane_solve_third_axis 0x44d860; the
//   draft had lost the EAX / ECX inputs and both callees' arguments)
// evidence: EAX is the ScenarioStructureBSP (collision BSP pointer at +0xb4), ECX a collision surface
//   index. collision_bsp_surface_closest_edge_point_2d (0x5015a0, EAX = collision BSP, stack
//   (surface, axis 2, sign 1, point_a, &closest)) finds the 2D point of the surface nearest to
//   point_a; decal_plane_solve_third_axis (AL = 1, SI = 2, EBX = &planes[surface.plane & 0x7fffffff],
//   EDI = &closest) lifts it onto the surface's plane into *out_point. Returns the distance from
//   point_a to that point (x87 sum order dx*dx + dz*dz + dy*dy).
//   Called from path_find_run (0x43aea1..0x43aeb2) with the structure BSP, the adjacent edge's
//   surface, the goal position and a scratch point.
// register convention: EAX -> structure_bsp, ECX -> surface, stack -> point_a, out_point.
//   // blam-cc: EAX -> structure_bsp, ECX -> surface, stack -> point_a, out_point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h> // uintptr_t: tag block pointers are 32-bit fields

extern double sqrt(double x); // FSQRT
extern uint32_t collision_bsp_surface_closest_edge_point_2d(ModelCollisionGeometryBSP *bsp, int32_t surface_index,
    uint16_t axis, uint8_t sign, real_point2d *point, real_point2d *out_point);
    // 0x5015a0, src/physics; blam-cc: EAX bsp, stack (surface_index, axis, sign, point, out_point)
extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
    const real_plane3d *plane, const real_point2d *known);
    // 0x44d860, src/math; blam-cc: stack out, AL component_sign, SI dominant_axis, EBX plane, EDI known

// Distance from point_a to the point of `surface` nearest to it (written to *out_point).
float path_find_vertex_distance(ScenarioStructureBSP *structure_bsp, int32_t surface, real_point3d *point_a,
    real_point3d *out_point)
{
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)(uintptr_t)((struct ScenarioStructureBSP *)structure_bsp)->collision_bsp.pointer;
    ModelCollisionGeometryBSPSurface *surfaces = (ModelCollisionGeometryBSPSurface *)(uintptr_t)collision_bsp->surfaces.pointer;
    real_plane3d *planes = (real_plane3d *)(uintptr_t)collision_bsp->planes.pointer;
    real_point2d closest;
    float dx, dy, dz;

    collision_bsp_surface_closest_edge_point_2d(collision_bsp, surface, 2, 1, (real_point2d *)point_a, &closest);
    decal_plane_solve_third_axis(out_point, 1, 2, &planes[surfaces[surface].plane & 0x7fffffff], &closest);

    dx = out_point->x - point_a->x;
    dy = out_point->y - point_a->y;
    dz = out_point->z - point_a->z;
    return (float)sqrt((double)(dx * dx + dz * dz + dy * dy));
}

#if 0
// ---- original Ghidra decompilation (FUN_0043b130 @ 0x43b130) ----
float10 FUN_0043b130(float *param_1,float *param_2)

{
  FUN_005015a0();
  FUN_0044d860(param_2);
  return SQRT(((float10)param_2[1] - (float10)param_1[1]) *
              ((float10)param_2[1] - (float10)param_1[1]) +
              ((float10)param_2[2] - (float10)param_1[2]) *
              ((float10)param_2[2] - (float10)param_1[2]) +
              ((float10)*param_2 - (float10)*param_1) * ((float10)*param_2 - (float10)*param_1));
}
#endif
