// physics_shape_build_proxies_from_query  (Ghidra: FUN_00503d90, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503d90, size 283 bytes
// name confidence: 0.35   rewrite confidence: 0.30 (raised from 0.15: phase-4 integration pass identified param_1 as the bsp from both call sites)
// evidence: out/phase2/results/physics_00.json: "Iterates the three geometry-list fields of a
//   collision query record (unaff_EDI[0x202], [0x101] and [0]) calling
//   FUN_00503a60/FUN_00503ae0/FUN_00503c50 respectively for each entry." unaff_EDI[0x202] (byte
//   0x808, vertex_count), [0x101] (byte 0x404, edge_count) and [0] (surface_count) match
//   collision_bsp_sphere_result exactly (out/phase4/physics_types_notes.md).
// register convention: unaff_EDI -> result (collision_bsp_sphere_result *). param_1..param_5 are
//   Ghidra-recognized stack parameters. param_1 is the BSP: both call sites prove it -- 0x506440
//   passes DAT_00746f98 (structure_collision_bsp) and 0x505200 passes
//   `(short)uVar7 * 0x60 + iVar6`, the selected ModelCollisionGeometryBSP permutation record of
//   the collision node it is iterating. (A previous rewrite of this file guessed param_1 was a
//   material type, which made 0x506440 pass a BSP pointer where a material index was expected;
//   corrected by the phase-4 integration pass.) param_2..param_5 are margin, thickness,
//   object_index and model, forwarded verbatim into physics_shape_add_surface_proxy.
//   // blam-cc: EDI -> result, stack -> bsp, margin, thickness, object_index, model
// UNSURE, VERY significantly: the vertex and edge loops call physics_shape_add_vertex_proxy and
// physics_shape_add_edge_proxy with NO visible arguments at all in Ghidra's decompile -- not
// even the loop index. This rewrite is a best-effort reconstruction of what those calls almost
// certainly need (bsp, the matrix/moving-frame, the resolved vertex position or edge
// start/end), assuming `bsp` and `matrix` are carried in registers this function's own decompile
// never exposes (added here as explicit parameters). The surface loop (`local_8`, walking
// result->surfaces[] directly and calling physics_shape_add_surface_proxy) is the one part of
// this function Ghidra decompiled with visible arguments and is rewritten with confidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void physics_shape_add_vertex_proxy(real_matrix4x3 *matrix, physics_model *model,
                                            real_point3d *vertex, int16_t material_type,
                                            float height_offset, float radius,
                                            uint32_t object_index, int32_t surface_index,
                                            uint8_t surface_flags,
                                            int8_t breakable_surface_index); // 0x503a60, this batch
extern void physics_shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp,
    real_matrix4x3 *matrix, float height_offset, float thickness, int32_t object_index,
    physics_model *model); // 0x503ae0: EAX edge, ECX bsp, stack (matrix, margin, thickness, object, model)
extern void physics_shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame,
                                             int32_t surface_index, float margin, float thickness,
                                             int32_t object_index,
                                             physics_model *model); // 0x503c50, this batch

// blam-cc: EDI -> result, stack -> bsp, margin, thickness, object_index, model
// UNSURE: matrix and material_type are added parameters this function's own decompile never
// shows; they are listed LAST so that the leading six match the real calling convention exactly.
void physics_shape_build_proxies_from_query(collision_bsp_sphere_result *result,
                                             ModelCollisionGeometryBSP *bsp,
                                             float margin, float thickness, int32_t object_index,
                                             physics_model *model,
                                             real_matrix4x3 *matrix, int16_t material_type)
{
    ModelCollisionGeometryBSPVertex *vertices =
        (ModelCollisionGeometryBSPVertex *)bsp->vertices.pointer;
    ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
    int32_t i;

    for (i = 0; i < result->vertex_count; i++) {
        real_point3d vertex;
        uint32_t vertex_index = (uint32_t)result->vertices[i];
        vertex.x = vertices[vertex_index].point.x;
        vertex.y = vertices[vertex_index].point.y;
        vertex.z = vertices[vertex_index].point.z;
        physics_shape_add_vertex_proxy(matrix, model, &vertex, material_type, margin, thickness,
                                        (uint32_t)object_index, -1, 0, -1);
    }

    for (i = 0; i < result->edge_count; i++) {
        // 0x503e20: the callee computes the edge's start vertex and direction itself
        physics_shape_add_edge_proxy(result->edges[i], bsp, matrix, margin, thickness, object_index, model);
    }

    for (i = 0; i < result->surface_count; i++) {
        physics_shape_add_surface_proxy(bsp, (float *)matrix, result->surfaces[i], margin,
                                         thickness, object_index, model);
    }
}

#if 0
Original Ghidra decompilation (0x503d90):

void FUN_00503d90(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5)

{
  int *unaff_EDI;
  int *local_8;
  int local_4;

  local_4 = 0;
  if (0 < unaff_EDI[0x202]) {
    do {
      FUN_00503a60();
      local_4 = local_4 + 1;
    } while (local_4 < unaff_EDI[0x202]);
  }
  local_4 = 0;
  if (0 < unaff_EDI[0x101]) {
    do {
      FUN_00503ae0();
      local_4 = local_4 + 1;
    } while (local_4 < unaff_EDI[0x101]);
  }
  local_4 = 0;
  local_8 = unaff_EDI;
  if (0 < *unaff_EDI) {
    do {
      local_8 = local_8 + 1;
      FUN_00503c50(*local_8,param_2,param_3,param_4,param_5);
      local_4 = local_4 + 1;
    } while (local_4 < *unaff_EDI);
  }
  return;
}
#endif
