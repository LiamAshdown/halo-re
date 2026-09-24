// rasterizer_render_target_set_active  (Ghidra: FUN_0052ccc0, unnamed)
// address 0x52ccc0, size 258 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: fully disassembled (`objdump -d -Mintel --start-address=0x52ccc0
//   --stop-address=0x52cdc5 bin/halo.exe`) because Ghidra drops the function's two stack
//   parameters and shows several vtable calls with fewer arguments than they take. Confirmed:
//   SetRenderTarget(device,0,surface) [+0x94, established via rasterizer_end_frame.c],
//   IDirect3DSurface9::GetDesc(&desc) [+0x30 -- a single out-parameter call, distinct from the
//   +0x30 LockRect noted elsewhere in this module for a *texture* object, which takes three
//   arguments], SetViewport(device,&viewport) [+0xbc, established via rasterizer_end_frame.c] and
//   Clear(device,0,NULL,flags,color,1.0,0) [+0xac, established via
//   rasterizer_render_loading_screen.c]. When `target_index` is 1 the viewport is built from
//   rasterizer_window.camera.viewport_bounds instead of the target's own surface description.
// register convention: target index in EAX; clear color and a clear-enable byte are stack
//   parameters (the byte lands two dwords into the stack args, i.e. after an otherwise-unread
//   first stack dword -- UNSURE why the gap, transcribed as-is).
//   // blam-cc: EAX -> target_index, stack -> (clear_color, clear)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h>

extern void *rasterizer_device;                        // 0x0071d174
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern int16_t rasterizer_active_render_target;         // 0x0069d350
extern rasterizer_window_parameters rasterizer_window;  // 0x007c1220

typedef int32_t (*d3d_set_render_target_fn)(void *device, uint32_t index, void *surface);
typedef int32_t (*d3d_get_desc_fn)(void *surface, d3d_surface_desc *out_desc);
typedef int32_t (*d3d_set_viewport_fn)(void *device, const d3d_viewport *viewport);
typedef int32_t (*d3d_clear_fn)(void *device, uint32_t count, const void *rects, uint32_t flags,
                                  uint32_t color, float z, uint32_t stencil);

// blam-cc: EAX -> target_index, stack -> (clear_color, clear)
// Switches the active off-screen render target used by the debug-overlay and dynamic-light-shadow
// drawing routines, and optionally clears it.
void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear)
{
    void *surface = 0;
    d3d_viewport viewport;
    void **vtable;

    if (target_index < 9 && target_index >= 0) {
        surface = (void *)(uintptr_t)rasterizer_render_targets[target_index].surface;
    }

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_render_target_fn)vtable[0x94 / 4])(rasterizer_device, 0, surface);

    rasterizer_active_render_target = target_index;

    if (target_index == 1) {
        viewport.x = rasterizer_window.camera.viewport_bounds.left;
        viewport.y = rasterizer_window.camera.viewport_bounds.top;
        viewport.width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
        viewport.height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;
    } else {
        d3d_surface_desc desc;
        void **surface_vtable = *(void ***)surface;
        ((d3d_get_desc_fn)surface_vtable[0x30 / 4])(surface, &desc);
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = desc.width;
        viewport.height = desc.height;
    }
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_viewport_fn)vtable[0xbc / 4])(rasterizer_device, &viewport);

    if (clear != 0) {
        uint32_t flags = (target_index == 1 || target_index == 2) ? 7 : 1;
        vtable = *(void ***)rasterizer_device;
        ((d3d_clear_fn)vtable[0xac / 4])(rasterizer_device, 0, 0, flags, clear_color, 1.0f, 0);
    }
}

#if 0
Original Ghidra decompilation (0x52ccc0): see `python tools/pack.py 0x52ccc0`; the two stack
parameters and several vtable call arguments are unreliable in the decompile, resolved above via
`objdump -d -Mintel --start-address=0x52ccc0 --stop-address=0x52cdc5 bin/halo.exe`.
#endif
