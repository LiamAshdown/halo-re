// path_find_score_avoidance_penalty  (Ghidra: path_find_score_avoidance_penalty, renamed)
// address 0x43b3b0, size 150 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: types/ai.h path_find_request.avoid_position(+0x28)/avoid_radius(+0x38)/
//   avoid_weight(+0x3c), confirmed by path_find_set_avoid_sphere.c writing exactly these
//   fields. phase-4 summary "computes a linear-falloff proximity penalty (and outputs the
//   raw distance) of a position relative to a radius/weight record, used as a cost term
//   inside the AI point search" (also used by path_find_run.c, this rewrite, for the A*
//   search's own obstacle-avoidance branch). Calls path_find_closest_point_on_segment
//   (0x43b2f0, a math helper this task's skip list excludes from rewriting).
// register convention: EBX -> context; stack -> out_distance.
//   // blam-cc: EBX -> context, stack -> out_distance
//
// UNSURE: path_find_closest_point_on_segment is called here with no visible arguments,
// writing through Ghidra's `local_c/local_8/local_4` outputs -- the same hidden-output
// pattern as path_find_compute_heuristic.c's use of the same callee.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT
extern void path_find_closest_point_on_segment(void); // 0x43b2f0, math helper, not rewritten here

// blam-cc: EBX -> context, stack -> out_distance
// Scores how much a candidate point should be penalized for passing near the context's
// avoid-sphere: 0 outside twice the radius, rising linearly to `avoid_weight` at the sphere
// center. Always reports the raw distance to the closest point on the segment through
// `out_distance`, clamped to FLT_MAX when the point is outside the radius.
float path_find_score_avoidance_penalty(path_find_context *context, float *out_distance)
{
    path_find_request *request = (path_find_request *)context;
    float local_c, local_8, local_4; // path_find_closest_point_on_segment's hidden outputs
    float dx, dy, dz;
    float distance2;

    path_find_closest_point_on_segment();

    dx = local_c - request->avoid_position.x;
    dy = local_8 - request->avoid_position.y;
    dz = local_4 - request->avoid_position.z;
    distance2 = dy * dy + dx * dx + dz * dz;

    if (distance2 < request->avoid_radius * request->avoid_radius) {
        float distance = (float)sqrt(distance2);
        *out_distance = distance;
        return (1.0f - distance / request->avoid_radius) * request->avoid_weight;
    }

    *out_distance = 3.4028235e+38f;
    return 0.0f;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043b3b0 @ 0x43b3b0) ----
float10 FUN_0043b3b0(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int unaff_EBX;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  path_find_closest_point_on_segment();
  fVar3 = local_c - *(float *)(unaff_EBX + 0x28);
  fVar1 = local_8 - *(float *)(unaff_EBX + 0x2c);
  fVar2 = local_4 - *(float *)(unaff_EBX + 0x30);
  fVar3 = fVar1 * fVar1 + fVar3 * fVar3 + fVar2 * fVar2;
  if (fVar3 < *(float *)(unaff_EBX + 0x38) * *(float *)(unaff_EBX + 0x38)) {
    fVar3 = SQRT(fVar3);
    fVar1 = *(float *)(unaff_EBX + 0x38);
    fVar2 = *(float *)(unaff_EBX + 0x3c);
    *param_1 = fVar3;
    return (float10)((1.0 - fVar3 / fVar1) * fVar2);
  }
  *param_1 = 3.4028235e+38;
  return (float10)0.0;
}
#endif
