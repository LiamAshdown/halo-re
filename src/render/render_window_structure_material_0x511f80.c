// render_window_structure_material_0x511f80  (not a Ghidra function; a structure pass callback thunk)
// address 0x511f80, size 5 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x511f80: jmp 0x521900 -- rasterizer_shader_environment_projected_light_draw with the arguments unchanged.
// blam-cc: stack -> material callback arguments (cdecl, forwarded)

#include "tags.h"
#include "memory.h"
#include "math.h"

extern void rasterizer_shader_environment_projected_light_draw(void *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, void *vertex_buffer); // 0x521900

void render_window_structure_material_0x511f80(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    rasterizer_shader_environment_projected_light_draw(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}
