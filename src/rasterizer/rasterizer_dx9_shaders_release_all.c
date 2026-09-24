// rasterizer_dx9_shaders_release_all  (Ghidra: rasterizer_dx9_shaders_release_all, already named)
// address 0x530090, size 48 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: functions.md summary ("Master DX9 shader-subsystem teardown: releases vertex
//   declarations, all vertex-shader interfaces, and all pixel-shader/effect resources"); the
//   0x0069e350..0x0069e550 loop stride (2 ints = 0x8 bytes) matches rasterizer_vertex_shader.
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

extern void rasterizer_dx9_vertex_declarations_release(void); // 0x530540
extern void rasterizer_dx9_pixel_shaders_release(void);        // 0x52ff60

// Releases vertex declarations, every created vertex-shader interface, then every pixel-shader
// effect and the effect pool (via rasterizer_dx9_pixel_shaders_release).
void rasterizer_dx9_shaders_release_all(void)
{
    int i;

    rasterizer_dx9_vertex_declarations_release();
    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        void *shader = (void *)rasterizer_vertex_shaders[i].shader;
        if (shader != 0) {
            ((void (*)(void *))(*(void ***)shader)[2])(shader); // Release()
            rasterizer_vertex_shaders[i].shader = 0;
        }
    }
    rasterizer_dx9_pixel_shaders_release();
}

#if 0
Original Ghidra decompilation (0x530090):

void __cdecl rasterizer_dx9_shaders_release_all(void)

{
  int *piVar1;
  int *piVar2;

  rasterizer_dx9_vertex_declarations_release();
  piVar2 = &DAT_0069e350;
  do {
    piVar1 = (int *)*piVar2;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *piVar2 = 0;
    }
    piVar2 = piVar2 + 2;
  } while ((int)piVar2 < 0x69e550);
  rasterizer_dx9_pixel_shaders_release();
  return;
}
#endif
