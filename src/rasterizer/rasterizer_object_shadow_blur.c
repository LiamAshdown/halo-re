// rasterizer_object_shadow_blur  (Ghidra: FUN_00530830; the phase 3 rewrite called it
//   rasterizer_motion_sensor_hud_panel_build)
// address 0x530830, size 1973 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: raw disassembly (phase 4 review). The only caller is
//   rasterizer_object_shadow_structure_draw 0x531570, which runs it once per shadow (while
//   0x0071d264 is clear) when the blur debug toggle 0x0068941e is set, and then binds render
//   target 4 instead of 3. The shadow family is reached from the object shadow code
//   (0x50f830 in render, 0x4d72a0 in model draw, 0x552b40 through the BSP polygon callback
//   0x511f50), not from the motion sensor; the motion sensor draws are 0x52b690 / 0x52bad0 /
//   0x52bc40.
// What it does: binds the silhouette target 3 to the four Texture handles of effect 45 with
//   clamp / linear / point-mip sampling, uploads four +-1/256 texel offset pairs as c13..c20,
//   points the device at target 4 (full-surface viewport), draws the full screen quad at
//   0x006e1b80 through every pass of effect 45, then draws a one texel border of eight
//   pre-transformed line vertices (FVF 0x144, 0x006e1be0) in black around the 128 x 128
//   target so that the projected shadow fades to nothing at the edge.
// register convention: none.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing;       // 0x0069c680
extern uint8_t unknown_0069c689;                            // 0x0069c689
extern uint8_t console_debug_toggle_6893f2;                 // 0x006893f2 object shadows enabled
extern uint8_t console_debug_toggle_68941e;                 // 0x0068941e object shadow blur enabled
extern int16_t rasterizer_active_render_target;             // 0x0069d350
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_dynamic_screen_vertex rasterizer_object_shadow_blur_quad[4];   // 0x006e1b80
extern rasterizer_screen_vertex rasterizer_object_shadow_border_lines[8];        // 0x006e1be0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *surface, d3d_surface_desc *desc);
typedef int32_t (__stdcall *d3d_set_viewport_fn)(void *self, const d3d_viewport *viewport);
typedef int32_t (__stdcall *d3dx_effect_set_texture_fn)(void *effect, uint32_t handle, uint32_t texture);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}
static void set_quad_vertex(int i, float x, float y, float u, float v)
{
    rasterizer_object_shadow_blur_quad[i].x = x;
    rasterizer_object_shadow_blur_quad[i].y = y;
    rasterizer_object_shadow_blur_quad[i].z = 0.0f;
    rasterizer_object_shadow_blur_quad[i].color = 0;
    rasterizer_object_shadow_blur_quad[i].u = u;
    rasterizer_object_shadow_blur_quad[i].v = v;
}
static void set_line_vertex(int i, float x, float y)
{
    rasterizer_object_shadow_border_lines[i].x = x;
    rasterizer_object_shadow_border_lines[i].y = y;
    rasterizer_object_shadow_border_lines[i].z = 0.0f;
    rasterizer_object_shadow_border_lines[i].rhw = 1.0f;
    rasterizer_object_shadow_border_lines[i].diffuse = 0;
    rasterizer_object_shadow_border_lines[i].u = 0.0f;
    rasterizer_object_shadow_border_lines[i].v = 0.0f;
}

void rasterizer_object_shadow_blur(void)
{
    // c13..c20: x / y selectors with a -+1/256 texel offset each (0x3b800000 = 1/256)
    static const float k_offsets[8][4] = {
        { 1.0f, 0.0f, 0.0f, -0.00390625f }, { 0.0f, 1.0f, 0.0f, -0.00390625f },
        { 1.0f, 0.0f, 0.0f,  0.00390625f }, { 0.0f, 1.0f, 0.0f,  0.00390625f },
        { 1.0f, 0.0f, 0.0f, -0.00390625f }, { 0.0f, 1.0f, 0.0f,  0.00390625f },
        { 1.0f, 0.0f, 0.0f,  0.00390625f }, { 0.0f, 1.0f, 0.0f, -0.00390625f },
    };
    void *effect;
    void *surface;
    d3d_surface_desc desc;
    d3d_viewport viewport;
    uint32_t passes;
    uint32_t stage;
    uint32_t pass;

    if (unknown_0069c689 != 0 || console_debug_toggle_6893f2 == 0 || console_debug_toggle_68941e == 0) {
        return;
    }
    effect = (void *)(uintptr_t)rasterizer_effects[45].effect;
    if (effect == NULL) {
        return;
    }
    for (stage = 0; stage < 4; stage++) {
        effect = (void *)(uintptr_t)rasterizer_effects[45].effect;
        ((d3dx_effect_set_texture_fn)(*(void ***)effect)[0xd0 / 4])(effect, rasterizer_effects[45].texture_handles[stage],
                                                                    rasterizer_render_targets[3].texture);
        set_sampler_state(stage, 1, 3);  // ADDRESSU CLAMP
        set_sampler_state(stage, 2, 3);  // ADDRESSV CLAMP
        set_sampler_state(stage, 5, 2);  // MAGFILTER LINEAR
        set_sampler_state(stage, 6, 2);  // MINFILTER LINEAR
        set_sampler_state(stage, 7, 1);  // MIPFILTER POINT
    }
    set_render_state(0x16, 3);           // CULLMODE CCW
    set_render_state(0xa8, 7);           // COLORWRITEENABLE rgb
    set_render_state(0x1b, 0);           // ALPHABLENDENABLE
    set_render_state(0x0f, 0);           // ALPHATESTENABLE
    set_render_state(0x07, 0);           // ZENABLE
    set_render_state(0x1c, 0);           // FOGENABLE
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &k_offsets[0][0], 8);

    surface = (void *)(uintptr_t)rasterizer_render_targets[4].surface;
    ((d3d_call2_fn)device_vtable()[0x94 / 4])(rasterizer_device, 0, (uint32_t)(uintptr_t)surface);
    rasterizer_active_render_target = 4;
    ((d3d_get_desc_fn)(*(void ***)surface)[0x30 / 4])(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    ((d3d_set_viewport_fn)device_vtable()[0xbc / 4])(rasterizer_device, &viewport);

    // half texel shifted full-target quad (0xbf810000 = -1.0078125, 0x3f7e0000 = 0.9921875)
    set_quad_vertex(0, -1.0078125f, 1.0078125f, 0.0f, 0.0f);
    set_quad_vertex(1, 0.9921875f, 1.0078125f, 1.0f, 0.0f);
    set_quad_vertex(2, 0.9921875f, -0.9921875f, 1.0f, 1.0f);
    set_quad_vertex(3, -1.0078125f, -0.9921875f, 0.0f, 1.0f);
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
                                               rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                               ((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[0].shader);
    effect = (void *)(uintptr_t)rasterizer_effects[45].effect;
    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        effect = (void *)(uintptr_t)rasterizer_effects[45].effect;
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, rasterizer_object_shadow_blur_quad,
                                                               sizeof(rasterizer_dynamic_screen_vertex));
    }
    effect = (void *)(uintptr_t)rasterizer_effects[45].effect;
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);

    set_render_state(0xa8, 7);
    set_render_state(0x1b, 0);
    set_render_state(0x0f, 0);
    // four black lines just outside / on the edge of the 128 x 128 target (a line list)
    set_line_vertex(0, -1.0f, 0.0f);
    set_line_vertex(1, 128.0f, 0.0f);
    set_line_vertex(2, 127.0f, -1.0f);
    set_line_vertex(3, 127.0f, 128.0f);
    set_line_vertex(4, 128.0f, 127.0f);
    set_line_vertex(5, -1.0f, 127.0f);
    set_line_vertex(6, 0.0f, 128.0f);
    set_line_vertex(7, 0.0f, -1.0f);
    set_texture_stage_state(0, 1, 2);    // COLOROP SELECTARG1
    set_texture_stage_state(0, 2, 0);    // COLORARG1 DIFFUSE
    set_texture_stage_state(0, 4, 2);    // ALPHAOP SELECTARG1
    set_texture_stage_state(0, 5, 0);    // ALPHAARG1 DIFFUSE
    set_texture_stage_state(1, 1, 1);    // COLOROP DISABLE
    set_texture_stage_state(1, 4, 1);    // ALPHAOP DISABLE
    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);   // SetPixelShader
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0);   // SetVertexShader
    ((d3d_call1_fn)device_vtable()[0x164 / 4])(rasterizer_device, 0x144); // SetFVF
    ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 2, 4, rasterizer_object_shadow_border_lines,
                                                           sizeof(rasterizer_screen_vertex));
    ((d3d_call1_fn)device_vtable()[0x164 / 4])(rasterizer_device, 0);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x530830): phase 3 file rasterizer_motion_sensor_hud_panel_build replaced

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00530830(void)

{
  uint uVar1;
  int iVar2;
  int *piVar3;
  uint uVar4;
  undefined4 *puVar5;
  int *piStack_148;
  undefined4 uStack_144;
  int *piStack_140;
  int **ppiStack_13c;
  int *piStack_138;
  undefined4 *puStack_134;
  int *piStack_130;
  undefined4 uStack_12c;
  int *piStack_128;
  int *piStack_124;
  undefined4 uStack_120;
  int **ppiStack_11c;
  int *piStack_118;
  int *piStack_114;
  undefined4 uStack_110;
  undefined4 uStack_10c;
  int *piStack_108;
  int *piStack_104;
  undefined4 uStack_100;
  int *piStack_fc;
  int *piStack_f8;
  int *piStack_f4;
  int *piStack_f0;
  undefined4 uStack_ec;
  int *piStack_e8;
  int *piStack_e4;
  undefined4 uStack_e0;
  undefined4 uStack_dc;
  int *piStack_d8;
  undefined4 uStack_d4;
  undefined4 uStack_d0;
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  undefined4 uStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  undefined4 uStack_6c;
  
  if ((((DAT_0069c689 == '\0') && (DAT_006893f2 != '\0')) && (DAT_0068941e != '\0')) &&
     (DAT_0069d9b0 != (int *)0x0)) {
    piVar3 = (int *)0x0;
    puVar5 = &DAT_0069d9b8;
    iVar2 = 4;
    do {
      uStack_d0 = DAT_0069d3a4;
      uStack_d4 = *puVar5;
      piStack_d8 = DAT_0069d9b0;
      uStack_dc = 0x5308a3;
      (**(code **)(*DAT_0069d9b0 + 0xd0))();
      uStack_dc = 3;
      uStack_e0 = 1;
      piStack_e8 = DAT_0071d174;
      uStack_ec = 0x5308b6;
      piStack_e4 = piVar3;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_ec = 3;
      piStack_f0 = (int *)0x2;
      piStack_f8 = DAT_0071d174;
      piStack_fc = (int *)0x5308c9;
      piStack_f4 = piVar3;
      (**(code **)(*DAT_0071d174 + 0x114))();
      piStack_fc = (int *)0x2;
      uStack_100 = 5;
      piStack_108 = DAT_0071d174;
      uStack_10c = 0x5308dc;
      piStack_104 = piVar3;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_10c = 2;
      uStack_110 = 6;
      piStack_118 = DAT_0071d174;
      ppiStack_11c = (int **)0x5308ef;
      piStack_114 = piVar3;
      (**(code **)(*DAT_0071d174 + 0x114))();
      ppiStack_11c = (int **)0x1;
      uStack_120 = 7;
      piStack_128 = DAT_0071d174;
      uStack_12c = 0x530902;
      piStack_124 = piVar3;
      (**(code **)(*DAT_0071d174 + 0x114))();
      piVar3 = (int *)((int)piVar3 + 1);
      puVar5 = puVar5 + 1;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
    uStack_d0 = 3;
    uStack_d4 = 0x16;
    piStack_d8 = DAT_0071d174;
    uStack_dc = 0x53091b;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_dc = 7;
    uStack_e0 = 0xa8;
    piStack_e4 = DAT_0071d174;
    piStack_e8 = (int *)0x530930;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    piStack_e8 = (int *)0x0;
    uStack_ec = 0x1b;
    piStack_f0 = DAT_0071d174;
    piStack_f4 = (int *)0x530941;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    piStack_f4 = (int *)0x0;
    piStack_f8 = (int *)0xf;
    piStack_fc = DAT_0071d174;
    uStack_100 = 0x530952;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_100 = 0;
    piStack_104 = (int *)0x7;
    piStack_108 = DAT_0071d174;
    uStack_10c = 0x530963;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_10c = 0;
    uStack_110 = 0x1c;
    piStack_114 = DAT_0071d174;
    piStack_118 = (int *)0x530974;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    piStack_118 = (int *)0x8;
    ppiStack_11c = &piStack_e8;
    uStack_120 = 0xd;
    piStack_e8 = (int *)0x3f800000;
    piStack_e4 = (int *)0x0;
    uStack_e0 = 0;
    uStack_dc = 0xbb800000;
    piStack_d8 = (int *)0x0;
    uStack_d4 = 0x3f800000;
    uStack_d0 = 0;
    uStack_84 = 0;
    uStack_80 = 0;
    uStack_7c = 0x3b800000;
    uStack_78 = 0;
    uStack_74 = 0x3f800000;
    uStack_70 = 0;
    uStack_6c = 0xbb800000;
    piStack_124 = DAT_0071d174;
    piStack_128 = (int *)0x530ab5;
    (**(code **)(*DAT_0071d174 + 0x178))();
    piVar3 = DAT_0069d3b4;
    piStack_128 = DAT_0069d3b4;
    uStack_12c = 0;
    piStack_130 = DAT_0071d174;
    puStack_134 = (undefined4 *)0x530acb;
    (**(code **)(*DAT_0071d174 + 0x94))();
    puStack_134 = &uStack_84;
    piStack_138 = piVar3;
    _DAT_0069d350 = 4;
    ppiStack_13c = (int **)0x530ae2;
    (**(code **)(*piVar3 + 0x30))();
    ppiStack_11c = (int **)uStack_74;
    piStack_118 = (int *)uStack_70;
    ppiStack_13c = &piStack_124;
    piStack_124 = (int *)0x0;
    uStack_120 = 0;
    piStack_114 = (int *)0x0;
    uStack_110 = 0x3f800000;
    piStack_140 = DAT_0071d174;
    uStack_144 = 0x530b23;
    (**(code **)(*DAT_0071d174 + 0xbc))();
    uStack_144 = DAT_006e1af0;
    _DAT_006e1b8c = 0;
    _DAT_006e1b90 = 0;
    _DAT_006e1b94 = 0;
    _DAT_006e1b80 = 0xbf810000;
    _DAT_006e1b84 = 0x3f810000;
    _DAT_006e1ba4 = 0;
    _DAT_006e1ba8 = 0x3f800000;
    _DAT_006e1bac = 0;
    _DAT_006e1b98 = 0x3f7e0000;
    _DAT_006e1b9c = 0x3f810000;
    _DAT_006e1bbc = 0;
    _DAT_006e1bc0 = 0x3f800000;
    _DAT_006e1bc4 = 0x3f800000;
    _DAT_006e1bb0 = 0x3f7e0000;
    _DAT_006e1bb4 = 0xbf7e0000;
    _DAT_006e1bd4 = 0;
    _DAT_006e1bd8 = 0;
    _DAT_006e1bdc = 0x3f800000;
    _DAT_006e1bc8 = 0xbf810000;
    _DAT_006e1bcc = 0xbf7e0000;
    _DAT_006e1bd0 = 0;
    _DAT_006e1bb8 = 0;
    _DAT_006e1ba0 = 0;
    _DAT_006e1b88 = 0;
    piStack_148 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10);
    uVar1 = DAT_0069e350;
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174);
    (**(code **)(*DAT_0069d9b0 + 0x100))(DAT_0069d9b0,&piStack_148,3);
    uVar4 = 0;
    if (uVar1 != 0) {
      do {
        (**(code **)(*DAT_0069d9b0 + 0x104))(DAT_0069d9b0,uVar4);
        (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1b80,0x18);
        uVar4 = uVar4 + 1;
      } while (uVar4 < uVar1);
    }
    (**(code **)(*DAT_0069d9b0 + 0x108))(DAT_0069d9b0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
    _DAT_006e1bf4 = 0;
    _DAT_006e1bf8 = 0;
    _DAT_006e1be0 = 0xbf800000;
    _DAT_006e1be4 = 0;
    _DAT_006e1c10 = 0;
    _DAT_006e1c14 = 0;
    _DAT_006e1bfc = 0x43000000;
    _DAT_006e1c00 = 0;
    _DAT_006e1c2c = 0;
    _DAT_006e1c30 = 0;
    _DAT_006e1c18 = 0x42fe0000;
    _DAT_006e1c1c = 0xbf800000;
    _DAT_006e1c48 = 0;
    _DAT_006e1c4c = 0;
    _DAT_006e1c34 = 0x42fe0000;
    _DAT_006e1c38 = 0x43000000;
    _DAT_006e1c64 = 0;
    _DAT_006e1c68 = 0;
    _DAT_006e1c50 = 0x43000000;
    _DAT_006e1c54 = 0x42fe0000;
    _DAT_006e1c80 = 0;
    _DAT_006e1c84 = 0;
    _DAT_006e1c6c = 0xbf800000;
    _DAT_006e1c70 = 0x42fe0000;
    _DAT_006e1c9c = 0;
    _DAT_006e1ca0 = 0;
    _DAT_006e1c88 = 0;
    _DAT_006e1c8c = 0x43000000;
    _DAT_006e1cb8 = 0;
    _DAT_006e1cbc = 0;
    _DAT_006e1ca4 = 0;
    _DAT_006e1ca8 = 0xbf800000;
    _DAT_006e1c44 = 0;
    _DAT_006e1c28 = 0;
    _DAT_006e1c0c = 0;
    _DAT_006e1bf0 = 0;
    _DAT_006e1c3c = 0;
    _DAT_006e1c20 = 0;
    _DAT_006e1c04 = 0;
    _DAT_006e1be8 = 0;
    _DAT_006e1c40 = 0x3f800000;
    _DAT_006e1c24 = 0x3f800000;
    _DAT_006e1c08 = 0x3f800000;
    _DAT_006e1bec = 0x3f800000;
    _DAT_006e1cb4 = 0;
    _DAT_006e1c98 = 0;
    _DAT_006e1c7c = 0;
    _DAT_006e1c60 = 0;
    _DAT_006e1cac = 0;
    _DAT_006e1c90 = 0;
    _DAT_006e1c74 = 0;
    _DAT_006e1c58 = 0;
    _DAT_006e1cb0 = 0x3f800000;
    _DAT_006e1c94 = 0x3f800000;
    _DAT_006e1c78 = 0x3f800000;
    _DAT_006e1c5c = 0x3f800000;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,0);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,0);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
    (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,0);
    (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0x144);
    (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,2,4,&DAT_006e1be0,0x1c);
    (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0);
    (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  }
  return;
}
#endif
