// physics_shape_polygon_test_ray  (Ghidra: FUN_005048d0, still unnamed there; name from
// out/phase2/results/physics_00.json: "Clips a segment's plane-crossing interval (Liang-Barsky
// style, local_2c/local_28 min/max) then walks the polygon's 2D edge loop (in_ECX+0x28, stride
// 8) refining the interval per edge -- the ray counterpart of FUN_00504120.")
// address 0x5048d0, size 731 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (step 1: checked against objdump -d 0x5048d0..0x504baa; x87 compare semantics now exact for NaN)
// evidence: physics_model_shape.plane_i/j/k/d (0x0c..0x18), .thickness (0x1c),
//   .projection_axis/sign (0x20/0x22), .vertex_count (0x24) and .vertices[8][2] (0x28) match
//   in_ECX+0xc.. exactly, the same fields physics_shape_polygon_test_point (0x504120, this
//   module) reads off the same struct; that sibling's own for-loop (index + wraparound-to-0,
//   cross-product-sign rejection) is the direct model for this function's per-edge test, just
//   parameterized along a swept segment [t_min, t_max] instead of a single point; the
//   "candidate = ...; if (already-better) goto no-op-reassignment" shape in every per-edge
//   branch collapses to the standard Cyrus-Beck update (denom >= 0 tightens t_max, denom < 0
//   tightens t_min), confirmed by collision_bsp_surface_clip_line_2d (0x5017f0, this module)
//   implementing the identical algorithm against a BSP edge loop instead of a plain vertex array.
// register convention: in_EAX -> origin, in_ECX -> shape (physics_model_shape *), in_EDX -> delta.
//   param_1/param_2 are Ghidra-recognized stack parameters (out_t, out_plane).
//   // blam-cc: EAX -> origin, ECX -> shape, EDX -> delta, stack -> out_t, out_plane
// UNSURE: one store this function makes before the edge loop (`local_24[0] = local_c[axis_i]`)
// is never read again anywhere in the decompile -- every per-edge use of the delta-on-plane 2D
// vector reads straight out of `local_c` by axis index instead. This rewrite omits that dead
// store (delta_proj is read directly off delta_on_plane, matching what the loop actually uses).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)

// Tests a world-space ray (origin, delta) against shape, the ray counterpart of
// physics_shape_polygon_test_point. Clips the ray's plane-crossing interval [t_min, t_max]
// against the polygon's thickness slab, then narrows it further against each boundary edge of
// the polygon's 2D projection (Cyrus-Beck line clipping); reports the entry fraction t_min when
// the clipped interval is still non-empty after every edge.
// blam-cc: EAX -> origin, ECX -> shape, EDX -> delta, stack -> out_t, out_plane
uint8_t physics_shape_polygon_test_ray(real_point3d *origin, physics_model_shape *shape,
                                         real_vector3d *delta, float *out_t, real_plane3d *out_plane)
{
    float dist = origin->x * shape->plane_i + shape->plane_k * origin->z +
                 shape->plane_j * origin->y - shape->plane_d;
    float dot_delta_normal =
        delta->i * shape->plane_i + shape->plane_j * delta->j + shape->plane_k * delta->k;
    float t_min = 0.0f;
    float t_max = 1.0f;
    real_point3d point_on_plane;
    real_vector3d delta_on_plane;
    projection_axis_pair proj_axes;
    float point_proj[2];
    float delta_proj[2];
    int32_t vertex_count;
    int32_t i;

    if (dot_delta_normal == 0.0f) {
        // 0x5049dc..0x5049f9: out only when dist < 0 or dist >= thickness (a NaN stays in)
        if (dist < 0.0f || dist >= shape->thickness) {
            return 0;
        }
    } else {
        float t_a = -(dist * (1.0f / dot_delta_normal));
        float t_b = -((dist - shape->thickness) * (1.0f / dot_delta_normal));

        // 0x504958: only a strictly positive dot takes the entering-from-front branch
        if (dot_delta_normal > 0.0f) {
            if (0.0f < t_a) {
                t_min = t_a;
            }
            if (t_b < 1.0f) {
                t_max = t_b;
            }
        } else {
            if (0.0f < t_b) {
                t_min = t_b;
            }
            if (t_a < 1.0f) {
                t_max = t_a;
            }
        }
        if (t_min > t_max) {
            return 0;
        }
    }

    dist = -dist;
    point_on_plane.x = dist * shape->plane_i + origin->x;
    point_on_plane.y = dist * shape->plane_j + origin->y;
    point_on_plane.z = dist * shape->plane_k + origin->z;

    dot_delta_normal = -dot_delta_normal;
    delta_on_plane.i = dot_delta_normal * shape->plane_i + delta->i;
    delta_on_plane.j = dot_delta_normal * shape->plane_j + delta->j;
    delta_on_plane.k = dot_delta_normal * shape->plane_k + delta->k;

    proj_axes = k_projection_axes[shape->projection_axis * 2 + shape->projection_sign];
    point_proj[0] = ((float *)&point_on_plane)[proj_axes.i];
    point_proj[1] = ((float *)&point_on_plane)[proj_axes.j];
    delta_proj[0] = ((float *)&delta_on_plane)[proj_axes.i];
    delta_proj[1] = ((float *)&delta_on_plane)[proj_axes.j];

    vertex_count = shape->vertex_count;
    for (i = 0; i < vertex_count; i++) {
        int32_t next = (i + 1 < vertex_count) ? (i + 1) : 0;
        float edge_x = shape->vertices[next][0] - shape->vertices[i][0];
        float edge_y = shape->vertices[next][1] - shape->vertices[i][1];
        float denom = edge_y * delta_proj[0] - delta_proj[1] * edge_x;
        float numer = (point_proj[1] - shape->vertices[i][1]) * edge_x -
                      edge_y * (point_proj[0] - shape->vertices[i][0]);

        if (denom == 0.0f) {
            if (numer < 0.0f) {
                return 0;
            }
        } else {
            float t = numer / denom;

            // 0x504b0f: only a strictly negative denominator raises t_min
            if (denom < 0.0f) {
                if (t > t_min) {
                    t_min = t;
                }
            } else {
                if (t < t_max) {
                    t_max = t;
                }
            }
            if (t_min > t_max) {
                return 0;
            }
        }
    }

    *out_t = t_min;
    out_plane->normal.i = shape->plane_i;
    out_plane->normal.j = shape->plane_j;
    out_plane->normal.k = shape->plane_k;
    out_plane->d = shape->plane_d + shape->thickness;
    return 1;
}

#if 0
Original Ghidra decompilation (0x5048d0):

uint FUN_005048d0(float *param_1,undefined4 *param_2)

{
  float fVar1;
  int iVar2;
  undefined4 uVar3;
  bool bVar4;
  float fVar5;
  float fVar6;
  float *in_EAX;
  uint uVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  int in_ECX;
  float *in_EDX;
  float *pfVar11;
  ushort uVar12;
  float local_2c;
  float local_28;
  float local_24 [4];
  float local_14;
  float local_10;
  float local_c [3];

  local_2c = 0.0;
  local_28 = 1.0;
  fVar5 = (*in_EAX * *(float *)(in_ECX + 0xc) +
          *(float *)(in_ECX + 0x14) * in_EAX[2] + *(float *)(in_ECX + 0x10) * in_EAX[1]) -
          *(float *)(in_ECX + 0x18);
  local_10 = *in_EDX * *(float *)(in_ECX + 0xc) +
             *(float *)(in_ECX + 0x10) * in_EDX[1] + *(float *)(in_ECX + 0x14) * in_EDX[2];
  if (local_10 == 0.0) {
    uVar7 = (uint)(ushort)((ushort)(fVar5 < 0.0) << 8 | (ushort)NAN(fVar5) << 10 |
                          (ushort)(fVar5 == 0.0) << 0xe);
    if (fVar5 >= 0.0) {
      fVar1 = *(float *)(in_ECX + 0x1c);
      uVar7 = (uint)(ushort)((ushort)(fVar5 < fVar1) << 8 | (ushort)(NAN(fVar5) || NAN(fVar1)) << 10
                            | (ushort)(fVar5 == fVar1) << 0xe);
      if (fVar5 < fVar1) goto LAB_005049fb;
    }
  }
  else {
    local_24[3] = -(fVar5 * (1.0 / local_10));
    fVar1 = -((fVar5 - *(float *)(in_ECX + 0x1c)) * (1.0 / local_10));
    if (local_10 <= 0.0) {
      if (0.0 < fVar1) {
        local_2c = fVar1;
      }
      if (local_24[3] < 1.0) {
        local_28 = local_24[3];
      }
    }
    else {
      if (0.0 < local_24[3]) {
        local_2c = local_24[3];
      }
      if (fVar1 < 1.0) {
        local_28 = fVar1;
      }
    }
    uVar7 = (uint)(ushort)((ushort)(local_2c < local_28) << 8 |
                           (ushort)(NAN(local_2c) || NAN(local_28)) << 10 |
                          (ushort)(local_2c == local_28) << 0xe);
    if (local_2c < local_28 || (local_2c == local_28) != 0) {
LAB_005049fb:
      fVar5 = -fVar5;
      local_24[0] = fVar5 * *(float *)(in_ECX + 0xc) + *in_EAX;
      local_24[1] = fVar5 * *(float *)(in_ECX + 0x10) + in_EAX[1];
      local_24[2] = fVar5 * *(float *)(in_ECX + 0x14) + in_EAX[2];
      local_10 = -local_10;
      local_c[0] = local_10 * *(float *)(in_ECX + 0xc) + *in_EDX;
      local_c[1] = local_10 * *(float *)(in_ECX + 0x10) + in_EDX[1];
      iVar8 = ((uint)*(byte *)(in_ECX + 0x22) + *(short *)(in_ECX + 0x20) * 2) * 4;
      local_c[2] = local_10 * *(float *)(in_ECX + 0x14) + in_EDX[2];
      iVar2 = *(int *)(in_ECX + 0x24);
      local_24[3] = local_24[*(short *)(&DAT_0065c29c + iVar8)];
      local_14 = local_24[*(short *)(&DAT_0065c29e + iVar8)];
      iVar9 = ((uint)*(byte *)(in_ECX + 0x22) + *(short *)(in_ECX + 0x20) * 2) * 4;
      local_24[0] = local_c[*(short *)(&DAT_0065c29c + iVar9)];
      if (0 < iVar2) {
        pfVar11 = (float *)(in_ECX + 0x28);
        uVar7 = 1;
        do {
          uVar10 = (iVar2 <= (int)uVar7) - 1 & uVar7;
          fVar5 = *(float *)(in_ECX + 0x28 + uVar10 * 8) - *pfVar11;
          fVar6 = *(float *)(in_ECX + 0x2c + uVar10 * 8) - pfVar11[1];
          fVar1 = fVar6 * local_c[*(short *)(&DAT_0065c29c + iVar9)] -
                  local_c[*(short *)(&DAT_0065c29e + iVar9)] * fVar5;
          fVar5 = (local_24[*(short *)(&DAT_0065c29e + iVar8)] - pfVar11[1]) * fVar5 -
                  fVar6 * (local_24[*(short *)(&DAT_0065c29c + iVar8)] - *pfVar11);
          if (fVar1 == 0.0) {
            uVar12 = (ushort)(fVar5 < 0.0) << 8 | (ushort)NAN(fVar5) << 10 |
                     (ushort)(fVar5 == 0.0) << 0xe;
            if (fVar5 < 0.0) goto LAB_00504b57;
          }
          else {
            fVar5 = fVar5 / fVar1;
            if (0.0 <= fVar1) {
              fVar1 = local_2c;
              fVar6 = fVar5;
              if (local_28 <= fVar5) goto LAB_00504b46;
            }
            else {
              fVar1 = fVar5;
              fVar6 = local_28;
              if (fVar5 <= local_2c) {
LAB_00504b46:
                fVar1 = local_2c;
                fVar6 = local_28;
              }
            }
            local_28 = fVar6;
            local_2c = fVar1;
            uVar12 = (ushort)(local_2c < local_28) << 8 |
                     (ushort)(NAN(local_2c) || NAN(local_28)) << 10 |
                     (ushort)(local_2c == local_28) << 0xe;
            if (local_2c >= local_28 && (local_2c == local_28) == 0) {
LAB_00504b57:
              return (uint)uVar12;
            }
          }
          pfVar11 = pfVar11 + 2;
          bVar4 = (int)uVar7 < iVar2;
          uVar7 = uVar7 + 1;
        } while (bVar4);
      }
      *param_1 = local_2c;
      *param_2 = *(undefined4 *)(in_ECX + 0xc);
      param_2[1] = *(undefined4 *)(in_ECX + 0x10);
      uVar3 = *(undefined4 *)(in_ECX + 0x14);
      param_2[2] = uVar3;
      param_2[3] = *(float *)(in_ECX + 0x18) + *(float *)(in_ECX + 0x1c);
      return CONCAT31((int3)((uint)uVar3 >> 8),1);
    }
  }
  return uVar7;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
