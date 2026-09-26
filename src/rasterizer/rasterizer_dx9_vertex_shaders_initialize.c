// rasterizer_dx9_vertex_shaders_initialize  (Ghidra: rasterizer_dx9_vertex_shaders_initialize, already named)
// address 0x5307b0, size 66 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: functions.md summary ("Resets the vertex-shader table and (re)loads all precompiled
//   vertex shaders, raising a fatal error referencing shaders\vsh.bin on failure").
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h> // memset

extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern const char *rasterizer_shader_file_name; // 0x00722bbc

extern uint32_t rasterizer_dx9_vertex_shaders_load_all(void); // 0x5306e0
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

// Clears the vertex-shader table, loads and creates all precompiled vertex shaders, and raises a
// fatal error dialog referencing shaders\vsh.bin if that fails.
uint8_t rasterizer_dx9_vertex_shaders_initialize(void)
{
    uint32_t ok;
    int32_t i;

    // 0x5307c0: clears only each entry's shader pointer (stride 8). The C used to memset the whole table, which
    // also wiped the static 'enabled' flags, so load_all created no vertex shaders at all -> black screen.
    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        rasterizer_vertex_shaders[i].shader = 0;
    }

    ok = rasterizer_dx9_vertex_shaders_load_all();
    if ((ok & 0xff) == 0) {
        rasterizer_shader_file_name = "shaders\\vsh.bin";
        shell_display_fatal_error_dialog(0x89, 0x7e, 1);
    }
    return (uint8_t)ok;
}

#if 0
Original Ghidra decompilation (0x5307b0):

char __cdecl rasterizer_dx9_vertex_shaders_initialize(void)

{
  undefined4 *puVar1;
  uint uVar2;

  puVar1 = &DAT_0069e350;
  do {
    *puVar1 = 0;
    puVar1 = puVar1 + 2;
  } while ((int)puVar1 < 0x69e550);
  uVar2 = rasterizer_dx9_vertex_shaders_load_all();
  if ((char)uVar2 == '\0') {
    DAT_00722bbc = "shaders\\vsh.bin";
    shell_display_fatal_error_dialog(0x89,0x7e,1);
  }
  return (char)uVar2;
}
#endif
