// rasterizer_shader_model_draw_limited  (Ghidra: FUN_00528be0, unnamed; the earlier placeholder
//   called it rasterizer_shader_environment_draw_limited)
// address 0x528be0, size 1614 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: installed in 0x007c0474 by rasterizer_shader_environment_select_draw_functions
//   0x52b630 when D3DCAPS9.MaxStreams <= 1, and called through it by
//   rasterizer_shader_environment_draw_dispatch 0x52b050 for every shader type but 3. Every
//   shader offset it reads is ShaderModel (types/tags.h): +0x28 flags (two_sided 2,
//   not_alpha_tested 4, alpha_blended_decal 8, disable_two_sided_culling 0x20), +0x4c
//   change_color_source, +0x9c/+0xa0 map_u/v_scale, +0xb0 base_map.tag_id, +0xfc the u/v/rotation
//   texture animation block. Rebuilt from the raw disassembly (Ghidra lost EBX, all eight
//   arguments and every device call argument).
// What it does: one fixed function pass of the base map modulated by the vertex color and a
//   TFACTOR alpha (change color and camouflage fade), with the texture animation as the
//   TEXTURE0 transform; models with the fixed function fog flag go through declaration 14 and no
//   vertex shader, the rest through vertex shader 27 into a ProcessVertices copy of the vertex
//   buffer (declaration 15).
// register convention: all seven arguments on the stack; rasterizer_shader_environment_draw_dispatch
//   0x52b050 pushes its own EBX (the dynamic vertex slot) as the seventh.
// blam-cc: stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern rasterizer_frame_time rasterizer_time;               // 0x007c1200
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern rasterizer_model_draw_context *rasterizer_active_model_context; // 0x0071d1f0
extern uint8_t rasterizer_camouflage_fade_active;           // 0x0071d1fe
extern float rasterizer_camouflage_fade;                    // 0x0071d200
extern uint8_t rasterizer_fog_enabled;                      // 0x0069c6a8
extern const ColorRGB *global_white_color;                  // 0x00686b04
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

extern double sqrt(double x);                   // inline x87 fsqrt
extern void rasterizer_apply_decal_zbias(void); // 0x5194e0
extern void rasterizer_clear_decal_zbias(void); // 0x519580
// blam-cc: ECX -> function_source, ESI -> animation, EBX -> out_u, EDI -> out_v, stack -> the rest
extern void shader_texture_animation_evaluate(const void *function_source, const void *animation,
                                              float *out_u, float *out_v, float u_scale, float v_scale,
                                              float unused_z, float unused_w, float unused_5,
                                              float time); // 0x53fe50
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
// blam-cc: ESI -> vertex_buffer
extern uint32_t rasterizer_dynamic_vertex_process_and_get_handle(rasterizer_vertex_buffer *vertex_buffer); // 0x51c790
// blam-cc: stack -> (index_buffer, dynamic_index_slot, vertex_buffer), EAX -> primitive_count,
//   ECX -> first_primitive, EBX -> dynamic_vertex_slot
extern void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                                      rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count,
                                                      int32_t first_primitive, int32_t dynamic_vertex_slot); // 0x51c730

typedef int32_t (*d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (*d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (*d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (*d3d_set_transform_fn)(void *self, uint32_t state, const float *matrix);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}

void rasterizer_shader_model_draw_limited(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
                                          int32_t dynamic_index_slot, int32_t primitive_count,
                                          rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    rasterizer_model_draw_context *context = rasterizer_active_model_context;
    uint8_t decal = (shader[0x28] >> 3) & 1;                    // alpha_blended_decal
    uint16_t flags;
    uint8_t cull = 1;
    ColorRGB color;
    float texture_matrix[4][4];
    float alpha;
    uint32_t factor;
    int i, j;

    if (context->flags & 8) {
        set_render_state(0x07, 0);    // ZENABLE off
    } else {
        set_render_state(0x07, 1);
        set_render_state(0x0e, decal ? 0 : 1);
        set_render_state(0x17, 4);
        if (decal) {
            rasterizer_apply_decal_zbias();
        } else {
            rasterizer_clear_decal_zbias();
        }
    }
    flags = *(uint16_t *)(shader + 0x28);
    if (flags & 2) {                                           // two_sided
        if (flags & 0x20) {                                    // disable_two_sided_culling
            float dx, dy, dz;

            context = rasterizer_active_model_context;
            cull = 0;
            dx = context->center.x - rasterizer_window.camera.position.x;
            dy = context->center.y - rasterizer_window.camera.position.y;
            dz = context->center.z - rasterizer_window.camera.position.z;
            if (!((float)sqrt(dx * dx + dy * dy + dz * dz) > 8.0f)) {
                cull = 1;
            }
        } else {
            cull = 1;
        }
    }
    set_render_state(0x16, cull ? 3 : 1);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, (decal || rasterizer_camouflage_fade_active) ? 1 : 0);
    set_render_state(0x13, 5);        // SRCALPHA
    set_render_state(0x14, 6);        // INVSRCALPHA
    set_render_state(0xab, 1);
    set_render_state(0x0f, (!rasterizer_camouflage_fade_active && !(shader[0x28] & 4)) ? 1 : 0);
    set_render_state(0x18, 0x7f);     // ALPHAREF
    set_render_state(0x1c, rasterizer_fog_enabled ? 1 : 0);

    context = rasterizer_active_model_context;
    {
        int16_t source = *(int16_t *)(shader + 0x4c);           // change_color_source

        if (source > 0 && source < 5) {
            const ColorRGB *colors = (const ColorRGB *)(uintptr_t)context->unknown_84[0];

            color = colors[source - 1];
        } else {
            color = *global_white_color;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            texture_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    shader_texture_animation_evaluate(&context->unknown_84[0], shader + 0xfc, texture_matrix[0], texture_matrix[1],
                                      context->unknown_c4 * *(float *)(shader + 0x9c),
                                      context->unknown_c8 * *(float *)(shader + 0xa0), 0.0f, 0.0f, 0.0f,
                                      (float)rasterizer_time.time);
    if (shader[0x28] & 2) {
        set_render_state(0x16, 1);    // two sided: CULLMODE NONE after all
    }
    alpha = rasterizer_camouflage_fade_active ? rasterizer_camouflage_fade : 1.0f;
    factor = (uint32_t)(int32_t)(alpha * 255.0f) << 8;                                     // __ftol
    factor = (factor | ((uint32_t)(int32_t)(color.red * 255.0f) & 0xff)) << 8;
    factor = (factor | ((uint32_t)(int32_t)(color.green * 255.0f) & 0xff)) << 8;
    factor = factor | ((uint32_t)(int32_t)(color.blue * 255.0f) & 0xff);
    set_render_state(0x3c, factor);   // TEXTUREFACTOR
    set_texture_stage_state(0, 1, 4); // COLOROP MODULATE
    set_texture_stage_state(0, 2, 2); // COLORARG1 TEXTURE
    set_texture_stage_state(0, 3, 0); // COLORARG2 DIFFUSE
    set_texture_stage_state(0, 4, 4); // ALPHAOP MODULATE
    set_texture_stage_state(0, 5, 2); // ALPHAARG1 TEXTURE
    set_texture_stage_state(0, 6, 3); // ALPHAARG2 TFACTOR
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);

    if (rasterizer_active_model_context->flags & 0x200) {
        // fixed function fog models: pretransformed declaration 14, no vertex shader
        ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0);
        ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[14].declaration);
        set_texture_stage_state(0, 0x18, 2);                  // TEXTURETRANSFORMFLAGS COUNT2
        chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0xb0), 0, 0, 1, frame);   // base_map
        ((d3d_set_transform_fn)device_vtable()[0xb0 / 4])(rasterizer_device, 0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, 0x18, 2);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                  dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
    } else {
        // skinned in vertex shader 27 into a processed copy, then drawn with declaration 15
        rasterizer_vertex_buffer processed = *vertex_buffer;

        ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[4].declaration);
        ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[27].shader);
        processed.hardware_buffer = index_buffer != NULL ? rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer) : 0;
        processed.type = _rasterizer_vertex_type_model_processed;
        ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0);
        ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[15].declaration);
        ((d3d_set_transform_fn)device_vtable()[0xb0 / 4])(rasterizer_device, 0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, 0x18, 2);
        chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0xb0), 0, 0, 1, frame);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                  dynamic_vertex_slot);
        set_texture_stage_state(0, 0x18, 0);
    }
    if (rasterizer_caps.raster_caps & 0x04000000) {
        set_render_state(0xc3, 0);
    }
    if (rasterizer_caps.raster_caps & 0x02000000) {
        set_render_state(0xaf, 0);
    }
}

#if 0
Original Ghidra decompilation (0x528be0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00528be0(int param_1)

{
  undefined4 *puVar1;
  short sVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  byte bVar9;
  int *piStack_194;
  undefined4 uStack_190;
  undefined4 *puStack_18c;
  int *piStack_188;
  undefined4 uStack_184;
  int *piStack_180;
  undefined4 uStack_17c;
  int *piStack_178;
  undefined4 uStack_174;
  int *piStack_170;
  undefined4 uStack_16c;
  int *piStack_168;
  undefined4 uStack_164;
  undefined4 uStack_160;
  undefined4 uStack_15c;
  int *piStack_158;
  undefined4 uStack_154;
  undefined4 uStack_150;
  undefined4 uStack_14c;
  int *piStack_148;
  undefined4 uStack_144;
  undefined4 uStack_140;
  undefined4 uStack_13c;
  int *piStack_138;
  int iStack_134;
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
  float fStack_f4;
  float fStack_f0;
  undefined4 *puStack_ec;
  int *piStack_e8;
  undefined4 uStack_e4;
  float fStack_e0;
  int *piStack_dc;
  undefined4 uStack_d8;
  uint uStack_d4;
  int *piStack_d0;
  undefined4 uStack_cc;
  undefined4 uStack_c8;
  int *piStack_c4;
  undefined4 uStack_c0;
  undefined4 uStack_bc;
  int *piStack_b8;
  undefined4 uStack_b4;
  undefined4 uStack_b0;
  int *piStack_ac;
  undefined4 uStack_a8;
  undefined4 uStack_a4;
  int *piStack_a0;
  undefined4 uStack_9c;
  undefined4 uStack_98;
  int *piStack_94;
  undefined4 uStack_90;
  undefined4 uStack_8c;
  int *piStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  int *piStack_7c;
  undefined4 uStack_78;
  uint uStack_74;
  int *piStack_70;
  undefined4 uStack_6c;
  undefined4 uStack_68;
  char cStack_8;
  
  bVar9 = *(byte *)(param_1 + 0x28) >> 3 & 1;
  if ((*DAT_0071d1f0 & 8) == 0) {
    uStack_68 = 1;
    uStack_6c = 7;
    piStack_70 = DAT_0071d174;
    uStack_74 = 0x528c26;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_74 = (uint)(bVar9 == 0);
    uStack_78 = 0xe;
    piStack_7c = DAT_0071d174;
    uStack_80 = 0x528c3e;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_80 = 4;
    uStack_84 = 0x17;
    piStack_88 = DAT_0071d174;
    uStack_8c = 0x528c50;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    if (bVar9 == 0) {
      uStack_74 = 0x528c60;
      rasterizer_clear_decal_zbias();
    }
    else {
      uStack_74 = 0x528c59;
      FUN_005194e0();
    }
  }
  else {
    uStack_68 = 0;
    uStack_6c = 7;
    piStack_70 = DAT_0071d174;
    uStack_74 = 0x528c17;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  if (((*(ushort *)(param_1 + 0x28) & 2) != 0) &&
     (((*(ushort *)(param_1 + 0x28) & 0x20) == 0 ||
      (cStack_8 = '\0',
      SQRT(((float)DAT_0071d1f0[0x2d] - DAT_007c1228) * ((float)DAT_0071d1f0[0x2d] - DAT_007c1228) +
           ((float)DAT_0071d1f0[0x2f] - DAT_007c1230) * ((float)DAT_0071d1f0[0x2f] - DAT_007c1230) +
           ((float)DAT_0071d1f0[0x2e] - DAT_007c122c) * ((float)DAT_0071d1f0[0x2e] - DAT_007c122c))
      <= 8.0)))) {
    cStack_8 = '\x01';
  }
  uStack_74 = (uint)(cStack_8 != '\0') * 2 + 1;
  uStack_78 = 0x16;
  piStack_7c = DAT_0071d174;
  uStack_80 = 0x528ce5;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_80 = 7;
  uStack_84 = 0xa8;
  piStack_88 = DAT_0071d174;
  uStack_8c = 0x528cfa;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  if ((bVar9 == 0) && (DAT_0071d1fe == '\0')) {
    uStack_8c = 0;
  }
  else {
    uStack_8c = 1;
  }
  uStack_90 = 0x1b;
  piStack_94 = DAT_0071d174;
  uStack_98 = 0x528d21;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_98 = 5;
  uStack_9c = 0x13;
  piStack_a0 = DAT_0071d174;
  uStack_a4 = 0x528d33;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_a4 = 6;
  uStack_a8 = 0x14;
  piStack_ac = DAT_0071d174;
  uStack_b0 = 0x528d45;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_b0 = 1;
  uStack_b4 = 0xab;
  piStack_b8 = DAT_0071d174;
  uStack_bc = 0x528d5a;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  if ((DAT_0071d1fe == '\0') && ((*(byte *)(param_1 + 0x28) & 4) == 0)) {
    uStack_bc = 1;
  }
  else {
    uStack_bc = 0;
  }
  uStack_c0 = 0xf;
  piStack_c4 = DAT_0071d174;
  uStack_c8 = 0x528d83;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_c8 = 0x7f;
  uStack_cc = 0x18;
  piStack_d0 = DAT_0071d174;
  uStack_d4 = 0x528d95;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_d4 = (uint)(DAT_0069c6a8 != '\0');
  uStack_d8 = 0x1c;
  piStack_dc = DAT_0071d174;
  fStack_e0 = 7.581343e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  sVar2 = *(short *)(param_1 + 0x4c);
  if ((sVar2 < 1) || (4 < sVar2)) {
    uStack_cc = *(undefined4 *)PTR_DAT_00686b04;
    uStack_c8 = *(undefined4 *)(PTR_DAT_00686b04 + 4);
    piStack_c4 = *(int **)(PTR_DAT_00686b04 + 8);
  }
  else {
    puVar1 = (undefined4 *)((DAT_0071d1f0[0x21] - 0xc) + sVar2 * 0xc);
    uStack_cc = *puVar1;
    uStack_c8 = puVar1[1];
    piStack_c4 = (int *)puVar1[2];
  }
  fStack_e0 = (float)_DAT_007c1200;
  uStack_e4 = 0;
  uStack_80 = 0;
  uStack_84 = 0;
  piStack_88 = (int *)0x0;
  uStack_8c = 0;
  piStack_94 = (int *)0x0;
  uStack_98 = 0;
  uStack_9c = 0;
  piStack_a0 = (int *)0x0;
  uStack_a8 = 0;
  piStack_ac = (int *)0x0;
  uStack_b0 = 0;
  uStack_b4 = 0;
  piStack_7c = (int *)0x3f800000;
  uStack_90 = 0x3f800000;
  uStack_a4 = 0x3f800000;
  piStack_b8 = (int *)0x3f800000;
  fStack_f0 = (float)DAT_0071d1f0[0x32] * *(float *)(param_1 + 0xa0);
  piStack_e8 = (int *)0x0;
  puStack_ec = (undefined4 *)0x0;
  fStack_f4 = (float)DAT_0071d1f0[0x31] * *(float *)(param_1 + 0x9c);
  piStack_f8 = (int *)0x528ecf;
  shader_texture_animation_evaluate();
  if ((*(byte *)(param_1 + 0x28) & 2) != 0) {
    fStack_e0 = 1.4013e-45;
    uStack_e4 = 0x16;
    piStack_e8 = DAT_0071d174;
    puStack_ec = (undefined4 *)0x528eeb;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  piVar4 = DAT_0071d174;
  iVar3 = *DAT_0071d174;
  fStack_e0 = 7.581839e-39;
  iVar5 = __ftol();
  fStack_e0 = 7.581867e-39;
  uVar6 = __ftol();
  fStack_e0 = 7.581902e-39;
  uVar7 = __ftol();
  fStack_e0 = 7.581937e-39;
  uVar8 = __ftol();
  fStack_e0 = (float)(((iVar5 << 8 | uVar6 & 0xff) << 8 | uVar7 & 0xff) << 8 | uVar8 & 0xff);
  uStack_e4 = 0x3c;
  piStack_e8 = piVar4;
  puStack_ec = (undefined4 *)0x528f6c;
  (**(code **)(iVar3 + 0xe4))();
  puStack_ec = (undefined4 *)&DAT_00000004;
  fStack_f0 = 1.4013e-45;
  fStack_f4 = 0.0;
  piStack_f8 = DAT_0071d174;
  uStack_fc = 0x528f80;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_fc = 2;
  uStack_100 = 2;
  uStack_104 = 0;
  piStack_108 = DAT_0071d174;
  uStack_10c = 0x528f94;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_10c = 0;
  uStack_110 = 3;
  uStack_114 = 0;
  piStack_118 = DAT_0071d174;
  uStack_11c = 0x528fa8;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_11c = 4;
  uStack_120 = 4;
  uStack_124 = 0;
  piStack_128 = DAT_0071d174;
  uStack_12c = 0x528fbc;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_12c = 2;
  uStack_130 = 5;
  iStack_134 = 0;
  piStack_138 = DAT_0071d174;
  uStack_13c = 0x528fd0;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_13c = 3;
  uStack_140 = 6;
  uStack_144 = 0;
  piStack_148 = DAT_0071d174;
  uStack_14c = 0x528fe4;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_14c = 1;
  uStack_150 = 1;
  uStack_154 = 1;
  piStack_158 = DAT_0071d174;
  uStack_15c = 0x528ff8;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_15c = 1;
  uStack_160 = 4;
  uStack_164 = 1;
  piStack_168 = DAT_0071d174;
  uStack_16c = 0x52900c;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  if ((*DAT_0071d1f0 & 0x200) == 0) {
    piStack_158 = (int *)*puStack_ec;
    uStack_154 = puStack_ec[1];
    uStack_150 = puStack_ec[2];
    uStack_14c = puStack_ec[3];
    piStack_148 = (int *)puStack_ec[4];
    uStack_16c = DAT_006e1ac0;
    piStack_170 = DAT_0071d174;
    uStack_174 = 0x529115;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_174 = DAT_0069e428;
    piStack_178 = DAT_0071d174;
    uStack_17c = 0x52912a;
    (**(code **)(*DAT_0071d174 + 0x170))();
    piVar4 = piStack_108;
    if (piStack_108 == (int *)0x0) {
      piStack_158 = (int *)0x0;
    }
    else {
      uStack_17c = 0x529137;
      piStack_158 = (int *)FUN_0051c790();
    }
    uStack_17c = 0;
    piStack_180 = DAT_0071d174;
    piStack_168 = (int *)CONCAT22(piStack_168._2_2_,0xf);
    uStack_184 = 0x529156;
    (**(code **)(*DAT_0071d174 + 0x170))();
    uStack_184 = DAT_006e1b44;
    piStack_188 = DAT_0071d174;
    puStack_18c = (undefined4 *)0x52916b;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    puStack_18c = &uStack_164;
    uStack_190 = 0x10;
    piStack_194 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0xb0))();
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,0x18,2);
    chimera__rasterizer_set_texture(0,0,1,piStack_138);
    rasterizer_dynamic_geometry_draw_dispatch((int)piVar4,uStack_130,(int)&piStack_194);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,0x18,0);
  }
  else {
    uStack_16c = 0;
    piStack_170 = DAT_0071d174;
    uStack_174 = 0x52902d;
    (**(code **)(*DAT_0071d174 + 0x170))();
    uStack_174 = DAT_006e1b38;
    piStack_178 = DAT_0071d174;
    uStack_17c = 0x529042;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_17c = 2;
    piStack_180 = (int *)&DAT_00000018;
    uStack_184 = 0;
    piStack_188 = DAT_0071d174;
    puStack_18c = (undefined4 *)0x529056;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    puStack_18c = (undefined4 *)uStack_11c;
    uStack_190 = 1;
    piStack_194 = (int *)0x0;
    chimera__rasterizer_set_texture(0);
    puStack_18c = &uStack_164;
    uStack_190 = 0x10;
    piStack_194 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0xb0))();
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,0x18,2);
    rasterizer_dynamic_geometry_draw_dispatch(iStack_134,uStack_130,(int)piStack_128);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,0x18,0);
  }
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,0);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,0);
  }
  return;
}
#endif
