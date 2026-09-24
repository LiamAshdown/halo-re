// physics_point_refresh_leaf  (Ghidra: FUN_00505540, still unnamed there; phase-2 guessed
// physics_point_test_and_clear)
// address 0x505540, size 102 bytes
// name confidence: 0.3   rewrite confidence: 0.2 -- among the lowest-confidence files in this
//   batch; see the UNSURE paragraphs below.
// evidence: out/phase4/physics_functions.md ("Thin wrapper that re-evaluates a physics point's
//   containing BSP leaf and refreshes its active/contact state."); the breakable-surfaces
//   pointer expression `DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78` is byte-identical to
//   collision_test_movement_segment.c's own call into the segment-query counterpart of this
//   sphere query, which resolves it as `global_structure_bsp_index * 0x20 + 1 +
//   breakable_surface_globals` per types/physics.h's own struct comment on
//   breakable_surface_globals ("the bit vector deliberately starts at offset 1"); that same
//   sibling call site's first (bsp) argument is `global_structure_collision_bsp` (0x00746f98), reused
//   here as the bsp3d_node_find_leaf / collision_bsp_query_sphere_init bsp argument, which is
//   otherwise invisible in this function's own tiny decompile.
// register convention: UNSURE throughout -- this function's own decompile shows only ONE
//   recognized parameter (param_1), used as collision_bsp_query_sphere_init's radius argument,
//   plus an "undefined8" return from bsp3d_node_find_leaf whose upper 32 bits get reused as
//   collision_bsp_query_sphere_init's center argument. Ghidra's undefined8 typing there is an
//   artifact (bsp3d_node_find_leaf genuinely returns a 32-bit value in EAX only); this rewrite
//   reads that "upper half" as EDX simply surviving unclobbered from the point argument
//   bsp3d_node_find_leaf itself took, which is the only reading that gives
//   collision_bsp_query_sphere_init a real_point3d* rather than raw garbage. That point argument
//   is reconstructed as a hidden ESI parameter, matching the identical "a point survives
//   untouched across two BSP-query callees" shape object_collision_context_test_point (0x504e90,
//   this batch) already established for ESI.
//   // blam-cc: ESI -> point, stack -> radius
// UNSURE (major): collision_bsp_query_sphere_init needs a full collision_bsp_sphere_result
// (0x1010 bytes) as its ESI "result" argument, far larger than this 102-byte function's own
// stack frame could hold; this rewrite declares one as a local anyway (a valid, behaviour-
// preserving translation since only the boolean return is ever consulted here) rather than
// guess at a shared global scratch buffer with no supporting evidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78
extern int16_t global_structure_bsp_index;                     // 0x0069e8d8
extern ModelCollisionGeometryBSP *global_structure_collision_bsp;      // 0x00746f98

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
                                      real_point3d *point); // 0x5013a0, this module
extern uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp,
                                                 int16_t breakable_surface_count,
                                                 collision_bsp_sphere_result *result,
                                                 uint32_t *breakable_surfaces, real_point3d *center,
                                                 float radius); // 0x501980, this module

// Re-resolves point's containing leaf in the current structure BSP and, if it still lands
// inside the tree at all, runs a zero-radius-ish sphere query (radius) at the same spot against
// the current BSP's still-intact breakable surfaces. Returns whether the point is still
// considered active (either outside the tree entirely, or a fresh sphere touch was found).
// blam-cc: ESI -> point, stack -> radius
uint8_t physics_point_refresh_leaf(real_point3d *point, float radius)
{
    if (bsp3d_node_find_leaf(0, global_structure_collision_bsp, point) != 0xffffffff) {
        collision_bsp_sphere_result result;
        uint32_t *breakable_surfaces =
            (uint32_t *)((uint8_t *)breakable_surface_state + 1 +
                         global_structure_bsp_index * 0x20);

        // the ECX breakable_surface_count is invisible at every structure-BSP call site in this
        // module; both this one and physics_model_build_from_sphere_query pass the neutral
        // maximum, so only the bit vector decides what counts as already broken
        if (!collision_bsp_query_sphere_init(global_structure_collision_bsp,
                                              k_maximum_breakable_surfaces_per_bsp, &result,
                                              breakable_surfaces, point, radius)) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x505540):

undefined1 FUN_00505540(undefined4 param_1)

{
  char cVar1;
  undefined8 uVar2;

  uVar2 = FUN_005013a0();
  if ((int)uVar2 != -1) {
    cVar1 = FUN_00501980(DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78,(int)((ulonglong)uVar2 >> 0x20),
                         param_1);
    if (cVar1 == '\0') {
      return 0;
    }
  }
  return 1;
}
#endif
