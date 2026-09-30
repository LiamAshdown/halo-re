// collision_bsp_surface_solve_third_axis  (Ghidra: FUN_00501470; was physics_shape_forward_call_helper,
//   renamed in the orphan pass 4 review)
// address 0x501470, size 47 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: objdump 0x501470..0x50149e. ECX is a collision BSP (ModelCollisionGeometryBSP): the
//   surfaces pointer is read from +0x40 and the planes pointer from +0x10. EAX is a surface index
//   (lea eax,[eax+eax*2], then a 0xc stride). The surface's plane index is masked with 0x7fffffff,
//   so the flip bit is ignored, and scaled by 0x10. The function then calls
//   decal_plane_solve_third_axis (0x44d860) with EBX = that plane, AL = the first stack byte and
//   the second stack argument pushed as its output, and returns that output. ESI (dominant_axis)
//   and EDI (the known 2D point) are not touched here; they pass straight through from the
//   caller. Both callers (0x43de02 path_find_trace_bsp_boundary and 0x55ac6e
//   biped_get_cached_look_at_position) set ESI = 2 and push component_sign 1.
//   The earlier rewrite modelled it as a one-argument thunk and dropped the BSP, the surface,
//   the axis and the known point.
// register convention: ECX collision_bsp, EAX surface_index, ESI dominant_axis, EDI known;
//   stack (component_sign, out).
//   // blam-cc: ECX -> collision_bsp, EAX -> surface_index, ESI -> dominant_axis (pass-through),
//   //           EDI -> known (pass-through), stack -> component_sign, out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_math.h"
#include "fn_physics.h"


    // 0x44d860, src/math; blam-cc: stack out, AL component_sign, SI dominant_axis, EBX plane, EDI known

// Lifts a 2D point projected along dominant_axis back onto the plane of a collision BSP surface.
real_point3d *collision_bsp_surface_solve_third_axis(ModelCollisionGeometryBSP *collision_bsp,
    int32_t surface_index, uint8_t component_sign, real_point3d *out, int32_t dominant_axis,
    const real_point2d *known)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)collision_bsp->surfaces.pointer;
    uint32_t plane_index = surfaces[surface_index].plane & 0x7fffffff;
    const real_plane3d *plane = (const real_plane3d *)((uint8_t *)collision_bsp->planes.pointer +
        plane_index * 0x10);

    decal_plane_solve_third_axis(out, component_sign, dominant_axis, plane, known);
    return out;
}

#if 0
Original Ghidra decompilation (0x501470):

undefined4 FUN_00501470(undefined4 param_1,undefined4 param_2)

{
  FUN_0044d860(param_2);
  return param_2;
}
#endif
