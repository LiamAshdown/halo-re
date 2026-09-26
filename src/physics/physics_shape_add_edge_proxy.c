// physics_shape_add_edge_proxy  (Ghidra: FUN_00503ae0, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503ae0, size 355 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase2/results/physics_00.json: "Reads a leaf-edge's two adjacent surfaces,
//   uses FUN_0044d8e0 (a dot/angle threshold of +/-0.0001) to skip near-coplanar or
//   matching-side edges, transforms endpoints, and forwards to FUN_00503490 (edge-to-pill/quad
//   proxy builder)." out/phase4/physics_types_notes.md: "0x503c50 passes the surface index in
//   the same second slot but forces it to -1 when an object index is present" -- matches the
//   `object_index == -1 ? left_surface : -1` swap here.
// register convention: in_EAX -> edge_index, in_ECX -> bsp (ModelCollisionGeometryBSP *).
//   param_1..param_4 are Ghidra-recognized stack parameters.
//   // blam-cc: EAX -> edge_index, ECX -> bsp,
//   //           stack -> matrix, model, near_vertex, edge_dir, height_offset, margin,
//   //           object_index
// UNSURE, significantly: physics_shape_edge_to_pill_and_quad (the callee) needs a model
// pointer, a near-vertex point and an edge-direction vector that this function's own decompile
// never shows being loaded (no visible register load for any of them). This rewrite adds them
// as explicit parameters -- model, and near_vertex/edge_dir derived from the edge's start/end
// vertices -- since that is the only way to express the documented behaviour in valid C; the
// exact registers Ghidra dropped are not recovered. Likewise, vector3d_scalar_triple_product's
// call site shows only its `a` (stack) argument; `b` (EAX) is reconstructed as the right
// surface's plane normal and `c` (EDX) as the edge direction, matching the "is this a real
// silhouette edge" role the surrounding flags-vs-threshold logic implies, but this is a guess
// pending disassembly confirmation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern float vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c); // 0x44d8e0, not physics;
                                                                  // returns cross(b,a) . c
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cbe50
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void physics_shape_edge_to_pill_and_quad(
    physics_model *model, real_point3d *near_vertex, real_vector3d *edge_dir,
    float height_offset, float thickness, uint32_t object_index, int32_t surface_index,
    uint8_t surface_flags, int8_t breakable_surface_index,
    int16_t material_type); // 0x503490, this batch

// REWRITTEN from objdump 0x503ae0..0x503c3c: the draft took the start vertex and edge direction as parameters,
// but the original computes both from the BSP (vertices at bsp+0x58, edge = {start, end, ..., left_surface +0x10,
// right_surface +0x14}), takes five stack arguments, and passes the scalar triple product (left normal on the
// stack, right normal in EAX, direction in EDX); returns when it is <= -0.0001 (same plane sides) or >= 0.0001
// (opposite sides). The transformed direction is written in place; the transformed start vertex goes to a local.
// blam-cc: EAX -> edge_index, ECX -> bsp, stack -> matrix, height_offset, thickness, object_index, model
void physics_shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp,
                                   real_matrix4x3 *matrix, float height_offset, float thickness,
                                   int32_t object_index, physics_model *model)
{
    ModelCollisionGeometryBSPEdge *edge =
        &((ModelCollisionGeometryBSPEdge *)bsp->edges.pointer)[edge_index];
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPSurface *left_surface = &surfaces[edge->left_surface];
    ModelCollisionGeometryBSPSurface *right_surface = &surfaces[edge->right_surface];
    uint32_t left_plane = left_surface->plane;
    uint32_t right_plane = right_surface->plane;
    real_point3d *start;
    real_point3d *end;
    real_vector3d direction;
    real_point3d transformed_start;
    real_point3d *near_vertex;
    int32_t surface_index;

    if (left_plane == right_plane) {
        return;
    }

    start = (real_point3d *)((uint8_t *)bsp->vertices.pointer + edge->start_vertex * 0x10);
    end = (real_point3d *)((uint8_t *)bsp->vertices.pointer + edge->end_vertex * 0x10);
    direction.i = end->x - start->x;
    direction.j = end->y - start->y;
    direction.k = end->z - start->z;

    if ((left_plane & 0x7fffffffu) != (right_plane & 0x7fffffffu)) {
        uint8_t *planes = (uint8_t *)bsp->planes.pointer;
        real_vector3d *left_normal = (real_vector3d *)(planes + (left_plane & 0x7fffffffu) * 0x10);
        real_vector3d *right_normal = (real_vector3d *)(planes + (right_plane & 0x7fffffffu) * 0x10);
        real triple = vector3d_scalar_triple_product(left_normal, right_normal, &direction);
        if (((left_plane & 0x80000000u) != 0) == ((right_plane & 0x80000000u) != 0)) {
            if (triple <= -0.0001f) {
                return;
            }
        } else if (!(triple < 0.0001f)) {
            return;
        }
    }

    surface_index = -1;
    if (object_index == -1) {
        surface_index = (int32_t)edge->left_surface;
    }

    near_vertex = start;
    if (matrix != 0) {
        matrix4x3_transform_vector(&direction, &direction, matrix);
        matrix4x3_transform_point(&transformed_start, start, matrix);
        near_vertex = &transformed_start;
    }

    physics_shape_edge_to_pill_and_quad(model, near_vertex, &direction, height_offset, thickness,
                                         (uint32_t)object_index, surface_index,
                                         left_surface->flags,
                                         left_surface->breakable_surface,
                                         (int16_t)left_surface->material);
}

#if 0
Original Ghidra decompilation (0x503ae0):

void FUN_00503ae0(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  uint *puVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int in_EAX;
  int in_ECX;
  int iVar5;
  float10 fVar6;

  iVar2 = *(int *)(*(int *)(in_ECX + 0x4c) + 0x10 + in_EAX * 0x18);
  puVar1 = (uint *)(*(int *)(in_ECX + 0x40) + iVar2 * 0xc);
  uVar3 = *puVar1;
  uVar4 = *(uint *)(*(int *)(in_ECX + 0x40) +
                   *(int *)(*(int *)(in_ECX + 0x4c) + in_EAX * 0x18 + 0x14) * 0xc);
  if (uVar3 == uVar4) {
    return;
  }
  iVar5 = uVar3 * 0x10 + *(int *)(in_ECX + 0x10);
  if ((uVar3 & 0x7fffffff) != (uVar4 & 0x7fffffff)) {
    if (((*puVar1 & 0x80000000) != 0) == ((uVar4 & 0x80000000) != 0)) {
      fVar6 = (float10)vector3d_scalar_triple_product(iVar5);
      if (fVar6 <= (float10)-0.0001) {
        return;
      }
    }
    else {
      fVar6 = (float10)vector3d_scalar_triple_product(iVar5);
      if ((float10)0.0001 <= fVar6) {
        return;
      }
    }
  }
  iVar5 = -1;
  if (param_4 == -1) {
    iVar5 = iVar2;
  }
  if (param_1 != 0) {
    matrix4x3_transform_vector(param_1);
    matrix4x3_transform_point(param_1);
  }
  FUN_00503490(param_2,param_3,param_4,iVar5,(char)puVar1[2],*(undefined1 *)((int)puVar1 + 9),
               *(undefined2 *)((int)puVar1 + 10));
  return;
}
#endif
