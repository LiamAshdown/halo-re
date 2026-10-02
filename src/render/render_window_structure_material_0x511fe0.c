// render_window_structure_material_0x511fe0  (not a Ghidra function; a structure pass callback thunk)
// address 0x511fe0, size 38 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x511fe0: calls rasterizer_shader_environment_lightmap_specular_draw (0x521f90) with EAX = argument 0 and
//   stack (arguments 1..5).
// blam-cc: stack -> material callback arguments (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void rasterizer_shader_environment_lightmap_specular_draw(void *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, void *vertex_buffer); // 0x521f90, EAX, stack

void render_window_structure_material_0x511fe0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    rasterizer_shader_environment_lightmap_specular_draw(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
