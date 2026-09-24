// collision_bsp_query_sphere_init  (Ghidra: FUN_00501980, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x501980, size 130 bytes
// name confidence: 0.4   rewrite confidence: 0.50 (raised from 0.45: phase-4 integration pass corrected the final test to surface_count/EDGE_count)
// evidence: out/phase4/physics_types_notes.md section 2: "0x08/0x0c/0x10 [of
//   collision_bsp_sphere_query] are filled from its three arguments; 0x00 (the bsp), 0x04 and
//   0x14 (the result block) arrive in registers and Ghidra lost the stores in 0x501980 -- they
//   arrive in registers. UNRESOLVED only in the sense of 'who writes them'; what they hold is
//   unambiguous from the readers." collision_bsp_query_sphere_node_recursive (0x501a10) reads
//   query->bsp, query->result and query->breakable_surface_count directly from the struct, so
//   this function must populate them before recursing.
// register convention: UNSURE exact registers; Ghidra shows zero explicit stores for query->bsp
//   (0x000), query->breakable_surface_count (0x004) and query->result (0x014), only the local
//   stack buffer that backs them being allocated. unaff_ESI is used directly, unmediated, as the
//   result pointer, which is the one register role this file can pin down with confidence.
//   // blam-cc: EAX -> bsp, ECX -> breakable_surface_count, ESI -> result,
//   //           stack -> breakable_surfaces, center, radius

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void collision_bsp_query_sphere_node_recursive(collision_bsp_sphere_query *query,
                                                        uint32_t node_index); // 0x501a10, this batch

// blam-cc: EAX -> bsp, ECX -> breakable_surface_count, ESI -> result,
//          stack -> breakable_surfaces, center, radius
uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp,
                                          int16_t breakable_surface_count,
                                          collision_bsp_sphere_result *result,
                                          uint32_t *breakable_surfaces, real_point3d *center,
                                          float radius)
{
    collision_bsp_sphere_query query;

    query.bsp = bsp;
    query.breakable_surface_count = breakable_surface_count;
    query.result = result;
    query.breakable_surfaces = breakable_surfaces;
    query.center = center;
    query.radius = radius;
    query.plane_count = 0;

    result->leaf_count = 0;
    result->surface_count = 0;
    result->vertex_count = 0;
    result->edge_count = 0;

    collision_bsp_query_sphere_node_recursive(&query, 0);

    // the original tests *unaff_ESI and unaff_ESI[0x101], i.e. 0x000 and 0x404 -- surface_count
    // and EDGE_count, not vertex_count (0x808)
    if ((result->surface_count < 1) && (result->edge_count < 1)) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x501980):

undefined4 FUN_00501980(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int *unaff_ESI;
  undefined1 local_228 [8];
  undefined4 local_220;
  undefined4 local_21c;
  undefined4 local_218;

  local_220 = param_1;
  local_21c = param_2;
  local_218 = param_3;
  unaff_ESI[0x303] = 0;
  *unaff_ESI = 0;
  unaff_ESI[0x101] = 0;
  unaff_ESI[0x202] = 0;
  FUN_00501a10(local_228,0);
  if ((*unaff_ESI < 1) && (unaff_ESI[0x101] < 1)) {
    return 0;
  }
  return 1;
}
#endif
