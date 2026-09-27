// collision_bsp_query_segment_init  (Ghidra: FUN_00502060, still unnamed there. Phase-2's
// physics_shape_pill_query_init guess doesn't match: this seeds a collision_bsp_segment_query,
// the same struct collision_bsp_query_segment_node_recursive (0x502140) walks, so this rewrite
// is named to match that.)
// address 0x502060, size 216 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x502060..0x502137 (query layout, three-way t_max clamp).)
// evidence: out/phase4/physics_types_notes.md section 2: "0x502060 writes 0x04, 0x08, 0x0c,
//   0x10, 0x14, 0x1c, 0x20, 0x24; 0x00 (flags) and 0x18 (result) come in registers" -- exactly
//   the fields param_1..param_5 and the -1/-1/0/-1 seeding below cover, with flags and result
//   the two fields this file adds as explicit register parameters to close the gap.
// register convention: UNSURE exact registers for flags/result (Ghidra shows zero explicit
//   stores for either); in_ECX is used directly, unmediated, to pre-seed result->t and
//   result->leaf_count, which is the one register role this file can pin down with confidence.
//   // blam-cc: EAX -> flags, ECX -> result,
//   //           stack -> bsp, breakable_surface_count, breakable_surfaces, origin, delta,
//   //           max_fraction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern uint8_t collision_bsp_query_segment_node_recursive(collision_bsp_segment_query *query,
                                                            uint32_t node_index, float t_min,
                                                            float t_max); // 0x502140, this batch

// blam-cc: EAX -> flags, ECX -> result,
//          stack -> bsp, breakable_surface_count, breakable_surfaces, origin, delta,
//          max_fraction
uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
                                          ModelCollisionGeometryBSP *bsp,
                                          int16_t breakable_surface_count,
                                          uint32_t *breakable_surfaces, real_point3d *origin,
                                          real_vector3d *delta, float max_fraction)
{
    collision_bsp_segment_query query;
    float clamped_max = max_fraction;

    query.flags = flags;
    query.bsp = bsp;
    query.breakable_surface_count = breakable_surface_count;
    query.breakable_surfaces = breakable_surfaces;
    query.origin = origin;
    query.delta = delta;
    query.result = result;
    query.last_leaf = -1;
    query.last_leaf_type = 0;
    query.crossing_plane = -1;

    if (max_fraction < 0.0f) {
        clamped_max = 0.0f;
    }
    result->t = clamped_max;
    result->leaf_count = 0;

    if (max_fraction < 0.0f) {
        return collision_bsp_query_segment_node_recursive(&query, 0, 0.0f, 0.0f);
    }
    if (max_fraction <= 1.0f) {
        return collision_bsp_query_segment_node_recursive(&query, 0, 0.0f, max_fraction);
    }
    return collision_bsp_query_segment_node_recursive(&query, 0, 0.0f, 1.0f);
}

#if 0
Original Ghidra decompilation (0x502060):

void FUN_00502060(undefined4 param_1,undefined2 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,float param_6)

{
  float fVar1;
  float *in_ECX;
  undefined1 local_28 [4];
  undefined4 local_24;
  undefined2 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_c;
  undefined1 local_8;
  undefined4 local_4;

  local_20 = param_2;
  local_24 = param_1;
  local_18 = param_4;
  local_1c = param_3;
  local_14 = param_5;
  fVar1 = param_6;
  if (param_6 < 0.0) {
    fVar1 = 0.0;
  }
  *in_ECX = fVar1;
  local_c = 0xffffffff;
  local_4 = 0xffffffff;
  in_ECX[5] = 0.0;
  local_8 = 0;
  if (0.0 <= param_6) {
    if (param_6 <= 1.0) {
      FUN_00502140(local_28,0,0,param_6);
      return;
    }
    FUN_00502140(local_28,0,0,0x3f800000);
    return;
  }
  FUN_00502140(local_28,0,0,0);
  return;
}
#endif
