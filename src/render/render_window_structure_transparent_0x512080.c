// render_window_structure_transparent_0x512080  (not a Ghidra function; a structure pass callback thunk)
// address 0x512080, size 63 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x512080: calls rasterizer_transparent_geometry_group_new (0x522300) with EAX = argument 7 and stack
//   (arguments 0..6, 8, 10, 11); argument 9 is not passed.
// blam-cc: stack -> transparent material callback arguments (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"

extern void rasterizer_transparent_geometry_group_new(void *shader, int16_t shader_permutation, uint32_t lightmap_bitmap,
    uint32_t dynamic_index_slot, uint32_t first_index, uint32_t primitive_count, uint32_t vertex_buffer, void *tint,
    uint32_t param_9, uint32_t flags, void *world_position); // 0x522300, stack, EAX = world_position

void render_window_structure_transparent_0x512080(void *shader_data, int16_t shader_permutation, void *bitmap,
    int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra, void *rendered_vertices,
    void *lightmap_vertices, void *coplanar_vector, void *lightmap_vertices_offset, int32_t zero)
{
    rasterizer_transparent_geometry_group_new(shader_data, shader_permutation, (uint32_t)bitmap, (uint32_t)render_context,
        (uint32_t)surface_offset, (uint32_t)(uint16_t)surface_count, (uint32_t)material_extra, lightmap_vertices,
        (uint32_t)lightmap_vertices_offset, (uint32_t)zero, rendered_vertices);
}
