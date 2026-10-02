// rasterizer_render_target_initialize  (Ghidra: FUN_0052ca20, unnamed)
// address 0x52ca20, size 547 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: rebuilt from the raw disassembly (Ghidra swapped the two GetRenderTarget branches
//   and lost the CreateTexture / Lock arguments). Callers: rasterizer_device_reset 0x515d90 and
//   rasterizer_initialize_direct3d 0x5169c0. It records the device back buffer as render target
//   0 (or as target 1 when 0x0069c68a is set), sizes target 1 like it (A8R8G8B8) and target 2 at
//   half size, creates a render target texture and its level 0 surface for every entry of
//   targets 1..8 that has no surface yet, then the shared four index quad index buffer
//   (0x0071d208: 0, 1, 2, 3) and the four vertex FVF 0x144 dynamic vertex buffer (0x0071d20c).
// register convention: none. Returns 1 when every device call succeeded.
// blam-cc: none
// UNSURE: 0x0069c68a is a capability byte (the back buffer itself serves as target 1 and no A8R8G8B8
//   copy is created); its writer is rasterizer_initialize_direct3d.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                             // 0x0071d174
extern uint8_t rasterizer_caps_flag_68a;                    // 0x0069c68a
extern uint8_t rasterizer_caps_flag_689;                            // 0x0069c689
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern void *rasterizer_render_target_index_buffer;         // 0x0071d208
extern void *rasterizer_render_target_vertex_buffer;        // 0x0071d20c

extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70
extern uint32_t __stdcall D3DXGetFVFVertexSize(uint32_t fvf); // 0x583b17 (d3dx9 static library)

typedef int32_t (__stdcall *d3d_get_render_target_fn)(void *self, uint32_t index, uint32_t *surface);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *surface, d3d_surface_desc *desc);
typedef int32_t (__stdcall *d3d_create_texture_fn)(void *self, uint32_t width, uint32_t height, uint32_t levels, uint32_t usage,
                                         uint32_t format, uint32_t pool, uint32_t *texture, void *shared);
typedef int32_t (__stdcall *d3d_get_surface_level_fn)(void *texture, uint32_t level, uint32_t *surface);
typedef int32_t (__stdcall *d3d_create_buffer_fn)(void *self, uint32_t length, uint32_t usage, uint32_t format_or_fvf,
                                        uint32_t pool, void **buffer, void *shared);
typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **data, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_fn)(void *self);

uint8_t rasterizer_render_target_initialize(void)
{
    void **device_vt = *(void ***)rasterizer_device;
    uint8_t ok = 1;
    d3d_surface_desc desc;
    uint16_t *indices;
    int i;

    if (rasterizer_caps_flag_68a) {
        if (((d3d_get_render_target_fn)device_vt[0x98 / 4])(rasterizer_device, 0, &rasterizer_render_targets[1].surface) < 0) {
            ok = 0;
        }
        if (((d3d_get_desc_fn)(*(void ***)(uintptr_t)rasterizer_render_targets[1].surface)[0x30 / 4])(
                (void *)(uintptr_t)rasterizer_render_targets[1].surface, &desc) < 0) {
            ok = 0;
        }
        rasterizer_render_targets[1].format = desc.format;
        rasterizer_render_targets[1].width = desc.width;
        rasterizer_render_targets[1].height = desc.height;
    } else {
        if (((d3d_get_render_target_fn)device_vt[0x98 / 4])(rasterizer_device, 0, &rasterizer_render_targets[0].surface) < 0) {
            ok = 0;
        }
        if (((d3d_get_desc_fn)(*(void ***)(uintptr_t)rasterizer_render_targets[0].surface)[0x30 / 4])(
                (void *)(uintptr_t)rasterizer_render_targets[0].surface, &desc) < 0) {
            ok = 0;
        }
        rasterizer_render_targets[0].format = desc.format;
        rasterizer_render_targets[0].width = desc.width;
        rasterizer_render_targets[1].width = desc.width;
        rasterizer_render_targets[0].height = desc.height;
        rasterizer_render_targets[1].height = desc.height;
        rasterizer_render_targets[1].format = 0x15;              // D3DFMT_A8R8G8B8
        rasterizer_render_targets[2].width = desc.width >> 1;
        rasterizer_render_targets[2].height = desc.height >> 1;
    }

    if (!rasterizer_caps_flag_689 && ok) {
        for (i = 1; i < k_rasterizer_render_targets; i++) {
            rasterizer_render_target *target = &rasterizer_render_targets[i];

            if (target->surface == 0 && !(rasterizer_caps_flag_68a && target->format == 0x15)) {
                if (((d3d_create_texture_fn)device_vt[0x5c / 4])(rasterizer_device, target->width, target->height, 1,
                                                                  1 /* D3DUSAGE_RENDERTARGET */, target->format,
                                                                  0 /* D3DPOOL_DEFAULT */, &target->texture, NULL) < 0) {
                    ok = 0;
                }
                if (target->texture == 0) {
                    shell_display_fatal_error_dialog(0x69, 0x72, 1);
                }
                if (((d3d_get_surface_level_fn)(*(void ***)(uintptr_t)target->texture)[0x48 / 4])(
                        (void *)(uintptr_t)target->texture, 0, &target->surface) < 0) {
                    ok = 0;
                }
            }
            if (!ok) {
                break;
            }
        }
    }

    if (((d3d_create_buffer_fn)device_vt[0x6c / 4])(rasterizer_device, 8, 8 /* WRITEONLY */, 0x65 /* INDEX16 */, 0,
                                                    &rasterizer_render_target_index_buffer, NULL) < 0) {
        ok = 0;
    }
    if (rasterizer_render_target_index_buffer == NULL) {
        return 0;
    }
    indices = NULL;
    if (((d3d_lock_fn)(*(void ***)rasterizer_render_target_index_buffer)[0x2c / 4])(
            rasterizer_render_target_index_buffer, 0, 8, (void **)&indices, 0) < 0) {
        ok = 0;
    }
    if (indices != NULL) {
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 3;
        if (((d3d_unlock_fn)(*(void ***)rasterizer_render_target_index_buffer)[0x30 / 4])(
                rasterizer_render_target_index_buffer) < 0) {
            return 0;
        }
    }
    if (ok) {
        if (((d3d_create_buffer_fn)device_vt[0x68 / 4])(rasterizer_device, D3DXGetFVFVertexSize(0x144) * 4,
                                                        0x208 /* WRITEONLY | DYNAMIC */, 0x144, 0,
                                                        &rasterizer_render_target_vertex_buffer, NULL) < 0) {
            return 0;
        }
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x52ca20):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_0052ca20(void)

{
  int *piVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  undefined2 *puVar6;
  undefined4 uStack_54;
  int *piStack_50;
  undefined4 uStack_4c;
  uint uStack_1c;
  uint uStack_18;
  
  piVar5 = DAT_0071d174;
  if (DAT_0069c68a == '\0') {
    iVar3 = (**(code **)(*DAT_0071d174 + 0x98))();
    iVar4 = (**(code **)(*DAT_0069d364 + 0x30))();
    bVar2 = -1 < iVar4 && -1 < iVar3;
    _DAT_0069d360 = piVar5;
    _DAT_0069d358 = uStack_1c;
    DAT_0069d380 = uStack_1c >> 1;
    _DAT_0069d35c = uStack_18;
    DAT_0069d384 = uStack_18 >> 1;
    DAT_0069d374 = (int *)&DAT_00000015;
  }
  else {
    iVar3 = (**(code **)(*DAT_0071d174 + 0x98))();
    iVar4 = (**(code **)(*DAT_0069d378 + 0x30))();
    bVar2 = -1 < iVar4 && -1 < iVar3;
    DAT_0069d374 = piVar5;
  }
  DAT_0069d370 = uStack_18;
  DAT_0069d36c = uStack_1c;
  if ((DAT_0069c689 == '\0') && (bVar2)) {
    piVar5 = (int *)&DAT_0069d374;
    do {
      if (0x69d413 < (int)piVar5) break;
      if ((piVar5[1] == 0) && ((DAT_0069c68a == '\0' || (*piVar5 != 0x15)))) {
        uStack_4c = 0;
        piVar1 = piVar5 + 2;
        uStack_54 = 0;
        piStack_50 = piVar1;
        iVar3 = (**(code **)(*DAT_0071d174 + 0x5c))(DAT_0071d174,piVar5[-2],piVar5[-1],1,1,*piVar5);
        if (iVar3 < 0) {
          bVar2 = false;
        }
        if (*piVar1 == 0) {
          shell_display_fatal_error_dialog(0x69,0x72,1);
        }
        iVar3 = (**(code **)(*(int *)*piVar1 + 0x48))((int *)*piVar1,0,piVar5 + 1);
        if (iVar3 < 0) {
          bVar2 = false;
        }
      }
      piVar5 = piVar5 + 5;
    } while (bVar2 != false);
  }
  uStack_4c = 0x65;
  piStack_50 = (int *)0x8;
  uStack_54 = 8;
  iVar3 = (**(code **)(*DAT_0071d174 + 0x6c))(DAT_0071d174);
  if (iVar3 < 0) {
    bVar2 = false;
  }
  if (DAT_0071d208 == (int *)0x0) {
    return false;
  }
  puVar6 = (undefined2 *)0x0;
  uStack_54 = 0;
  iVar3 = (**(code **)(*DAT_0071d208 + 0x2c))(DAT_0071d208,0,8,&uStack_54,0);
  if (iVar3 < 0) {
    bVar2 = false;
  }
  if (puVar6 != (undefined2 *)0x0) {
    *puVar6 = 0;
    puVar6[1] = 1;
    puVar6[2] = 2;
    puVar6[3] = 3;
    iVar3 = (**(code **)(*DAT_0071d208 + 0x30))(DAT_0071d208);
    if (iVar3 < 0) {
      return false;
    }
  }
  if (bVar2 != false) {
    iVar3 = FUN_00583b17(0x144);
    iVar3 = (**(code **)(*DAT_0071d174 + 0x68))
                      (DAT_0071d174,iVar3 << 2,0x208,0x144,0,&DAT_0071d20c,0);
    if (iVar3 < 0) {
      return false;
    }
  }
  return bVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
