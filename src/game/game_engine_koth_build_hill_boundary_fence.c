// game_engine_koth_build_hill_boundary_fence  (Ghidra: FUN_0046a670; named per its summary)
// address 0x46a670, size 1001 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Generates and submits a fence-like quad mesh (with
//   per-edge normals and UVs) along the starting-location boundary polygon"); reuses
//   game_engine_koth_build_hill_boundary.c's boundary point array (0x006b0f54, this batch) and
//   calls game_engine_koth_submit_hill_marker_geometry (0x46b2f0, this batch) once per edge,
//   passing the vertex-source register argument that function reads as `in_EAX` -- worked out
//   here by stack-offset arithmetic on Ghidra's own local names (local_118 is the record base;
//   every other local_XX name's offset from local_118, divided by 4, gives its float index) to
//   be four 17-float corner records: [0..2] position, [3..5] shared face normal, [6..11] always
//   zero, [12] U, [13] V (1.0 at the two ground corners, 0.2 at the two top corners), [14..16]
//   never written. Corners 0/1 share the current boundary point's (x,y) (0 at ground, 1 raised
//   0.8), corners 2/3 share the next point's (x,y) (2 raised 0.8, 3 at ground) -- a vertical
//   fence quad between consecutive boundary points. floor(math module, not yet
//   rewritten) is called once per total perimeter length; its result is only ever used as a
//   reciprocal-of-reciprocal here, so it is very likely a period/tile-count helper for the U
//   coordinate, but its own semantics are not recovered.
// register convention: no parameters.
// UNSURE: FUN_00623e40's true signature/role; offsets 6..11 and 14..16 of each corner record
//   (color/tangent/padding, never written here).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "fn_game.h"

extern Globals *global_globals;                    // 0x00746fa0
extern int32_t king_starting_location_count;        // 0x006b0f50
extern real_point3d king_hill_boundary_points[12];  // 0x006b0f54 (this batch)

extern double sqrt(double x); // x87 FSQRT
extern double fabs(double x); // ABS is a single x87 FABS instruction
extern double floor(double x); // CRT floor (0x623e40: SSE2-dispatched; its x87 path reports _FpCodeFloor 11)


// koth_fence_corner (the 17-float per-corner vertex record; four of them -- a ground/top pair at
// the current point, then a top/ground pair at the next point -- make one fence quad) now lives
// in types/game.h, folded there by the phase-4 review.

// Builds one vertical "fence" quad per boundary edge, with a shared outward face normal per
// edge and running U/fixed V texture coordinates, submitting each quad via
// game_engine_koth_submit_hill_marker_geometry.
void game_engine_koth_build_hill_boundary_fence(void)
{
    uint32_t hill_shader_tag;
    int32_t count = king_starting_location_count;
    float total_length = 0.0f;
    double length_period;
    float running_length = 0.0f;
    float previous_u = 0.0f;
    int32_t i;

    hill_shader_tag = *(uint32_t *)((uint8_t *)global_globals->multiplayer_information.pointer + 0x38);

    if (count > 0) {
        for (i = 0; i < count; i++) {
            real_point3d *cur = &king_hill_boundary_points[i];
            real_point3d *nxt = &king_hill_boundary_points[(i + 1 == count) ? 0 : i + 1];
            float dx = nxt->x - cur->x, dy = nxt->y - cur->y, dz = nxt->z - cur->z;
            total_length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) + total_length;
        }
    }

    length_period = floor((double)(total_length + 0.5f));

    if (count > 0) {
        float inv_scale = (float)(1.0 / length_period);

        for (i = 0; i < count; i++) {
            real_point3d *cur = &king_hill_boundary_points[i];
            real_point3d *nxt = &king_hill_boundary_points[(i + 1 == count) ? 0 : i + 1];
            float z_a = cur->z + 0.8f;
            float z_b = nxt->z + 0.8f;
            float nx, ny, nz, nlen;
            float u_end;
            koth_fence_corner quad[4];
            int32_t c;

            for (c = 0; c < 4; c++) {
                int32_t j;
                for (j = 0; j < 0x11; j++) {
                    ((float *)&quad[c])[j] = 0.0f;
                }
            }

            running_length = (float)sqrt((double)((nxt->x - cur->x) * (nxt->x - cur->x) +
                                    (nxt->y - cur->y) * (nxt->y - cur->y) +
                                    (nxt->z - cur->z) * (nxt->z - cur->z))) + running_length;
            u_end = (float)(length_period / (double)total_length) * running_length;

            // Cross product of (edge vector, vertical rise) simplifies (see header derivation)
            // to an outward horizontal normal, scaled by the 0.8 rise; renormalized below.
            nx = -(nxt->y - cur->y) * (z_a - cur->z);
            ny = (nxt->x - cur->x) * (z_a - cur->z);
            nz = 0.0f;
            nlen = (float)sqrt((double)(nx * nx + ny * ny + nz * nz));
            if (fabs((double)nlen) >= 0.0001) {
                float inv = 1.0f / nlen;
                nx *= inv; ny *= inv; nz *= inv;
            }

            quad[0].position[0] = cur->x; quad[0].position[1] = cur->y; quad[0].position[2] = cur->z;
            quad[1].position[0] = cur->x; quad[1].position[1] = cur->y; quad[1].position[2] = z_a;
            quad[2].position[0] = nxt->x; quad[2].position[1] = nxt->y; quad[2].position[2] = z_b;
            quad[3].position[0] = nxt->x; quad[3].position[1] = nxt->y; quad[3].position[2] = nxt->z;

            for (c = 0; c < 4; c++) {
                quad[c].normal[0] = nx; quad[c].normal[1] = ny; quad[c].normal[2] = nz;
            }

            quad[0].u = previous_u * inv_scale; quad[0].v = 1.0f;
            quad[1].u = previous_u * inv_scale; quad[1].v = 0.2f;
            quad[2].u = u_end * inv_scale;      quad[2].v = 0.2f;
            quad[3].u = u_end * inv_scale;      quad[3].v = 1.0f;

            {
                float length_period_f = (float)length_period;
                game_engine_koth_submit_hill_marker_geometry(hill_shader_tag, (uint32_t *)0, (uint32_t *)0,
                    *(uint32_t *)&length_period_f, 0x3f800000, (float *)quad);
            }

            previous_u = u_end;
        }
    }
}

#if 0
Original Ghidra decompilation (0x46a670), from tools/pack.py 0x46a670:

void FUN_0046a670(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  undefined4 uVar8;
  float10 fVar9;
  uint uVar10;
  uint uVar11;
  int iVar12;
  uint uVar13;
  uint uVar14;
  float *pfVar15;
  float *pfVar16;
  float10 fVar17;
  float local_168;
  uint local_150;
  float local_14c;
  float local_118 [4];
  float local_108;
  float local_104;
  float local_e8;
  undefined4 local_e4;
  float local_d4;
  float local_d0;
  float local_cc;
  float local_c8;
  float local_c4;
  float local_c0;
  float local_a4;
  undefined4 local_a0;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_60;
  undefined4 local_5c;
  float local_4c;
  undefined4 local_48;
  undefined4 local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_1c;
  undefined4 local_18;

  uVar10 = DAT_006b0f50;
  fVar1 = 0.0;
  local_168 = 0.0;
  uVar8 = *(undefined4 *)(*(int *)(DAT_00746fa0 + 0x168) + 0x38);
  if (0 < (int)DAT_006b0f50) {
    uVar13 = 1;
    pfVar15 = (float *)&DAT_006b0f5c;
    uVar14 = DAT_006b0f50;
    do {
      uVar11 = (uVar13 == DAT_006b0f50) - 1 & uVar13;
      uVar13 = uVar13 + 1;
      uVar14 = uVar14 - 1;
      fVar1 = SQRT(((float)(&DAT_006b0f54)[uVar11 * 3] - pfVar15[-2]) *
                   ((float)(&DAT_006b0f54)[uVar11 * 3] - pfVar15[-2]) +
                   ((float)(&DAT_006b0f58)[uVar11 * 3] - pfVar15[-1]) *
                   ((float)(&DAT_006b0f58)[uVar11 * 3] - pfVar15[-1]) +
                   ((float)(&DAT_006b0f5c)[uVar11 * 3] - *pfVar15) *
                   ((float)(&DAT_006b0f5c)[uVar11 * 3] - *pfVar15)) + fVar1;
      pfVar15 = pfVar15 + 3;
      local_168 = fVar1;
    } while (uVar14 != 0);
  }
  fVar17 = (float10)FUN_00623e40((double)(local_168 + 0.5));
  local_14c = 0.0;
  fVar1 = (float)((float10)1.0 / fVar17);
  fVar9 = (float10)local_168;
  local_168 = 0.0;
  if (0 < (int)uVar10) {
    uVar14 = 1;
    pfVar15 = (float *)&DAT_006b0f54;
    local_150 = uVar10;
    do {
      uVar13 = (uVar14 == uVar10) - 1 & uVar14;
      fVar2 = (float)(&DAT_006b0f54)[uVar13 * 3];
      fVar3 = *pfVar15;
      fVar4 = (float)(&DAT_006b0f58)[uVar13 * 3];
      fVar5 = pfVar15[1];
      fVar6 = (float)(&DAT_006b0f5c)[uVar13 * 3];
      fVar7 = pfVar15[2];
      pfVar16 = local_118;
      for (iVar12 = 0x44; iVar12 != 0; iVar12 = iVar12 + -1) {
        *pfVar16 = 0.0;
        pfVar16 = pfVar16 + 1;
      }
      local_d4 = *pfVar15;
      local_d0 = pfVar15[1];
      local_118[0] = *pfVar15;
      local_118[1] = pfVar15[1];
      local_118[2] = pfVar15[2];
      uVar13 = (uVar14 == uVar10) - 1 & uVar14;
      local_4c = (float)(&DAT_006b0f54)[uVar13 * 3];
      local_48 = (&DAT_006b0f58)[uVar13 * 3];
      local_44 = (&DAT_006b0f5c)[uVar13 * 3];
      local_90 = (float)(&DAT_006b0f54)[uVar13 * 3];
      local_8c = (float)(&DAT_006b0f58)[uVar13 * 3];
      local_168 = SQRT((fVar2 - fVar3) * (fVar2 - fVar3) +
                       (fVar4 - fVar5) * (fVar4 - fVar5) + (fVar6 - fVar7) * (fVar6 - fVar7)) +
                  local_168;
      fVar2 = (float)((float10)1.0 / (((float10)1.0 / fVar17) * fVar9)) * local_168;
      local_cc = pfVar15[2] + 0.8;
      local_88 = (float)(&DAT_006b0f5c)[uVar13 * 3] + 0.8;
      local_118[3] = (local_88 - local_cc) * (local_d0 - local_118[1]) -
                     (local_8c - local_d0) * (local_cc - local_118[2]);
      local_108 = (local_90 - local_d4) * (local_cc - local_118[2]) -
                  (local_88 - local_cc) * (local_d4 - local_118[0]);
      local_104 = (local_8c - local_d0) * (local_d4 - local_118[0]) -
                  (local_90 - local_d4) * (local_d0 - local_118[1]);
      fVar3 = SQRT(local_118[3] * local_118[3] + local_108 * local_108 + local_104 * local_104);
      if (0.0001 <= ABS(fVar3)) {
        fVar3 = 1.0 / fVar3;
        local_118[3] = local_118[3] * fVar3;
        local_108 = local_108 * fVar3;
        local_104 = local_104 * fVar3;
      }
      local_e8 = local_14c * fVar1;
      local_60 = fVar2 * fVar1;
      local_e4 = 0x3f800000;
      local_a0 = 0x3e4ccccd;
      local_5c = 0x3e4ccccd;
      local_18 = 0x3f800000;
      local_c8 = local_118[3];
      local_c4 = local_108;
      local_c0 = local_104;
      local_a4 = local_e8;
      local_84 = local_118[3];
      local_80 = local_108;
      local_7c = local_104;
      local_40 = local_118[3];
      local_3c = local_108;
      local_38 = local_104;
      local_1c = local_60;
      FUN_0046b2f0(uVar8,0,0,1.0 / fVar1,0x3f800000);
      pfVar15 = pfVar15 + 3;
      uVar14 = uVar14 + 1;
      local_150 = local_150 - 1;
      local_14c = fVar2;
    } while (local_150 != 0);
  }
  return;
}
#endif
