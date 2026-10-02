// render_frustum_compute_box_overlap_area  (Ghidra: FUN_0050dac0; new name, evidence below)
// address 0x50dac0, size 763 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase4/render_functions.md's phase-2 summary: "Computes the projected screen-space
//   overlap area of a portal-like rectangle against the current view, used for portal
//   culling/visibility weighting." The box argument's six floats are read in exactly the
//   real_rectangle3d order (x.lower/upper, y.lower/upper, z.lower/upper) and the frustum argument
//   uses the same four projection fields (+0x144/0x158/0x164/0x168) as
//   render_frustum_compute_screen_clip_bounds (0x50ddc0) and render_project_world_point_to_screen
//   (0x50de30).
// register convention: ECX = box (real_rectangle3d*, view-space), EDX = frustum (render_frustum*).
//   // blam-cc: ECX=box, EDX=frustum
// UNSURE: each of the four clamped screen-space bounds is computed twice in the decompiled code
//   (once to decide whether to clamp, once more -- via an identical re-comparison -- to produce
//   the actual value), which is exactly clamp(min(a,b), -1) / clamp(max(a,b), 1); the rewrite
//   below computes each min/max once instead of reproducing the redundant recomputation, which
//   does not change the result.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Projects a view-space axis-aligned box (as seen at both its nearest and farthest z) onto the
// screen and returns the area of its clamped (-1..1) NDC bounding rectangle as a fraction of the
// full 2x2 NDC area (0 when the box's projected x or y range does not overlap the screen at all).
// If the box straddles or is behind the camera (z.lower < 0 <= z.upper) it is treated as fully
// covering the view and this returns 1.0.
real render_frustum_compute_box_overlap_area(real_rectangle3d *box, render_frustum *frustum) // blam-cc: ECX=box, EDX=frustum
{
    float inverse_z_near, inverse_z_far;
    float candidate_a, candidate_b;
    float x_lo, x_hi, y_lo, y_hi;
    float area;

    if (box->z.lower >= 0.0f) {
        return 0.0f;
    }
    if (box->z.upper >= 0.0f) {
        return 1.0f;
    }

    inverse_z_near = -1.0f / box->z.lower;
    inverse_z_far = -1.0f / box->z.upper;

    candidate_a = (frustum->projection[2][0] * box->z.lower + frustum->projection[0][0] * box->x.lower) * inverse_z_near;
    candidate_b = (frustum->projection[2][0] * box->z.upper + frustum->projection[0][0] * box->x.lower) * inverse_z_far;
    x_lo = (candidate_a < candidate_b) ? candidate_a : candidate_b;
    if (x_lo < -1.0f) {
        x_lo = -1.0f;
    }

    candidate_a = (frustum->projection[2][1] * box->z.lower + frustum->projection[1][1] * box->y.lower) * inverse_z_near;
    candidate_b = (frustum->projection[2][1] * box->z.upper + frustum->projection[1][1] * box->y.lower) * inverse_z_far;
    y_lo = (candidate_a < candidate_b) ? candidate_a : candidate_b;
    if (y_lo < -1.0f) {
        y_lo = -1.0f;
    }

    candidate_a = (frustum->projection[2][0] * box->z.lower + frustum->projection[0][0] * box->x.upper) * inverse_z_near;
    candidate_b = (frustum->projection[2][0] * box->z.upper + frustum->projection[0][0] * box->x.upper) * inverse_z_far;
    x_hi = (candidate_a > candidate_b) ? candidate_a : candidate_b;
    if (x_hi > 1.0f) {
        x_hi = 1.0f;
    }

    candidate_a = (frustum->projection[2][1] * box->z.lower + frustum->projection[1][1] * box->y.upper) * inverse_z_near;
    candidate_b = (frustum->projection[2][1] * box->z.upper + frustum->projection[1][1] * box->y.upper) * inverse_z_far;
    y_hi = (candidate_a > candidate_b) ? candidate_a : candidate_b;
    if (y_hi > 1.0f) {
        y_hi = 1.0f;
    }

    area = (x_hi - x_lo) * (y_hi - y_lo) * 0.25f;
    return (area > 0.0f) ? area : 0.0f;
}

#if 0
Original Ghidra decompilation (0x50dac0):

float10 FUN_0050dac0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float *in_ECX;
  int in_EDX;
  float10 fVar9;
  float10 fVar10;
  float10 fVar11;
  float10 fVar12;
  float10 fVar13;

  if (in_ECX[4] < 0.0) {
    if (0.0 <= in_ECX[5]) {
      return (float10)1.0;
    }
    fVar2 = *(float *)(in_EDX + 0x144);
    fVar3 = *(float *)(in_EDX + 0x164);
    fVar4 = *(float *)(in_EDX + 0x158);
    fVar6 = fVar2 * *in_ECX;
    fVar1 = (float)-((float10)1.0 / (float10)in_ECX[4]);
    fVar5 = -(1.0 / in_ECX[5]);
    if ((fVar3 * in_ECX[4] + fVar6) * fVar1 <= (fVar3 * in_ECX[5] + fVar6) * fVar5) {
      fVar7 = (fVar3 * in_ECX[4] + fVar6) * fVar1;
    }
    else {
      fVar7 = (fVar3 * in_ECX[5] + fVar6) * fVar5;
    }
    if (fVar7 <= -1.0) {
      fVar9 = (float10)-1.0;
    }
    else {
      fVar9 = ((float10)fVar3 * (float10)in_ECX[4] + (float10)fVar6) * (float10)fVar1;
      fVar6 = (fVar3 * in_ECX[5] + fVar6) * fVar5;
      if ((float10)fVar6 < fVar9) {
        fVar9 = (float10)fVar6;
      }
    }
    fVar6 = *(float *)(in_EDX + 0x168) * in_ECX[4];
    fVar7 = *(float *)(in_EDX + 0x168) * in_ECX[5];
    fVar10 = (float10)fVar4 * (float10)in_ECX[2];
    if ((fVar6 + fVar4 * in_ECX[2]) * fVar1 <= (fVar7 + fVar4 * in_ECX[2]) * fVar5) {
      fVar11 = ((float10)fVar6 + fVar10) * (float10)fVar1;
    }
    else {
      fVar11 = ((float10)fVar7 + fVar10) * (float10)fVar5;
    }
    if (fVar11 <= (float10)-1.0) {
      fVar10 = (float10)-1.0;
    }
    else if ((fVar6 + fVar4 * in_ECX[2]) * fVar1 <= (fVar4 * in_ECX[2] + fVar7) * fVar5) {
      fVar10 = (fVar10 + (float10)fVar6) * (float10)fVar1;
    }
    else {
      fVar10 = (fVar10 + (float10)fVar7) * (float10)fVar5;
    }
    fVar8 = fVar3 * in_ECX[4];
    fVar3 = fVar3 * in_ECX[5];
    fVar11 = (float10)fVar2 * (float10)in_ECX[1];
    if ((fVar8 + fVar2 * in_ECX[1]) * fVar1 <= (fVar2 * in_ECX[1] + fVar3) * fVar5) {
      fVar12 = ((float10)fVar3 + fVar11) * (float10)fVar5;
    }
    else {
      fVar12 = ((float10)fVar8 + fVar11) * (float10)fVar1;
    }
    if (fVar12 <= (float10)1.0) {
      if ((fVar8 + fVar2 * in_ECX[1]) * fVar1 <= (fVar2 * in_ECX[1] + fVar3) * fVar5) {
        fVar11 = (fVar11 + (float10)fVar3) * (float10)fVar5;
      }
      else {
        fVar11 = (fVar11 + (float10)fVar8) * (float10)fVar1;
      }
    }
    else {
      fVar11 = (float10)1.0;
    }
    fVar12 = (float10)fVar4 * (float10)in_ECX[3];
    if ((fVar6 + fVar4 * in_ECX[3]) * fVar1 <= (fVar4 * in_ECX[3] + fVar7) * fVar5) {
      fVar13 = ((float10)fVar7 + fVar12) * (float10)fVar5;
    }
    else {
      fVar13 = ((float10)fVar6 + fVar12) * (float10)fVar1;
    }
    if (fVar13 <= (float10)1.0) {
      if ((fVar6 + fVar4 * in_ECX[3]) * fVar1 <= (fVar4 * in_ECX[3] + fVar7) * fVar5) {
        fVar12 = (fVar12 + (float10)fVar7) * (float10)fVar5;
      }
      else {
        fVar12 = (fVar12 + (float10)fVar6) * (float10)fVar1;
      }
    }
    else {
      fVar12 = (float10)1.0;
    }
    fVar9 = (fVar11 - fVar9) * (fVar12 - fVar10) * (float10)0.25;
    if ((float10)0.0 < fVar9) {
      return fVar9;
    }
  }
  return (float10)0.0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
