// polygon2d_convex_hull_build  (Ghidra: polygon2d_convex_hull_build, already named)
// address 0x4caae0, size 651 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: out/phase4/math_functions.md ("Computes the convex hull of a 2D point array using
//   an angular gift-wrapping walk, returning hull vertex indices"). First calls
//   polygon2d_points_classify @0x4caa40 and bails out unless the points are in general
//   position; then finds the bottom-most (min y, tie-broken by min x) start vertex, and walks a
//   classic Jarvis-march gift wrap: from the current hull vertex, pick the point with the
//   smallest positive turn angle (atan2 of the candidate relative to the current vertex, minus
//   the running cumulative edge angle, wrapped into [-0.0001, 2*pi)) until the walk closes back
//   on the start vertex.
// register convention: points array in EAX (in_EAX); count and output hull-index array as the
//   two recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> points, stack -> (count, hull)
// UNSURE: fVar11 (the running "best next vertex" candidate) is tracked as a float holding an
//   integer index in the original; represented directly as int16_t here since every value it
//   ever holds is a small non-negative point index (exact in float, so this is not a behaviour
//   change). The degenerate fallback path (reached only if the gift wrap runs for `count`
//   iterations without ever closing back on the start vertex -- i.e. should not happen for a
//   valid simple polygon) trims a trailing repeated run from the hull array; the exact return
//   value on its two dead-end branches (`uVar2 < 1`) is ambiguous in the decompile (a stale
//   register is returned via CONCAT22), so this rewrite returns the input count there rather
//   than guess at a stale value.

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// atan2/fabs are the C-library spellings of the x87 FPATAN/FABS instructions the original code
// uses directly (Ghidra's fpatan()/ABS() pseudo-calls); declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double atan2(double y, double x);
extern double fabs(double x);


// Computes the convex hull of a 2D point array using an angular gift-wrapping walk, returning
// hull vertex indices.
int16_t polygon2d_convex_hull_build(real_point2d *points, int16_t count, int16_t *hull)
{
    int16_t classification;
    int16_t hull_count;
    int16_t current;
    int16_t start;
    int16_t i;
    real min_y;
    real min_x;
    double cumulative_angle;
    int duplicate_seen;

    classification = polygon2d_points_classify(points, count);
    if (classification != 2) {
        return 0;
    }

    min_y = 3.4028235e+38f;
    min_x = 3.4028235e+38f;
    start = 0;
    for (i = 0; i < count; i++) {
        real y = points[i].y;
        real x = points[i].x;
        if ((y < min_y - 0.0001f) ||
            (y < min_y && x < min_x + 0.0001f) ||
            (y < min_y + 0.0001f && x < min_x - 0.0001f)) {
            min_x = x;
            min_y = y;
            start = i;
        }
    }

    hull_count = 0;
    current = start;
    cumulative_angle = 0.0;
    duplicate_seen = 0;

    for (;;) {
        real_point2d *cur_pt;
        int16_t best_next;
        double best_angle;

        hull[hull_count] = current;
        hull_count++;
        if (!(hull_count < count)) {
            break;
        }

        best_angle = 3.4028235e+38;
        best_next = current;
        cur_pt = &points[current];
        for (i = 0; i < count; i++) {
            real_point2d *cand = &points[i];
            if (cand->x != cur_pt->x || cand->y != cur_pt->y) {
                double angle = atan2((double)(cand->y - cur_pt->y), (double)(cand->x - cur_pt->x));
                double delta = angle - cumulative_angle;
                while (delta < -0.0001) {
                    delta += 6.2831855;
                }
                if (delta < best_angle) {
                    best_angle = delta;
                    best_next = i;
                }
            }
        }
        cumulative_angle += best_angle;
        current = best_next;

        if (!duplicate_seen) {
            if (0.0001f <= (real)fabs((double)(points[current].x - points[hull[0]].x)) ||
                0.0001f <= (real)fabs((double)(points[current].y - points[hull[0]].y))) {
                duplicate_seen = 1;
            }
        }
        if (current == hull[0]) {
            return hull_count;
        }
        if (!duplicate_seen) {
            continue;
        }
        if (0.0001f <= (real)fabs((double)(points[current].x - points[hull[0]].x)) ||
            0.0001f <= (real)fabs((double)(points[current].y - points[hull[0]].y))) {
            continue;
        }
        return hull_count;
    }

    // Degenerate fallback: the walk ran for `count` steps without closing. Trim a trailing
    // repeated run from the end of the hull array.
    {
        int16_t trim = (int16_t)(hull_count - 2);
        int16_t n;

        if (trim < 1) {
            return count;
        }
        while (hull[trim] != hull[hull_count - 1]) {
            trim--;
            if (trim < 1) {
                return count;
            }
        }
        n = (int16_t)((hull_count - 1) - trim);
        if (0 < n) {
            for (i = 0; i < n; i++) {
                hull[i] = hull[trim + i];
            }
            return n;
        }
        return count;
    }
}

#if 0
Original Ghidra decompilation (0x4caae0):

uint polygon2d_convex_hull_build(undefined4 param_1,short *param_2)

{
  bool bVar1;
  float *in_EAX;
  uint uVar2;
  short *psVar3;
  short sVar4;
  float *pfVar5;
  float *pfVar6;
  uint uVar7;
  float fVar8;
  short sVar9;
  short sVar10;
  float fVar11;
  float fVar12;
  float10 fVar13;
  float10 fVar14;
  int local_c;
  float local_8;
  float local_4;

  local_c = 0;
  uVar2 = FUN_004caa40(param_1);
  if ((short)uVar2 != 2) {
    return uVar2 & 0xffff0000;
  }
  fVar13 = (float10)0.0;
  local_4 = 3.4028235e+38;
  fVar11 = 3.4028235e+38;
  fVar8 = 0.0;
  sVar9 = (short)param_1;
  bVar1 = false;
  local_8 = 3.4028235e+38;
  fVar12 = local_4;
  if (0 < sVar9) {
    pfVar5 = in_EAX + 1;
    do {
      if (((*pfVar5 < local_8 - 0.0001) || ((*pfVar5 < local_8 && (pfVar5[-1] < local_4 + 0.0001))))
         || ((*pfVar5 < local_8 + 0.0001 && (pfVar5[-1] < local_4 - 0.0001)))) {
        local_4 = pfVar5[-1];
        local_8 = *pfVar5;
        fVar11 = fVar8;
      }
      fVar8 = (float)((int)fVar8 + 1);
      pfVar5 = pfVar5 + 2;
      fVar12 = local_4;
    } while (SUB42(fVar8,0) < sVar9);
  }
LAB_004cabb0:
  sVar4 = (short)local_c;
  local_4 = 3.4028235e+38;
  if (sVar4 < sVar9) {
    sVar10 = SUB42(fVar11,0);
    param_2[sVar4] = sVar10;
    local_c = local_c + 1;
    fVar8 = 0.0;
    fVar11 = fVar12;
    if (0 < sVar9) {
      pfVar5 = in_EAX + sVar10 * 2;
      pfVar6 = in_EAX;
      do {
        if ((*pfVar6 != *pfVar5) || (pfVar6[1] != pfVar5[1])) {
          fVar14 = (float10)fpatan((float10)pfVar6[1] - (float10)pfVar5[1],
                                   (float10)*pfVar6 - (float10)*pfVar5);
          fVar14 = fVar14 - fVar13;
          if (fVar14 < (float10)-0.0001) {
            do {
              fVar14 = fVar14 + (float10)6.2831855;
            } while (fVar14 < (float10)-0.0001);
          }
          if (fVar14 < (float10)local_4) {
            local_4 = (float)fVar14;
            fVar11 = fVar8;
          }
        }
        fVar8 = (float)((int)fVar8 + 1);
        pfVar6 = pfVar6 + 2;
      } while (SUB42(fVar8,0) < sVar9);
    }
    fVar13 = fVar13 + (float10)local_4;
    sVar4 = SUB42(fVar11,0);
    if (!bVar1) {
      if ((0.0001 <= ABS(in_EAX[sVar4 * 2] - in_EAX[*param_2 * 2])) ||
         (bVar1 = false, 0.0001 <= ABS(in_EAX[sVar4 * 2 + 1] - in_EAX[*param_2 * 2 + 1]))) {
        bVar1 = true;
      }
    }
    sVar10 = *param_2;
    if (sVar4 == sVar10) goto LAB_004cacf9;
    fVar12 = fVar11;
    if (!bVar1) goto LAB_004cabb0;
    if ((0.0001 <= ABS(in_EAX[sVar4 * 2] - in_EAX[sVar10 * 2])) ||
       (0.0001 <= ABS(in_EAX[sVar4 * 2 + 1] - in_EAX[sVar10 * 2 + 1]))) goto LAB_004cabb0;
LAB_004cacf9:
    uVar2 = (uint)param_2 & 0xffff0000;
    sVar9 = (short)local_c;
  }
  else {
    uVar2 = local_c - 2;
    if ((short)uVar2 < 1) goto LAB_004cad2f;
    while (param_2[(short)uVar2] != param_2[(short)(local_c + -1)]) {
      uVar2 = uVar2 - 1;
      if ((short)uVar2 < 1) {
LAB_004cad2f:
        return CONCAT22((short)(uVar2 >> 0x10),sVar4);
      }
    }
    uVar7 = (local_c + -1) - uVar2;
    sVar9 = (short)uVar7;
    if (0 < sVar9) {
      psVar3 = param_2 + (short)uVar2;
      uVar7 = uVar7 & 0xffff;
      do {
        *param_2 = *psVar3;
        psVar3 = psVar3 + 1;
        param_2 = param_2 + 1;
        uVar7 = uVar7 - 1;
      } while (uVar7 != 0);
      return CONCAT22((short)((uint)psVar3 >> 0x10),sVar9);
    }
  }
  return CONCAT22((short)(uVar2 >> 0x10),sVar9);
}
#endif
