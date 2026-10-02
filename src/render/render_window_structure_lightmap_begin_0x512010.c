// render_window_structure_lightmap_begin_0x512010  (not a Ghidra function; a structure pass callback thunk)
// address 0x512010, size 9 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x512010: EAX = the argument, jmp 0x520910 (rasterizer_shader_environment_set_lightmap).
// blam-cc: stack -> bitmap_data (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void rasterizer_shader_environment_set_lightmap(void *lightmap); // 0x520910, EAX

void render_window_structure_lightmap_begin_0x512010(void *bitmap_data)
{
    rasterizer_shader_environment_set_lightmap(bitmap_data);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
