// structure_picked_polygon_material  (not a Ghidra function; a material callback)
// address 0x511f30, size 6 bytes
// name confidence: 0.5  rewrite confidence: 1.0
// evidence: pushed as the material callback of structure_leaf_faces_for_each by structure_picked_polygon_draw
//   (0x552930 push 0x511f30); only reachable through that pointer.
// objdump 0x511f30: jmp DWORD PTR ds:0x7c048c -- forwards its arguments unchanged to the procedure the rasterizer
//   stored in 0x007c048c (the shader_environment self-illumination draw for the card).
// blam-cc: stack -> (shader_data, shader_permutation, render_context, surface_offset, surface_count, material_extra)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern void *unknown_007c048c; // 0x007c048c

void structure_picked_polygon_material(void *shader_data, int16_t shader_permutation, int32_t render_context,
    int32_t surface_offset, int16_t surface_count, void *material_extra)
{
    ((structure_material_callback)unknown_007c048c)(shader_data, shader_permutation, render_context, surface_offset,
        surface_count, material_extra);
}
