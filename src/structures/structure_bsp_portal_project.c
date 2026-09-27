// structure_bsp_portal_project  (Ghidra: FUN_00554850, still unnamed there)
// address 0x554850, size 354 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: full stack-frame reconstruction from disassembly (objdump -d -M intel bin/halo.exe,
//   0x554850..0x5549b1). The prologue is `sub esp,0xc04` then one `push esi`, so arg_n sits at
//   esp+0xc08+4n; three more registers are pushed only on the non-early-return path, which is why
//   the same argument is read at two different displacements (e.g. arg3 at +0xc14 then +0xc40).
//   Corrections over the previous rewrite of this file, all forced by the disassembly:
//     - EDX is a fourth register parameter (`mov esi,edx` is the first thing after the push): the
//       SOURCE vertex array, a real_point3d[vertex_count]. The previous rewrite had no such
//       parameter and instead invented `vertices = camera + 0x10`.
//     - `camera + 0x10` is the matrix4x3, i.e. the third (stack) argument of
//       matrix4x3_transform_point, whose real convention is EAX -> out, EDX -> point, stack -> m
//       (src/math/matrix4x3_transform_point.c). The loop transforms vertex i from the source array
//       into slot i of the local buffer (`lea edi,[esp+0x14]` / `sub edi,esi` sets up
//       destination = local + (source - source_base), then `esi += 0xc` per iteration).
//     - The clip then runs in place on that local buffer (polygon3d_clip_to_plane gets the same
//       pointer as both source and destination), and the projection loop reads it back.
//   Constants read out of the image: 0x00672c00 is the double 0.1 (the "camera is on the plane"
//   epsilon), 0x00672ac0 is 0.0, 0x00672ba8 is -1.0 (the perspective divide numerator).
//   Both call sites (0x5536ad in structure_bsp_mirror_query and 0x554a0b in
//   structure_bsp_portal_test_and_project) set up EAX = &collision_bsp->planes[plane_index],
//   ECX = 0x007c3114 (the render camera position), EDX = the polygon's vertex pointer, and push
//   0x007c3168 (the camera's transform context), the vertex count, the winding and the out
//   polygon -- which is what pins arg1..arg4.
// register convention: EAX -> plane, ECX -> camera_ref, EDX -> vertices. Stack: camera (its +0x10
//   is the real_matrix4x3), vertex_count, winding (+1/-1), out.
//   // blam-cc: EAX -> plane, ECX -> camera_ref, EDX -> vertices, stack -> the rest
// UNSURE: camera_ref (ECX) is the render camera block at 0x007c3114 at both call sites; this
//   function reads only its first three floats (the position) and the byte 0x24 further on, which
//   types/structures.h records as "+0x24 relative to the camera flips the winding". Whether that
//   byte belongs to the same record or to the next global is not settled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

// math module. blam-cc: EAX -> out, EDX -> point, stack -> m
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
                                       real_matrix4x3 *m); // 0x4cbde0
extern int16_t polygon3d_clip_to_plane(int16_t count, real_point3d *in, real_plane3d *plane, int16_t max_count, real_point3d *out, uint8_t *clipped_flag, real epsilon, char keep_coplanar); // 0x4cb380
extern real_plane3d near_clip_plane;     // 0x0065e64c
extern double k_plane_side_epsilon;      // 0x00672c00, a QWORD in .rdata: the double 0.1
extern float k_projection_numerator;     // 0x00672ba8 == -1.0

// Transforms a portal (or mirror) polygon into view space, clips it against the near plane and
// perspective-projects the survivors into `out`. Returns 2 when the camera sits within 0.1 of the
// polygon's plane (the caller treats that as "do not clip at all"), 1 when the camera is behind
// the plane, and otherwise whether the projected polygon degenerated to fewer than 3 points.
// blam-cc: EAX -> plane, ECX -> camera_ref, EDX -> vertices, stack -> the rest
uint8_t structure_bsp_portal_project(real_plane3d *plane, void *camera_ref, real_point3d *vertices,
                                      void *camera, uint32_t vertex_count, int16_t winding,
                                      polygon2d *out)
{
    real_point3d *camera_position = (real_point3d *)camera_ref;
    real_point3d clipped[0x100];
    float side;
    int16_t clipped_count;
    int16_t i, stop, step;
    int16_t written;
    uint32_t n;

    out->point_count = 0;

    side = (camera_position->x * plane->normal.i + camera_position->y * plane->normal.j +
            camera_position->z * plane->normal.k - plane->d) * (float)(int32_t)winding;
    if (*((int8_t *)camera_ref + 0x24) != 0) {
        winding = -winding;   // the original negates the stack slot, read back at 0x55492d
    }
    if ((side < 0.0f ? -side : side) < k_plane_side_epsilon) {
        return 2;             // camera effectively on the plane: caller skips clipping entirely
    }
    if (side <= 0.0f) {
        return 1;             // camera behind the plane: fully culled
    }

    for (n = 0; n < (vertex_count & 0xffff); n = n + 1) {
        matrix4x3_transform_point(&clipped[n], &vertices[n],
                                  (real_matrix4x3 *)((uint8_t *)camera + 0x10));
    }

    clipped_count = polygon3d_clip_to_plane(vertex_count, clipped, &near_clip_plane, 0x100,
                                            clipped, 0, 9.99999975e-05f, 1); // float bits 0x38d1b717
    out->point_count = clipped_count;

    if (winding == 1) {
        i = 0; stop = clipped_count; step = 1;
    } else {
        i = (int16_t)(clipped_count - 1); stop = -1; step = -1;
    }
    written = 0;
    while (i != stop) {
        float inverse_z = k_projection_numerator / clipped[i].z;
        out->points[written].x = inverse_z * clipped[i].x;
        out->points[written].y = inverse_z * clipped[i].y;
        i = (int16_t)(i + step);
        written = (int16_t)(written + 1);
    }
    return (uint8_t)(out->point_count < 3);
}

#if 0
Original Ghidra decompilation (0x554850):

undefined1 FUN_00554850(int param_1,uint param_2,short param_3,short *param_4)

{
  float fVar1;
  short sVar2;
  short sVar3;
  float *in_EAX;
  int iVar4;
  short sVar5;
  float *in_ECX;
  uint uVar6;
  float afStackY_60c00 [98291];
  float local_c00 [768];

  *param_4 = 0;
  fVar1 = ((*in_ECX * *in_EAX + in_ECX[1] * in_EAX[1] + in_ECX[2] * in_EAX[2]) - in_EAX[3]) *
          (float)(int)param_3;
  if (*(char *)(in_ECX + 9) != '\0') {
    param_3 = -param_3;
  }
  if (ABS(fVar1) < 0.1) {
    return 2;
  }
  if (0.0 < fVar1) {
    if (0 < (short)param_2) {
      uVar6 = param_2 & 0xffff;
      do {
        matrix4x3_transform_point(param_1 + 0x10);
        uVar6 = uVar6 - 1;
      } while (uVar6 != 0);
    }
    sVar3 = polygon3d_clip_to_plane(param_2,local_c00,&DAT_0065e64c,0x100,local_c00,0,0x38d1b717,1);
    *param_4 = sVar3;
    if (param_3 == 1) {
      sVar5 = 0;
    }
    else {
      sVar5 = sVar3 + -1;
      sVar3 = -1;
    }
    sVar2 = 0;
    for (; sVar5 != sVar3; sVar5 = sVar5 + param_3) {
      iVar4 = (int)sVar5;
      fVar1 = local_c00[iVar4 * 3 + 2];
      *(float *)(param_4 + sVar2 * 4 + 2) = (-1.0 / fVar1) * local_c00[iVar4 * 3];
      *(float *)(param_4 + sVar2 * 4 + 4) = (-1.0 / fVar1) * local_c00[iVar4 * 3 + 1];
      sVar2 = sVar2 + 1;
    }
    return *param_4 < 3;
  }
  return true;
}
#endif
