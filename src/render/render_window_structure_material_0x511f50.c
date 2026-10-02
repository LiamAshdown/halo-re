// render_window_structure_material_0x511f50  (not a Ghidra function; a structure pass callback thunk)
// address 0x511f50, size 28 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x511f50: calls rasterizer_object_shadow_structure_draw (0x531570) with EAX = argument 5 (the vertex buffer) and
//   stack (arguments 2, 3, 4: dynamic index slot, first primitive, primitive count).
// blam-cc: stack -> material callback arguments (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void rasterizer_object_shadow_structure_draw(void *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive,
    int32_t primitive_count); // 0x531570, EAX, stack

void render_window_structure_material_0x511f50(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    rasterizer_object_shadow_structure_draw(material_extra, render_context, first_surface, surface_count);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
