// polygon2d_clip_to_plane  (Ghidra: polygon2d_clip_to_plane, already named)
// address 0x4caff0, size 898 bytes
// VERIFIED against disassembly 0x4caff0..0x4cb372 (2026-09-30)
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: out/phase4/math_types_notes.md ("polygon2d_clip_to_plane's param_5 is an in/out
//   uint32_t* edge bitmask, one bit per output vertex, marking vertices created by the clip");
//   real_plane2d layout confirmed there too. Classic Sutherland-Hodgman clip of a 2D polygon
//   against one line: walks each edge, emits the plane-crossing intersection point when an edge
//   changes side, and copies through vertices already on the keep side; near-duplicate output
//   vertices (within epsilon of the first or previous output vertex) are dropped; an output
//   buffer overflow aborts and returns -1 after copying the untouched input back to the output;
//   a fully-kept or fully-discarded input polygon (never truly straddling the plane within
//   epsilon) short-circuits to copying the input through unchanged / returning an empty polygon.
// register convention: output vertex array in EDX (in_EDX); the 7 recognized stack parameters
//   (count, input vertices, plane, max_count, edge_bitmask, clipped_flag, epsilon) in that
//   order.
//   // blam-cc: EDX -> out, stack -> (count, in, plane, max_count, edge_bitmask, clipped_flag, epsilon)

#include "tags.h"
#include "math.h"

// fabs is a single x87 FABS instruction in the original code (Ghidra's ABS() pseudo-function);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double fabs(double x);

// Clips a 2D polygon against a single line/plane, emitting the clipped vertex list.
int16_t polygon2d_clip_to_plane(real_point2d *out, int16_t count, real_point2d *in,
                                 real_plane2d *plane, int16_t max_count, uint32_t *edge_bitmask,
                                 uint8_t *clipped_flag, real epsilon)
{
    real_point2d scratch[512]; // aliasing scratch buffer, sized like the original's 1023 floats
    real_point2d *prev;
    real_point2d *cur;
    int16_t output_count;
    int16_t next_count;
    int16_t i;
    uint32_t edge_bits;
    int saw_kept_vertex;    // bVar5
    int saw_discarded_vertex; // bVar6
    int prev_side;
    int cur_side;
    real dist;

    output_count = 0;
    saw_kept_vertex = 0;
    saw_discarded_vertex = 0;
    edge_bits = 0;
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
    prev_side = (0.0f <= (prev->x * plane->normal.i + prev->y * plane->normal.j) - plane->d);

    if (count < 1) {
        output_count = 0;
        goto after_loop;
    }

    for (i = 0; i < count; i++) {
        cur = &in[i];
        dist = (plane->normal.i * cur->x + cur->y * plane->normal.j) - plane->d;
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
            denom = (prev->y - cur->y) * plane->normal.j + (prev->x - cur->x) * plane->normal.i;
            if (denom == 0.0f) {
                t = 0.0f;
            } else {
                t = (-1.0f / denom) * ((plane->normal.i * cur->x + cur->y * plane->normal.j) - plane->d);
                if (0.0f <= t) {
                    if (1.0f < t) {
                        t = 1.0f;
                    }
                } else {
                    t = 0.0f;
                }
            }
            out[output_count].x = t * (prev->x - cur->x) + cur->x;
            edge_bits |= 1u << (output_count & 0x1f);
            next_count = (int16_t)(output_count + 1);
            out[output_count].y = t * (prev->y - cur->y) + cur->y;
            if (next_count != 1 &&
                (((real)fabs((double)(out[next_count - 1].x - out[0].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[0].y)) < epsilon) ||
                 ((real)fabs((double)(out[next_count - 1].x - out[next_count - 2].x)) < epsilon &&
                  (real)fabs((double)(out[next_count - 1].y - out[next_count - 2].y)) < epsilon))) {
                next_count = output_count;
            }
            goto after_edge;
        }
    after_edge:
        output_count = next_count;
        if (cur_side) {
            if (next_count == max_count) {
                output_count = -1;
                goto overflow;
            }
            out[next_count] = *cur;
            if (edge_bitmask == 0 || ((1u << (i & 0x1f)) & *edge_bitmask) == 0) {
                edge_bits &= ~(1u << (next_count & 0x1f));
            } else {
                edge_bits |= 1u << (next_count & 0x1f);
            }
            output_count = (int16_t)(next_count + 1);
            if (output_count != 1 &&
                ((((real)fabs((double)(out[output_count - 1].x - out[0].x)) < epsilon &&
                   (real)fabs((double)(out[output_count - 1].y - out[0].y)) < epsilon) ||
                  ((real)fabs((double)(out[output_count - 1].x - out[output_count - 2].x)) < epsilon &&
                   (real)fabs((double)(out[output_count - 1].y - out[output_count - 2].y)) < epsilon)))) {
                output_count = next_count;
            }
        }
        prev_side = cur_side;
        prev = cur;
    }

    if (output_count != -1) {
        if (output_count < 3) {
            output_count = 0;
            goto after_loop;
        }
        goto degenerate_check;
    }

overflow:
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    goto done;

after_loop:
degenerate_check:
    if (!saw_kept_vertex) {
        output_count = 0;
        goto done;
    }
    if (saw_discarded_vertex) {
        goto done;
    }
    for (i = 0; i < count; i++) {
        out[i] = in[i];
    }
    output_count = count;

done:
    if (edge_bitmask != 0) {
        *edge_bitmask = edge_bits;
    }
    return output_count;
}

#if 0
Original Ghidra decompilation (0x4caff0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

short polygon2d_clip_to_plane
                (short param_1,float *param_2,float *param_3,short param_4,uint *param_5,
                undefined1 *param_6,float param_7)

{
  float fVar1;
  float fVar2;
  bool bVar3;
  float fVar4;
  bool bVar5;
  bool bVar6;
  short sVar7;
  int iVar8;
  float *in_EDX;
  short sVar9;
  short sVar10;
  float *pfVar11;
  uint uVar12;
  bool local_101b;
  uint local_1018;
  float *local_100c;
  float local_1008 [1023];
  undefined4 uStack_c;

  uStack_c = 0x4cb000;
  sVar10 = 0;
  bVar5 = false;
  bVar6 = false;
  local_1018 = 0;
  if (param_6 != (undefined1 *)0x0) {
    *param_6 = 0;
  }
  if (param_2 == in_EDX) {
    pfVar11 = local_1008;
    for (iVar8 = ((int)param_1 & 0x1fffffffU) << 1; iVar8 != 0; iVar8 = iVar8 + -1) {
      *pfVar11 = *param_2;
      param_2 = param_2 + 1;
      pfVar11 = pfVar11 + 1;
    }
    for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
      *(undefined1 *)pfVar11 = *(undefined1 *)param_2;
      param_2 = (float *)((int)param_2 + 1);
      pfVar11 = (float *)((int)pfVar11 + 1);
    }
    param_2 = local_1008;
  }
  uVar12 = (uint)param_1;
  local_100c = param_2 + uVar12 * 2 + -2;
  sVar7 = 0;
  local_101b = 0.0 <= (*local_100c * *param_3 + local_100c[1] * param_3[1]) - param_3[2];
  if (param_1 < 1) {
LAB_004cb31d:
    sVar10 = 0;
LAB_004cb31f:
    if (!bVar5) {
      sVar10 = 0;
      goto LAB_004cb345;
    }
    if (bVar6) goto LAB_004cb345;
    for (iVar8 = (uVar12 & 0x1fffffff) << 1; sVar10 = param_1, iVar8 != 0; iVar8 = iVar8 + -1) {
      *in_EDX = *param_2;
      param_2 = param_2 + 1;
      in_EDX = in_EDX + 1;
    }
  }
  else {
    do {
      pfVar11 = param_2 + sVar7 * 2;
      fVar1 = (*param_3 * *pfVar11 + pfVar11[1] * param_3[1]) - param_3[2];
      bVar3 = 0.0 <= fVar1;
      if (fVar1 <= param_7) {
        if (fVar1 < -param_7) {
          bVar6 = true;
        }
      }
      else {
        bVar5 = true;
      }
      sVar9 = sVar10;
      if (bVar3 != local_101b) {
        if (sVar10 != param_4) {
          if (param_6 != (undefined1 *)0x0) {
            *param_6 = 1;
          }
          fVar1 = local_100c[1];
          fVar2 = pfVar11[1];
          fVar4 = (fVar1 - fVar2) * param_3[1] + (*local_100c - *pfVar11) * *param_3;
          if (fVar4 == 0.0) {
            fVar4 = 0.0;
          }
          else {
            fVar4 = (-1.0 / fVar4) * ((*param_3 * *pfVar11 + pfVar11[1] * param_3[1]) - param_3[2]);
            if (0.0 <= fVar4) {
              if (1.0 < fVar4) {
                fVar4 = 1.0;
              }
            }
            else {
              fVar4 = 0.0;
            }
          }
          in_EDX[sVar10 * 2] = fVar4 * (*local_100c - *pfVar11) + *pfVar11;
          local_1018 = local_1018 | 1 << ((byte)sVar10 & 0x1f);
          sVar9 = sVar10 + 1;
          in_EDX[sVar10 * 2 + 1] = fVar4 * (fVar1 - fVar2) + pfVar11[1];
          if ((sVar9 != 1) &&
             (((iVar8 = (int)sVar9, ABS(in_EDX[iVar8 * 2 + -2] - *in_EDX) < param_7 &&
               (ABS(in_EDX[iVar8 * 2 + -1] - in_EDX[1]) < param_7)) ||
              ((ABS(in_EDX[iVar8 * 2 + -2] - in_EDX[iVar8 * 2 + -4]) < param_7 &&
               (ABS(in_EDX[iVar8 * 2 + -1] - in_EDX[iVar8 * 2 + -3]) < param_7)))))) {
            sVar9 = sVar10;
          }
          goto LAB_004cb223;
        }
LAB_004cb32b:
        sVar10 = -1;
        goto LAB_004cb32e;
      }
LAB_004cb223:
      sVar10 = sVar9;
      if (bVar3) {
        if (sVar9 == param_4) goto LAB_004cb32b;
        in_EDX[sVar9 * 2] = *pfVar11;
        in_EDX[sVar9 * 2 + 1] = pfVar11[1];
        if ((param_5 == (uint *)0x0) || ((1 << ((byte)sVar7 & 0x1f) & *param_5) == 0)) {
          local_1018 = local_1018 & ~(1 << ((byte)sVar9 & 0x1f));
        }
        else {
          local_1018 = local_1018 | 1 << ((byte)sVar9 & 0x1f);
        }
        sVar10 = sVar9 + 1;
        if ((sVar10 != 1) &&
           (((iVar8 = (int)sVar10, ABS(in_EDX[iVar8 * 2 + -2] - *in_EDX) < param_7 &&
             (ABS(in_EDX[iVar8 * 2 + -1] - in_EDX[1]) < param_7)) ||
            ((ABS(in_EDX[iVar8 * 2 + -2] - in_EDX[iVar8 * 2 + -4]) < param_7 &&
             (ABS(in_EDX[iVar8 * 2 + -1] - in_EDX[iVar8 * 2 + -3]) < param_7)))))) {
          sVar10 = sVar9;
        }
      }
      sVar7 = sVar7 + 1;
      local_101b = bVar3;
      local_100c = pfVar11;
    } while (sVar7 < param_1);
    if (sVar10 != -1) {
      if (sVar10 < 3) goto LAB_004cb31d;
      goto LAB_004cb31f;
    }
LAB_004cb32e:
    for (iVar8 = (uVar12 & 0x1fffffff) << 1; iVar8 != 0; iVar8 = iVar8 + -1) {
      *in_EDX = *param_2;
      param_2 = param_2 + 1;
      in_EDX = in_EDX + 1;
    }
  }
  for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
    *(undefined1 *)in_EDX = *(undefined1 *)param_2;
    param_2 = (float *)((int)param_2 + 1);
    in_EDX = (float *)((int)in_EDX + 1);
  }
LAB_004cb345:
  if (param_5 != (uint *)0x0) {
    *param_5 = local_1018;
  }
  return sVar10;
}
#endif
