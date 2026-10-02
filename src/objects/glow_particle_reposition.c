// glow_particle_reposition
// address 0x4fde40, size 1832 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fde40 |
//   lightning_segment_reposition | glow_particle_reposition")
// rewrite confidence: 0.15 (by far the largest and most arithmetic-dense function in this file
//   group: it locates which marker-chain segment a particle's `t` falls in, then builds a
//   Catmull-Rom-style four-point control set -- with three different branches for a 2-marker
//   chain, a 3-marker chain, and the general N-marker case -- and evaluates a cubic through it
//   via vector3d_cubic_interpolate (out-of-module, generic math helper; see file header of the antenna_update
//   file group's cubic-interpolate note in objects_types_notes.md) before rotating the offset
//   about the chain tangent by a phase angle and adding it to the particle position. Kept as a
//   literal, offset-based transliteration; only the anchors independently confirmed elsewhere
//   in this file group (entry+4 marker_count, entry+0x238 cumulative_length, entry+0x22a
//   marker_order, entry+8+i*0x6c marker i with node_transform at +0x38) are named through
//   `glow`/`object_marker`. Everything reached only through this function's own local temporaries
//   is left as raw floats matching Ghidra's own names.
// evidence: types/objects.h glow (marker_count 0x04, markers 0x08, marker_order 0x22a,
//   cumulative_length 0x238); object_marker (node_transform.position 0x60, node_transform.up
//   0x54, node_transform.left 0x48, node_transform.forward 0x3c).
// register convention: Ghidra recognizes param_1 (the particle) and param_2 (a float, the phase
//   rate) cleanly; `in_EDX` (the glow entry) is an unresolved implicit input, consistent with
//   every other function in this file group taking the glow entry that way.
// blam-cc: EDX -> entry, stack -> particle, phase_rate (UNSURE on the EDX placement; not
//   independently confirmed by disassembling a call site)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void vector3d_cubic_interpolate(real_point3d *out, real_point3d *control_points /*4 elements*/,
                          float t0, float t1, float t2, float t3, float t); // 0x4fcb00, out of
    // module scope by design (see out/phase4/objects_types_notes.md's "not objects-module code"
    // section); per-axis cubic interpolation applied to a 3D vector through four control points
extern double sin(double x);
extern double cos(double x);

void glow_particle_reposition(glow *entry /*EDX*/, uint8_t *particle, float phase_rate)
    // blam-cc: EDX -> entry, stack -> particle, phase_rate
{
    uint8_t *e = (uint8_t *)entry;
    float t = *(float *)(particle + 0x28);
    int16_t segment_count = *(int16_t *)(e + 4) - 1;
    int16_t seg = 0;
    int32_t idx;

    // local_a4/local_74/local_30: four (x,y,z) control points each. Ghidra sized local_a4 and
    // local_74 at only 6 floats (matching the 2- and 3-marker branches, which fill just two
    // points), but the general-marker-count branch below fills all four; over-sized to 12
    // floats uniformly to accommodate that path without overrunning the buffer.
    real_point3d c0[4], c1[4], c2[4];
    float t0 = 0.0f, t1 = 0.0f, t2 = 0.0f, t3 = 0.0f;
    real_point3d out1, out2;

    if (segment_count < 1) {
        idx = (seg > segment_count) ? segment_count : seg;
    } else {
        idx = 0;
        do {
            float lo = *(float *)(e + 0x238 + idx * 4);
            if (lo != t && lo < t && t < *(float *)(e + 0x23c + idx * 4)) break;
            seg = seg + 1;
            idx = seg;
        } while (idx < segment_count);
        if (seg < 0) idx = 0;
    }
    seg = (int16_t)idx;
    *(int16_t *)(particle + 2) = seg;

    {
        int16_t marker_count = *(int16_t *)(e + 4);

        if (marker_count == 2) {
            c0[0].x = *(float *)(e + 0x68); c0[0].y = *(float *)(e + 0x6c); c0[0].z = *(float *)(e + 0x70);
            {
                float ex = *(float *)(e + 0xd4), ey = *(float *)(e + 0xd8), ez = *(float *)(e + 0xdc);
                c1[0].x = *(float *)(e + 0x5c); c1[0].y = *(float *)(e + 0x60); c1[0].z = *(float *)(e + 100);
                {
                    float fx = *(float *)(e + 200), fy = *(float *)(e + 0xcc), fz = *(float *)(e + 0xd0);
                    t0 = *(float *)(e + 0x238); t1 = *(float *)(e + 0x23c);

                    c0[1].x = (ex - c0[0].x) * 0.25f + c0[0].x;
                    c0[1].y = (ey - c0[0].y) * 0.25f + c0[0].y;
                    c0[1].z = (ez - c0[0].z) * 0.25f + c0[0].y;

                    c2[0].x = (ex - c0[0].x) * 0.75f + c0[0].x;
                    c2[0].y = (ey - c0[0].y) * 0.75f + c0[0].y;
                    c2[0].z = (ez - c0[0].z) * 0.75f + c0[0].y;

                    {
                        float a8 = fz - c1[0].z;
                        c1[1].x = (fx - c1[0].x) * 0.25f + c1[0].x;
                        c1[1].y = (fy - c1[0].y) * 0.25f + c1[0].y;
                        c1[1].z = a8 * 0.25f + c1[0].y;
                        c2[1].x = (fx - c1[0].x) * 0.75f + c1[0].x;
                        c2[1].y = (fy - c1[0].y) * 0.75f + c1[0].y;
                        c2[1].z = a8 * 0.75f + c1[0].y;
                    }
                    t2 = (t1 - t0) * 0.25f + t0;
                    t3 = (t1 - t0) * 0.75f + t0;
                }
            }
            goto evaluate;
        }

        if (marker_count != 3) {
            int16_t bound = marker_count - 1;
            int16_t s2 = 0;
            int32_t idx2;

            if (bound < 1) {
                idx2 = (s2 > bound) ? bound : s2;
            } else {
                idx2 = 0;
                do {
                    float lo = *(float *)(e + 0x238 + idx2 * 4);
                    float hi = *(float *)(e + 0x23c + idx2 * 4);
                    if (lo != t && lo < t && hi != t && t < hi) break;
                    s2 = s2 + 1;
                    idx2 = s2;
                } while (idx2 < bound);
                if (s2 < 0) idx2 = 0;
            }
            {
                int32_t hi_idx = idx2 + 1;
                int16_t lo16 = (int16_t)idx2;
                int32_t span = hi_idx - lo16;

                while (span + 1 < 4) {
                    if (lo16 > 0) lo16 = lo16 - 1;
                    if (hi_idx < bound) hi_idx = hi_idx + 1;
                    span = hi_idx - lo16;
                }
                idx2 = lo16;
            }

            t0 = *(float *)(e + 0x238 + idx2 * 4);
            t1 = *(float *)(e + 0x238 + idx2 * 4 + 4);
            t2 = *(float *)(e + 0x238 + idx2 * 4 + 8);
            t3 = *(float *)(e + 0x238 + idx2 * 4 + 0xc);

            {
                int16_t *order = (int16_t *)(e + 0x22a + idx2 * 2);
                int32_t byte_off = 0;
                int32_t k;

                for (k = 0; k < 4; k++) {
                    uint8_t *m = e + order[k] * 0x6c;
                    float fx = *(float *)(m + 0x60), fy = *(float *)(m + 0x4c);
                    float fz = *(float *)(m + 0x48), fw = *(float *)(m + 100);
                    float ux = *(float *)(m + 0x68), uy = *(float *)(m + 0x6c), uz = *(float *)(m + 0x70);
                    float lx = *(float *)(m + 0x5c), ly = *(float *)(m + 0x60), lzv = *(float *)(m + 100);
                    float b0 = fx * fy - fz * fw;

                    *(float *)((uint8_t *)c0 + byte_off) = ux;
                    *(float *)((uint8_t *)c0 + byte_off + 4) = uy;
                    *(float *)((uint8_t *)c0 + byte_off + 8) = uz;
                    *(float *)((uint8_t *)c1 + byte_off) = lx;
                    *(float *)((uint8_t *)c1 + byte_off + 4) = ly;
                    *(float *)((uint8_t *)c1 + byte_off + 8) = lzv;

                    {
                        float a2 = *(float *)(m + 0x44) * *(float *)(m + 100) -
                                   lx * *(float *)(m + 0x4c);
                        float a3 = *(float *)(m + 0x48) * lx - *(float *)(m + 0x44) * *(float *)(m + 0x60);
                        *(float *)((uint8_t *)c2 + byte_off) = b0;
                        *(float *)((uint8_t *)c2 + byte_off + 4) = a2;
                        *(float *)((uint8_t *)c2 + byte_off + 8) = a3;
                    }

                    byte_off += 0xc;
                }
            }
            goto evaluate;
        }

        // marker_count == 3
        c0[0].x = *(float *)(e + 0x68); c0[0].y = *(float *)(e + 0x6c); c0[0].z = *(float *)(e + 0x70);
        {
            float ex = *(float *)(e + 0x140), ey = *(float *)(e + 0x144), ez = *(float *)(e + 0x148);
            c1[0].x = *(float *)(e + 0x5c); c1[0].y = *(float *)(e + 0x60); c1[0].z = *(float *)(e + 100);
            {
                float fx = *(float *)(e + 0x134), fy = *(float *)(e + 0x138), fz = *(float *)(e + 0x13c);
                t0 = *(float *)(e + 0x238); t1 = *(float *)(e + 0x240);

                if (seg == 0) {
                    c0[1].x = *(float *)(e + 0xd4); c0[1].y = *(float *)(e + 0xd8); c0[1].z = *(float *)(e + 0xdc);
                    c1[1].x = *(float *)(e + 200); c1[1].y = *(float *)(e + 0xcc); c1[1].z = *(float *)(e + 0xd0);
                    t2 = *(float *)(e + 0x23c);
                    c2[0].x = (ex - c0[1].x) * 0.5f + c0[1].x;
                    c2[0].y = (ey - c0[1].y) * 0.5f + c0[1].y;
                    c2[0].z = (ez - c0[1].z) * 0.5f + c0[1].y;
                    {
                        float a8 = fz - c1[1].z;
                        c2[1].x = (fx - c1[1].x) * 0.5f + c1[1].x;
                        c2[1].y = (fy - c1[1].y) * 0.5f + c1[1].y;
                        c2[1].z = a8 * 0.5f + c1[1].y;
                    }
                    t3 = (t1 - t2) * 0.5f + t2;
                    goto evaluate;
                }
                if (seg != 1) {
                    goto evaluate;
                }
                c2[0].x = *(float *)(e + 0xd4); c2[0].y = *(float *)(e + 0xd8); c2[0].z = *(float *)(e + 0xdc);
                t2 = *(float *)(e + 200); // reused as c2[1].x-equivalent below to mirror the original naming
                c0[1].x = (c2[0].x - c0[0].x) * 0.5f + c0[0].x;
                c0[1].y = (c2[0].y - c0[0].y) * 0.5f + c0[0].y;
                c0[1].z = (c2[0].z - c0[0].z) * 0.5f + c0[0].y;
                {
                    float local_54 = *(float *)(e + 0xd0);
                    float a8 = local_54 - c1[0].z;
                    c1[1].x = (t2 - c1[0].x) * 0.5f + c1[0].x;
                    c1[1].y = (*(float *)(e + 0xcc) - c1[0].y) * 0.5f + c1[0].y;
                    c1[1].z = a8 * 0.5f + c1[0].y;
                }
                t3 = (*(float *)(e + 0x23c) - t0) * 0.5f;
                t3 = t3 + t0;
            }
        }
    }

evaluate:
    // The first call writes straight into the particle's position (particle+0x2c..0x38, which
    // is exactly &out0 aliased onto *(real_point3d *)(particle+0x2c) in the original); the other
    // two produce the "up" and cross-product vectors the billboard offset below rotates.
    vector3d_cubic_interpolate((real_point3d *)(particle + 0x2c), c0, t0, t1, t2, t3, *(float *)(particle + 0x28));
    vector3d_cubic_interpolate(&out1, c1, t0, t1, t2, t3, *(float *)(particle + 0x28));
    vector3d_cubic_interpolate(&out2, c2, t0, t1, t2, t3, *(float *)(particle + 0x28));

    {
        double angle = (double)phase_rate * (double)(*(float *)(particle + 0x28)) +
                        (double)(*(float *)(particle + 8));
        double s = sin(angle);
        double c = cos(angle);
        float scale = *(float *)(particle + 0x1c);

        *(float *)(particle + 0x2c) = (float)((out1.x * s + out2.x * c) * scale) + *(float *)(particle + 0x2c);
        *(float *)(particle + 0x30) = (float)((out1.y * s + out2.y * c) * scale) + *(float *)(particle + 0x30);
        *(float *)(particle + 0x34) = (float)((out1.z * s + out2.z * c) * scale) + *(float *)(particle + 0x34);
    }
}

#if 0
Original Ghidra decompilation (0x4fde40):

```c

void FUN_004fde40(int param_1,float param_2)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  undefined4 uVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int in_EDX;
  int iVar11;
  int iVar12;
  float10 fVar13;
  float10 fVar14;
  float local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4 [6];
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74 [6];
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  short *local_44;
  int local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30 [12];
  
  iVar8 = *(short *)(in_EDX + 4) + -1;
  sVar7 = 0;
  if (iVar8 < 1) {
LAB_004fde93:
    iVar9 = (int)sVar7;
    if (iVar8 < sVar7) {
      iVar9 = iVar8;
    }
  }
  else {
    iVar9 = 0;
    do {
      fVar2 = *(float *)(in_EDX + 0x238 + iVar9 * 4);
      if ((fVar2 < *(float *)(param_1 + 0x28) != (fVar2 == *(float *)(param_1 + 0x28))) &&
         (*(float *)(param_1 + 0x28) < *(float *)(in_EDX + 0x23c + iVar9 * 4))) break;
      sVar7 = sVar7 + 1;
      iVar9 = (int)sVar7;
    } while (iVar9 < iVar8);
    if (-1 < sVar7) goto LAB_004fde93;
    iVar9 = 0;
  }
  sVar7 = (short)iVar9;
  *(short *)(param_1 + 2) = sVar7;
  iVar8 = (int)*(short *)(in_EDX + 4);
  if (iVar8 == 2) {
    local_a4[0] = *(float *)(in_EDX + 0x68);
    local_a4[1] = *(float *)(in_EDX + 0x6c);
    local_a4[2] = *(float *)(in_EDX + 0x70);
    local_80 = *(float *)(in_EDX + 0xd4);
    local_7c = *(float *)(in_EDX + 0xd8);
    local_78 = *(float *)(in_EDX + 0xdc);
    local_74[0] = *(float *)(in_EDX + 0x5c);
    local_74[1] = *(float *)(in_EDX + 0x60);
    local_74[2] = *(float *)(in_EDX + 100);
    local_50 = *(float *)(in_EDX + 200);
    local_a4[3] = (local_80 - local_a4[0]) * 0.25 + local_a4[0];
    local_4c = *(float *)(in_EDX + 0xcc);
    local_48 = *(float *)(in_EDX + 0xd0);
    local_c0 = *(float *)(in_EDX + 0x238);
    local_b4 = *(float *)(in_EDX + 0x23c);
    local_a4[4] = (local_7c - local_a4[1]) * 0.25 + local_a4[1];
    local_a4[5] = (local_78 - local_a4[2]) * 0.25 + local_a4[1];
    local_8c = (local_80 - local_a4[0]) * 0.75 + local_a4[0];
    local_88 = (local_7c - local_a4[1]) * 0.75 + local_a4[1];
    local_84 = (local_78 - local_a4[2]) * 0.75 + local_a4[1];
    local_a8 = local_48 - local_74[2];
    local_74[3] = (local_50 - local_74[0]) * 0.25 + local_74[0];
    local_74[4] = (local_4c - local_74[1]) * 0.25 + local_74[1];
    local_74[5] = local_a8 * 0.25 + local_74[1];
    local_5c = (local_50 - local_74[0]) * 0.75 + local_74[0];
    local_58 = (local_4c - local_74[1]) * 0.75 + local_74[1];
    local_54 = local_a8 * 0.75 + local_74[1];
    local_bc = (local_b4 - local_c0) * 0.25 + local_c0;
    fVar2 = (local_b4 - local_c0) * 0.75;
  }
  else {
    if (iVar8 != 3) {
      iVar8 = iVar8 + -1;
      sVar7 = 0;
      if (iVar8 < 1) {
LAB_004fdef4:
        iVar9 = (int)sVar7;
        if (iVar8 < sVar7) {
          iVar9 = iVar8;
        }
      }
      else {
        iVar9 = 0;
        do {
          fVar2 = *(float *)(in_EDX + 0x238 + iVar9 * 4);
          if ((fVar2 < *(float *)(param_1 + 0x28) != (fVar2 == *(float *)(param_1 + 0x28))) &&
             (fVar2 = *(float *)(in_EDX + 0x23c + iVar9 * 4),
             *(float *)(param_1 + 0x28) < fVar2 != (*(float *)(param_1 + 0x28) == fVar2))) break;
          sVar7 = sVar7 + 1;
          iVar9 = (int)sVar7;
        } while (iVar9 < iVar8);
        if (-1 < sVar7) goto LAB_004fdef4;
        iVar9 = 0;
      }
      iVar12 = iVar9 + 1;
      iVar10 = (int)(short)iVar12;
      sVar7 = (short)iVar9;
      iVar11 = iVar10 - sVar7;
      while (iVar11 + 1 < 4) {
        if (0 < (short)iVar9) {
          iVar9 = iVar9 + -1;
        }
        sVar7 = (short)iVar9;
        if (iVar10 < iVar8) {
          iVar12 = iVar12 + 1;
        }
        iVar10 = (int)(short)iVar12;
        iVar11 = iVar10 - sVar7;
      }
      iVar9 = (int)sVar7;
      local_c0 = *(float *)(in_EDX + 0x238 + iVar9 * 4);
      iVar8 = in_EDX + 0x238 + iVar9 * 4;
      local_bc = *(float *)(iVar8 + 4);
      local_b8 = *(float *)(iVar8 + 8);
      local_b4 = *(float *)(iVar8 + 0xc);
      local_44 = (short *)(in_EDX + 0x22a + iVar9 * 2);
      local_40 = 4;
      iVar8 = 0;
      do {
        iVar9 = *local_44 * 0x6c + in_EDX;
        fVar2 = *(float *)(iVar9 + 0x60);
        fVar3 = *(float *)(iVar9 + 0x4c);
        *(undefined4 *)((int)local_a4 + iVar8) = *(undefined4 *)(iVar9 + 0x68);
        fVar4 = *(float *)(iVar9 + 0x48);
        fVar5 = *(float *)(iVar9 + 100);
        uVar6 = *(undefined4 *)(iVar9 + 0x70);
        *(undefined4 *)((int)local_a4 + iVar8 + 4) = *(undefined4 *)(iVar9 + 0x6c);
        *(undefined4 *)((int)local_a4 + iVar8 + 8) = uVar6;
        pfVar1 = (float *)(iVar9 + 0x5c);
        local_b0 = fVar2 * fVar3 - fVar4 * fVar5;
        fVar2 = *(float *)(iVar9 + 0x44);
        *(float *)((int)local_74 + iVar8) = *pfVar1;
        fVar3 = *(float *)(iVar9 + 100);
        fVar4 = *pfVar1;
        uVar6 = *(undefined4 *)(iVar9 + 100);
        fVar5 = *(float *)(iVar9 + 0x4c);
        *(undefined4 *)((int)local_74 + iVar8 + 4) = *(undefined4 *)(iVar9 + 0x60);
        *(undefined4 *)((int)local_74 + iVar8 + 8) = uVar6;
        local_ac = fVar2 * fVar3 - fVar4 * fVar5;
        fVar2 = *(float *)(iVar9 + 0x48);
        fVar3 = *pfVar1;
        fVar4 = *(float *)(iVar9 + 0x44);
        fVar5 = *(float *)(iVar9 + 0x60);
        *(float *)((int)local_30 + iVar8) = local_b0;
        *(float *)((int)local_30 + iVar8 + 4) = local_ac;
        local_a8 = fVar2 * fVar3 - fVar4 * fVar5;
        *(float *)((int)local_30 + iVar8 + 8) = local_a8;
        local_44 = local_44 + 1;
        local_40 = local_40 + -1;
        iVar8 = iVar8 + 0xc;
      } while (local_40 != 0);
      local_40 = 0;
      goto LAB_004fe440;
    }
    local_a4[0] = *(float *)(in_EDX + 0x68);
    local_a4[1] = *(float *)(in_EDX + 0x6c);
    local_a4[2] = *(float *)(in_EDX + 0x70);
    local_80 = *(float *)(in_EDX + 0x140);
    local_7c = *(float *)(in_EDX + 0x144);
    local_78 = *(float *)(in_EDX + 0x148);
    local_74[0] = *(float *)(in_EDX + 0x5c);
    local_74[1] = *(float *)(in_EDX + 0x60);
    local_74[2] = *(float *)(in_EDX + 100);
    local_50 = *(float *)(in_EDX + 0x134);
    local_4c = *(float *)(in_EDX + 0x138);
    local_48 = *(float *)(in_EDX + 0x13c);
    local_c0 = *(float *)(in_EDX + 0x238);
    local_b4 = *(float *)(in_EDX + 0x240);
    if (sVar7 == 0) {
      local_a4[3] = *(float *)(in_EDX + 0xd4);
      local_a4[4] = *(float *)(in_EDX + 0xd8);
      local_a4[5] = *(float *)(in_EDX + 0xdc);
      local_74[3] = *(float *)(in_EDX + 200);
      local_74[4] = *(float *)(in_EDX + 0xcc);
      local_74[5] = *(float *)(in_EDX + 0xd0);
      local_bc = *(float *)(in_EDX + 0x23c);
      local_8c = (local_80 - local_a4[3]) * 0.5 + local_a4[3];
      local_88 = (local_7c - local_a4[4]) * 0.5 + local_a4[4];
      local_84 = (local_78 - local_a4[5]) * 0.5 + local_a4[4];
      local_a8 = local_48 - local_74[5];
      local_5c = (local_50 - local_74[3]) * 0.5 + local_74[3];
      local_58 = (local_4c - local_74[4]) * 0.5 + local_74[4];
      local_54 = local_a8 * 0.5 + local_74[4];
      local_b8 = (local_b4 - local_bc) * 0.5 + local_bc;
      goto LAB_004fe440;
    }
    if (sVar7 != 1) goto LAB_004fe440;
    local_8c = *(float *)(in_EDX + 0xd4);
    local_88 = *(float *)(in_EDX + 0xd8);
    local_84 = *(float *)(in_EDX + 0xdc);
    local_5c = *(float *)(in_EDX + 200);
    local_58 = *(float *)(in_EDX + 0xcc);
    local_54 = *(float *)(in_EDX + 0xd0);
    local_a4[3] = (local_8c - local_a4[0]) * 0.5 + local_a4[0];
    local_a4[4] = (local_88 - local_a4[1]) * 0.5 + local_a4[1];
    local_a4[5] = (local_84 - local_a4[2]) * 0.5 + local_a4[1];
    local_a8 = local_54 - local_74[2];
    local_74[3] = (local_5c - local_74[0]) * 0.5 + local_74[0];
    local_74[4] = (local_58 - local_74[1]) * 0.5 + local_74[1];
    local_74[5] = local_a8 * 0.5 + local_74[1];
    fVar2 = (*(float *)(in_EDX + 0x23c) - local_c0) * 0.5;
  }
  local_b8 = fVar2 + local_c0;
LAB_004fe440:
  FUN_004fcb00(param_1 + 0x2c,local_a4,local_c0,local_bc,local_b8,local_b4,
               *(undefined4 *)(param_1 + 0x28));
  FUN_004fcb00(&local_3c,local_74,local_c0,local_bc,local_b8,local_b4,
               *(undefined4 *)(param_1 + 0x28));
  FUN_004fcb00(&local_b0,local_30,local_c0,local_bc,local_b8,local_b4,
               *(undefined4 *)(param_1 + 0x28));
  fVar13 = (float10)param_2 * (float10)*(float *)(param_1 + 0x28) + (float10)*(float *)(param_1 + 8)
  ;
  fVar14 = (float10)fsin(fVar13);
  fVar13 = (float10)fcos(fVar13);
  *(float *)(param_1 + 0x2c) =
       (float)(((float10)local_3c * fVar14 + (float10)local_b0 * fVar13) *
               (float10)*(float *)(param_1 + 0x1c) + (float10)*(float *)(param_1 + 0x2c));
  *(float *)(param_1 + 0x30) =
       (float)(((float10)local_38 * fVar14 + (float10)local_ac * fVar13) *
               (float10)*(float *)(param_1 + 0x1c) + (float10)*(float *)(param_1 + 0x30));
  *(float *)(param_1 + 0x34) =
       (float)(((float10)local_34 * fVar14 + (float10)local_a8 * fVar13) *
               (float10)*(float *)(param_1 + 0x1c) + (float10)*(float *)(param_1 + 0x34));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
