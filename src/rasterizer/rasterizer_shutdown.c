// rasterizer_shutdown  (Ghidra: rasterizer_shutdown, already named)
// address 0x518450, size 370 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: a straightforward, purely mechanical teardown sequence releasing every cached
//   resource this module owns (scratch memory, decal geometry sub-caches, transparent group
//   pools, font atlas, decal static vertex buffer, pixel/vertex shaders, render targets, lens
//   flare occlusion queries, gamma, GDI objects, the window, and finally the D3D device/
//   Direct3D9 object).
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_scratch_memory;      // 0x0071d13c
extern uint32_t rasterizer_scratch_memory_used; // 0x0071d140
extern void *GlobalFree(void *mem); // Win32
extern font_glyph_cache g_font_glyph_cache; // 0x006d8828, see font_glyph_cache_clear_all.c
extern void *rasterizer_device;   // 0x0071d174
extern void *rasterizer_detail_object_vertex_buffer; // 0x0071d1c8
extern void *lens_flare_occlusion_queries[k_lens_flare_occlusion_queries]; // 0x006e1dc8
extern void *rasterizer_window_icon_dc;     // 0x0071d184, see rasterizer_create_game_window.c
extern void *rasterizer_hwnd;               // 0x007461c4
extern void *rasterizer_window_icon_bitmap; // 0x0071d188
extern void *rasterizer_capture_surfaces[4]; // 0x0069c66c
extern void *rasterizer_direct3d; // 0x0071d178

extern void rasterizer_ksml_ui_shutdown(void); // 0x5198a0 (this session)
extern void rasterizer_dynamic_geometry_dispose(void); // 0x51bcd0 (already in tree)
extern void chimera__rasterizer_dispose_free_memory(void); // 0x515430 (this session)
extern void font_glyph_cache_clear_all(void); // 0x514cb0 (this session)
extern void bitmap_data_free(void); // 0x43f880, UNSURE arguments
extern void rasterizer_dx9_shaders_release_all(void); // 0x530090
extern void rasterizer_render_target_dispose(void); // 0x52cc50, outside this session's range
extern void chimera__registry_check_3(void); // 0x5226c0
extern int32_t ReleaseDC(void *hwnd, void *hdc); // Win32
extern int32_t DeleteObject(void *object); // Win32
extern int32_t ShowWindow(void *hwnd, int32_t cmd_show); // Win32
extern int32_t DestroyWindow(void *hwnd); // Win32

// Full rasterizer teardown: releases every cached D3D resource, destroys the window, and
// releases the Direct3D device and Direct3D9 object.
void __cdecl rasterizer_shutdown(void)
{
    void **vtable;
    int32_t i;

    rasterizer_ksml_ui_shutdown();

    if (rasterizer_scratch_memory != (void *)0) {
        GlobalFree(rasterizer_scratch_memory);
    }
    rasterizer_scratch_memory = (void *)0;
    rasterizer_scratch_memory_used = 0;

    rasterizer_dynamic_geometry_dispose();
    chimera__rasterizer_dispose_free_memory();

    if (g_font_glyph_cache.initialized != 0) {
        font_glyph_cache_clear_all();
        bitmap_data_free(); // UNSURE arguments
        g_font_glyph_cache.initialized = 0;
    }

    if (rasterizer_device != (void *)0 && rasterizer_detail_object_vertex_buffer != (void *)0) {
        vtable = *(void ***)rasterizer_detail_object_vertex_buffer;
        ((void (__stdcall *)(void *))vtable[2])(rasterizer_detail_object_vertex_buffer); // Release
        rasterizer_detail_object_vertex_buffer = (void *)0;
    }

    rasterizer_dx9_shaders_release_all();
    rasterizer_render_target_dispose();

    for (i = 0; i < 0x400; i++) {
        if (lens_flare_occlusion_queries[i] != (void *)0) {
            vtable = *(void ***)lens_flare_occlusion_queries[i];
            ((void (__stdcall *)(void *))vtable[2])(lens_flare_occlusion_queries[i]); // Release
            lens_flare_occlusion_queries[i] = (void *)0;
        }
    }

    chimera__registry_check_3();

    if (rasterizer_window_icon_dc != (void *)0) {
        ReleaseDC(rasterizer_hwnd, rasterizer_window_icon_dc);
        rasterizer_window_icon_dc = (void *)0;
    }
    if (rasterizer_window_icon_bitmap != (void *)0) {
        DeleteObject(rasterizer_window_icon_bitmap);
        rasterizer_window_icon_bitmap = (void *)0;
    }
    ShowWindow(rasterizer_hwnd, 0); // SW_HIDE
    DestroyWindow(rasterizer_hwnd);
    rasterizer_hwnd = (void *)0;

    for (i = 0; i < 4; i++) {
        if (rasterizer_capture_surfaces[i] != (void *)0) {
            vtable = *(void ***)rasterizer_capture_surfaces[i];
            ((void (__stdcall *)(void *))vtable[2])(rasterizer_capture_surfaces[i]); // Release
            rasterizer_capture_surfaces[i] = (void *)0;
        }
    }

    if (rasterizer_device != (void *)0) {
        vtable = *(void ***)rasterizer_device;
        ((void (__stdcall *)(void *))vtable[2])(rasterizer_device); // Release
    }
    rasterizer_device = (void *)0;

    if (rasterizer_direct3d != (void *)0) {
        vtable = *(void ***)rasterizer_direct3d;
        ((void (__stdcall *)(void *))vtable[2])(rasterizer_direct3d); // Release
    }
    rasterizer_direct3d = (void *)0;
}

#if 0
Original Ghidra decompilation (0x518450):

void __cdecl rasterizer_shutdown(void)

{
  int *piVar1;
  int *piVar2;

  rasterizer_ksml_ui_shutdown();
  if (DAT_0071d13c != (HGLOBAL)0x0) {
    GlobalFree(DAT_0071d13c);
  }
  DAT_0071d13c = (HGLOBAL)0x0;
  DAT_0071d140 = 0;
  FUN_0051bcd0();
  chimera__rasterizer_dispose_free_memory();
  if (DAT_006d8828 != '\0') {
    font_glyph_cache_clear_all();
    bitmap_group_free();
    DAT_006d8828 = '\0';
  }
  if ((DAT_0071d174 != (int *)0x0) && (DAT_0071d1c8 != (int *)0x0)) {
    (**(code **)(*DAT_0071d1c8 + 8))(DAT_0071d1c8);
    DAT_0071d1c8 = (int *)0x0;
  }
  rasterizer_dx9_shaders_release_all();
  FUN_0052cc50();
  piVar2 = &DAT_006e1dc8;
  do {
    piVar1 = (int *)*piVar2;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *piVar2 = 0;
    }
    piVar2 = piVar2 + 1;
  } while ((int)piVar2 < 0x6e2dc8);
  chimera__registry_check_3();
  if (DAT_0071d184 != (HDC)0x0) {
    ReleaseDC(DAT_007461c4,DAT_0071d184);
    DAT_0071d184 = (HDC)0x0;
  }
  if (DAT_0071d188 != (HGDIOBJ)0x0) {
    DeleteObject(DAT_0071d188);
    DAT_0071d188 = (HGDIOBJ)0x0;
  }
  ShowWindow(DAT_007461c4,0);
  DestroyWindow(DAT_007461c4);
  DAT_007461c4 = (HWND)0x0;
  if (DAT_0069c66c != (int *)0x0) {
    (**(code **)(*DAT_0069c66c + 8))(DAT_0069c66c);
    DAT_0069c66c = (int *)0x0;
  }
  if (DAT_0069c670 != (int *)0x0) {
    (**(code **)(*DAT_0069c670 + 8))(DAT_0069c670);
    DAT_0069c670 = (int *)0x0;
  }
  if (DAT_0069c674 != (int *)0x0) {
    (**(code **)(*DAT_0069c674 + 8))(DAT_0069c674);
    DAT_0069c674 = (int *)0x0;
  }
  if (DAT_0069c678 != (int *)0x0) {
    (**(code **)(*DAT_0069c678 + 8))(DAT_0069c678);
    DAT_0069c678 = (int *)0x0;
  }
  if (DAT_0071d174 != (int *)0x0) {
    (**(code **)(*DAT_0071d174 + 8))(DAT_0071d174);
  }
  DAT_0071d174 = (int *)0x0;
  if (DAT_0071d178 != (int *)0x0) {
    (**(code **)(*DAT_0071d178 + 8))(DAT_0071d178);
  }
  DAT_0071d178 = (int *)0x0;
  return;
}
#endif
