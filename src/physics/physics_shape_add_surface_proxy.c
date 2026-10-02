// physics_shape_add_surface_proxy  (Ghidra: FUN_00503c50, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503c50, size 307 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// VERIFIED 2026-09-27 (static loop) against objdump 0x503c50..0x503d82: vertex fetch (EDI bsp), plane fetch, the
// plane transform (new normal = rotation columns, d = new_normal . translation + d * scale) and the 11-argument
// physics_shape_surface_to_polygon push order all match.
// evidence: out/phase2/results/physics_00.json: "Fetches a surface's vertex loop via
//   FUN_00501400, transforms each vertex with matrix4x3_transform_point, applies an
//   angular+linear velocity correction (13-float unaff_ESI matrix) via structure_bsp_plane_fetch_signed, then
//   forwards to FUN_005038a0." out/phase4/physics_types_notes.md: "0x503c50 passes the surface
//   index in the same second slot but forces it to -1 when an object index is present" --
//   matches the `object_index != -1 ? -1 : surface_index` swap here.
// register convention: in_EAX -> bsp (ModelCollisionGeometryBSP *), unaff_ESI -> moving_frame
//   (a nullable 13-float block: [0] a d-scale, [1..9] a 3x3 rotation, [10..12] a translation;
//   UNSURE of the exact field semantics beyond "accounts for a moving reference frame" per
//   phase-2's evidence). param_1..param_5 are Ghidra-recognized stack parameters.
//   // blam-cc: EAX -> bsp, ESI -> moving_frame,
//   //           stack -> surface_index, margin, thickness, object_index, model
// UNSURE, significantly: three call sites here show zero or partial visible arguments
// (collision_bsp_surface_get_vertices needs bsp via EDI, reconstructed from this function's own
// EAX; the plane fetch structure_bsp_plane_fetch_signed needs an output pointer, bsp and a plane index, all
// register-passed and dropped; matrix4x3_transform_point's per-vertex loop shows no arguments
// at all). This rewrite reconstructs the plane fetch and per-vertex transform as the only
// semantically sensible calls given the surrounding code (fetch surface->plane, negating it per
// its sign-bit convention; transform every collected vertex in place through the matrix), but
// the exact registers are not recovered from this file alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t collision_bsp_surface_get_vertices(ModelCollisionGeometryBSP *bsp,
                                                    int32_t surface_index,
                                                    real_point3d *out_vertices); // 0x501400, this batch
extern void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index);
                          // 0x44dad0, src/structures; blam-cc: EAX out, EDX signed_index, stack
                          // planes_owner (0x503c74..0x503c7f); copies
                                                     // bsp->planes[plane_index & 0x7fffffff],
                                                     // negated if the sign bit is set
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void physics_shape_surface_to_polygon(int16_t vertex_count, real_point3d *vertices,
                                              real_plane3d *plane, float margin, float thickness,
                                              uint32_t object_index, int32_t surface_index,
                                              uint8_t surface_flags,
                                              int8_t breakable_surface_index,
                                              int16_t material_type,
                                              physics_model *model); // 0x5038a0, this batch

// blam-cc: EAX -> bsp, ESI -> moving_frame,
//          stack -> surface_index, margin, thickness, object_index, model
void physics_shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame,
                                      int32_t surface_index, float margin, float thickness,
                                      int32_t object_index, physics_model *model)
{
    ModelCollisionGeometryBSPSurface *surface =
        &((ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer)[surface_index];
    // matches the original's 96-byte stack buffer (8 * sizeof(real_point3d)) exactly; since
    // collision_bsp_surface_get_vertices does not clamp the count it writes, a surface with
    // more than 8 edges overflows this buffer in the original binary too -- preserved as-is,
    // not "fixed", per k_maximum_physics_model_shape_vertices.
    real_point3d vertices[8];
    real_plane3d plane;
    int16_t vertex_count = collision_bsp_surface_get_vertices(bsp, surface_index, vertices);

    structure_bsp_plane_fetch_signed(&plane, bsp, (int32_t)surface->plane);

    if (moving_frame != 0) {
        int16_t i;
        for (i = 0; i < vertex_count; i++) {
            matrix4x3_transform_point(&vertices[i], &vertices[i], (real_matrix4x3 *)moving_frame);
        }
        {
            float new_i = plane.normal.j * moving_frame[4] + plane.normal.k * moving_frame[7] +
                          plane.normal.i * moving_frame[1];
            float new_j = plane.normal.j * moving_frame[5] + plane.normal.k * moving_frame[8] +
                          plane.normal.i * moving_frame[2];
            float new_k = plane.normal.k * moving_frame[9] + plane.normal.i * moving_frame[3] +
                          plane.normal.j * moving_frame[6];
            plane.d = new_i * moving_frame[10] + plane.d * moving_frame[0] +
                      new_j * moving_frame[11] + new_k * moving_frame[12];
            plane.normal.i = new_i;
            plane.normal.j = new_j;
            plane.normal.k = new_k;
        }
    }

    {
        int32_t out_surface_index = surface_index;
        if (object_index != -1) {
            out_surface_index = -1;
        }
        physics_shape_surface_to_polygon(vertex_count, vertices, &plane, margin, thickness,
                                          (uint32_t)object_index, out_surface_index,
                                          surface->flags, surface->breakable_surface,
                                          (int16_t)surface->material, model);
    }
}

#if 0
Original Ghidra decompilation (0x503c50):

void FUN_00503c50(int param_1,undefined4 param_2,undefined4 param_3,int param_4,undefined4 param_5)

{
  int iVar1;
  float fVar2;
  float fVar3;
  int in_EAX;
  uint uVar4;
  float *unaff_ESI;
  uint uVar5;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  undefined1 local_60 [96];

  iVar1 = *(int *)(in_EAX + 0x40) + param_1 * 0xc;
  uVar4 = FUN_00501400(param_1,local_60);
  FUN_0044dad0();
  if (unaff_ESI != (float *)0x0) {
    if (0 < (short)uVar4) {
      uVar5 = uVar4 & 0xffff;
      do {
        matrix4x3_transform_point();
        uVar5 = uVar5 - 1;
      } while (uVar5 != 0);
    }
    fVar3 = local_6c * unaff_ESI[4] + local_68 * unaff_ESI[7] + local_70 * unaff_ESI[1];
    fVar2 = local_6c * unaff_ESI[5] + local_68 * unaff_ESI[8] + local_70 * unaff_ESI[2];
    local_68 = local_68 * unaff_ESI[9] + local_70 * unaff_ESI[3] + local_6c * unaff_ESI[6];
    local_64 = fVar3 * unaff_ESI[10] +
               local_64 * *unaff_ESI + fVar2 * unaff_ESI[0xb] + local_68 * unaff_ESI[0xc];
    local_70 = fVar3;
    local_6c = fVar2;
  }
  if (param_4 != -1) {
    param_1 = -1;
  }
  FUN_005038a0(uVar4,local_60,&local_70,param_2,param_3,param_4,param_1,*(undefined1 *)(iVar1 + 8),
               *(undefined1 *)(iVar1 + 9),*(undefined2 *)(iVar1 + 10),param_5);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
