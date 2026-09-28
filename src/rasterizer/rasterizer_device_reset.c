// rasterizer_device_reset  (Ghidra: rasterizer_device_reset, already named)
// address 0x515d90, size 547 bytes
// name confidence: 0.55  rewrite confidence: 0.3
// evidence: releases every cached D3D resource this module owns before calling
//   IDirect3DDevice9::Reset (vtable +0x40) and rebuilding render state on success: vertex
//   buffer slots (0x007bf060, stride 0x14 matches rasterizer_vertex_buffer_slot), vertex
//   declarations (0x006e1a90..0x006e1b80, stride 0xc matches rasterizer_vertex_declaration, 20
//   entries), vertex shaders (0x0069e350..0x0069e550, stride 8 matches rasterizer_vertex_shader,
//   64 entries) and lens flare occlusion queries (0x006e1dc8..0x006e2dc8, stride 4, 0x400
//   entries, matches k_lens_flare_occlusion_queries).
// register convention: none recognized in the Ghidra-visible signature, but unaff_EBX (read
//   after the Reset call) is clearly a hidden pointer parameter.
//   // blam-cc: unaff_EBX -> extra_params
// UNSURE, substantially: `puStack_14` is declared by Ghidra as a genuinely uninitialized
//   pointer and is passed directly to Reset(); the D3DPRESENT_PARAMETERS it should point to
//   (almost certainly built by rasterizer_build_present_parameters, which is not in this
//   function's callee list at all) is not constructed anywhere in this decompiled pack. Rather
//   than invent that call, `puStack_14` is kept exactly as Ghidra shows it: declared,
//   uninitialized, and dereferenced. Every `(**(code**)(*ptr+N))(...)` COM call is a vtable slot
//   whose exact D3D method name is not asserted, only its slot and argument count as shown.

// REWRITTEN (first-boot track, objdump 0x515d90..0x515fb2): the present parameters are the stack argument (Ghidra's
//   "uninitialized" puStack_14 is that parameter; there is no EBX input). Also fixed: SetTexture(stage, NULL),
//   SetVertexShader(NULL) and SetPixelShader(NULL) take their NULL argument (the old calls left the __stdcall stack
//   unbalanced); the viewport is the full D3DVIEWPORT9 {0, 0, width, height, 0.0, 1.0}; the splash is redrawn with
//   mode 1; the result is a byte.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern void *rasterizer_device; // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern int32_t rasterizer_vertex_buffer_slot_high_water; // 0x0071d258
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots]; // 0x007bf060
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern void *lens_flare_occlusion_queries[k_lens_flare_occlusion_queries]; // 0x006e1dc8
extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern uint8_t rasterizer_pending_clear; // 0x0071d16e

extern void rasterizer_render_loading_screen(int32_t mode); // 0x5157e0 (this session)
extern void rasterizer_set_default_render_states(void);           // 0x5160d0
extern void rasterizer_editbox_log_dump(void); // 0x5196b0 (this session)
extern void rasterizer_ksml_ui_shutdown(void); // 0x5198a0 (this session)
extern uint8_t rasterizer_render_target_initialize(void);                                  // 0x52ca20 render target pool create
extern void rasterizer_render_target_dispose(void);    // 0x52cc50, outside this session's range
extern void rasterizer_dx9_pixel_shaders_release(void); // 0x52ff60
extern int32_t rasterizer_dx9_effects_initialize(void);  // 0x5300d0
extern void rasterizer_vertex_buffer_slot_recreate_lost(void);    // 0x530690, outside this session's range
extern int32_t rasterizer_lens_flare_occlusion_queries_create(void); // 0x536f70
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70
extern void __stdcall Sleep(uint32_t ms); // Win32

typedef int32_t (__stdcall *d3d_set_software_vertex_processing_fn)(void *device, int32_t software); // +0x134
typedef int32_t (__stdcall *d3d_set_texture_fn)(void *device, uint32_t stage, void *texture);          // +0x104
typedef int32_t (__stdcall *d3d_set_shader_fn)(void *device, void *shader);                          // +0x170, +0x1ac
typedef int32_t (__stdcall *d3d_reset_fn)(void *device, void *present_params);                       // +0x40
typedef int32_t (__stdcall *d3d_set_viewport_fn)(void *device, const void *viewport);                // +0xbc
typedef int32_t (__stdcall *d3d_release_fn)(void *object);

typedef struct d3d_viewport9 {
    uint32_t x, y, width, height;
    float min_z, max_z;
} d3d_viewport9;

// stack -> present_parameters (the D3DPRESENT_PARAMETERS to reset with; rasterizer_build_present_parameters fills it)
// Unbinds textures and shaders, releases every D3D resource the rasterizer created (vertex buffers, render
// targets, vertex declarations, vertex and pixel shaders, occlusion queries), resets the device, then restores the
// state: present parameters copied, a full viewport, default render states, the splash drawn, effects / render
// targets / occlusion queries recreated and the lost vertex buffers rebuilt. Returns 1 when all of it worked. A
// failed reset shows the fatal error dialog for D3DERR_DRIVERINTERNALERROR, otherwise sleeps 50 ms and returns 0.
uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters)
{
    void **vtable;
    uint32_t i;
    int32_t hr;
    uint8_t ok;
    d3d_viewport9 viewport;

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_software_vertex_processing_fn)vtable[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
    for (i = 0; i < rasterizer_caps.max_simultaneous_textures; i++) {
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_texture_fn)vtable[0x104 / 4])(rasterizer_device, i, 0);
    }
    vtable = *(void ***)rasterizer_device;
    ((d3d_set_shader_fn)vtable[0x170 / 4])(rasterizer_device, 0); // SetVertexShader(NULL)
    vtable = *(void ***)rasterizer_device;
    ((d3d_set_shader_fn)vtable[0x1ac / 4])(rasterizer_device, 0); // SetPixelShader(NULL)

    rasterizer_ksml_ui_shutdown();

    for (i = 0; i < (uint32_t)rasterizer_vertex_buffer_slot_high_water; i++) {
        void *buffer = (void *)rasterizer_vertex_buffer_slots[i].hardware_buffer;
        if (buffer != 0) {
            ((d3d_release_fn)(*(void ***)buffer)[2])(buffer);
            rasterizer_vertex_buffer_slots[i].hardware_buffer = 0;
        }
    }
    rasterizer_render_target_dispose();
    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        void *declaration = (void *)rasterizer_vertex_declarations[i].declaration;
        if (declaration != 0) {
            ((d3d_release_fn)(*(void ***)declaration)[2])(declaration);
        }
    }
    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        rasterizer_vertex_declarations[i].declaration = 0;
        rasterizer_vertex_declarations[i].fvf = 0;
        rasterizer_vertex_declarations[i].usage = 0;
    }
    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        void *shader = (void *)rasterizer_vertex_shaders[i].shader;
        if (shader != 0) {
            ((d3d_release_fn)(*(void ***)shader)[2])(shader);
            rasterizer_vertex_shaders[i].shader = 0;
        }
    }
    rasterizer_dx9_pixel_shaders_release();
    for (i = 0; i < k_lens_flare_occlusion_queries; i++) {
        void *query = lens_flare_occlusion_queries[i];
        if (query != 0) {
            ((d3d_release_fn)(*(void ***)query)[2])(query);
            lens_flare_occlusion_queries[i] = 0;
        }
    }

    vtable = *(void ***)rasterizer_device;
    hr = ((d3d_reset_fn)vtable[0x40 / 4])(rasterizer_device, present_parameters);
    if (hr < 0 || rasterizer_device == 0) {
        if (hr == (int32_t)0x88760827) { // D3DERR_DRIVERINTERNALERROR
            shell_display_fatal_error_dialog(0x81, 0x82, 1);
            return 0;
        }
        Sleep(0x32);
        return 0;
    }

    rasterizer_present_parameters = *present_parameters; // 0xe dwords
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = ((uint32_t *)present_parameters)[0];  // BackBufferWidth
    viewport.height = ((uint32_t *)present_parameters)[1]; // BackBufferHeight
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    vtable = *(void ***)rasterizer_device;
    ok = ((d3d_set_viewport_fn)vtable[0xbc / 4])(rasterizer_device, &viewport) >= 0;
    rasterizer_pending_clear = 0;
    rasterizer_set_default_render_states();
    rasterizer_render_loading_screen(1); // 0x515f29: mov eax,1

    if (ok && rasterizer_dx9_effects_initialize() && rasterizer_render_target_initialize() &&
        rasterizer_lens_flare_occlusion_queries_create()) {
        ok = 1;
    } else {
        ok = 0;
    }
    rasterizer_vertex_buffer_slot_recreate_lost();
    rasterizer_editbox_log_dump();
    return ok;
}

#if 0
Original Ghidra decompilation (0x515d90):

undefined4 rasterizer_device_reset(void)

{
  int *piVar1;
  char cVar2;
  bool bVar3;
  int iVar4;
  undefined4 *unaff_EBX;
  uint uVar5;
  int *piVar6;
  undefined4 *puVar7;
  int *piStack_38;
  int *piStack_34;
  undefined4 uStack_30;
  int *piStack_2c;
  uint uStack_28;
  undefined4 *puStack_14;

  uStack_28 = (uint)DAT_0069c680;
  piStack_2c = DAT_0071d174;
  uStack_30 = 0x515dac;
  (**(code **)(*DAT_0071d174 + 0x134))();
  uVar5 = 0;
  if (DAT_007c1158 != 0) {
    do {
      uStack_30 = 0;
      piStack_38 = DAT_0071d174;
      piStack_34 = (int *)uVar5;
      (**(code **)(*DAT_0071d174 + 0x104))();
      uVar5 = uVar5 + 1;
    } while (uVar5 < DAT_007c1158);
  }
  uStack_30 = 0;
  piStack_34 = DAT_0071d174;
  piStack_38 = (int *)0x515de2;
  (**(code **)(*DAT_0071d174 + 0x170))();
  piStack_38 = (int *)0x0;
  (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174);
  rasterizer_ksml_ui_shutdown();
  uVar5 = 0;
  if (DAT_0071d258 != 0) {
    piVar6 = &DAT_007bf060;
    do {
      piVar1 = (int *)*piVar6;
      if (piVar1 != (int *)0x0) {
        (**(code **)(*piVar1 + 8))(piVar1);
        *piVar6 = 0;
      }
      uVar5 = uVar5 + 1;
      piVar6 = piVar6 + 5;
    } while (uVar5 < DAT_0071d258);
  }
  FUN_0052cc50();
  piVar6 = &DAT_006e1a90;
  do {
    piVar1 = (int *)*piVar6;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
    }
    piVar6 = piVar6 + 3;
  } while ((int)piVar6 < 0x6e1b80);
  puVar7 = &DAT_006e1a90;
  for (iVar4 = 0x3c; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  piVar6 = &DAT_0069e350;
  do {
    piVar1 = (int *)*piVar6;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *piVar6 = 0;
    }
    piVar6 = piVar6 + 2;
  } while ((int)piVar6 < 0x69e550);
  rasterizer_dx9_pixel_shaders_release();
  piVar6 = &DAT_006e1dc8;
  do {
    piVar1 = (int *)*piVar6;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *piVar6 = 0;
    }
    piVar6 = piVar6 + 1;
  } while ((int)piVar6 < 0x6e2dc8);
  iVar4 = (**(code **)(*DAT_0071d174 + 0x40))(DAT_0071d174,puStack_14);
  piVar6 = DAT_0071d174;
  if ((-1 < iVar4) && (DAT_0071d174 != (int *)0x0)) {
    puVar7 = &DAT_007c04a0;
    for (iVar4 = 0xe; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar7 = *puStack_14;
      puStack_14 = puStack_14 + 1;
      puVar7 = puVar7 + 1;
    }
    uStack_30 = *unaff_EBX;
    piStack_2c = (int *)unaff_EBX[1];
    piStack_38 = (int *)0x0;
    piStack_34 = (int *)0x0;
    uStack_28 = 0;
    iVar4 = (**(code **)(*piVar6 + 0xbc))(piVar6,&piStack_38);
    DAT_0071d16e = 0;
    rasterizer_set_default_render_states();
    FUN_005157e0();
    if ((((-1 < iVar4) && (cVar2 = rasterizer_dx9_effects_initialize(), cVar2 != '\0')) &&
        (cVar2 = FUN_0052ca20(), cVar2 != '\0')) &&
       (bVar3 = rasterizer_lens_flare_occlusion_queries_create(), bVar3)) {
      FUN_00530690();
      rasterizer_editbox_log_dump();
      return 1;
    }
    FUN_00530690();
    rasterizer_editbox_log_dump();
    return 0;
  }
  if (iVar4 == -0x7789f7d9) {
    shell_display_fatal_error_dialog(0x81,0x82,1);
    return 0;
  }
  Sleep(0x32);
  return 0;
}
#endif
