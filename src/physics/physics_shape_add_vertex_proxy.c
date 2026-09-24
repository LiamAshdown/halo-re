// physics_shape_add_vertex_proxy  (Ghidra: FUN_00503a60, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503a60, size 126 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase2/results/physics_00.json: "Wrapper that transforms a BSP vertex into
//   world/local space (if a matrix is given) and appends it as a sphere collision proxy."
// register convention: UNSURE. Ghidra's decompile shows this function forwarding only two of
//   physics_shape_vertex_to_sphere's six trailing arguments
//   (`FUN_00503360(param_2,param_3);`); the rest are almost certainly passed through unchanged
//   via stack slots this wrapper shares with its own caller (a tail-forward Ghidra did not fully
//   attribute to the call site), which this rewrite cannot recover with confidence. It is
//   written here taking the full parameter list physics_shape_vertex_to_sphere needs, with the
//   optional matrix transform applied in place to `vertex` first, which is the only way to
//   express the documented behaviour ("transforms a BSP vertex... if a matrix is given") in
//   valid C; treat the exact parameter identity as UNSURE pending disassembly confirmation.
//   // blam-cc: stack -> matrix, model, vertex, material_type, height_offset, radius,
//   //           object_index, surface_index, surface_flags, breakable_surface_index

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

void physics_shape_add_vertex_proxy(real_matrix4x3 *matrix, physics_model *model,
                                     real_point3d *vertex, int16_t material_type,
                                     float height_offset, float radius, uint32_t object_index,
                                     int32_t surface_index, uint8_t surface_flags,
                                     int8_t breakable_surface_index)
{
    if (matrix != 0) {
        matrix4x3_transform_point(vertex, vertex, matrix);
    }
    physics_shape_vertex_to_sphere(model, vertex, material_type, height_offset, radius,
                                    object_index, surface_index, surface_flags,
                                    breakable_surface_index);
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
