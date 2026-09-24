// polygon3d_clip_to_plane  (Ghidra: polygon3d_clip_to_plane, already named)
// address 0x4cb380, size 958 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/math_types_notes.md real_plane3d section (four floats, distance
//   subtracted last, confirmed by this function's own `pfVar18[2]*param_3[2] +
//   param_3[1]*pfVar18[1] + *param_3**pfVar18 - param_3[3]`); out/phase4/math_functions.md
//   ("Clips a 3D polygon against a single plane, emitting the clipped vertex list"). Same
//   Sutherland-Hodgman structure and near-duplicate-vertex merging as
//   polygon2d_clip_to_plane @0x4caff0, extended to 3 components, plus one extra behaviour: when
//   the whole input polygon is within epsilon of the plane (neither a vertex clearly kept nor
//   clearly discarded), the `keep_coplanar` flag decides whether that counts as "keep everything"
//   or "discard everything".
// register convention: none -- all 8 parameters are stack parameters exactly as Ghidra declared
//   them (count, in, plane, max_count, out, clipped_flag, epsilon, keep_coplanar).
// UNSURE: `keep_coplanar` (param_8, a char) is named from its behaviour (see above), not
//   confirmed against a caller; this function has no other callers in this module to
//   cross-check against.

#include "tags.h"
#include "math.h"

// fabs is a single x87 FABS instruction in the original code (Ghidra's ABS() pseudo-function);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double fabs(double x);

// Clips a 3D polygon against a single plane, emitting the clipped vertex list.
int16_t polygon3d_clip_to_plane(int16_t count, real_point3d *in, real_plane3d *plane,
                                 int16_t max_count, real_point3d *out, uint8_t *clipped_flag,
                                 real epsilon, char keep_coplanar)
{
    real_point3d scratch[512]; // aliasing scratch buffer, sized like the original's 1537 floats
    real_point3d *prev;
    real_point3d *cur;
    int16_t output_count;
    int16_t next_count;
    int16_t i;
    int saw_kept_vertex;      // bVar9
    char saw_discarded_vertex; // local_181e
    int prev_side;
    int cur_side;
    real dist;

    output_count = 0;
    saw_kept_vertex = 0;
    saw_discarded_vertex = 0;
    if (clipped_flag != 0) {
        *clipped_flag = 0;
    }
    if (in == out) {
        for (i = 0; i < count; i++) {
            scratch[i] = in[i];
        }
        in = scratch;
    }

    prev = &in[count - 1];
    prev_side = (0.0f <= (plane->normal.i * prev->x + prev->z * plane->normal.k + prev->y * plane->normal.j) - plane->d);

    if (count < 1) {
        output_count = 0;
        goto tail;
    }

    for (i = 0; i < count; i++) {
        cur = &in[i];
        dist = (cur->z * plane->normal.k + plane->normal.j * cur->y + plane->normal.i * cur->x) - plane->d;
        cur_side = (0.0f <= dist);
        if (dist <= epsilon) {
            if (dist < -epsilon) {
                saw_discarded_vertex = 1;
            }
        } else {
            saw_kept_vertex = 1;
        }
        next_count = output_count;
        if (cur_side != prev_side) {
            real denom, t;

            if (output_count == max_count) {
                output_count = -1;
                goto overflow;
            }
            if (clipped_flag != 0) {
                *clipped_flag = 1;
            }
            denom = (prev->z - cur->z) * plane->normal.k + (prev->y - cur->y) * plane->normal.j +
                    (prev->x - cur->x) * plane->normal.i;
            t = -(((cur->z * plane->normal.k + plane->normal.j * cur->y + plane->normal.i * cur->x) - plane->d) / denom);
            if (0.0f <= t) {
                if (1.0f < t) {
                    t = 1.0f;
                }
            } else {
                t = 0.0f;
            }
            out[output_count].x = t * (prev->x - cur->x) + cur->x;
            out[output_count].y = (prev->y - cur->y) * t + cur->y;
            out[output_count].z = t * (prev->z - cur->z) + cur->z;
            next_count = (int16_t)(output_count + 1);
            if (next_count != 1 &&
                (((real)fabs((double)(out[next_count - 1].x - out[0].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[0].y)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].z - out[0].z)) < epsilon) ||
                 ((real)fabs((double)(out[next_count - 1].x - out[next_count - 2].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[next_count - 2].y)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].z - out[next_count - 2].z)) < epsilon))) {
                next_count = output_count;
            }
        }
        output_count = next_count;
        if (cur_side) {
            if (max_count <= next_count) {
                output_count = -1;
                goto overflow;
            }
            out[next_count] = *cur;
            output_count = (int16_t)(next_count + 1);
            if (output_count != 1 &&
                (((real)fabs((double)(out[output_count - 1].x - out[0].x)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].y - out[0].y)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].z - out[0].z)) < epsilon) ||
                 ((real)fabs((double)(out[output_count - 1].x - out[output_count - 2].x)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].y - out[output_count - 2].y)) < epsilon &&
                  (real)fabs((double)(out[output_count - 1].z - out[output_count - 2].z)) < epsilon))) {
                output_count = next_count;
            }
        }
        prev_side = cur_side;
        prev = cur;
    }

    if (output_count == -1) {
        goto overflow;
    }
    if (output_count < 3) {
        output_count = 0;
    }

tail:
    if (saw_kept_vertex) {
        if (saw_discarded_vertex != 0) {
            return output_count; // genuine partial clip, keep the computed result
        }
        // fully kept (no vertex truly outside): fall through to copy input verbatim
    } else if (saw_discarded_vertex != 0 || keep_coplanar == 0) {
        return 0; // fully discarded, or coplanar-and-not-kept
    }
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    return count;

overflow:
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    return output_count;
}

#if 0
Original Ghidra decompilation (0x4cb380):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4
polygon3d_clip_to_plane
          (short param_1,float *param_2,float *param_3,short param_4,float *param_5,
          undefined1 *param_6,float param_7,char param_8)

{
  float *pfVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  bool bVar7;
  float fVar8;
  bool bVar9;
  uint uVar10;
  bool bVar11;
  undefined1 *puVar12;
  uint uVar13;
  int iVar14;
  uint uVar15;
  short sVar16;
  short sVar17;
  float *pfVar18;
  char local_181e;
  uint local_181c;
  float local_1808 [1537];

  sVar17 = 0;
  bVar9 = false;
  local_181e = '\0';
  if (param_6 != (undefined1 *)0x0) {
    *param_6 = 0;
  }
  puVar12 = param_6;
  if (param_2 == param_5) {
    puVar12 = (undefined1 *)(param_1 * 0xc);
    pfVar18 = local_1808;
    for (uVar13 = param_1 * 3 & 0x3fffffff; uVar13 != 0; uVar13 = uVar13 - 1) {
      *pfVar18 = *param_2;
      param_2 = param_2 + 1;
      pfVar18 = pfVar18 + 1;
    }
    for (iVar14 = 0; iVar14 != 0; iVar14 = iVar14 + -1) {
      *(undefined1 *)pfVar18 = *(undefined1 *)param_2;
      param_2 = (float *)((int)param_2 + 1);
      pfVar18 = (float *)((int)pfVar18 + 1);
    }
    param_2 = local_1808;
  }
  uVar15 = param_1 * 0xc;
  pfVar18 = param_2 + param_1 * 3 + -3;
  uVar13 = (uint)puVar12 & 0xffff0000;
  local_181c = 0;
  bVar11 = 0.0 <= (*param_3 * *pfVar18 + pfVar18[2] * param_3[2] + pfVar18[1] * param_3[1]) -
                  param_3[3];
  if (param_1 < 1) {
LAB_004cb6cf:
    sVar17 = 0;
  }
  else {
    do {
      pfVar1 = param_2 + (short)local_181c * 3;
      fVar3 = (pfVar1[2] * param_3[2] + param_3[1] * pfVar1[1] + *param_3 * *pfVar1) - param_3[3];
      bVar7 = 0.0 <= fVar3;
      if (fVar3 <= param_7) {
        if (fVar3 < -param_7) {
          local_181e = '\x01';
        }
      }
      else {
        bVar9 = true;
      }
      sVar16 = sVar17;
      if (bVar7 != bVar11) {
        if (sVar17 == param_4) {
          sVar17 = -1;
          goto LAB_004cb6f7;
        }
        if (param_6 != (undefined1 *)0x0) {
          *param_6 = 1;
        }
        fVar3 = pfVar18[1];
        fVar4 = pfVar1[1];
        fVar5 = pfVar18[2];
        fVar6 = pfVar1[2];
        fVar8 = -(((pfVar1[2] * param_3[2] + param_3[1] * pfVar1[1] + *param_3 * *pfVar1) -
                  param_3[3]) /
                 ((fVar5 - fVar6) * param_3[2] +
                 (fVar3 - fVar4) * param_3[1] + (*pfVar18 - *pfVar1) * *param_3));
        if (0.0 <= fVar8) {
          if (1.0 < fVar8) {
            fVar8 = 1.0;
          }
        }
        else {
          fVar8 = 0.0;
        }
        pfVar2 = param_5 + sVar17 * 3;
        sVar16 = sVar17 + 1;
        *pfVar2 = fVar8 * (*pfVar18 - *pfVar1) + *pfVar1;
        pfVar2[1] = (fVar3 - fVar4) * fVar8 + pfVar1[1];
        pfVar2[2] = fVar8 * (fVar5 - fVar6) + pfVar1[2];
        if ((sVar16 != 1) &&
           ((((iVar14 = (int)sVar16, ABS(param_5[iVar14 * 3 + -3] - *param_5) < param_7 &&
              (ABS(param_5[iVar14 * 3 + -2] - param_5[1]) < param_7)) &&
             (ABS(param_5[iVar14 * 3 + -1] - param_5[2]) < param_7)) ||
            (((ABS(param_5[iVar14 * 3 + -3] - param_5[iVar14 * 3 + -6]) < param_7 &&
              (ABS(param_5[iVar14 * 3 + -2] - param_5[iVar14 * 3 + -5]) < param_7)) &&
             (ABS(param_5[iVar14 * 3 + -1] - param_5[iVar14 * 3 + -4]) < param_7)))))) {
          sVar16 = sVar17;
        }
      }
      sVar17 = sVar16;
      if (bVar7) {
        if (param_4 <= sVar16) {
          sVar17 = -1;
          goto LAB_004cb6f7;
        }
        pfVar18 = param_5 + sVar16 * 3;
        *pfVar18 = *pfVar1;
        pfVar18[1] = pfVar1[1];
        sVar17 = sVar16 + 1;
        pfVar18[2] = pfVar1[2];
        if ((sVar17 != 1) &&
           ((((iVar14 = (int)sVar17, ABS(param_5[iVar14 * 3 + -3] - *param_5) < param_7 &&
              (ABS(param_5[iVar14 * 3 + -2] - param_5[1]) < param_7)) &&
             (ABS(param_5[iVar14 * 3 + -1] - param_5[2]) < param_7)) ||
            (((ABS(param_5[iVar14 * 3 + -3] - param_5[iVar14 * 3 + -6]) < param_7 &&
              (ABS(param_5[iVar14 * 3 + -2] - param_5[iVar14 * 3 + -5]) < param_7)) &&
             (ABS(param_5[iVar14 * 3 + -1] - param_5[iVar14 * 3 + -4]) < param_7)))))) {
          sVar17 = sVar16;
        }
      }
      uVar13 = local_181c + 1;
      pfVar18 = pfVar1;
      bVar11 = bVar7;
      local_181c = uVar13;
    } while ((short)uVar13 < param_1);
    if (sVar17 == -1) {
LAB_004cb6f7:
      for (uVar13 = uVar15 >> 2; uVar13 != 0; uVar13 = uVar13 - 1) {
        *param_5 = *param_2;
        param_2 = param_2 + 1;
        param_5 = param_5 + 1;
      }
      for (iVar14 = 0; uVar10 = uVar15, iVar14 != 0; iVar14 = iVar14 + -1) {
        *(undefined1 *)param_5 = *(undefined1 *)param_2;
        param_2 = (float *)((int)param_2 + 1);
        param_5 = (float *)((int)param_5 + 1);
      }
      goto LAB_004cb70c;
    }
    if (sVar17 < 3) goto LAB_004cb6cf;
  }
  if (bVar9) {
    uVar10 = CONCAT31((int3)(uVar13 >> 8),local_181e);
    if (local_181e != '\0') {
LAB_004cb70c:
      return CONCAT22((short)(uVar10 >> 0x10),sVar17);
    }
  }
  else if ((local_181e != '\0') || (param_8 == '\0')) {
    return 0;
  }
  for (uVar15 = uVar15 >> 2; uVar15 != 0; uVar15 = uVar15 - 1) {
    *param_5 = *param_2;
    param_2 = param_2 + 1;
    param_5 = param_5 + 1;
  }
  for (iVar14 = 0; iVar14 != 0; iVar14 = iVar14 + -1) {
    *(undefined1 *)param_5 = *(undefined1 *)param_2;
    param_2 = (float *)((int)param_2 + 1);
    param_5 = (float *)((int)param_5 + 1);
  }
  return CONCAT22((short)(uVar13 >> 0x10),param_1);
}
#endif
