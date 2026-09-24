// rasterizer_render_loading_screen  (Ghidra: FUN_005157e0, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Renders and presents a loading/splash screen
// resource to the device, or simply clears the screen as a fallback.")
// address 0x5157e0, size 321 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: mode == 1 creates an offscreen surface (device vtable +0x90, 6 args matching
//   CreateOffscreenPlainSurface's shape), loads a resource into it via FUN_0057f80c (outside
//   this session's range), binds it as the render target (+0x98, +0x88) and presents through
//   rasterizer_capture_and_present, releasing the surface either way; any other nonzero mode returns immediately,
//   and mode == 0 (or a failed load) falls through to a plain ColorFill-shaped call (+0xac, 7
//   args, color 0x3f800000 == white) around rasterizer_capture_and_present.
// register convention: mode in in_EAX. // blam-cc: EAX -> mode
// UNSURE: FUN_0057f80c's signature (outside this session's range) is a bare best-effort typing
//   from its 9 literal arguments; `piVar2`/`local_8` follow the same "vtable pointer used as an
//   object" pattern flagged elsewhere in this module (rasterizer_dynamic_index_cache_draw.c) --
//   piVar2 is read as NULL and then its own +8 vtable slot is called, which cannot be a real
//   Release() on a NULL object; transliterated verbatim rather than guessed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device; // 0x0071d174
extern void *shell_module_handle;                                   // 0x00722bb8 HINSTANCE

typedef int32_t (*d3d_create_offscreen_surface_fn)(void *device, uint32_t width, uint32_t height,
                                                     uint32_t format, uint32_t pool, void **out_surface,
                                                     uint32_t shared_handle);
typedef int32_t (*d3d_device_call2_fn)(void *device, uint32_t a, void *b);
typedef int32_t (*d3d_device_call5_fn)(void *device, uint32_t a, uint32_t b, void *c, uint32_t d, uint32_t e);
typedef int32_t (*d3d_device_call6_fn)(void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                                        uint32_t color, uint32_t e);
typedef int32_t (*d3d_release_fn)(void *object);

extern int32_t FUN_0057f80c(void *surface, uint32_t a, uint32_t b, uint32_t resource_id, uint32_t c,
                             uint32_t d, uint32_t e, uint32_t f, uint32_t g); // 0x57f80c, UNSURE signature
// blam-cc: EAX -> tile, stack -> bitmap
extern void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); // 0x518180

// blam-cc: EAX -> mode
// Loads and presents a loading/splash screen resource (mode == 1), falling back to clearing the
// screen white for any other mode that reaches the tail path (mode == 0, or a failed load).
void rasterizer_render_loading_screen(int32_t mode)
{
    void **vtable;

    if (mode != 0) {
        if (mode != 1) {
            return;
        }
        if (rasterizer_device != (void *)0) {
            void *surface = (void *)0;
            int32_t hr;

            vtable = *(void ***)rasterizer_device;
            hr = ((d3d_create_offscreen_surface_fn)vtable[0x90 / 4])(rasterizer_device, 0x280, 0x1e0, 0x16, 0, &surface, 0);
            if (hr >= 0) {
                hr = FUN_0057f80c(surface, 0, 0, (uint32_t)shell_module_handle, 0x86, 0, 0xffffffff, 0, 0);
                if (hr >= 0) {
                    void *render_target_slot = 0;

                    vtable = *(void ***)rasterizer_device;
                    ((d3d_device_call2_fn)vtable[0x98 / 4])(rasterizer_device, 0, &render_target_slot);

                    vtable = *(void ***)rasterizer_device;
                    ((d3d_device_call5_fn)vtable[0x88 / 4])(rasterizer_device, 0, 0, &render_target_slot, 0, 0);

                    rasterizer_capture_and_present((const int16_t *)0, (BitmapData *)0); // EAX = 0, push 0

                    {
                        void *unresolved = (void *)0; // UNSURE: piVar2, see file header

                        vtable = *(void ***)rasterizer_device;
                        ((d3d_device_call5_fn)vtable[0x88 / 4])(rasterizer_device, 0, 0, (void *)0, 0, 0);

                        vtable = *(void ***)unresolved;
                        ((d3d_release_fn)vtable[2])(unresolved); // UNSURE: Release on a NULL object
                    }
                }
                vtable = *(void ***)surface;
                ((d3d_release_fn)vtable[2])(surface);
                if (hr >= 0) {
                    return;
                }
            }
        }
    }

    if (rasterizer_device != (void *)0) {
        vtable = *(void ***)rasterizer_device;
        ((d3d_device_call6_fn)vtable[0xac / 4])(rasterizer_device, 0, 0, 7, 0, 0x3f800000, 0);
        rasterizer_capture_and_present((const int16_t *)0, (BitmapData *)0); // EAX = 0, push 0
        vtable = *(void ***)rasterizer_device;
        ((d3d_device_call6_fn)vtable[0xac / 4])(rasterizer_device, 0, 0, 7, 0, 0x3f800000, 0);
    }
}

#if 0
Original Ghidra decompilation (0x5157e0):

void FUN_005157e0(void)

{
  int in_EAX;
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 *puVar5;
  int *local_8;
  undefined4 local_4;

  if (in_EAX != 0) {
    if (in_EAX != 1) {
      return;
    }
    if (DAT_0071d174 == (int *)0x0) {
      return;
    }
    local_4 = 0;
    local_8 = (int *)0x0;
    iVar1 = (**(code **)(*DAT_0071d174 + 0x90))(DAT_0071d174,0x280,0x1e0,0x16,0,&local_8,0);
    if (-1 < iVar1) {
      iVar1 = FUN_0057f80c(local_8,0,0,DAT_00722bb8,0x86,0,0xffffffff,0,0);
      if (-1 < iVar1) {
        puVar5 = &local_4;
        uVar4 = 0;
        (**(code **)(*DAT_0071d174 + 0x98))(DAT_0071d174,0,puVar5);
        uVar3 = 0;
        (**(code **)(*DAT_0071d174 + 0x88))(DAT_0071d174,uVar4,0,puVar5,0,0);
        FUN_00518180(0);
        piVar2 = (int *)0x0;
        (**(code **)(*DAT_0071d174 + 0x88))(DAT_0071d174,uVar4,0,uVar3,0,0);
        (**(code **)(*piVar2 + 8))(piVar2);
      }
      (**(code **)(*local_8 + 8))(local_8);
      if (-1 < iVar1) {
        return;
      }
    }
  }
  if (DAT_0071d174 != (int *)0x0) {
    (**(code **)(*DAT_0071d174 + 0xac))(DAT_0071d174,0,0,7,0,0x3f800000,0);
    FUN_00518180(0);
    (**(code **)(*DAT_0071d174 + 0xac))(DAT_0071d174,0,0,7,0,0x3f800000,0);
  }
  return;
}
#endif
