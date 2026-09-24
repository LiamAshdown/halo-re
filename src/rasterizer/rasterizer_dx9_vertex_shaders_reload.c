// rasterizer_dx9_vertex_shaders_reload  (Ghidra: rasterizer_dx9_vertex_shaders_reload, already named)
// address 0x530800, size 41 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: functions.md summary ("Releases and re-creates all vertex shaders (used e.g. after a
//   device reset)"); Ghidra types this `void`, but rasterizer_dx9_effects_initialize.c's caller
//   reads an `extraout_AL` success flag right after calling it with no intervening register
//   writes, i.e. the AL byte rasterizer_dx9_vertex_shaders_initialize leaves behind really does
//   propagate out through this function's own (register) return -- modeled here as uint8_t.
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

extern uint8_t rasterizer_dx9_vertex_shaders_initialize(void); // 0x5307b0

// Releases every created vertex shader, then reloads and recreates them all (used after a device
// reset). Returns whatever rasterizer_dx9_vertex_shaders_initialize returns.
uint8_t rasterizer_dx9_vertex_shaders_reload(void)
{
    int i;

    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        void *shader = (void *)rasterizer_vertex_shaders[i].shader;
        if (shader != 0) {
            ((void (__stdcall *)(void *))(*(void ***)shader)[2])(shader); // Release()
            rasterizer_vertex_shaders[i].shader = 0;
        }
    }
    return rasterizer_dx9_vertex_shaders_initialize();
}

#if 0
Original Ghidra decompilation (0x530800):

void __cdecl rasterizer_dx9_vertex_shaders_reload(void)

{
  int *piVar1;
  int *piVar2;

  piVar2 = &DAT_0069e350;
  do {
    piVar1 = (int *)*piVar2;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *piVar2 = 0;
    }
    piVar2 = piVar2 + 2;
  } while ((int)piVar2 < 0x69e550);
  rasterizer_dx9_vertex_shaders_initialize();
  return;
}
#endif
