// render_window_structure_material_0x5120c0  (not a Ghidra function; a structure pass callback thunk)
// address 0x5120c0, size 33 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x5120c0: calls rasterizer_water_ripple_draw (0x51ee60) with EAX = argument 5 (vertex buffer) and stack
//   (argument 0 shader, then arguments 2, 3, 4).
// blam-cc: stack -> material callback arguments (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void rasterizer_water_ripple_draw(void *vertex_buffer, void *shader, int32_t dynamic_index_slot, int32_t first_primitive,
    int32_t primitive_count); // 0x51ee60, EAX, stack

void render_window_structure_material_0x5120c0(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    rasterizer_water_ripple_draw(material_extra, shader_data, render_context, first_surface, surface_count);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
