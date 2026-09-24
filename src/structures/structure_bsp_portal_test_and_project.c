// structure_bsp_portal_test_and_project  (Ghidra: FUN_005549c0, still unnamed there)
// address 0x5549c0, size 86 bytes
// name confidence: 0.5 -- phase4 summary: "Tests and clips a specific portal's polygon against
//   the current view, used to gate whether the portal flood should recurse through it." Named
//   here for exactly that: it resolves a portal's plane and forwards to
//   structure_bsp_portal_project.
// rewrite confidence: 0.55 -- Ghidra's decompile is short but calls structure_bsp_portal_project
//   (0x554850) with only one visible argument; objdump disassembly recovers the rest, including
//   that this function's caller-supplied output buffer (EDX) is passed straight through, unread
//   here -- confirmed by camera_cluster_portal_flood_recursive.c (this batch), which is the only
//   caller and sets EDX to its own polygon2d-shaped stack buffer before calling.
// evidence: objdump -M intel disassembly of 0x5549c0..0x554a20; types/tags.h
//   ScenarioStructureBSPClusterPortal (plane_index at +4) and ModelCollisionGeometryBSPPlane.
// register convention: in_CX -> portal_index, in_EDX -> out (forwarded to
//   structure_bsp_portal_project's out parameter, never read here). Stack: param_1 -> same_side
//   (whether the recursion is crossing from the portal's front side).
//   // blam-cc: CX -> portal_index, EDX -> out, stack -> same_side
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

extern real_point3d render_camera_position;  // 0x007c3114, render camera block (read, not owned)
extern uint8_t render_camera_projection;     // 0x007c3168, the transform context, +0x10 is the matrix

// blam-cc: EAX -> plane, ECX -> camera_ref, EDX -> vertices, stack -> the rest
extern uint8_t structure_bsp_portal_project(real_plane3d *plane, void *camera_ref,
                                             real_point3d *vertices, void *camera,
                                             uint32_t vertex_count, int16_t winding,
                                             polygon2d *out); // 0x554850, this module

// blam-cc: CX -> portal_index, EDX -> out, stack -> same_side
uint8_t structure_bsp_portal_test_and_project(char same_side, int16_t portal_index, polygon2d *out)
{
    ScenarioStructureBSPClusterPortal *portal =
        &((ScenarioStructureBSPClusterPortal *)global_structure_bsp->cluster_portals.pointer)[portal_index];
    // The disassembly resolves the planes through global_structure_bsp->collision_bsp (tag +0xb4), not
    // through the 0x00746f90 global that structure_bsp_query_surfaces / structure_bsp_portal_sphere_test
    // use; both are ModelCollisionGeometryBSP pointers for the same resident BSP.
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    ModelCollisionGeometryBSPPlane *plane =
        &((ModelCollisionGeometryBSPPlane *)collision_bsp->planes.pointer)[portal->plane_index];
    return structure_bsp_portal_project((real_plane3d *)&plane->plane, &render_camera_position,
                                         (real_point3d *)portal->vertices.pointer,
                                         &render_camera_projection, portal->vertices.count,
                                         (int16_t)((same_side == 0) * 2 - 1), out);
}

#if 0
Original Ghidra decompilation (0x5549c0):

void FUN_005549c0(char param_1)

{
  short in_CX;

  FUN_00554850(&DAT_007c3168,*(undefined2 *)(in_CX * 0x40 + *(int *)(DAT_00746f9c + 0x158) + 0x34),
               (uint)(param_1 == '\0') * 2 + -1);
  return;
}
#endif
