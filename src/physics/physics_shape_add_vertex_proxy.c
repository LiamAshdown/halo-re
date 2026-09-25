// physics_shape_add_vertex_proxy  (Ghidra: FUN_00503a60, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503a60, size 126 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase2/results/physics_00.json: "Wrapper that transforms a BSP vertex into
//   world/local space (if a matrix is given) and appends it as a sphere collision proxy."
// register convention: RESOLVED against objdump 0x503a60..0x503ad4 (was UNSURE/guessed before).
//   ECX -> bsp (ModelCollisionGeometryBSP*, the +0x40/0x4c/0x58 surfaces/edges/vertices
//   TagReflexives documented at the top of types/physics.h -- NOT physics_model as this file
//   previously guessed); EAX -> vertex_index (0x503a63 `shl eax,0x4` = vertex stride 0x10);
//   EBX -> object_index, tested against -1 at entry (0x503a6b) to decide whether the resolved
//   surface_index is kept (object_index == -1, a world/static proxy) or forced to -1 (a real
//   object attached to it, 0x503a91/0x503a96), and later forwarded unchanged as
//   physics_shape_vertex_to_sphere's own object_index argument (0x503ac6 `push ebx`).
//   material_type, surface_index, surface_flags and breakable_surface_index are NOT real inputs
//   at all -- every one of them is resolved here from vertex->first_edge->left_surface's own
//   ModelCollisionGeometryBSPSurface record (material/flags/breakable_surface at +0xa/+8/+9),
//   which is why this file's very first draft could reconstruct only `vertex`/`model` and gave
//   up on the rest. `matrix`, `model`, `height_offset` and `radius` remain genuine stack
//   parameters, reloaded from the stack (0x503abc/0x503ac1) right before the call.
//   // blam-cc: ECX -> bsp, EAX -> vertex_index, EBX -> object_index, stack -> matrix, model, height_offset, radius
// FIXED (register inputs, objdump): EAX, EBX and ECX are genuine live-ins the notes did not map;
// this whole rewrite (previously an admitted guess, rewrite confidence 0.25) is replaced with a
// transcription of the real disassembly now that these registers are resolved.
// UNSURE: physics_shape_build_proxies_from_query.c (outside this batch) still calls this
// function with the old (matrix, model, vertex, material_type, ...) 10-argument shape; it needs
// a follow-up pass to forward bsp/vertex_index/object_index instead.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void physics_shape_vertex_to_sphere(physics_model *model, real_point3d *vertex,
                                            int16_t material_type, float height_offset,
                                            float radius, uint32_t object_index,
                                            int32_t surface_index, uint8_t surface_flags,
                                            int8_t breakable_surface_index); // 0x503360, this batch

void physics_shape_add_vertex_proxy(ModelCollisionGeometryBSP *bsp, uint32_t vertex_index,
                                     uint32_t object_index, real_matrix4x3 *matrix,
                                     physics_model *model, float height_offset, float radius)
{
    ModelCollisionGeometryBSPVertex *vertex_rec =
        &((ModelCollisionGeometryBSPVertex *)bsp->vertices.pointer)[vertex_index];
    ModelCollisionGeometryBSPEdge *edge =
        &((ModelCollisionGeometryBSPEdge *)bsp->edges.pointer)[vertex_rec->first_edge];
    ModelCollisionGeometryBSPSurface *surface =
        &((ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer)[edge->left_surface];
    int32_t surface_index = (object_index == 0xffffffff) ? (int32_t)edge->left_surface : -1;
    real_point3d transformed;
    real_point3d *vertex_point;

    if (matrix != 0) {
        matrix4x3_transform_point(&transformed, (real_point3d *)&vertex_rec->point, matrix);
        vertex_point = &transformed;
    } else {
        vertex_point = (real_point3d *)&vertex_rec->point;
    }

    physics_shape_vertex_to_sphere(model, vertex_point, (int16_t)surface->material, height_offset,
                                    radius, object_index, surface_index, surface->flags,
                                    surface->breakable_surface);
}

#if 0
Original Ghidra decompilation (0x503a60):

void FUN_00503a60(int param_1,undefined4 param_2,undefined4 param_3)

{
  if (param_1 != 0) {
    matrix4x3_transform_point(param_1);
  }
  FUN_00503360(param_2,param_3);
  return;
}
#endif
