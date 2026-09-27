// render_window_structure_material_0x511f40  (not a Ghidra function; a structure pass callback thunk)
// address 0x511f40, size 6 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x511f40: jmp DWORD PTR ds:0x7c0494 -- forwards every argument to the procedure the rasterizer stored in 0x007c0494.
// blam-cc: stack -> material callback arguments (cdecl, forwarded)

#include "tags.h"
#include "memory.h"
#include "math.h"

extern void *shader_environment_procedure_007c0494; // 0x007c0494

void render_window_structure_material_0x511f40(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra)
{
    ((void (*)(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t first_surface, int32_t surface_count, void *material_extra))shader_environment_procedure_007c0494)(shader_data, shader_permutation, render_context, first_surface, surface_count, material_extra);
}
