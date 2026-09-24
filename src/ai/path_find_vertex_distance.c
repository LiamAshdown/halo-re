// path_find_vertex_distance  (Ghidra: path_find_vertex_distance, renamed)
// address 0x43b130, size 137 bytes
// name confidence: 0.35  rewrite confidence: 0.3
// evidence: phase-4 summary "computes the distance between two pathfinding vertices after
// resolving their coordinates through a transform step". FUN_0044d860 and collision_bsp_surface_closest_edge_point_2d are
// outside this rewrite's address range and not established here.
// register convention: stack -> point_a, point_b (both Ghidra-recognized formal parameters).
//   // blam-cc: stack -> point_a, point_b
//
// UNSURE: collision_bsp_surface_closest_edge_point_2d is called here with no visible arguments even though the function
// clearly needs to resolve `point_a`'s coordinates (by analogy with FUN_0044d860(point_b)
// immediately after); declared and called as a no-argument stub, matching Ghidra's own
// decompile exactly, rather than guessing its real parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT
extern void collision_bsp_surface_closest_edge_point_2d(void); // 0x5015a0, not in this rewrite's range; see header UNSURE
extern void FUN_0044d860(real_point3d *point); // 0x44d860, not in this rewrite's range

float path_find_vertex_distance(real_point3d *point_a, real_point3d *point_b)
{
    float dx, dy, dz;

    collision_bsp_surface_closest_edge_point_2d();
    FUN_0044d860(point_b);

    dx = point_b->x - point_a->x;
    dy = point_b->y - point_a->y;
    dz = point_b->z - point_a->z;
    return (float)sqrt(dy * dy + dz * dz + dx * dx);
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
