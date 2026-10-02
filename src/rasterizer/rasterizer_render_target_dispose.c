// rasterizer_render_target_dispose  (Ghidra: FUN_0052cc50, unnamed)
// address 0x52cc50, size 111 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: walks rasterizer_render_targets[9] (0x0069d358, stride 0x14) releasing each entry's
//   surface and texture (COM vtable+8 is IUnknown::Release), matching the type header's own note
//   for this exact function/global pair, then resets rasterizer_active_render_target to 0xffff
//   (none) and releases the render-target index/vertex buffers.
// register convention: none -- no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern int16_t rasterizer_active_render_target;    // 0x0069d350
extern void *rasterizer_render_target_index_buffer;  // 0x0071d208
extern void *rasterizer_render_target_vertex_buffer; // 0x0071d20c

typedef int32_t (__stdcall *d3d_release_fn)(void *self);

static void release_com(uint32_t *slot)
{
    void *object = (void *)(uintptr_t)*slot;
    if (object != 0) {
        void **vtable = *(void ***)object;
        ((d3d_release_fn)vtable[8 / 4])(object);
        *slot = 0;
    }
}

// Releases the off-screen render-target pool's surfaces/textures and the two supporting device
// buffers created by rasterizer_render_target_initialize, resetting all handles to none.
void rasterizer_render_target_dispose(void)
{
    int i;

    for (i = 0; i < k_rasterizer_render_targets; i++) {
        release_com(&rasterizer_render_targets[i].surface);
        release_com(&rasterizer_render_targets[i].texture);
    }

    rasterizer_active_render_target = -1;

    if (rasterizer_render_target_index_buffer != 0) {
        void **vtable = *(void ***)rasterizer_render_target_index_buffer;
        ((d3d_release_fn)vtable[8 / 4])(rasterizer_render_target_index_buffer);
        rasterizer_render_target_index_buffer = 0;
    }
    if (rasterizer_render_target_vertex_buffer != 0) {
        void **vtable = *(void ***)rasterizer_render_target_vertex_buffer;
        ((d3d_release_fn)vtable[8 / 4])(rasterizer_render_target_vertex_buffer);
        rasterizer_render_target_vertex_buffer = 0;
    }
}

#if 0
Original Ghidra decompilation (0x52cc50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0052cc50(void)

{
  int *piVar1;
  int *piVar2;

  piVar2 = &DAT_0069d368;
  do {
    piVar1 = (int *)piVar2[-1];
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      piVar2[-1] = 0;
    }
    piVar1 = (int *)*piVar2;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *piVar2 = 0;
    }
    piVar2 = piVar2 + 5;
  } while ((int)piVar2 < 0x69d41c);
  _DAT_0069d350 = 0xffff;
  if (DAT_0071d208 != (int *)0x0) {
    (**(code **)(*DAT_0071d208 + 8))(DAT_0071d208);
    DAT_0071d208 = (int *)0x0;
  }
  if (DAT_0071d20c != (int *)0x0) {
    (**(code **)(*DAT_0071d20c + 8))(DAT_0071d20c);
    DAT_0071d20c = (int *)0x0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
