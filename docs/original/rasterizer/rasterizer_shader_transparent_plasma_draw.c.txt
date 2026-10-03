// rasterizer_shader_transparent_plasma_draw  (Ghidra: FUN_0052c4a0, unnamed; the earlier
//   draft called it rasterizer_light_glow_draw)
// address 0x52c4a0, size 1402 bytes
// VERIFIED against disassembly 0x52c4a0..0x52ca1a (2026-09-30): tint/intensity/offset from lighting_extra, c13..c18 layout for both pixel shader paths, texture and sampler setup, the 11 render states, c10..c12 and the effect pass loop
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: its only caller is the shader_type 11 (transparent_plasma) case of
//   rasterizer_transparent_geometry_group_draw 0x533850, and every shader offset it reads is a
//   ShaderTransparentPlasma field (types/tags.h): intensity_source/exponent +0x2c/+0x30,
//   offset_source/amount/exponent +0x34/+0x38/+0x3c, perpendicular and parallel brightness and
//   tint +0x60..+0x7c, tint_color_source +0x80, primary and secondary animation period,
//   direction, noise map scale and noise map +0xc0..+0xe0 and +0x108..+0x128. Chimera places its
//   plasma_af_set_sampler_states signature inside it (0x52c8f1). Rebuilt from the raw
//   disassembly: Ghidra lost EBX, both pow calls and every device call argument.
// register convention: EBX -> group.
// blam-cc: EBX -> group
// NOTE: group.lighting_extra is read as {ColorRGB *colors, float *function_values} here (the
//   tint comes from colors[tint_color_source - 1]); elsewhere only the second dword is read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_frame_time rasterizer_time;               // 0x007c1200
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern GlobalsRasterizerData *rasterizer_globals_data;      // 0x0071d164
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern const ColorRGB *global_white_color;                  // 0x00686b04 points at 1, 1, 1
extern uint8_t console_debug_toggle_689423;                 // 0x00689423 plasma rendering enabled

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
// blam-cc: EDX -> group
extern uint32_t transparent_geometry_group_get_vertex_type_reference(transparent_geometry_group *group); // 0x515400
// blam-cc: ECX -> group, stack -> flag
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x533660
// blam-cc: ST1 -> base, ST0 -> exponent (CRT _CIpow)
extern double pow(double base, double exponent); // 0x6283c0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
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

#define PLASMA_FLOAT(offset) (*(const float *)(shader + (offset)))

void rasterizer_shader_transparent_plasma_draw(transparent_geometry_group *group)
{
    const uint8_t *shader;
    const ColorRGB *tint;
    float intensity;
    float offset;
    float primary_phase, secondary_phase;
    float primary_scale, secondary_scale;
    float vertex_constants[6][4];
    float color_constants[3][4];
    void *effect;
    uint32_t passes;
    uint32_t pass;
    int16_t vertex_type;
    int i;

    if (!console_debug_toggle_689423) {
        return;
    }
    shader = (const uint8_t *)(uintptr_t)group->shader;
    tint = global_white_color;
    intensity = 1.0f;
    offset = 0.0f;
    if (group->lighting_extra != 0) {
        const uint32_t *function_source = (const uint32_t *)(uintptr_t)group->lighting_extra;
        const ColorRGB *colors = (const ColorRGB *)(uintptr_t)function_source[0];
        const float *function_values = (const float *)(uintptr_t)function_source[1];
        int16_t source;

        source = *(const int16_t *)(shader + 0x80);                // tint_color_source
        if (colors != NULL && source >= 1 && source <= 4) {
            tint = &colors[source - 1];
        }
        if (function_values != NULL) {
            source = *(const int16_t *)(shader + 0x2c);            // intensity_source
            if (source >= 1 && source <= 4) {
                intensity = (float)pow(function_values[source - 1], PLASMA_FLOAT(0x30));
            }
            source = *(const int16_t *)(shader + 0x34);            // offset_source
            if (source >= 1 && source <= 4) {
                offset = (float)pow(function_values[source - 1], PLASMA_FLOAT(0x3c)) * PLASMA_FLOAT(0x38);
            }
        }
    }
    effect = (void *)(uintptr_t)rasterizer_effects[44].effect;
    if (effect == NULL) {
        return;
    }

    secondary_scale = PLASMA_FLOAT(0x118);                         // secondary_noise_map_scale
    primary_phase = (float)(rasterizer_time.time / PLASMA_FLOAT(0xc0));   // / primary_animation_period
    secondary_phase = (float)(rasterizer_time.time / PLASMA_FLOAT(0x108)); // / secondary_animation_period
    primary_scale = PLASMA_FLOAT(0xd0);                            // primary_noise_map_scale
    if (offset < 0.0005f) {
        offset = 0.0f;
    }
    // c13..c15: primary noise transform (scale on the diagonal, animated translation in w)
    // c16..c18: secondary noise transform
    for (i = 0; i < 6; i++) {
        vertex_constants[i][0] = 0.0f;
        vertex_constants[i][1] = 0.0f;
        vertex_constants[i][2] = 0.0f;
    }
    vertex_constants[0][0] = primary_scale;
    vertex_constants[1][1] = primary_scale;
    vertex_constants[2][2] = primary_scale;
    vertex_constants[0][3] = primary_phase * PLASMA_FLOAT(0xc4);   // primary_animation_direction
    vertex_constants[1][3] = primary_phase * PLASMA_FLOAT(0xc8);
    vertex_constants[2][3] = primary_phase * PLASMA_FLOAT(0xcc);
    vertex_constants[3][3] = secondary_phase * PLASMA_FLOAT(0x10c); // secondary_animation_direction
    vertex_constants[4][3] = secondary_phase * PLASMA_FLOAT(0x110);
    vertex_constants[5][3] = secondary_phase * PLASMA_FLOAT(0x114);
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        // fixed function: fixed 0.4 secondary scale, a 0.01 z bias, glow and video noise maps
        vertex_constants[0][2] = 0.01f;
        vertex_constants[3][0] = 0.4f;
        vertex_constants[4][1] = 0.4f;
        vertex_constants[5][2] = secondary_scale;
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &vertex_constants[0][0], 6);
        chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0x6c), 0, 0, 0,
                                        (int16_t)group->shader_permutation);       // glow
        chimera__rasterizer_set_texture(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0x138), 1, 0, 0,
                                        (int16_t)group->shader_permutation);       // video_noise_map
    } else {
        vertex_constants[0][2] = offset;
        vertex_constants[3][0] = secondary_scale;
        vertex_constants[4][1] = secondary_scale;
        vertex_constants[5][2] = secondary_scale;
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &vertex_constants[0][0], 6);
        chimera__rasterizer_set_texture(*(const uint32_t *)(shader + 0xe0), 0, 1, 0,
                                        (int16_t)group->shader_permutation);       // primary_noise_map, 3D
        set_sampler_state(0, 3, 1);                                // ADDRESSW WRAP
        chimera__rasterizer_set_texture(*(const uint32_t *)(shader + 0x128), 1, 1, 0,
                                        (int16_t)group->shader_permutation);       // secondary_noise_map, 3D
        set_sampler_state(1, 3, 1);
    }
    for (i = 0; i < 2; i++) {
        set_sampler_state(i, 1, 1);    // ADDRESSU WRAP
        set_sampler_state(i, 2, 1);    // ADDRESSV WRAP
        set_sampler_state(i, 5, 2);    // LINEAR
        set_sampler_state(i, 6, 2);
        set_sampler_state(i, 7, 2);
    }
    set_render_state(0x16, 1);         // CULLMODE NONE
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x13, 5);         // SRCBLEND SRCALPHA
    set_render_state(0x14, 2);         // DESTBLEND ONE
    set_render_state(0xab, 1);         // BLENDOP ADD
    set_render_state(0x0f, 0);
    set_render_state(0x07, 1);         // ZENABLE
    set_render_state(0x0e, 0);         // ZWRITEENABLE off
    set_render_state(0x17, 4);         // ZFUNC LESSEQUAL
    set_render_state(0x1c, 0);

    // c10 white, c11 (perpendicular - parallel) tint and brightness, c12 parallel tint and brightness
    for (i = 0; i < 4; i++) {
        color_constants[0][i] = 1.0f;
    }
    color_constants[1][0] = (PLASMA_FLOAT(0x64) - PLASMA_FLOAT(0x74)) * tint->red;
    color_constants[1][1] = (PLASMA_FLOAT(0x68) - PLASMA_FLOAT(0x78)) * tint->green;
    color_constants[1][2] = (PLASMA_FLOAT(0x6c) - PLASMA_FLOAT(0x7c)) * tint->blue;
    color_constants[1][3] = (PLASMA_FLOAT(0x60) - PLASMA_FLOAT(0x70)) * intensity;
    color_constants[2][0] = tint->red * PLASMA_FLOAT(0x74);
    color_constants[2][1] = PLASMA_FLOAT(0x78) * tint->green;
    color_constants[2][2] = PLASMA_FLOAT(0x7c) * tint->blue;
    color_constants[2][3] = intensity * PLASMA_FLOAT(0x70);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 10, &color_constants[0][0], 3);

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
    vertex_type = (int16_t)transparent_geometry_group_get_vertex_type_reference(group);
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[vertex_type].declaration);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[59].shader);
    for (pass = 0; pass < passes; pass++) {
        effect = (void *)(uintptr_t)rasterizer_effects[44].effect;
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    effect = (void *)(uintptr_t)rasterizer_effects[44].effect;
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x52c4a0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0052c4a0(void)

{
  int iVar1;
  short sVar2;
  void *unaff_EBX;
  int *piVar3;
  float *pfVar4;
  float10 fVar5;
  int *piVar6;
  int *piVar7;
  int *piStack_1ac;
  undefined4 uStack_1a8;
  undefined4 uStack_1a4;
  int *piStack_1a0;
  undefined4 uStack_19c;
  float fStack_198;
  int *piStack_194;
  undefined4 uStack_190;
  undefined4 uStack_18c;
  int *piStack_188;
  float fStack_184;
  float fStack_180;
  int *piStack_17c;
  float fStack_178;
  float fStack_174;
  int *piStack_170;
  float fStack_16c;
  float fStack_168;
  int *piStack_164;
  undefined4 uStack_160;
  undefined4 uStack_15c;
  int *piStack_158;
  undefined4 uStack_154;
  undefined4 uStack_150;
  int *piStack_14c;
  undefined4 uStack_148;
  undefined4 uStack_144;
  int *piStack_140;
  undefined4 uStack_13c;
  undefined4 uStack_138;
  int *piStack_134;
  undefined4 uStack_130;
  undefined4 uStack_12c;
  int *piStack_128;
  undefined4 uStack_124;
  undefined4 uStack_120;
  undefined4 uStack_11c;
  int *piStack_118;
  undefined4 uStack_114;
  undefined4 uStack_110;
  undefined4 uStack_10c;
  int *piStack_108;
  undefined4 uStack_104;
  undefined4 uStack_100;
  undefined4 uStack_fc;
  int *piStack_f8;
  undefined4 uStack_f4;
  undefined4 uStack_f0;
  undefined4 uStack_ec;
  int *piStack_e8;
  undefined4 uStack_e4;
  undefined4 uStack_e0;
  undefined4 uStack_dc;
  int *piStack_d8;
  undefined4 uStack_d4;
  undefined4 uStack_d0;
  undefined4 uStack_cc;
  int *piStack_c8;
  undefined4 uStack_c4;
  undefined4 uStack_c0;
  undefined4 uStack_bc;
  int *piStack_b8;
  undefined4 uStack_b4;
  undefined4 uStack_b0;
  undefined4 uStack_ac;
  int *piStack_a8;
  undefined4 uStack_a4;
  undefined4 uStack_a0;
  uint uStack_9c;
  int *piStack_98;
  undefined4 uStack_94;
  undefined4 uStack_90;
  uint uStack_8c;
  int *piStack_88;
  undefined4 uStack_84;
  undefined4 *puStack_80;
  float local_6c;
  undefined4 local_60;
  undefined4 local_5c;
  float local_58;
  float local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  float local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  float local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  float local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  float local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  float local_4;
  
  if (DAT_00689423 != '\0') {
    piVar6 = *(int **)((int)unaff_EBX + 0x74);
    iVar1 = *(int *)((int)unaff_EBX + 0xc);
    local_6c = 0.0;
    pfVar4 = (float *)PTR_DAT_00686b04;
    if (piVar6 != (int *)0x0) {
      if (((*piVar6 != 0) && (sVar2 = *(short *)(iVar1 + 0x80), 0 < sVar2)) && (sVar2 < 5)) {
        pfVar4 = (float *)(*piVar6 + -0xc + sVar2 * 0xc);
      }
      if (piVar6[1] != 0) {
        if ((0 < *(short *)(iVar1 + 0x2c)) && (*(short *)(iVar1 + 0x2c) < 5)) {
          puStack_80 = (undefined4 *)0x52c51c;
          FUN_006283c0();
        }
        if ((0 < *(short *)(iVar1 + 0x34)) && (*(short *)(iVar1 + 0x34) < 5)) {
          puStack_80 = (undefined4 *)0x52c53f;
          fVar5 = (float10)FUN_006283c0();
          local_6c = (float)(fVar5 * (float10)*(float *)(iVar1 + 0x38));
        }
      }
    }
    if (DAT_0069d990 != (int *)0x0) {
      local_8 = *(undefined4 *)(iVar1 + 0x118);
      local_34 = (float)_DAT_007c1200 / *(float *)(iVar1 + 0xc0);
      local_4 = (float)_DAT_007c1200 / *(float *)(iVar1 + 0x108);
      local_60 = *(undefined4 *)(iVar1 + 0xd0);
      if (local_6c < 0.0005) {
        local_6c = 0.0;
      }
      local_54 = local_34 * *(float *)(iVar1 + 0xc4);
      local_18 = 0;
      local_20 = 0;
      local_28 = 0;
      local_2c = 0;
      local_3c = 0;
      local_40 = 0;
      local_44 = local_34 * *(float *)(iVar1 + 200);
      local_48 = 0;
      local_50 = 0;
      local_5c = 0;
      local_c = 0;
      local_10 = 0;
      local_34 = local_34 * *(float *)(iVar1 + 0xcc);
      local_24 = local_4 * *(float *)(iVar1 + 0x10c);
      local_14 = local_4 * *(float *)(iVar1 + 0x110);
      local_4 = local_4 * *(float *)(iVar1 + 0x114);
      local_4c = local_60;
      local_38 = local_60;
      if (DAT_007c118c < 0xffff0101) {
        puStack_80 = &local_60;
        uStack_84 = 0xd;
        local_58 = 0.01;
        local_30 = 0x3ecccccd;
        local_1c = 0x3ecccccd;
        piStack_88 = DAT_0071d174;
        uStack_8c = 0x52c696;
        (**(code **)(*DAT_0071d174 + 0x178))();
        uStack_8c = (uint)*(ushort *)((int)unaff_EBX + 0x10);
        uStack_90 = 0;
        uStack_94 = 0;
        piStack_98 = (int *)0x0;
        uStack_9c = 0x52c6b1;
        chimera__rasterizer_set_texture();
        uStack_9c = (uint)*(ushort *)((int)unaff_EBX + 0x10);
        uStack_a0 = 0;
        uStack_a4 = 0;
        piStack_a8 = (int *)0x1;
        uStack_ac = 0x52c6ce;
        chimera__rasterizer_set_texture();
      }
      else {
        local_58 = local_6c;
        puStack_80 = &local_60;
        uStack_84 = 0xd;
        piStack_88 = DAT_0071d174;
        uStack_8c = 0x52c705;
        local_30 = local_8;
        local_1c = local_8;
        (**(code **)(*DAT_0071d174 + 0x178))();
        uStack_8c = (uint)*(ushort *)((int)unaff_EBX + 0x10);
        uStack_90 = 0;
        uStack_94 = 1;
        piStack_98 = (int *)0x0;
        uStack_9c = 0x52c71d;
        chimera__rasterizer_set_texture();
        uStack_8c = 1;
        uStack_90 = 3;
        uStack_94 = 0;
        piStack_98 = DAT_0071d174;
        uStack_9c = 0x52c734;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_9c = (uint)*(ushort *)((int)unaff_EBX + 0x10);
        uStack_a0 = 0;
        uStack_a4 = 1;
        piStack_a8 = (int *)0x1;
        uStack_ac = 0x52c74c;
        chimera__rasterizer_set_texture();
        uStack_9c = 1;
        uStack_a0 = 3;
        uStack_a4 = 1;
        piStack_a8 = DAT_0071d174;
        uStack_ac = 0x52c763;
        (**(code **)(*DAT_0071d174 + 0x114))();
      }
      uStack_8c = 1;
      uStack_90 = 1;
      uStack_94 = 0;
      piStack_98 = DAT_0071d174;
      uStack_9c = 0x52c777;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_9c = 1;
      uStack_a0 = 2;
      uStack_a4 = 0;
      piStack_a8 = DAT_0071d174;
      uStack_ac = 0x52c78b;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_ac = 2;
      uStack_b0 = 5;
      uStack_b4 = 0;
      piStack_b8 = DAT_0071d174;
      uStack_bc = 0x52c79f;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_bc = 2;
      uStack_c0 = 6;
      uStack_c4 = 0;
      piStack_c8 = DAT_0071d174;
      uStack_cc = 0x52c7b3;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_cc = 2;
      uStack_d0 = 7;
      uStack_d4 = 0;
      piStack_d8 = DAT_0071d174;
      uStack_dc = 0x52c7c7;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_dc = 1;
      uStack_e0 = 1;
      uStack_e4 = 1;
      piStack_e8 = DAT_0071d174;
      uStack_ec = 0x52c7db;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_ec = 1;
      uStack_f0 = 2;
      uStack_f4 = 1;
      piStack_f8 = DAT_0071d174;
      uStack_fc = 0x52c7ef;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_fc = 2;
      uStack_100 = 5;
      uStack_104 = 1;
      piStack_108 = DAT_0071d174;
      uStack_10c = 0x52c803;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_10c = 2;
      uStack_110 = 6;
      uStack_114 = 1;
      piStack_118 = DAT_0071d174;
      uStack_11c = 0x52c817;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_11c = 2;
      uStack_120 = 7;
      uStack_124 = 1;
      piStack_128 = DAT_0071d174;
      uStack_12c = 0x52c82b;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_12c = 1;
      uStack_130 = 0x16;
      piStack_134 = DAT_0071d174;
      uStack_138 = 0x52c83d;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_138 = 7;
      uStack_13c = 0xa8;
      piStack_140 = DAT_0071d174;
      uStack_144 = 0x52c852;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_144 = 1;
      uStack_148 = 0x1b;
      piStack_14c = DAT_0071d174;
      uStack_150 = 0x52c864;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_150 = 5;
      uStack_154 = 0x13;
      piStack_158 = DAT_0071d174;
      uStack_15c = 0x52c876;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_15c = 2;
      uStack_160 = 0x14;
      piStack_164 = DAT_0071d174;
      fStack_168 = 7.602448e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_168 = 1.4013e-45;
      fStack_16c = 2.39622e-43;
      piStack_170 = DAT_0071d174;
      fStack_174 = 7.602477e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_174 = 0.0;
      fStack_178 = 2.10195e-44;
      piStack_17c = DAT_0071d174;
      fStack_180 = 7.602502e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_180 = 1.4013e-45;
      fStack_184 = 9.80909e-45;
      piStack_188 = DAT_0071d174;
      uStack_18c = 0x52c8c1;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_18c = 0;
      uStack_190 = 0xe;
      piStack_194 = DAT_0071d174;
      fStack_198 = 7.602553e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_198 = 5.60519e-45;
      uStack_19c = 0x17;
      piStack_1a0 = DAT_0071d174;
      uStack_1a4 = 0x52c8e5;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_1a4 = 0;
      uStack_1a8 = 0x1c;
      piStack_1ac = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      piStack_194 = (int *)0x3f800000;
      uStack_190 = 0x3f800000;
      uStack_18c = 0x3f800000;
      fStack_184 = (*(float *)(iVar1 + 100) - *(float *)(iVar1 + 0x74)) * *pfVar4;
      piStack_188 = (int *)0x3f800000;
      fStack_180 = (*(float *)(iVar1 + 0x68) - *(float *)(iVar1 + 0x78)) * pfVar4[1];
      piStack_17c = (int *)((*(float *)(iVar1 + 0x6c) - *(float *)(iVar1 + 0x7c)) * pfVar4[2]);
      fStack_178 = (*(float *)(iVar1 + 0x60) - *(float *)(iVar1 + 0x70)) * fStack_198;
      fStack_174 = *pfVar4 * *(float *)(iVar1 + 0x74);
      piStack_170 = (int *)(*(float *)(iVar1 + 0x78) * pfVar4[1]);
      fStack_16c = *(float *)(iVar1 + 0x7c) * pfVar4[2];
      fStack_168 = fStack_198 * *(float *)(iVar1 + 0x70);
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&piStack_194,3);
      piVar7 = DAT_0069d990;
      (**(code **)(*DAT_0069d990 + 0x100))(DAT_0069d990,&piStack_1ac,3);
      piVar6 = DAT_0071d174;
      iVar1 = *DAT_0071d174;
      sVar2 = FUN_00515400();
      (**(code **)(iVar1 + 0x15c))(piVar6,(&DAT_006e1a90)[sVar2 * 3]);
      piVar6 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e528);
      piVar3 = (int *)0x0;
      if (piVar7 != (int *)0x0) {
        do {
          (**(code **)(*DAT_0069d990 + 0x104))(DAT_0069d990,piVar3);
          rasterizer_transparent_geometry_group_draw_vertices(unaff_EBX,(void *)0x0,(char)piVar6);
          piVar3 = (int *)((int)piVar3 + 1);
        } while (piVar3 < piVar7);
      }
      (**(code **)(*DAT_0069d990 + 0x108))(DAT_0069d990);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
