// path_find_test_direct_reachability  (Ghidra: path_find_test_direct_reachability, renamed)
// address 0x43a0a0, size 234 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (collision query FIXED against objdump)
// evidence: phase-4 summary "tests whether two 3D points are effectively coincident along a
// computed interpolation fraction, used as a pathfinding proximity check"; already referenced
// as "direct-line reachability" from src/ai/actor_firing_position_near_point.c. Calls
// collision_bsp_query_segment_init (physics module, out of range: "initializes a segment/pill collision query
// record with a clamped parametric fraction before running the recursive BSP node test").
// register convention: EAX -> point_a, ECX -> point_b, ESI -> out_position (all `in_`/
//   `unaff_`-prefixed registers Ghidra could not attribute to named parameters); stack ->
//   context, out_success.
//   // blam-cc: EAX -> point_a, ECX -> point_b, ESI -> out_position, stack -> context,
//   //   out_success
//
// UNSURE: `context` is read only at +0xb4 (a BSP-like handle forwarded to collision_bsp_query_segment_init) and
// is not otherwise established here; it does not line up with any named field of
// path_find_context (0xb4 would fall inside nodes[0]'s heap_index, which makes no sense for
// a BSP pointer), so it is almost certainly a different, smaller caller-owned record.
// Declared as an opaque `void *` rather than asserting a type. collision_bsp_query_segment_init's fourth
// argument (the output fraction, Ghidra's `local_418`) is not shown at this call site at
// all; declared and passed here as an explicit out-parameter because the rest of this
// function's logic only makes sense if it is written, matching this module's convention for
// a hidden-output-pointer callee (see e.g. the PLAN.md note on effect_random_velocity_vector
// in this repository's history for the same situation in another module).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "physics.h"

extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
    ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, real_point3d *origin,
    real_vector3d *delta, float max_fraction); // 0x502060, EAX, ECX, stack

// blam-cc: EAX -> point_a, ECX -> point_b, ESI -> out_position, stack -> context, out_success
uint8_t path_find_test_direct_reachability(const real_point3d *point_a, const real_point3d *point_b,
                                           real_point3d *out_position, void *context, uint8_t *out_success)
{
    float fraction;
    float dx, dy, dz;
    uint8_t hit;
    uint8_t success;

    // FIXED (objdump 0x43a0a6..0x43a0ff): the segment from point_b along (point_a - point_b), EAX = 1, ECX = the
    //   result, stack = (context +0xb4 bsp, 0, 0, point_b, &delta, FLT_MAX). The draft passed four unrelated values.
    {
        collision_bsp_segment_result result;
        real_vector3d delta;

        delta.i = point_a->x - point_b->x;
        delta.j = point_a->y - point_b->y;
        delta.k = point_a->z - point_b->z;
        hit = collision_bsp_query_segment_init(1, &result, *(ModelCollisionGeometryBSP **)((uint8_t *)context + 0xb4),
            0, 0, (real_point3d *)point_b, &delta, 3.4028235e+38f);
        fraction = result.t;
    }

    success = 0;
    if ((hit == 0) || (1.0f <= fraction)) {
        success = 1;
    } else {
        dx = point_a->x - point_b->x;
        dy = point_a->y - point_b->y;
        dz = point_a->z - point_b->z;
        if ((1.0f - fraction) * (1.0f - fraction) * (dx * dx + dy * dy + dz * dz) < 0.1f) {
            success = 1;
        }
    }

    if (out_success != 0) {
        *out_success = success;
    }
    if (out_position != 0) {
        *out_position = *point_a;
    }
    return success;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043a0a0 @ 0x43a0a0) ----
undefined1 FUN_0043a0a0(int param_1,undefined1 *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  char cVar10;
  float *in_EAX;
  undefined1 uVar11;
  float *in_ECX;
  undefined1 uVar12;
  float *unaff_ESI;
  float local_418;

  fVar1 = *in_EAX;
  fVar2 = *in_ECX;
  fVar7 = *in_EAX;
  fVar3 = in_EAX[1];
  fVar8 = in_EAX[1];
  fVar4 = in_ECX[1];
  fVar9 = in_EAX[2];
  uVar12 = 0;
  fVar5 = in_EAX[2];
  fVar6 = in_ECX[2];
  cVar10 = FUN_00502060(*(undefined4 *)(param_1 + 0xb4),0,0);
  if (((cVar10 == '\0') || (1.0 <= local_418)) ||
     ((1.0 - local_418) * (1.0 - local_418) *
      ((fVar3 - fVar4) * (fVar3 - fVar4) +
      (fVar1 - fVar2) * (fVar1 - fVar2) + (fVar5 - fVar6) * (fVar5 - fVar6)) < 0.1)) {
    uVar12 = 1;
    uVar11 = 1;
  }
  else {
    uVar11 = 0;
  }
  if (param_2 != (undefined1 *)0x0) {
    *param_2 = uVar11;
  }
  if (unaff_ESI != (float *)0x0) {
    *unaff_ESI = fVar7;
    unaff_ESI[1] = fVar8;
    unaff_ESI[2] = fVar9;
  }
  return uVar12;
}
#endif
