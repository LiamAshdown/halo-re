// physics_shape_build_proxies_from_query  (Ghidra: FUN_00503d90, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503d90, size 283 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (step 1: rewritten from objdump -d
//   0x503d90..0x503eaa; the draft guessed the helper arguments and added two parameters the
//   function does not have)
// evidence: walks the three lists of a collision_bsp_sphere_result and turns every entry into
//   physics_model proxies: each vertex through physics_shape_add_vertex_proxy (0x503a60: EAX
//   vertex, ECX bsp, EBX object_index, stack matrix, margin, thickness, model), each edge
//   through physics_shape_add_edge_proxy (0x503ae0: EAX edge, ECX bsp, stack matrix, margin,
//   thickness, object_index, model) and each surface through physics_shape_add_surface_proxy
//   (0x503c50: EAX bsp, ESI matrix, stack surface, margin, thickness, object_index, model).
//   Callers: physics_model_build_from_sphere_query (0x5064f4) passes the structure BSP, EAX = 0
//   (no matrix) and object_index -1; object_collision_context_gather_sphere_shapes (0x5052f4)
//   passes an object's collision BSP, its node matrix in EAX and the object index.
// register convention: EDI result, EAX matrix (may be 0), five stack arguments.
//   // blam-cc: EDI -> result, EAX -> matrix, stack -> bsp, margin, thickness, object_index, model

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void physics_shape_add_vertex_proxy(ModelCollisionGeometryBSP *bsp, uint32_t vertex_index,
    uint32_t object_index, real_matrix4x3 *matrix, float height_offset, float radius,
    physics_model *model); // 0x503a60, blam-cc: ECX bsp, EAX vertex_index, EBX object_index
extern void physics_shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp,
    real_matrix4x3 *matrix, float height_offset, float thickness, int32_t object_index,
    physics_model *model); // 0x503ae0, blam-cc: EAX edge_index, ECX bsp
extern void physics_shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame,
    int32_t surface_index, float margin, float thickness, int32_t object_index,
    physics_model *model); // 0x503c50, blam-cc: EAX bsp, ESI moving_frame

// blam-cc: EDI -> result, EAX -> matrix, stack -> bsp, margin, thickness, object_index, model
// Adds a sphere/pill proxy for every vertex, pill/quad proxies for every edge and a polygon
// proxy for every surface the sphere query found, transformed by `matrix` when there is one.
void physics_shape_build_proxies_from_query(collision_bsp_sphere_result *result, real_matrix4x3 *matrix,
    ModelCollisionGeometryBSP *bsp, float margin, float thickness, int32_t object_index,
    physics_model *model)
{
    int32_t i;

    for (i = 0; i < result->vertex_count; i++) {
        physics_shape_add_vertex_proxy(bsp, (uint32_t)result->vertices[i], (uint32_t)object_index, matrix,
                                        margin, thickness, model);
    }
    for (i = 0; i < result->edge_count; i++) {
        physics_shape_add_edge_proxy(result->edges[i], bsp, matrix, margin, thickness, object_index, model);
    }
    for (i = 0; i < result->surface_count; i++) {
        physics_shape_add_surface_proxy(bsp, (float *)matrix, result->surfaces[i], margin, thickness,
                                         object_index, model);
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
