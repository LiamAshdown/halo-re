// rasterizer_screen_effect_render_fixed_function  (Ghidra: chimera__widescreen_screen_effect, a
//   Chimera signature name kept as a hint only)
// address 0x52e2d0, size 2406 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: rebuilt from the raw disassembly (Ghidra lost EAX, the stack frame and every device
//   call argument). first_person_weapon_update_screen_effects 0x494730 calls it with EAX = 0 on
//   the branch that does not call rasterizer_screen_effect_render 0x52d8a0; it uses no effect,
//   only texture stage states, SetTransform and the static quad 0x006e1a30, so it is the fixed
//   function version of the same screen effect. Chimera hooks it for its widescreen fix, hence
//   the old name.
// What it does: with the two video maps (+0x23) it runs the second of (count + 1) * 2 passes
//   only, modulating the frame by +0x28 times +0x34 (DESTCOLOR x ZERO); otherwise, with a mask
//   bitmap and a convolution type, it multiplies the frame by the inverted mask, or by the
//   inverted mask tinted by the desaturation tint and intensity (DESTCOLOR x SRCCOLOR).
// register convention: EAX -> input (handed to cinematic_screen_effect_update 0x512360).
// blam-cc: EAX -> input

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern uint8_t rasterizer_software_vertex_processing;       // 0x0069c680
extern uint8_t console_debug_toggle_689428;                 // 0x00689428 screen effects enabled
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_dynamic_screen_vertex rasterizer_screen_effect_quad[4];        // 0x006e1a30

// blam-cc: EAX -> input, returns EAX
extern weapon_screen_effect_parameters *cinematic_screen_effect_update(weapon_screen_effect_parameters *input); // 0x512360
// blam-cc: ESI -> bitmap, stack -> stage

// blam-cc: ECX -> width, EAX -> height, stack -> (params, pass, pass_count, shift_down)


typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_transform_fn)(void *self, uint32_t state, const float *matrix);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}
static void set_sampler_states(uint32_t sampler, uint32_t address, uint32_t filter, uint32_t mip_filter)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 1, address);
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 2, address);
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 5, filter);
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 6, filter);
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 7, mip_filter);
}
static void set_blend(uint32_t source, uint32_t destination)
{
    set_render_state(0x1b, 1);
    set_render_state(0x13, source);
    set_render_state(0x14, destination);
    set_render_state(0xab, 1);
}
static void draw_screen_quad(void)
{
    ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, rasterizer_screen_effect_quad,
                                                           sizeof(rasterizer_dynamic_screen_vertex));
}

void rasterizer_screen_effect_render_fixed_function(weapon_screen_effect_parameters *input)
{
    weapon_screen_effect_parameters *p;
    uint8_t *raw;
    int32_t width, height;
    int i, j;

    p = cinematic_screen_effect_update(input);
    if (p == NULL) {
        return;
    }
    raw = (uint8_t *)p;
    if (p->convolution_type == 0 && p->mask_bitmap_data == 0 && !(p->night_vision_intensity > 0.0f) &&
        !(p->desaturation_intensity > 0.0f) && raw[0x23] == 0) {
        return;
    }
    if (!console_debug_toggle_689428 || rasterizer_window.type != 1) {
        return;
    }
    width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
    height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;

    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);   // SetPixelShader(NULL)
    set_render_state(0x16, 1);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 0);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 0);
    set_render_state(0x1c, 0);
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
                                               rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                               ((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[0].shader);

    // full screen quad, texture coordinates in pixels (bottom left origin)
    for (i = 0; i < 4; i++) {
        rasterizer_screen_effect_quad[i].z = 0.0f;
        rasterizer_screen_effect_quad[i].color = 0xffffffff;
    }
    rasterizer_screen_effect_quad[0].x = -1.0f;
    rasterizer_screen_effect_quad[0].y = -1.0f;
    rasterizer_screen_effect_quad[0].u = 0.0f;
    rasterizer_screen_effect_quad[0].v = (float)(uint32_t)height;
    rasterizer_screen_effect_quad[1].x = 1.0f;
    rasterizer_screen_effect_quad[1].y = -1.0f;
    rasterizer_screen_effect_quad[1].u = (float)(uint32_t)width;
    rasterizer_screen_effect_quad[1].v = (float)(uint32_t)height;
    rasterizer_screen_effect_quad[2].x = 1.0f;
    rasterizer_screen_effect_quad[2].y = 1.0f;
    rasterizer_screen_effect_quad[2].u = (float)(uint32_t)width;
    rasterizer_screen_effect_quad[2].v = 0.0f;
    rasterizer_screen_effect_quad[3].x = -1.0f;
    rasterizer_screen_effect_quad[3].y = 1.0f;
    rasterizer_screen_effect_quad[3].u = 0.0f;
    rasterizer_screen_effect_quad[3].v = 0.0f;

    if (raw[0x23]) {
        int16_t pass_count = (int16_t)((uint16_t)(p->unknown_00 + 1) << 1);
        int16_t pass;
        float identity[4][4];

        for (pass = 0; pass < pass_count; pass++) {
            if (pass != 1) {
                continue;
            }
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    identity[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
            rasterizer_screen_effect_compute_uv_transform((uint32_t)width, (uint32_t)height, p, 1, pass_count, 1);
            rasterizer_bind_texture_d3d9(0, (BitmapData *)(uintptr_t)*(uint32_t *)(raw + 0x28));
            set_sampler_states(0, 3, 1, 1);    // CLAMP, POINT
            rasterizer_bind_texture_d3d9(1, (BitmapData *)(uintptr_t)*(uint32_t *)(raw + 0x34));
            set_sampler_states(1, 1, 2, 1);    // WRAP, LINEAR
            set_texture_stage_state(0, 1, 2);  // COLOROP SELECTARG1
            set_texture_stage_state(0, 2, 2);  // COLORARG1 TEXTURE
            set_texture_stage_state(0, 4, 2);  // ALPHAOP SELECTARG1
            set_texture_stage_state(0, 5, 2);  // ALPHAARG1 TEXTURE
            set_texture_stage_state(1, 1, 4);  // COLOROP MODULATE
            set_texture_stage_state(1, 2, 2);  // COLORARG1 TEXTURE
            set_texture_stage_state(1, 3, 1);  // COLORARG2 CURRENT
            set_texture_stage_state(1, 4, 2);  // ALPHAOP SELECTARG1
            set_texture_stage_state(1, 5, 1);  // ALPHAARG1 CURRENT
            set_texture_stage_state(2, 1, 1);  // DISABLE
            set_texture_stage_state(2, 4, 1);
            set_blend(9, 1);                   // DESTCOLOR, ZERO
            draw_screen_quad();
            set_texture_stage_state(0, 0x18, 0);   // TEXTURETRANSFORMFLAGS off
            ((d3d_set_transform_fn)device_vtable()[0xb0 / 4])(rasterizer_device, 0x10, &identity[0][0]); // D3DTS_TEXTURE0
            set_texture_stage_state(1, 0x18, 0);
            ((d3d_set_transform_fn)device_vtable()[0xb0 / 4])(rasterizer_device, 0x11, &identity[0][0]); // D3DTS_TEXTURE1
        }
    } else if (p->mask_bitmap_data != 0) {
        BitmapData *mask = (BitmapData *)(uintptr_t)p->mask_bitmap_data;

        set_sampler_states(0, 3, 2, 1);
        set_sampler_states(1, 3, 2, 1);
        rasterizer_screen_effect_compute_uv_transform((uint32_t)width, (uint32_t)height, p, 0, 1, 0);
        if (p->convolution_type != 0) {
            if (p->desaturation_tint[0] == 0.0f && p->desaturation_tint[1] == 0.0f && p->desaturation_tint[2] == 0.0f) {
                set_blend(9, 1);                   // DESTCOLOR, ZERO
                rasterizer_bind_texture_d3d9(0, mask);
                set_texture_stage_state(0, 1, 2);      // COLOROP SELECTARG1
                set_texture_stage_state(0, 2, 0x12);   // COLORARG1 TEXTURE | COMPLEMENT
                set_texture_stage_state(0, 4, 2);
                set_texture_stage_state(0, 5, 2);
                set_texture_stage_state(1, 1, 1);
                set_texture_stage_state(1, 4, 1);  // the binary jumps into the shared tail here
            } else {
                float amount = p->desaturation_intensity;
                float base = (1.0f - amount) * 0.5f;
                uint32_t color;

                set_blend(9, 3);                   // DESTCOLOR, SRCCOLOR
                color = (uint32_t)(int32_t)((p->desaturation_tint[0] * amount + base) * 255.0f) & 0xff;  // __ftol
                color |= (uint32_t)(int32_t)(amount * 255.0f) << 8;
                color <<= 8;
                color |= (uint32_t)(int32_t)((p->desaturation_tint[1] * amount + base) * 255.0f) & 0xff;
                color <<= 8;
                color |= (uint32_t)(int32_t)((p->desaturation_tint[2] * amount + base) * 255.0f) & 0xff;
                set_render_state(0x3c, color);         // TEXTUREFACTOR: alpha amount, rgb tint
                rasterizer_bind_texture_d3d9(0, mask);
                set_texture_stage_state(0, 1, 4);      // COLOROP MODULATE
                set_texture_stage_state(0, 2, 3);      // COLORARG1 TFACTOR
                set_texture_stage_state(0, 3, 0x12);   // COLORARG2 TEXTURE | COMPLEMENT
                set_texture_stage_state(0, 4, 2);
                set_texture_stage_state(0, 5, 1);      // ALPHAARG1 CURRENT
                set_texture_stage_state(1, 1, 1);
                set_texture_stage_state(1, 4, 1);
                set_texture_stage_state(2, 1, 1);
                set_texture_stage_state(2, 4, 1);
            }
            draw_screen_quad();
        }
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x52e2d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chimera__widescreen_screen_effect(void)

{
  int *piVar1;
  int *extraout_EAX;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 *puStack_268;
  int *piStack_264;
  undefined4 uStack_260;
  undefined4 uStack_25c;
  undefined4 uStack_258;
  int *piStack_254;
  undefined4 uStack_250;
  undefined4 uStack_24c;
  undefined *puStack_248;
  undefined4 uStack_244;
  int *piStack_240;
  undefined4 uStack_23c;
  undefined4 uStack_238;
  int *piStack_234;
  undefined4 uStack_230;
  undefined4 uStack_22c;
  int *piStack_228;
  undefined4 uStack_224;
  undefined4 uStack_220;
  int *piStack_21c;
  undefined4 uStack_218;
  undefined4 uStack_214;
  int *piStack_210;
  undefined4 uStack_20c;
  undefined4 uStack_208;
  undefined4 uStack_204;
  int *piStack_200;
  undefined4 uStack_1fc;
  undefined4 uStack_1f8;
  undefined4 uStack_1f4;
  int *piStack_1f0;
  undefined4 uStack_1ec;
  undefined4 uStack_1e8;
  undefined4 uStack_1e4;
  int *piStack_1e0;
  undefined4 uStack_1dc;
  undefined4 uStack_1d8;
  undefined4 uStack_1d4;
  int *piStack_1d0;
  undefined4 uStack_1cc;
  undefined4 uStack_1c8;
  undefined4 uStack_1c4;
  int *piStack_1c0;
  undefined4 uStack_1bc;
  undefined4 uStack_1b8;
  undefined4 uStack_1b4;
  int *piStack_1b0;
  undefined4 uStack_1ac;
  undefined4 uStack_1a8;
  undefined4 uStack_1a4;
  int *piStack_1a0;
  undefined4 uStack_19c;
  undefined4 uStack_198;
  undefined4 uStack_194;
  int *piStack_190;
  undefined4 uStack_18c;
  undefined4 uStack_188;
  undefined4 uStack_184;
  int *piStack_180;
  int *piStack_17c;
  undefined4 uStack_178;
  undefined4 uStack_174;
  int *piStack_170;
  int *piStack_16c;
  undefined4 uStack_168;
  int *piStack_164;
  int *piStack_160;
  int *piStack_15c;
  undefined *puStack_158;
  undefined4 uStack_154;
  int *piStack_150;
  int *piStack_14c;
  undefined4 uStack_148;
  undefined4 uStack_144;
  int *piStack_140;
  int *piStack_13c;
  undefined4 uStack_138;
  undefined4 uStack_134;
  int *piStack_130;
  int *piStack_12c;
  undefined4 uStack_128;
  undefined4 uStack_124;
  int *piStack_120;
  int *piStack_11c;
  undefined4 uStack_118;
  undefined4 uStack_114;
  int *piStack_110;
  int *piStack_10c;
  undefined4 uStack_108;
  undefined4 uStack_104;
  int *piStack_100;
  int *piStack_fc;
  undefined4 uStack_f8;
  uint uStack_f4;
  int *piStack_f0;
  undefined4 uStack_ec;
  undefined4 uStack_e8;
  int *piStack_e4;
  int *piStack_e0;
  undefined4 uStack_dc;
  int *piStack_d8;
  undefined4 uStack_d4;
  int *piStack_d0;
  int *piStack_cc;
  int *piStack_c8;
  uint uStack_c4;
  int *piStack_c0;
  undefined4 uStack_bc;
  int *piStack_b8;
  uint uStack_b4;
  int *piStack_b0;
  int iStack_ac;
  int *piStack_a8;
  undefined4 uStack_a4;
  undefined4 uStack_a0;
  int *piStack_9c;
  undefined4 uStack_98;
  undefined4 uStack_94;
  int *piStack_90;
  undefined4 uStack_8c;
  undefined4 uStack_88;
  int *piStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  int *piStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  int *piStack_6c;
  undefined4 uStack_68;
  undefined4 uStack_64;
  int *piStack_60;
  undefined4 uStack_5c;
  
  screen_effect_update();
  if (((extraout_EAX != (int *)0x0) &&
      ((((*(short *)((int)extraout_EAX + 2) != 0 || (extraout_EAX[2] != 0)) ||
        (0.0 < (float)extraout_EAX[3])) ||
       ((0.0 < (float)extraout_EAX[4] || (*(char *)((int)extraout_EAX + 0x23) != '\0')))))) &&
     ((DAT_00689428 != '\0' && ((short)DAT_007c1220 == 1)))) {
    iVar5 = (int)(short)DAT_007c1254;
    iVar7 = (int)(short)DAT_007c1258;
    iVar6 = (int)DAT_007c1258._2_2_ - (int)DAT_007c1254._2_2_;
    uStack_5c = 0;
    piStack_60 = DAT_0071d174;
    uStack_64 = 0x52e36a;
    (**(code **)(*DAT_0071d174 + 0x1ac))();
    uStack_64 = 1;
    uStack_68 = 0x16;
    piStack_6c = DAT_0071d174;
    uStack_70 = 0x52e37c;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_70 = 7;
    uStack_74 = 0xa8;
    piStack_78 = DAT_0071d174;
    uStack_7c = 0x52e391;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_7c = 0;
    uStack_80 = 0x1b;
    piStack_84 = DAT_0071d174;
    uStack_88 = 0x52e3a3;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_88 = 0;
    uStack_8c = 0xf;
    piStack_90 = DAT_0071d174;
    uStack_94 = 0x52e3b5;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_94 = 0;
    uStack_98 = 7;
    piStack_9c = DAT_0071d174;
    uStack_a0 = 0x52e3c7;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_a0 = 0;
    uStack_a4 = 0x1c;
    piStack_a8 = DAT_0071d174;
    iStack_ac = 0x52e3d9;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    iStack_ac = DAT_006e1af0;
    piStack_b0 = DAT_0071d174;
    uStack_b4 = 0x52e3ee;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_b4 = -(uint)(DAT_0069c680 != 0) & 0x10 | DAT_006e1af8 & 0x10;
    piStack_b8 = DAT_0071d174;
    uStack_bc = 0x52e415;
    (**(code **)(*DAT_0071d174 + 0x134))();
    uStack_bc = DAT_0069e350;
    piStack_c0 = DAT_0071d174;
    uStack_c4 = 0x52e42a;
    (**(code **)(*DAT_0071d174 + 0x170))();
    _DAT_006e1a44 = (float)(iVar7 - iVar5);
    _DAT_006e1a3c = 0xffffffff;
    _DAT_006e1a40 = 0;
    if (iVar7 - iVar5 < 0) {
      _DAT_006e1a44 = _DAT_006e1a44 + 4.2949673e+09;
    }
    _DAT_006e1a58 = (float)iVar6;
    _DAT_006e1a30 = 0xbf800000;
    _DAT_006e1a34 = 0xbf800000;
    _DAT_006e1a54 = 0xffffffff;
    if (iVar6 < 0) {
      _DAT_006e1a58 = _DAT_006e1a58 + 4.2949673e+09;
    }
    _DAT_006e1a48 = 0x3f800000;
    _DAT_006e1a4c = 0xbf800000;
    _DAT_006e1a6c = 0xffffffff;
    _DAT_006e1a74 = 0;
    _DAT_006e1a60 = 0x3f800000;
    _DAT_006e1a64 = 0x3f800000;
    _DAT_006e1a84 = 0xffffffff;
    _DAT_006e1a88 = 0;
    _DAT_006e1a8c = 0;
    _DAT_006e1a78 = 0xbf800000;
    _DAT_006e1a7c = 0x3f800000;
    _DAT_006e1a80 = 0;
    _DAT_006e1a68 = 0;
    _DAT_006e1a50 = 0;
    _DAT_006e1a38 = 0;
    _DAT_006e1a5c = _DAT_006e1a44;
    _DAT_006e1a70 = _DAT_006e1a58;
    if (*(char *)((int)extraout_EAX + 0x23) == '\0') {
      iStack_ac = iVar6;
      if (extraout_EAX[2] != 0) {
        uStack_c4 = 3;
        piStack_c8 = (int *)0x1;
        piStack_cc = (int *)0x0;
        piStack_d0 = DAT_0071d174;
        uStack_d4 = 0x52e88d;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_d4 = 3;
        piStack_d8 = (int *)0x2;
        uStack_dc = 0;
        piStack_e0 = DAT_0071d174;
        piStack_e4 = (int *)0x52e8a1;
        (**(code **)(*DAT_0071d174 + 0x114))();
        piStack_e4 = (int *)0x2;
        uStack_e8 = 5;
        uStack_ec = 0;
        piStack_f0 = DAT_0071d174;
        uStack_f4 = 0x52e8b5;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_f4 = 2;
        uStack_f8 = 6;
        piStack_fc = (int *)0x0;
        piStack_100 = DAT_0071d174;
        uStack_104 = 0x52e8c9;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_104 = 1;
        uStack_108 = 7;
        piStack_10c = (int *)0x0;
        piStack_110 = DAT_0071d174;
        uStack_114 = 0x52e8dd;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_114 = 3;
        uStack_118 = 1;
        piStack_11c = (int *)0x1;
        piStack_120 = DAT_0071d174;
        uStack_124 = 0x52e8f1;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_124 = 3;
        uStack_128 = 2;
        piStack_12c = (int *)0x1;
        piStack_130 = DAT_0071d174;
        uStack_134 = 0x52e905;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_134 = 2;
        uStack_138 = 5;
        piStack_13c = (int *)0x1;
        piStack_140 = DAT_0071d174;
        uStack_144 = 0x52e919;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_144 = 2;
        uStack_148 = 6;
        piStack_14c = (int *)0x1;
        piStack_150 = DAT_0071d174;
        uStack_154 = 0x52e92d;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_154 = 1;
        puStack_158 = (undefined *)0x7;
        piStack_15c = (int *)0x1;
        piStack_160 = DAT_0071d174;
        piStack_164 = (int *)0x52e941;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_c4 = 0;
        piStack_c8 = (int *)0x1;
        piStack_cc = (int *)0x0;
        uStack_d4 = 0x52e951;
        piStack_d0 = extraout_EAX;
        rasterizer_screen_effect_compute_uv_transform();
        if (*(short *)((int)extraout_EAX + 2) != 0) {
          if ((((float)extraout_EAX[5] == 0.0) && ((float)extraout_EAX[6] == 0.0)) &&
             ((float)extraout_EAX[7] == 0.0)) {
            uStack_c4 = 1;
            piStack_c8 = (int *)0x1b;
            piStack_cc = DAT_0071d174;
            piStack_d0 = (int *)0x52e9b3;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            piStack_d0 = (int *)0x9;
            uStack_d4 = 0x13;
            piStack_d8 = DAT_0071d174;
            uStack_dc = 0x52e9c5;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_dc = 1;
            piStack_e0 = (int *)&DAT_00000014;
            piStack_e4 = DAT_0071d174;
            uStack_e8 = 0x52e9d7;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_e8 = 1;
            uStack_ec = 0xab;
            piStack_f0 = DAT_0071d174;
            uStack_f4 = 0x52e9ec;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_f4 = 0;
            uStack_f8 = 0x52e9f6;
            FUN_00518680();
            uStack_f4 = 2;
            uStack_f8 = 1;
            piStack_fc = (int *)0x0;
            piStack_100 = DAT_0071d174;
            uStack_104 = 0x52ea0d;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_104 = 0x12;
            uStack_108 = 2;
            piStack_10c = (int *)0x0;
            piStack_110 = DAT_0071d174;
            uStack_114 = 0x52ea21;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_114 = 2;
            uStack_118 = 4;
            piStack_11c = (int *)0x0;
            piStack_120 = DAT_0071d174;
            uStack_124 = 0x52ea35;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_124 = 2;
            uStack_128 = 5;
            piStack_12c = (int *)0x0;
            piStack_130 = DAT_0071d174;
            uStack_134 = 0x52ea49;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_134 = 1;
            uStack_138 = 1;
            piStack_13c = (int *)0x1;
            piStack_140 = DAT_0071d174;
            uStack_144 = 0x52ea5d;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_14c = (int *)0x1;
          }
          else {
            uStack_c4 = 1;
            piStack_c8 = (int *)0x1b;
            piStack_cc = DAT_0071d174;
            piStack_d0 = (int *)0x52ea7a;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            piStack_d0 = (int *)0x9;
            uStack_d4 = 0x13;
            piStack_d8 = DAT_0071d174;
            uStack_dc = 0x52ea8c;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_dc = 3;
            piStack_e0 = (int *)&DAT_00000014;
            piStack_e4 = DAT_0071d174;
            uStack_e8 = 0x52ea9e;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_e8 = 1;
            uStack_ec = 0xab;
            piStack_f0 = DAT_0071d174;
            uStack_f4 = 0x52eab3;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            piVar1 = DAT_0071d174;
            iVar5 = *DAT_0071d174;
            uStack_f4 = 0x52eadd;
            uVar2 = __ftol();
            uStack_f4 = 0x52eaf3;
            iVar6 = __ftol();
            uStack_f4 = 0x52eb0e;
            uVar3 = __ftol();
            uStack_f4 = 0x52eb2b;
            uVar4 = __ftol();
            uStack_f4 = ((uVar2 & 0xff | iVar6 << 8) << 8 | uVar3 & 0xff) << 8 | uVar4 & 0xff;
            uStack_f8 = 0x3c;
            piStack_fc = piVar1;
            piStack_100 = (int *)0x52eb3e;
            (**(code **)(iVar5 + 0xe4))();
            piStack_100 = (int *)0x0;
            uStack_104 = 0x52eb48;
            FUN_00518680();
            piStack_100 = (int *)&DAT_00000004;
            uStack_104 = 1;
            uStack_108 = 0;
            piStack_10c = DAT_0071d174;
            piStack_110 = (int *)0x52eb5f;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_110 = (int *)0x3;
            uStack_114 = 2;
            uStack_118 = 0;
            piStack_11c = DAT_0071d174;
            piStack_120 = (int *)0x52eb73;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_120 = (int *)0x12;
            uStack_124 = 3;
            uStack_128 = 0;
            piStack_12c = DAT_0071d174;
            piStack_130 = (int *)0x52eb87;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_130 = (int *)0x2;
            uStack_134 = 4;
            uStack_138 = 0;
            piStack_13c = DAT_0071d174;
            piStack_140 = (int *)0x52eb9b;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_140 = (int *)0x1;
            uStack_144 = 5;
            uStack_148 = 0;
            piStack_14c = DAT_0071d174;
            piStack_150 = (int *)0x52ebaf;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_150 = (int *)0x1;
            uStack_154 = 1;
            puStack_158 = (undefined *)0x1;
            piStack_15c = DAT_0071d174;
            piStack_160 = (int *)0x52ebc3;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_160 = (int *)0x1;
            piStack_164 = (int *)&DAT_00000004;
            uStack_168 = 1;
            piStack_16c = DAT_0071d174;
            piStack_170 = (int *)0x52ebd7;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_170 = (int *)0x1;
            uStack_174 = 1;
            uStack_178 = 2;
            piStack_17c = DAT_0071d174;
            piStack_180 = (int *)0x52ebeb;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            piStack_14c = (int *)0x2;
          }
          uStack_144 = 1;
          uStack_148 = 4;
          piStack_150 = DAT_0071d174;
          uStack_154 = 0x52ebff;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_154 = 0x18;
          puStack_158 = &DAT_006e1a30;
          piStack_15c = (int *)0x2;
          piStack_160 = (int *)&DAT_00000006;
          piStack_164 = DAT_0071d174;
          uStack_168 = 0x52ec18;
          (**(code **)(*DAT_0071d174 + 0x14c))();
        }
      }
    }
    else {
      piStack_b0 = (int *)0x0;
      piStack_c8 = (int *)((uint)(ushort)((short)*extraout_EAX + 1) << 1);
      iStack_ac = (int)piStack_c8;
      if (0 < (short)piStack_c8) {
        do {
          if ((short)piStack_b0 == 1) {
            uStack_c4 = 1;
            piStack_cc = (int *)0x1;
            piStack_a8 = (int *)0x3f800000;
            uStack_a4 = 0;
            uStack_a0 = 0;
            piStack_9c = (int *)0x0;
            uStack_98 = 0;
            uStack_94 = 0x3f800000;
            piStack_90 = (int *)0x0;
            uStack_8c = 0;
            uStack_88 = 0;
            piStack_84 = (int *)0x0;
            uStack_80 = 0x3f800000;
            uStack_7c = 0;
            piStack_78 = (int *)0x0;
            uStack_74 = 0;
            uStack_70 = 0;
            piStack_6c = (int *)0x3f800000;
            uStack_d4 = 0x52e5df;
            rasterizer_screen_effect_compute_uv_transform();
            uStack_d4 = 0;
            piStack_d8 = (int *)0x52e5e9;
            FUN_00518680();
            uStack_c4 = 3;
            piStack_c8 = (int *)0x1;
            piStack_cc = (int *)0x0;
            piStack_d0 = DAT_0071d174;
            uStack_d4 = 0x52e600;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_d4 = 3;
            piStack_d8 = (int *)0x2;
            uStack_dc = 0;
            piStack_e0 = DAT_0071d174;
            piStack_e4 = (int *)0x52e614;
            (**(code **)(*DAT_0071d174 + 0x114))();
            piStack_e4 = (int *)0x1;
            uStack_e8 = 5;
            uStack_ec = 0;
            piStack_f0 = DAT_0071d174;
            uStack_f4 = 0x52e628;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_f4 = 1;
            uStack_f8 = 6;
            piStack_fc = (int *)0x0;
            piStack_100 = DAT_0071d174;
            uStack_104 = 0x52e63c;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_104 = 1;
            uStack_108 = 7;
            piStack_10c = (int *)0x0;
            piStack_110 = DAT_0071d174;
            uStack_114 = 0x52e650;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_114 = 1;
            uStack_118 = 0x52e65a;
            FUN_00518680();
            uStack_114 = 1;
            uStack_118 = 1;
            piStack_11c = (int *)0x1;
            piStack_120 = DAT_0071d174;
            uStack_124 = 0x52e671;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_124 = 1;
            uStack_128 = 2;
            piStack_12c = (int *)0x1;
            piStack_130 = DAT_0071d174;
            uStack_134 = 0x52e685;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_134 = 2;
            uStack_138 = 5;
            piStack_13c = (int *)0x1;
            piStack_140 = DAT_0071d174;
            uStack_144 = 0x52e699;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_144 = 2;
            uStack_148 = 6;
            piStack_14c = (int *)0x1;
            piStack_150 = DAT_0071d174;
            uStack_154 = 0x52e6ad;
            (**(code **)(*DAT_0071d174 + 0x114))();
            uStack_154 = 1;
            puStack_158 = (undefined *)0x7;
            piStack_15c = (int *)0x1;
            piStack_160 = DAT_0071d174;
            piStack_164 = (int *)0x52e6c1;
            (**(code **)(*DAT_0071d174 + 0x114))();
            piStack_164 = (int *)0x2;
            uStack_168 = 1;
            piStack_16c = (int *)0x0;
            piStack_170 = DAT_0071d174;
            uStack_174 = 0x52e6d5;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_174 = 2;
            uStack_178 = 2;
            piStack_17c = (int *)0x0;
            piStack_180 = DAT_0071d174;
            uStack_184 = 0x52e6e9;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_184 = 2;
            uStack_188 = 4;
            uStack_18c = 0;
            piStack_190 = DAT_0071d174;
            uStack_194 = 0x52e6fd;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_194 = 2;
            uStack_198 = 5;
            uStack_19c = 0;
            piStack_1a0 = DAT_0071d174;
            uStack_1a4 = 0x52e711;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_1a4 = 4;
            uStack_1a8 = 1;
            uStack_1ac = 1;
            piStack_1b0 = DAT_0071d174;
            uStack_1b4 = 0x52e725;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_1b4 = 2;
            uStack_1b8 = 2;
            uStack_1bc = 1;
            piStack_1c0 = DAT_0071d174;
            uStack_1c4 = 0x52e739;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_1c4 = 1;
            uStack_1c8 = 3;
            uStack_1cc = 1;
            piStack_1d0 = DAT_0071d174;
            uStack_1d4 = 0x52e74d;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_1d4 = 2;
            uStack_1d8 = 4;
            uStack_1dc = 1;
            piStack_1e0 = DAT_0071d174;
            uStack_1e4 = 0x52e761;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_1e4 = 1;
            uStack_1e8 = 5;
            uStack_1ec = 1;
            piStack_1f0 = DAT_0071d174;
            uStack_1f4 = 0x52e775;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_1f4 = 1;
            uStack_1f8 = 1;
            uStack_1fc = 2;
            piStack_200 = DAT_0071d174;
            uStack_204 = 0x52e789;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_204 = 1;
            uStack_208 = 4;
            uStack_20c = 2;
            piStack_210 = DAT_0071d174;
            uStack_214 = 0x52e79d;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            uStack_214 = 1;
            uStack_218 = 0x1b;
            piStack_21c = DAT_0071d174;
            uStack_220 = 0x52e7af;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_220 = 9;
            uStack_224 = 0x13;
            piStack_228 = DAT_0071d174;
            uStack_22c = 0x52e7c1;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_22c = 1;
            uStack_230 = 0x14;
            piStack_234 = DAT_0071d174;
            uStack_238 = 0x52e7d3;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_238 = 1;
            uStack_23c = 0xab;
            piStack_240 = DAT_0071d174;
            uStack_244 = 0x52e7e8;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_244 = 0x18;
            puStack_248 = &DAT_006e1a30;
            uStack_24c = 2;
            uStack_250 = 6;
            piStack_254 = DAT_0071d174;
            uStack_258 = 0x52e801;
            (**(code **)(*DAT_0071d174 + 0x14c))();
            uStack_258 = 0;
            uStack_25c = 0x18;
            uStack_260 = 0;
            piStack_264 = DAT_0071d174;
            puStack_268 = (undefined4 *)0x52e815;
            (**(code **)(*DAT_0071d174 + 0x10c))();
            puStack_268 = &uStack_24c;
            (**(code **)(*DAT_0071d174 + 0xb0))(DAT_0071d174,0x10);
            (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,0x18,0);
            (**(code **)(*DAT_0071d174 + 0xb0))(DAT_0071d174,0x11,&puStack_268);
            piStack_c8 = (int *)iStack_ac;
          }
          piStack_b0 = (int *)((int)piStack_b0 + 1);
        } while ((short)piStack_b0 < (short)piStack_c8);
      }
    }
    uStack_c4 = (uint)DAT_0069c680;
    piStack_c8 = DAT_0071d174;
    piStack_cc = (int *)0x52ec2e;
    (**(code **)(*DAT_0071d174 + 0x134))();
  }
  return;
}
#endif
