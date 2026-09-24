// path_find_closest_point_on_segment  (Ghidra: path_find_closest_point_on_segment, already named)
// address 0x43b2f0, size 192 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase2/ai/07.md / out/phase2/results/ai_07.json (called from FUN_0043a310 and
//   FUN_0043b3b0 as a path-node "leash"/look-ahead helper). out/phase4/ai_types_notes.md lists
//   this address among the "plain math helpers that belong with types/math.h" living inside the
//   ai address range. Standard closest-point-on-segment projection: t = dot(start - point,
//   end - start) / |end - start|^2; when t falls inside [0,1] the output is the lerped point,
//   otherwise the output is unconditionally the segment's end point (not clamped per side) --
//   preserved exactly even though a "closest point" routine would normally clamp to start when
//   t < 0.
// register convention (confirmed via objdump on both call sites, 0x43a380 and 0x43b3ce): point
//   in EAX (in_EAX), segment start in ECX (in_ECX), segment end in EDX (in_EDX), output point in
//   ESI (unaff_ESI, a genuine register parameter -- never assigned inside the function). No stack
//   arguments.
//   // blam-cc: EAX -> point, ECX -> segment_start, EDX -> segment_end, ESI -> out
//
// review fix (phase 4 math gate): the first draft wrote the bounds test as 0 <= t && t <= 1,
//   which sends a NaN t (zero-length segment) to the end-point path; the binary sends it to
//   the lerp path. Also re-ordered the dot-product sums to the FPU trace.
// UNSURE: t is computed from (segment_start - point), the opposite sign of the textbook
//   closest-point parameter, so a point that projects inside the segment usually gets t < 0
//   and out = segment_end. Verified against the disassembly (`fld [ecx]; fsub [eax]`); whether
//   the callers intend this (or pass the arguments in the other order) is open.
// UNSURE: DAT_00672ac0 / DAT_00672ac4 are the MSVC float literals 0.0f / 1.0f (per
// types/math.h's note that every DAT_00672xxx is an inlined literal), not named globals; the
// disassembly bounds check confirms 0.0 <= t <= 1.0 exactly as Ghidra decompiled it.

#include "tags.h"
#include "math.h"

// Projects `point` onto the segment [segment_start, segment_end]; if the projection parameter
// falls inside [0,1] the lerped point is written to `out`, otherwise `out` is set to
// segment_end unconditionally (this routine does not clamp to segment_start on the other side).
void path_find_closest_point_on_segment(const real_point3d *point, const real_point3d *segment_start,
                                         const real_point3d *segment_end, real_point3d *out)
{
    real dx = segment_end->x - segment_start->x;
    real dy = segment_end->y - segment_start->y;
    real dz = segment_end->z - segment_start->z;
    real t;

    // Summation order from the FPU trace: numerator (sy*dy + sz*dz) + sx*dx, denominator
    // (dy*dy + dx*dx) + dz*dz, where s = segment_start - point (note: start minus point, so t
    // is the NEGATED usual projection parameter; preserved literally, see the UNSURE above).
    t = ((segment_start->y - point->y) * dy +
         (segment_start->z - point->z) * dz +
         (segment_start->x - point->x) * dx) /
        (dy * dy + dx * dx + dz * dz);

    // `test ah,0x5 / jnp` exits only on t < 0 and `test ah,0x41 / je` exits only on t > 1, so an
    // unordered t (0/0 for a zero-length segment) takes the lerp path and writes NaNs.
    if (!(t < 0.0f) && !(t > 1.0f)) {
        out->x = dx * t + segment_start->x;
        out->y = dy * t + segment_start->y;
        out->z = dz * t + segment_start->z;
        return;
    }
    out->x = segment_end->x;
    out->y = segment_end->y;
    out->z = segment_end->z;
}

#if 0
Original Ghidra decompilation (0x43b2f0):

void path_find_closest_point_on_segment(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;

  fVar1 = *in_EDX - *in_ECX;
  fVar2 = in_EDX[1] - in_ECX[1];
  fVar3 = in_EDX[2] - in_ECX[2];
  fVar4 = ((*in_ECX - *in_EAX) * fVar1 +
          (in_ECX[2] - in_EAX[2]) * fVar3 + (in_ECX[1] - in_EAX[1]) * fVar2) /
          (fVar3 * fVar3 + fVar1 * fVar1 + fVar2 * fVar2);
  if ((0.0 <= fVar4) && (fVar4 <= 1.0)) {
    *unaff_ESI = fVar1 * fVar4 + *in_ECX;
    unaff_ESI[1] = fVar2 * fVar4 + in_ECX[1];
    unaff_ESI[2] = fVar3 * fVar4 + in_ECX[2];
    return;
  }
  *unaff_ESI = *in_EDX;
  unaff_ESI[1] = in_EDX[1];
  unaff_ESI[2] = in_EDX[2];
  return;
}
#endif
