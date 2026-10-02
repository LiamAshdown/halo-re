// rasterizer_render_loading_screen  (Ghidra: FUN_005157e0, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Renders and presents a loading/splash screen
// resource to the device, or simply clears the screen as a fallback.")
// address 0x5157e0, size 321 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: mode == 1 creates an offscreen surface (device vtable +0x90, 6 args matching
//   CreateOffscreenPlainSurface's shape), loads a resource into it via FUN_0057f80c (outside
//   this session's range), binds it as the render target (+0x98, +0x88) and presents through
//   rasterizer_capture_and_present, releasing the surface either way; any other nonzero mode returns immediately,
//   and mode == 0 (or a failed load) falls through to a plain ColorFill-shaped call (+0xac, 7
//   args, color 0x3f800000 == white) around rasterizer_capture_and_present.
// register convention: mode in in_EAX. // blam-cc: EAX -> mode
// FIXED (first-boot track, objdump 0x5157e0..0x515920): the object released after the two copies is the render
//   target that GetRenderTarget(0) (+0x98) wrote into the second local -- the old transliteration read it as a
//   never-written NULL, which crashed the standalone boot. The +0x88 calls are StretchRect(splash, NULL,
//   render_target, NULL, D3DTEXF_NONE), made twice around the present; FUN_0057f80c is D3DXLoadSurfaceFromResourceA;
//   the fallback's 0x3f800000 is Clear's Z (1.0), the color is 0 (black).

#include "d3d.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device; // 0x0071d174
extern void *shell_module_handle;                                   // 0x00722bb8 HINSTANCE

typedef int32_t (__stdcall *d3d_create_offscreen_surface_fn)(void *device, uint32_t width, uint32_t height,
                                                     uint32_t format, uint32_t pool, void **out_surface,
                                                     uint32_t shared_handle);
typedef int32_t (__stdcall *d3d_device_call2_fn)(void *device, uint32_t a, void *b);
typedef int32_t (__stdcall *d3d_device_call5_fn)(void *device, uint32_t a, uint32_t b, void *c, uint32_t d, uint32_t e);
typedef int32_t (__stdcall *d3d_device_call6_fn)(void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                                        uint32_t color, uint32_t e);
typedef int32_t (__stdcall *d3d_release_fn)(void *object);

// D3DXLoadSurfaceFromResourceA, statically linked D3DX at 0x57f80c: (dest surface, dest palette, dest rect, module,
// resource, src rect, filter, color key, src info). objdump 0x57f80c..: finds the resource, then loads it from memory
// with the last four arguments passed through. The standalone build takes it from the DirectX SDK's d3dx9.lib.
// blam-cc: EAX -> tile, stack -> bitmap
extern void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); // 0x518180

// blam-cc: EAX -> mode
// Loads and presents a loading/splash screen resource (mode == 1), falling back to clearing the
// screen white for any other mode that reaches the tail path (mode == 0, or a failed load).
void rasterizer_render_loading_screen(int32_t mode)
{
    void **vtable;

    if (mode != 0) {
        void *splash = 0;        // [esp+0]
        void *render_target = 0; // [esp+4]
        int32_t hr;

        if (mode != 1 || rasterizer_device == (void *)0) {
            return;
        }
        vtable = *(void ***)rasterizer_device;
        hr = ((d3d_create_offscreen_surface_fn)vtable[0x90 / 4])(rasterizer_device, 0x280, 0x1e0,
            0x16 /* D3DFMT_X8R8G8B8 */, 0 /* D3DPOOL_DEFAULT */, &splash, 0); // CreateOffscreenPlainSurface
        if (hr >= 0) {
            hr = D3DXLoadSurfaceFromResourceA((LPDIRECT3DSURFACE9)splash, 0, 0, (HMODULE)shell_module_handle, MAKEINTRESOURCEA(0x86), 0,
                                              0xffffffff /* D3DX_DEFAULT */, 0, 0);
            if (hr >= 0) {
                vtable = *(void ***)rasterizer_device;
                ((d3d_device_call2_fn)vtable[0x98 / 4])(rasterizer_device, 0, &render_target); // GetRenderTarget(0)

                vtable = *(void ***)rasterizer_device;
                ((d3d_device_call5_fn)vtable[0x88 / 4])(rasterizer_device, (uint32_t)splash, 0, render_target, 0,
                                                        0 /* D3DTEXF_NONE */); // StretchRect
                rasterizer_capture_and_present((const int16_t *)0, (BitmapData *)0); // EAX = 0, push 0

                // the same copy again, so both swap-chain buffers show the splash
                vtable = *(void ***)rasterizer_device;
                ((d3d_device_call5_fn)vtable[0x88 / 4])(rasterizer_device, (uint32_t)splash, 0, render_target, 0, 0);

                vtable = *(void ***)render_target;
                ((d3d_release_fn)vtable[2])(render_target);
            }
            vtable = *(void ***)splash;
            ((d3d_release_fn)vtable[2])(splash);
            if (hr >= 0) {
                return;
            }
        }
    }

    // mode 0, or the splash could not be shown: Clear(0, NULL, target|zbuffer|stencil, black, z 1.0, stencil 0)
    // around a present
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
