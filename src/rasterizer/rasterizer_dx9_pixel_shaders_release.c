// rasterizer_dx9_pixel_shaders_release  (Ghidra: rasterizer_dx9_pixel_shaders_release, already named)
// address 0x52ff60, size 281 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: frees exactly the same rasterizer_effects[] constant-handle blocks that
//   rasterizer_dx9_shaders_initialize.c allocates (same index ranges, confirmed by address
//   arithmetic), then Releases every effect and the effect pool -- the matching teardown.
// register convention: none -- __cdecl, no arguments.

#include "win32.h"
#include "tags.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern void *rasterizer_effect_pool;                                  // 0x0071d254

static void free_constant_handles(int first, int last)
{
    int i;
    for (i = first; i <= last; i++) {
        if (rasterizer_effects[i].constant_handles != 0) {
            GlobalFree((void *)rasterizer_effects[i].constant_handles);
            rasterizer_effects[i].constant_handles = 0;
        }
    }
}

// Frees every shader-constant handle table allocated by rasterizer_dx9_shaders_initialize and
// releases every pixel-shader effect interface and the effect-pool object it created.
void rasterizer_dx9_pixel_shaders_release(void)
{
    int i;

    free_constant_handles(116, 121);
    free_constant_handles(32, 34);
    free_constant_handles(37, 39);
    free_constant_handles(106, 106);
    free_constant_handles(107, 107);
    free_constant_handles(108, 108);
    free_constant_handles(0, 3);
    free_constant_handles(114, 114);
    free_constant_handles(40, 43);

    for (i = 0; i < k_rasterizer_pixel_shader_effects; i++) {
        void *effect = (void *)rasterizer_effects[i].effect;
        if (effect != 0) {
            ((void (__stdcall *)(void *))(*(void ***)effect)[2])(effect); // Release()
            rasterizer_effects[i].effect = 0;
        }
    }
    if (rasterizer_effect_pool != 0) {
        ((void (__stdcall *)(void *))(*(void ***)rasterizer_effect_pool)[2])(rasterizer_effect_pool); // Release()
        rasterizer_effect_pool = 0;
    }
}

#if 0
Original Ghidra decompilation (0x52ff60):

void __cdecl rasterizer_dx9_pixel_shaders_release(void)

{
  int *piVar1;
  undefined4 *puVar2;
  int *piVar3;

  puVar2 = &DAT_0069e2a8;
  do {
    if ((HGLOBAL)*puVar2 != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)*puVar2);
      *puVar2 = 0;
    }
    puVar2 = puVar2 + 8;
  } while ((int)puVar2 < 0x69e349);
  puVar2 = &DAT_0069d828;
  do {
    if ((HGLOBAL)*puVar2 != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)*puVar2);
      *puVar2 = 0;
    }
    puVar2 = puVar2 + 8;
  } while ((int)puVar2 < 0x69d869);
  puVar2 = &DAT_0069d8c8;
  do {
    if ((HGLOBAL)*puVar2 != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)*puVar2);
      *puVar2 = 0;
    }
    puVar2 = puVar2 + 8;
  } while ((int)puVar2 < 0x69d909);
  if (DAT_0069e168 != (HGLOBAL)0x0) {
    GlobalFree(DAT_0069e168);
    DAT_0069e168 = (HGLOBAL)0x0;
  }
  if (DAT_0069e188 != (HGLOBAL)0x0) {
    GlobalFree(DAT_0069e188);
    DAT_0069e188 = (HGLOBAL)0x0;
  }
  if (DAT_0069e1a8 != (HGLOBAL)0x0) {
    GlobalFree(DAT_0069e1a8);
    DAT_0069e1a8 = (HGLOBAL)0x0;
  }
  puVar2 = &DAT_0069d428;
  do {
    if ((HGLOBAL)*puVar2 != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)*puVar2);
      *puVar2 = 0;
    }
    puVar2 = puVar2 + 8;
  } while ((int)puVar2 < 0x69d489);
  if (DAT_0069e268 != (HGLOBAL)0x0) {
    GlobalFree(DAT_0069e268);
    DAT_0069e268 = (HGLOBAL)0x0;
  }
  puVar2 = &DAT_0069d928;
  do {
    if ((HGLOBAL)*puVar2 != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)*puVar2);
      *puVar2 = 0;
    }
    puVar2 = puVar2 + 8;
  } while ((int)puVar2 < 0x69d989);
  piVar3 = &DAT_0069d410;
  do {
    piVar1 = (int *)*piVar3;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *piVar3 = 0;
    }
    piVar3 = piVar3 + 8;
  } while ((int)piVar3 < 0x69e350);
  if (DAT_0071d254 != (int *)0x0) {
    (**(code **)(*DAT_0071d254 + 8))(DAT_0071d254);
    DAT_0071d254 = (int *)0x0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
