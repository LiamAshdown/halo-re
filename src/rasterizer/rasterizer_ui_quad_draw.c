// rasterizer_ui_quad_draw  (Ghidra: FUN_0051c9a0; the phase 3 rewrite called it
//   lens_flare_render_element)
// address 0x51c9a0, size 3292 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: raw disassembly 0x51c9a0..0x51d67b (phase 4 review). Every caller is in the
//   interface module (0x449780, ui_draw_rotated_screen_quad 0x494d70, ui_draw_screen_quad
//   0x498b20, hud_draw_rotated_bitmap_quad 0x4acd50, hud_draw_multitexture_overlay 0x4acfe0);
//   they build a ui_quad_render_state (types/interface.h) on the stack, pass it in EAX and push a
//   pointer to four hud_quad_vertex records. Nothing here touches lens flares. The phase 3 file
//   was a sketch that dropped every draw call.
// What it does (all in the 3D window with 0x00689402 set): common states (no culling, rgb
//   writes, alpha blend and alpha test with ref 1, no z, no fog), the framebuffer blend mode of
//   the state, declaration 8 and vertex shader 36; c13..c17 map the 640 x 480 virtual screen to
//   clip space (0.003125, -0.0041667) with the optional geometry offset and half-pixel shift,
//   c17.xy = map_texel_scales[0]; c18..c23 carry the other texel scales, per-map axis selectors
//   (the three flag bytes at +8), the three map offsets and the three map scales; each present
//   map gets wrap / clamp addressing and linear or point filtering (+0x8a). Then one of:
//   - meter (meter_parameters set): the maps are bound directly and the quad is drawn twice
//     with fixed function stages, first with ALPHAFUNC LESSEQUAL against the alpha byte of the
//     first meter colour and TFACTOR = that colour (MODULATE), then ALPHAFUNC GREATER with the
//     colour at +8 (MODULATE2X), both with ONE x ONE blending;
//   - single map: TFACTOR = (fade0, tint0) and a two stage MODULATE combiner, one draw;
//   - two or three maps (ps_1_1 and up only): effect 49 plus an offset picked by the two blend
//     modes, the maps set as the effect textures, the tints and fades (and the four floats at
//     +0x64) as pixel constants c0..c5, one draw per pass.
// register convention: EAX -> state, stack -> vertices.
// blam-cc: EAX -> state, stack -> vertices
// UNSURE: the effect path uploads six pixel constant registers from a 24 float local of which
//   only the first 16 floats are written; the last two registers carry whatever the stack
//   held (transcribed as zero here). The ps_1_0 TFACTOR branch inside the effect path is only
//   reachable when an effect was picked, which the code above never does below ps_1_1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing;       // 0x0069c680
extern uint8_t console_debug_toggle_689402;                 // 0x00689402
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern const ColorRGB *global_white_color;                  // 0x00686b04
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410

// blam-cc: CX -> mode
extern void chimera__rasterizer_set_framebuffer_blend_function(int16_t mode); // 0x5185d0
// blam-cc: ESI -> bitmap, stack -> stage
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680
// blam-cc: EAX -> bitmap, stack -> (wait, allocate_if_missing)
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550

typedef int32_t (*d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (*d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (*d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (*d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (*d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);
typedef int32_t (*d3dx_effect_set_texture_fn)(void *effect, uint32_t handle, uint32_t texture);
typedef int32_t (*d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (*d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (*d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}
static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, stage, type, value);
}
static void set_texture(uint32_t stage, uint32_t texture)
{
    ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, stage, texture);
}
static void draw_quad(const hud_quad_vertex *vertices)
{
    ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, vertices,
                                                           sizeof(hud_quad_vertex)); // TRIANGLEFAN
}
// __ftol of each channel times 255; alpha is not masked but its high bits shift out
static uint32_t pack_argb(float alpha, float red, float green, float blue)
{
    uint32_t value = (uint32_t)(int32_t)(red * 255.0f) & 0xff;

    value = (value | ((uint32_t)(int32_t)(alpha * 255.0f) << 8)) << 8;
    value = (value | ((uint32_t)(int32_t)(green * 255.0f) & 0xff)) << 8;
    return value | ((uint32_t)(int32_t)(blue * 255.0f) & 0xff);
}

void rasterizer_ui_quad_draw(ui_quad_render_state *state, hud_quad_vertex *vertices)
{
    const uint8_t *axis_flags = (const uint8_t *)&state->unknown_08;
    rasterizer_effect_slot *slot = NULL;
    uint8_t ok = 1;
    float screen[5][4];     // c13..c17
    float maps[6][4];       // c18..c23
    float pixel[6][4];      // c0..c5 of the effect path
    float offset_x, offset_y;
    int16_t width, height;
    int16_t stage;

    if (console_debug_toggle_689402 == 0 || rasterizer_window.type != 1) {
        return;
    }
    set_render_state(0x16, 1);           // CULLMODE NONE
    set_render_state(0xa8, 7);           // COLORWRITEENABLE rgb
    set_render_state(0x1b, 1);           // ALPHABLENDENABLE
    set_render_state(0x0f, 1);           // ALPHATESTENABLE
    set_render_state(0x18, 1);           // ALPHAREF
    set_render_state(0x07, 0);           // ZENABLE
    set_render_state(0x1c, 0);           // FOGENABLE
    chimera__rasterizer_set_framebuffer_blend_function(state->framebuffer_blend_function);
    if (((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
            rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration) < 0) {
        ok = 0;
    }
    if (((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
            ((rasterizer_software_vertex_processing ? 0x10 : 0) |
             rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10) < 0) {
        ok = 0;
    }
    if (((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[36].shader) < 0) {
        ok = 0;
    }

    // camera.viewport_bounds at 0x007c1254
    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    offset_x = (state->geometry_offset != NULL) ? state->geometry_offset[0] * 0.003125f : 0.0f;
    offset_y = (state->geometry_offset != NULL) ? state->geometry_offset[1] * -0.004166667f : 0.0f;

    screen[0][0] = 0.003125f; screen[0][1] = 0.0f; screen[0][2] = 0.0f;
    screen[0][3] = offset_x - (1.0f / (float)width + 1.0f);
    screen[1][0] = 0.0f; screen[1][1] = -0.004166667f; screen[1][2] = 0.0f;
    screen[1][3] = 1.0f / (float)height + offset_y + 1.0f;
    screen[2][0] = 0.0f; screen[2][1] = 0.0f; screen[2][2] = 0.0f; screen[2][3] = 0.5f;
    screen[3][0] = 0.0f; screen[3][1] = 0.0f; screen[3][2] = 0.0f; screen[3][3] = 1.0f;
    screen[4][0] = state->map_texel_scales[0].x;
    screen[4][1] = state->map_texel_scales[0].y;
    screen[4][2] = 0.0f;
    screen[4][3] = 1.0f;

    maps[0][0] = state->map_texel_scales[1].x;
    maps[0][1] = state->map_texel_scales[1].y;
    maps[0][2] = state->map_texel_scales[2].x;
    maps[0][3] = state->map_texel_scales[2].y;
    maps[1][0] = axis_flags[0] ? 1.0f : 0.0f;
    maps[1][1] = axis_flags[0] ? 0.0f : 1.0f;
    maps[1][2] = axis_flags[1] ? 1.0f : 0.0f;
    maps[1][3] = axis_flags[1] ? 0.0f : 1.0f;
    maps[2][0] = axis_flags[2] ? 1.0f : 0.0f;
    maps[2][1] = axis_flags[2] ? 0.0f : 1.0f;
    maps[2][2] = (state->map_offsets[0] != NULL) ? state->map_offsets[0]->x : 0.0f;
    maps[2][3] = (state->map_offsets[0] != NULL) ? state->map_offsets[0]->y : 0.0f;
    maps[3][0] = (state->map_offsets[1] != NULL) ? state->map_offsets[1]->x : 0.0f;
    maps[3][1] = (state->map_offsets[1] != NULL) ? state->map_offsets[1]->y : 0.0f;
    maps[3][2] = (state->map_offsets[2] != NULL) ? state->map_offsets[2]->x : 0.0f;
    maps[3][3] = (state->map_offsets[2] != NULL) ? state->map_offsets[2]->y : 0.0f;
    maps[4][0] = state->map_scales[0].x;
    maps[4][1] = state->map_scales[0].y;
    maps[4][2] = state->map_scales[1].x;
    maps[4][3] = state->map_scales[1].y;
    maps[5][0] = state->map_scales[2].x;
    maps[5][1] = state->map_scales[2].y;
    maps[5][2] = 0.0f;
    maps[5][3] = 0.0f;
    if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &screen[0][0], 5) < 0) {
        ok = 0;
    }
    if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0x12, &maps[0][0], 6) < 0) {
        ok = 0;
    }

    for (stage = 0; stage < 3 && state->maps[stage] != NULL; stage++) {
        uint32_t address = state->wrap_modes[stage] ? 1 : 3;            // WRAP : CLAMP
        uint32_t filter = state->single_local_player ? 1 : 2;           // POINT : LINEAR

        set_sampler_state(stage, 1, address);
        set_sampler_state(stage, 2, address);
        set_sampler_state(stage, 5, filter);
        set_sampler_state(stage, 6, filter);
        set_sampler_state(stage, 7, filter);
    }

    if (state->meter_parameters != NULL) {
        const uint32_t *meter = (const uint32_t *)state->meter_parameters;

        for (stage = 0; stage < 3 && state->maps[stage] != NULL; stage++) {
            texture_cache_get(state->maps[stage], 1, 1);
            set_texture(stage, *(const uint32_t *)((const uint8_t *)state->maps[stage] + 0x28));
        }
        ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);      // SetPixelShader
        set_render_state(0x0f, 1);                                           // ALPHATESTENABLE
        set_render_state(0x19, 4);                                           // ALPHAFUNC LESSEQUAL
        set_render_state(0x18, ((const uint8_t *)meter)[3]);                 // ALPHAREF
        set_render_state(0xa8, 0xf);
        set_render_state(0x1b, 1);
        set_render_state(0x13, 2);                                           // SRCBLEND ONE
        set_render_state(0x14, 2);                                           // DESTBLEND ONE
        set_render_state(0xab, 1);                                           // BLENDOP ADD
        set_render_state(0x3c, meter[0]);                                    // TEXTUREFACTOR
        set_texture_stage_state(0, 1, 4);    // COLOROP MODULATE
        set_texture_stage_state(0, 2, 2);    // COLORARG1 TEXTURE
        set_texture_stage_state(0, 3, 3);    // COLORARG2 TFACTOR
        set_texture_stage_state(0, 4, 2);    // ALPHAOP SELECTARG1
        set_texture_stage_state(0, 5, 2);    // ALPHAARG1 TEXTURE
        set_texture_stage_state(1, 1, 1);
        set_texture_stage_state(1, 4, 1);
        draw_quad(vertices);
        set_render_state(0x19, 5);                                           // ALPHAFUNC GREATER
        set_render_state(0x3c, meter[2]);                                    // TEXTUREFACTOR
        set_render_state(0xa8, 0xf);
        set_texture_stage_state(0, 1, 5);    // COLOROP MODULATE2X
        set_texture_stage_state(0, 2, 2);
        set_texture_stage_state(0, 3, 3);
        set_texture_stage_state(0, 4, 2);
        set_texture_stage_state(0, 5, 2);
        set_texture_stage_state(1, 1, 1);
        set_texture_stage_state(1, 4, 1);
        draw_quad(vertices);
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
        return;
    }

    if (state->maps[0] != NULL) {
        const ColorRGB *tint0 = (state->map_tints[0] != NULL) ? state->map_tints[0] : global_white_color;
        float fade0 = (state->map_fades[0] != NULL) ? *state->map_fades[0] : 1.0f;
        int16_t effect_index = 0x31;

        set_sampler_state(0, 1, 3);
        set_sampler_state(0, 2, 3);
        if (state->maps[1] == NULL && state->maps[2] == NULL) {
            set_render_state(0x3c, pack_argb(fade0, tint0->red, tint0->green, tint0->blue));
            rasterizer_bind_texture_d3d9(0, state->maps[0]);
            set_render_state(0x0f, 0);
            set_texture_stage_state(0, 1, 4);    // COLOROP MODULATE
            set_texture_stage_state(0, 2, 2);    // COLORARG1 TEXTURE
            set_texture_stage_state(0, 3, 3);    // COLORARG2 TFACTOR
            set_texture_stage_state(0, 4, 4);    // ALPHAOP MODULATE
            set_texture_stage_state(0, 5, 2);    // ALPHAARG1 TEXTURE
            set_texture_stage_state(0, 6, 0);    // ALPHAARG2 DIFFUSE
            set_texture_stage_state(1, 1, 4);    // COLOROP MODULATE
            set_texture_stage_state(1, 2, 1);    // COLORARG1 CURRENT
            set_texture_stage_state(1, 3, 0);    // COLORARG2 DIFFUSE
            set_texture_stage_state(1, 4, 2);    // ALPHAOP SELECTARG1
            set_texture_stage_state(1, 5, 1);    // ALPHAARG1 CURRENT
            set_texture_stage_state(2, 1, 1);
            set_texture_stage_state(2, 4, 1);
            ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);
            draw_quad(vertices);
            ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
            return;
        }
        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
            return;
        }
        {
            const ColorRGB *tint1 = (state->map_tints[1] != NULL) ? state->map_tints[1] : global_white_color;
            const ColorRGB *tint2 = (state->map_tints[2] != NULL) ? state->map_tints[2] : global_white_color;
            const float *extra = (const float *)state->unknown_64;   // +0x64..+0x70

            pixel[0][0] = tint0->red; pixel[0][1] = tint0->green; pixel[0][2] = tint0->blue; pixel[0][3] = fade0;
            pixel[1][0] = tint1->red; pixel[1][1] = tint1->green; pixel[1][2] = tint1->blue;
            pixel[1][3] = (state->map_fades[1] != NULL) ? *state->map_fades[1] : 1.0f;
            pixel[2][0] = tint2->red; pixel[2][1] = tint2->green; pixel[2][2] = tint2->blue;
            pixel[2][3] = (state->map_fades[2] != NULL) ? *state->map_fades[2] : 1.0f;
            pixel[3][0] = extra[1]; pixel[3][1] = extra[2]; pixel[3][2] = extra[3]; pixel[3][3] = extra[0];
            pixel[4][0] = pixel[4][1] = pixel[4][2] = pixel[4][3] = 0.0f;   // see UNSURE
            pixel[5][0] = pixel[5][1] = pixel[5][2] = pixel[5][3] = 0.0f;
        }
        if (state->maps[1] != NULL) {
            switch (state->zero_to_one_blend) {          // jump table 0x0051d67c
            case 1: effect_index = 0x3b; break;
            case 2: effect_index = 0x45; break;
            case 3: effect_index = 0x40; break;
            case 4: effect_index = 0x36; break;
            case 5: effect_index = 0; break;
            default: break;
            }
        }
        if (state->maps[2] != NULL) {
            switch (state->one_to_two_blend) {           // jump table 0x0051d690
            case 1: effect_index += 2; break;
            case 2: effect_index += 4; break;
            case 3: effect_index += 3; break;
            case 4: effect_index += 1; break;
            default: break;
            }
        }
        slot = &rasterizer_effects[effect_index];
    }

    if (ok && slot != NULL && slot->effect != 0) {
        void *effect = (void *)(uintptr_t)slot->effect;
        uint32_t passes;
        uint32_t pass;

        for (stage = 0; stage < 3 && state->maps[stage] != NULL; stage++) {
            texture_cache_get(state->maps[stage], 1, 1);
            ((d3dx_effect_set_texture_fn)(*(void ***)effect)[0xd0 / 4])(
                effect, slot->texture_handles[stage], *(const uint32_t *)((const uint8_t *)state->maps[stage] + 0x28));
        }
        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            set_render_state(0x3c, pack_argb(pixel[0][3], pixel[0][0], pixel[0][1], pixel[0][2]));
        } else {
            ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, &pixel[0][0], 6);
        }
        ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
        for (pass = 0; pass < passes; pass++) {
            ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
            draw_quad(vertices);
        }
        ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x51c9a0): phase 3 file lens_flare_render_element replaced

void FUN_0051c9a0(void)

{
  float *pfVar1;
  float fVar2;
  int *in_EAX;
  int iVar3;
  uint *puVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  undefined4 *puVar9;
  int *piVar10;
  short sVar11;
  undefined4 uStack_1e0;
  undefined4 *puStack_1dc;
  undefined4 uStack_1d8;
  int *piStack_1d4;
  int *piStack_1d0;
  undefined4 *puStack_1cc;
  uint uStack_1c8;
  int *piStack_1c4;
  uint uStack_1c0;
  int *piStack_1bc;
  uint uStack_1b8;
  undefined4 uStack_18c;
  undefined4 uStack_188;
  int *piStack_184;
  undefined4 uStack_180;
  int iStack_17c;
  int *piStack_178;
  int iStack_174;
  int iStack_170;
  int *piStack_16c;
  undefined4 uStack_168;
  undefined4 uStack_164;
  int *piStack_160;
  undefined4 uStack_15c;
  undefined4 uStack_158;
  undefined4 uStack_11c;
  undefined4 uStack_118;
  undefined4 uStack_114;
  float fStack_110;
  undefined4 uStack_10c;
  undefined4 uStack_108;
  undefined4 uStack_104;
  float fStack_100;
  undefined4 uStack_fc;
  undefined4 uStack_f8;
  undefined4 uStack_f4;
  undefined4 uStack_f0;
  uint uStack_ec;
  int *piStack_e8;
  uint uStack_e4;
  undefined4 *puStack_e0;
  uint uStack_dc;
  uint uStack_d8;
  uint uStack_d4;
  undefined4 uStack_d0;
  undefined4 uStack_cc;
  undefined4 uStack_c8;
  undefined4 uStack_c4;
  undefined4 uStack_c0;
  int iStack_bc;
  int iStack_b8;
  int iStack_b4;
  int iStack_b0;
  undefined4 uStack_a0;
  
  if ((DAT_00689402 != '\0') && ((short)DAT_007c1220 == 1)) {
    uStack_158 = 1;
    uStack_15c = 0x16;
    piVar10 = (int *)0x0;
    piStack_160 = DAT_0071d174;
    uStack_164 = 0x51c9e2;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_164 = 7;
    uStack_168 = 0xa8;
    piStack_16c = DAT_0071d174;
    iStack_170 = 0x51c9f7;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    iStack_170 = 1;
    iStack_174 = 0x1b;
    piStack_178 = DAT_0071d174;
    iStack_17c = 0x51ca09;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    iStack_17c = 1;
    uStack_180 = 0xf;
    piStack_184 = DAT_0071d174;
    uStack_188 = 0x51ca1b;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_188 = 1;
    uStack_18c = 0x18;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    (**(code **)(*DAT_0071d174 + 0xe4))();
    (**(code **)(*DAT_0071d174 + 0xe4))();
    chimera__rasterizer_set_framebuffer_blend_function();
    (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_1b8 = -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10;
    piStack_1bc = DAT_0071d174;
    uStack_1c0 = 0x51caa0;
    (**(code **)(*DAT_0071d174 + 0x134))();
    uStack_1c0 = DAT_0069e470;
    piStack_1c4 = DAT_0071d174;
    uStack_1c8 = 0x51cabe;
    iVar3 = (**(code **)(*DAT_0071d174 + 0x170))();
    if (iVar3 < 0) {
      uStack_1b8 = uStack_1b8 & 0xffffff;
    }
    pfVar1 = (float *)in_EAX[1];
    if (pfVar1 == (float *)0x0) {
      fStack_110 = 0.0;
      fVar2 = 0.0;
    }
    else {
      fStack_110 = *pfVar1 * 0.003125;
      fVar2 = pfVar1[1] * -0.004166667;
    }
    iStack_17c = in_EAX[0x12];
    uStack_dc = in_EAX[0x10];
    uStack_d8 = in_EAX[0x11];
    iStack_170 = in_EAX[0x15];
    piStack_178 = (int *)in_EAX[0x13];
    iStack_174 = in_EAX[0x14];
    uStack_11c = 0x3b4ccccd;
    uStack_118 = 0;
    uStack_114 = 0;
    uStack_10c = 0;
    uStack_108 = 0xbb888889;
    uStack_104 = 0;
    uStack_fc = 0;
    uStack_f8 = 0;
    uStack_f4 = 0;
    uStack_f0 = 0x3f000000;
    uStack_ec = 0;
    piStack_e8 = (int *)0x0;
    uStack_e4 = 0;
    puStack_e0 = (undefined4 *)0x3f800000;
    uStack_d4 = 0;
    uStack_d0 = 0x3f800000;
    piStack_16c = (int *)0x3f800000;
    fStack_110 = fStack_110 -
                 (1.0 / (float)(int)(short)(DAT_007c1258._2_2_ - DAT_007c1254._2_2_) + 1.0);
    fStack_100 = 1.0 / (float)(int)(short)((short)DAT_007c1258 - (short)DAT_007c1254) + fVar2 + 1.0;
    if ((char)in_EAX[2] == '\0') {
      piStack_16c = (int *)0x0;
      uStack_168 = 0x3f800000;
    }
    else {
      uStack_168 = 0;
    }
    uStack_164 = 0x3f800000;
    if (*(char *)((int)in_EAX + 9) == '\0') {
      uStack_164 = 0;
      piStack_160 = (int *)0x3f800000;
    }
    else {
      piStack_160 = (int *)0x0;
    }
    uStack_15c = 0x3f800000;
    if (*(char *)((int)in_EAX + 10) == '\0') {
      uStack_15c = 0;
      uStack_158 = 0x3f800000;
    }
    else {
      uStack_158 = 0;
    }
    uStack_1c8 = 5;
    puStack_1cc = &uStack_11c;
    piStack_1d0 = (int *)0xd;
    piStack_1d4 = DAT_0071d174;
    uStack_1d8 = 0x51cda9;
    iVar3 = (**(code **)(*DAT_0071d174 + 0x178))();
    if (iVar3 < 0) {
      uStack_1c8 = uStack_1c8 & 0xffffff;
    }
    uStack_1d8 = 6;
    puStack_1dc = &uStack_18c;
    uStack_1e0 = 0x12;
    iVar3 = (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174);
    if (iVar3 < 0) {
      uStack_1d8 = uStack_1d8 & 0xffffff;
    }
    sVar11 = 0;
    do {
      iVar3 = (int)sVar11;
      if (in_EAX[iVar3 + 3] == 0) break;
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,1,
                 (*(char *)(iVar3 + 0x18 + (int)in_EAX) == '\0') * '\x02' + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,2,
                 (*(char *)(iVar3 + 0x18 + (int)in_EAX) == '\0') * '\x02' + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,5,(*(char *)((int)in_EAX + 0x8a) == '\0') + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,6,(*(char *)((int)in_EAX + 0x8a) == '\0') + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,7,(*(char *)((int)in_EAX + 0x8a) == '\0') + '\x01');
      sVar11 = sVar11 + 1;
    } while (sVar11 < 3);
    puVar9 = (undefined4 *)*in_EAX;
    if (puVar9 != (undefined4 *)0x0) {
      sVar11 = 0;
      do {
        iVar3 = in_EAX[sVar11 + 3];
        if (iVar3 == 0) break;
        texture_cache_get(1,1);
        (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,(int)sVar11,*(undefined4 *)(iVar3 + 0x28))
        ;
        sVar11 = sVar11 + 1;
      } while (sVar11 < 3);
      (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x19,4);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,*(undefined1 *)((int)puVar9 + 3));
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,0xf);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,2);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,2);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,*puVar9);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,3);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      piVar10 = piStack_16c;
      (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,piStack_16c,0x18);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x19,5);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,puVar9[2]);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,0xf);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,5);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,3);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,piVar10,0x18);
      goto LAB_0051d109;
    }
    if (in_EAX[3] != 0) {
      sVar11 = 0x31;
      puVar4 = (uint *)in_EAX[0x16];
      if ((uint *)in_EAX[0x16] == (uint *)0x0) {
        puVar4 = (uint *)PTR_DAT_00686b04;
      }
      uStack_1c8 = *puVar4;
      piStack_1c4 = (int *)puVar4[1];
      uStack_1c0 = puVar4[2];
      if ((int *)in_EAX[0x1e] == (int *)0x0) {
        puStack_1cc = (undefined4 *)0x3f800000;
      }
      else {
        puStack_1cc = *(undefined4 **)in_EAX[0x1e];
      }
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,3);
      piVar10 = DAT_0071d174;
      if ((in_EAX[4] == 0) && (in_EAX[5] == 0)) {
        iVar3 = *DAT_0071d174;
        uVar5 = __ftol();
        iVar6 = __ftol();
        uVar7 = __ftol();
        uVar8 = __ftol();
        (**(code **)(iVar3 + 0xe4))
                  (piVar10,0x3c,
                   ((uVar5 & 0xff | iVar6 << 8) << 8 | uVar7 & 0xff) << 8 | uVar8 & 0xff);
        FUN_00518680(0);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,3);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,2,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,3,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,5,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,1,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,4,1);
        (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
        (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,piStack_178,0x18);
LAB_0051d109:
        (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
        return;
      }
      if (DAT_007c118c < 0xffff0101) goto LAB_0051d109;
      puVar4 = (uint *)in_EAX[0x17];
      if ((uint *)in_EAX[0x17] == (uint *)0x0) {
        puVar4 = (uint *)PTR_DAT_00686b04;
      }
      uStack_1b8 = *puVar4;
      uStack_d8 = puVar4[1];
      uStack_d4 = puVar4[2];
      if ((undefined4 *)in_EAX[0x1f] == (undefined4 *)0x0) {
        uStack_d0 = 0x3f800000;
      }
      else {
        uStack_d0 = *(undefined4 *)in_EAX[0x1f];
      }
      puVar9 = (undefined4 *)in_EAX[0x18];
      if ((undefined4 *)in_EAX[0x18] == (undefined4 *)0x0) {
        puVar9 = (undefined4 *)PTR_DAT_00686b04;
      }
      uStack_cc = *puVar9;
      uStack_c8 = puVar9[1];
      uStack_c4 = puVar9[2];
      if ((undefined4 *)in_EAX[0x20] == (undefined4 *)0x0) {
        uStack_c0 = 0x3f800000;
      }
      else {
        uStack_c0 = *(undefined4 *)in_EAX[0x20];
      }
      uStack_ec = uStack_1c8;
      piStack_e8 = piStack_1c4;
      uStack_e4 = uStack_1c0;
      puStack_e0 = puStack_1cc;
      iStack_bc = in_EAX[0x1a];
      iStack_b8 = in_EAX[0x1b];
      iStack_b4 = in_EAX[0x1c];
      iStack_b0 = in_EAX[0x19];
      if (in_EAX[4] != 0) {
        switch((short)in_EAX[0x21]) {
        case 1:
          sVar11 = 0x3b;
          break;
        case 2:
          sVar11 = 0x45;
          break;
        case 3:
          sVar11 = 0x40;
          break;
        case 4:
          sVar11 = 0x36;
          break;
        case 5:
          sVar11 = 0;
        }
      }
      if (in_EAX[5] != 0) {
        switch(*(undefined2 *)((int)in_EAX + 0x86)) {
        case 1:
          sVar11 = sVar11 + 2;
          break;
        case 2:
          sVar11 = sVar11 + 4;
          break;
        case 3:
          sVar11 = sVar11 + 3;
          break;
        case 4:
          sVar11 = sVar11 + 1;
        }
      }
      piVar10 = &DAT_0069d410 + sVar11 * 8;
      piStack_1d0 = piVar10;
      uStack_dc = uStack_1b8;
    }
    if (((uStack_1d8._3_1_ != '\0') && (piVar10 != (int *)0x0)) && (*piVar10 != 0)) {
      sVar11 = 0;
      do {
        iVar3 = in_EAX[sVar11 + 3];
        if (iVar3 == 0) break;
        texture_cache_get(1,1);
        (**(code **)(*(int *)*piStack_1d0 + 0xd0))
                  ((int *)*piStack_1d0,piStack_1d0[sVar11 + 2],*(undefined4 *)(iVar3 + 0x28));
        sVar11 = sVar11 + 1;
      } while (sVar11 < 3);
      piVar10 = DAT_0071d174;
      if (DAT_007c118c < 0xffff0101) {
        iVar3 = *DAT_0071d174;
        uVar5 = __ftol();
        iVar6 = __ftol();
        uVar7 = __ftol();
        uVar8 = __ftol();
        uVar5 = ((uVar5 & 0xff | iVar6 << 8) << 8 | uVar7 & 0xff) << 8 | uVar8 & 0xff;
        (**(code **)(iVar3 + 0xe4))(piVar10,0x3c);
      }
      else {
        uVar5 = 6;
        (**(code **)(*DAT_0071d174 + 0x1b4))(DAT_0071d174,0,&uStack_ec);
      }
      puVar9 = puStack_1dc;
      (**(code **)(*(int *)*puStack_1dc + 0x100))((int *)*puStack_1dc,&uStack_1e0,3);
      uVar7 = 0;
      if (uVar5 != 0) {
        do {
          piVar10 = (int *)*puVar9;
          (**(code **)(*piVar10 + 0x104))(piVar10,uVar7);
          (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,uStack_a0,0x18);
          uVar7 = uVar7 + 1;
        } while (uVar7 < uVar5);
      }
      piVar10 = (int *)*puVar9;
      (**(code **)(*piVar10 + 0x108))(piVar10);
    }
    (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  }
  return;
}
#endif
