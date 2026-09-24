// rasterizer_shader_model_draw_fixed_function  (Ghidra: FUN_00529230, unnamed; the earlier
//   placeholder called it rasterizer_shader_environment_draw_multitexture)
// address 0x529230, size 3020 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: installed in 0x007c0474 by rasterizer_shader_environment_select_draw_functions
//   0x52b630 when the device has more than one stream but no ps_1_1, and called through it by
//   rasterizer_shader_environment_draw_dispatch 0x52b050 for every shader type but 3. The prologue
//   (z, cull, blend, alpha test, fog, change color, texture animation, TFACTOR) is byte for byte
//   the one of rasterizer_shader_model_draw_limited 0x528be0; the shader offsets are ShaderModel
//   (types/tags.h), +0xc8 being multipurpose_map.tag_id. Rebuilt from the raw disassembly.
// What it does: fixed function fog models draw the base map in one pass (declaration 14). The
//   rest are skinned into a ProcessVertices copy (vertex shader 27, declaration 15) and drawn
//   either in one pass, or, with a change color source other than 2, in two: base map times the
//   multipurpose map in black fog, then an additive fog colored pass with z EQUAL.
// register convention: all seven arguments on the stack; rasterizer_shader_environment_draw_dispatch
//   0x52b050 pushes its own EBX (the dynamic vertex slot) as the seventh.
// blam-cc: stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot)
// UNSURE: why change color source 2 takes the single pass path, and the doubled ALPHAARG1 of stage
//   0 in the opaque two pass setup (0 then 2; the binary sets ALPHAARG1 twice and never ALPHAARG2).
// reconciled: R43 rasterizer_model_draw_context unknown_84[2] -> change_colors/function_values (the render_animation pair), unknown_c0/c4/c8 -> bounding_radius/base_map_u_scale/base_map_v_scale (same offsets)

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
extern uint32_t color_rgb_float_to_int(const ColorRGB *color); // 0x4ab5d0
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
static void set_stage(uint32_t stage, uint32_t color_op, uint32_t color_arg1, uint32_t color_arg2,
                      uint32_t alpha_op, uint32_t alpha_arg1, uint32_t alpha_arg2)
{
    set_texture_stage_state(stage, 1, color_op);
    set_texture_stage_state(stage, 2, color_arg1);
    set_texture_stage_state(stage, 3, color_arg2);
    set_texture_stage_state(stage, 4, alpha_op);
    set_texture_stage_state(stage, 5, alpha_arg1);
    set_texture_stage_state(stage, 6, alpha_arg2);
}
static void set_transform(uint32_t state, const float *matrix)
{
    ((d3d_set_transform_fn)device_vtable()[0xb0 / 4])(rasterizer_device, state, matrix);
}

void rasterizer_shader_model_draw_fixed_function(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
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
    int16_t source;
    int i, j;

    // prologue shared with rasterizer_shader_model_draw_limited 0x528be0
    if (context->flags & 8) {
        set_render_state(0x07, 0);
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
    if (flags & 2) {
        if (flags & 0x20) {
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
    set_render_state(0x13, 5);
    set_render_state(0x14, 6);
    set_render_state(0xab, 1);
    set_render_state(0x0f, (!rasterizer_camouflage_fade_active && !(shader[0x28] & 4)) ? 1 : 0);
    set_render_state(0x18, 0x7f);
    set_render_state(0x1c, rasterizer_fog_enabled ? 1 : 0);

    context = rasterizer_active_model_context;
    source = *(int16_t *)(shader + 0x4c);                         // change_color_source
    if (source > 0 && source < 5) {
        color = ((const ColorRGB *)(uintptr_t)context->change_colors)[source - 1];
    } else {
        color = *global_white_color;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            texture_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    shader_texture_animation_evaluate(&context->change_colors, shader + 0xfc, texture_matrix[0], texture_matrix[1],
                                      context->base_map_u_scale * *(float *)(shader + 0x9c),
                                      context->base_map_v_scale * *(float *)(shader + 0xa0), 0.0f, 0.0f, 0.0f,
                                      (float)rasterizer_time.time);
    if (shader[0x28] & 2) {
        set_render_state(0x16, 1);
    }
    alpha = rasterizer_camouflage_fade_active ? rasterizer_camouflage_fade : 1.0f;
    factor = (uint32_t)(int32_t)(alpha * 255.0f) << 8;                                     // __ftol
    factor = (factor | ((uint32_t)(int32_t)(color.red * 255.0f) & 0xff)) << 8;
    factor = (factor | ((uint32_t)(int32_t)(color.green * 255.0f) & 0xff)) << 8;
    factor = factor | ((uint32_t)(int32_t)(color.blue * 255.0f) & 0xff);
    set_render_state(0x3c, factor);

    if (rasterizer_active_model_context->flags & 0x200) {
        ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0);
        ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[14].declaration);
        set_texture_stage_state(0, 0x18, 2);
        chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0xb0), 0, 0, 1, frame);   // base_map
        set_transform(0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, 0x18, 2);
        set_stage(0, 4, 2, 0, 4, 2, 3);   // TEXTURE x DIFFUSE, TEXTURE x TFACTOR
        set_texture_stage_state(1, 1, 1);
        set_texture_stage_state(1, 4, 1);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                  dynamic_vertex_slot);
    } else {
        rasterizer_vertex_buffer processed = *vertex_buffer;

        ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[4].declaration);
        ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[27].shader);
        processed.hardware_buffer = index_buffer != NULL ? rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer) : 0;
        processed.type = _rasterizer_vertex_type_model_processed;
        ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0);
        ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[15].declaration);
        set_transform(0x10, &texture_matrix[0][0]);
        set_texture_stage_state(0, 0x18, 2);

        source = *(int16_t *)(shader + 0x4c);
        if (source > 0 && source != 2) {
            // pass 1: base map x multipurpose map, fog black
            set_texture_stage_state(1, 0xb, 0);                  // TEXCOORDINDEX 0
            set_transform(0x11, &texture_matrix[0][0]);
            set_texture_stage_state(1, 0x18, 2);
            set_render_state(0x22, 0xff000000);                  // FOGCOLOR black
            chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0xb0), 0, 0, 1, frame);   // base_map
            chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0xc8), 1, 0, 1, frame);   // multipurpose_map
            set_render_state(0x1b, 1);
            set_render_state(0x0f, 0);
            set_render_state(0x13, 5);   // SRCALPHA
            if (rasterizer_camouflage_fade_active) {
                set_render_state(0x14, 6);   // INVSRCALPHA
                set_render_state(0xab, 1);
                set_stage(0, 4, 0, 2, 4, 3, 2);                  // DIFFUSE x TEXTURE, TFACTOR x TEXTURE
                set_texture_stage_state(1, 1, 4);                // CURRENT x TFACTOR
                set_texture_stage_state(1, 2, 1);
                set_texture_stage_state(1, 3, 3);
                set_texture_stage_state(1, 4, 2);                // alpha: CURRENT
                set_texture_stage_state(1, 5, 1);
            } else {
                set_render_state(0x14, 1);   // ZERO
                set_render_state(0xab, 1);
                set_texture_stage_state(0, 1, 4);
                set_texture_stage_state(0, 2, 0);
                set_texture_stage_state(0, 3, 2);
                set_texture_stage_state(0, 4, 4);
                set_texture_stage_state(0, 5, 0);
                set_texture_stage_state(0, 5, 2);
                set_stage(1, 4, 1, 3, 4, 1, 2);                  // CURRENT x TFACTOR, CURRENT x TEXTURE
            }
            set_texture_stage_state(2, 1, 1);
            set_texture_stage_state(2, 4, 1);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                      dynamic_vertex_slot);

            // pass 2: add the fog colored remainder where the first pass wrote depth
            set_render_state(0x22, color_rgb_float_to_int(&rasterizer_window.fog.atmospheric_color)); // FOGCOLOR
            set_render_state(0x13, 5);   // SRCALPHA
            set_render_state(0x14, 2);   // ONE
            set_render_state(0xab, 1);
            set_render_state(0x0e, 0);   // ZWRITEENABLE off
            set_render_state(0x17, 3);   // ZFUNC EQUAL
            set_stage(0, 4, 2, 0, 4, 3, 2);                      // TEXTURE x DIFFUSE, TFACTOR x TEXTURE
            set_texture_stage_state(1, 1, 4);                    // COMPLEMENT(TEXTURE.a) x CURRENT
            set_texture_stage_state(1, 2, 0x32);
            set_texture_stage_state(1, 3, 1);
            set_texture_stage_state(1, 4, 2);
            set_texture_stage_state(1, 5, 1);
            set_texture_stage_state(2, 1, 1);
            set_texture_stage_state(2, 4, 1);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                      dynamic_vertex_slot);
            set_texture_stage_state(1, 0xb, 1);                  // TEXCOORDINDEX 1
            set_texture_stage_state(1, 0x18, 0);
        } else {
            chimera__rasterizer_set_texture(*(uint32_t *)(shader + 0xb0), 0, 0, 1, frame);
            set_stage(0, 4, 2, 0, 4, 2, 3);
            set_texture_stage_state(1, 1, 1);
            set_texture_stage_state(1, 4, 1);
            rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                      dynamic_vertex_slot);
        }
    }
    set_texture_stage_state(0, 0x18, 0);
    if (rasterizer_caps.raster_caps & 0x04000000) {
        set_render_state(0xc3, 0);
    }
    if (rasterizer_caps.raster_caps & 0x02000000) {
        set_render_state(0xaf, 0);
    }
}

#if 0
Original Ghidra decompilation (0x529230):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00529230(int param_1)

{
  undefined4 *puVar1;
  short sVar2;
  int iVar3;
  undefined4 uVar4;
  int *piVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  byte bVar10;
  int *piStack_370;
  undefined4 uStack_36c;
  undefined4 uStack_368;
  undefined4 uStack_364;
  int *piStack_360;
  undefined4 uStack_35c;
  undefined4 uStack_358;
  undefined4 uStack_354;
  int *piStack_350;
  undefined4 uStack_34c;
  undefined4 uStack_348;
  undefined4 uStack_344;
  int *piStack_340;
  undefined4 uStack_33c;
  undefined4 uStack_338;
  undefined4 uStack_334;
  int *piStack_330;
  undefined4 uStack_32c;
  undefined4 uStack_328;
  undefined4 uStack_324;
  int *piStack_320;
  undefined4 uStack_31c;
  undefined4 uStack_318;
  undefined4 uStack_314;
  int *piStack_310;
  undefined4 uStack_30c;
  undefined4 uStack_308;
  undefined4 uStack_304;
  int *piStack_300;
  undefined4 uStack_2fc;
  undefined4 uStack_2f8;
  undefined4 uStack_2f4;
  int *piStack_2f0;
  undefined4 uStack_2ec;
  undefined4 uStack_2e8;
  undefined4 uStack_2e4;
  int *piStack_2e0;
  undefined4 uStack_2dc;
  undefined4 uStack_2d8;
  undefined4 uStack_2d4;
  int *piStack_2d0;
  undefined4 uStack_2cc;
  undefined4 uStack_2c8;
  undefined4 uStack_2c4;
  int *piStack_2c0;
  undefined4 uStack_2bc;
  undefined4 uStack_2b8;
  undefined4 uStack_2b4;
  int *piStack_2b0;
  undefined4 uStack_2ac;
  undefined4 uStack_2a8;
  int *piStack_2a4;
  undefined4 uStack_2a0;
  undefined4 uStack_29c;
  int *piStack_298;
  undefined4 uStack_294;
  undefined4 uStack_290;
  int *piStack_28c;
  undefined4 uStack_288;
  undefined4 uStack_284;
  int *piStack_280;
  undefined4 uStack_27c;
  int *piStack_258;
  undefined4 uStack_254;
  undefined4 uStack_250;
  undefined4 uStack_24c;
  int *piStack_248;
  undefined4 uStack_244;
  undefined4 uStack_240;
  undefined4 uStack_23c;
  int *piStack_238;
  undefined4 uStack_234;
  undefined4 uStack_230;
  undefined4 uStack_22c;
  int *piStack_228;
  undefined4 uStack_224;
  undefined4 uStack_220;
  undefined4 uStack_21c;
  int *piStack_218;
  undefined4 uStack_214;
  undefined4 uStack_210;
  undefined4 uStack_20c;
  int *piStack_208;
  undefined4 uStack_204;
  undefined4 uStack_200;
  undefined4 uStack_1fc;
  int *piStack_1f8;
  undefined4 uStack_1f4;
  undefined4 uStack_1f0;
  undefined4 uStack_1ec;
  int *piStack_1e8;
  undefined4 uStack_1e4;
  undefined4 uStack_1e0;
  undefined4 uStack_1dc;
  int *piStack_1d8;
  undefined4 uStack_1d4;
  undefined4 uStack_1d0;
  undefined4 uStack_1cc;
  int *piStack_1c8;
  undefined4 uStack_1c4;
  undefined4 uStack_1c0;
  undefined4 uStack_1bc;
  int *piStack_1b8;
  int *piStack_1b4;
  int *piVar11;
  int **is_static;
  int *piStack_194;
  undefined4 uStack_190;
  int *piStack_18c;
  undefined4 uStack_188;
  int *piStack_184;
  int *piStack_180;
  undefined4 uStack_17c;
  undefined4 uStack_178;
  int *piStack_174;
  int *piStack_170;
  undefined4 uStack_16c;
  int *piStack_168;
  int *piStack_164;
  int *piStack_160;
  int *piStack_15c;
  undefined4 uStack_158;
  int *piStack_154;
  int *piStack_150;
  undefined4 uStack_14c;
  undefined4 uStack_148;
  int *piStack_144;
  int *piStack_140;
  undefined4 uStack_13c;
  undefined4 *puStack_138;
  int *piStack_134;
  undefined4 uStack_130;
  undefined4 uStack_12c;
  int **ppiStack_128;
  int *piStack_124;
  undefined4 uStack_120;
  undefined4 uStack_11c;
  undefined4 uStack_118;
  int *piStack_114;
  undefined4 uStack_110;
  undefined4 *puStack_10c;
  int *piStack_108;
  undefined4 uStack_104;
  int *piStack_100;
  undefined4 uStack_fc;
  int *piStack_f8;
  float fStack_f4;
  int *piStack_f0;
  undefined4 uStack_ec;
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
  undefined4 *puStack_6c;
  undefined4 uStack_68;
  char cStack_8;
  
  bVar10 = *(byte *)(param_1 + 0x28) >> 3 & 1;
  if ((*DAT_0071d1f0 & 8) == 0) {
    uStack_68 = 1;
    puStack_6c = (undefined4 *)0x7;
    piStack_70 = DAT_0071d174;
    uStack_74 = 0x529276;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_74 = (uint)(bVar10 == 0);
    uStack_78 = 0xe;
    piStack_7c = DAT_0071d174;
    uStack_80 = 0x52928e;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_80 = 4;
    uStack_84 = 0x17;
    piStack_88 = DAT_0071d174;
    uStack_8c = 0x5292a0;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    if (bVar10 == 0) {
      uStack_74 = 0x5292b0;
      rasterizer_clear_decal_zbias();
    }
    else {
      uStack_74 = 0x5292a9;
      FUN_005194e0();
    }
  }
  else {
    uStack_68 = 0;
    puStack_6c = (undefined4 *)0x7;
    piStack_70 = DAT_0071d174;
    uStack_74 = 0x529267;
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
  uStack_80 = 0x529335;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_80 = 7;
  uStack_84 = 0xa8;
  piStack_88 = DAT_0071d174;
  uStack_8c = 0x52934a;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  if ((bVar10 == 0) && (DAT_0071d1fe == '\0')) {
    uStack_8c = 0;
  }
  else {
    uStack_8c = 1;
  }
  uStack_90 = 0x1b;
  piStack_94 = DAT_0071d174;
  uStack_98 = 0x529371;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_98 = 5;
  uStack_9c = 0x13;
  piStack_a0 = DAT_0071d174;
  uStack_a4 = 0x529383;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_a4 = 6;
  uStack_a8 = 0x14;
  piStack_ac = DAT_0071d174;
  uStack_b0 = 0x529395;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_b0 = 1;
  uStack_b4 = 0xab;
  piStack_b8 = DAT_0071d174;
  uStack_bc = 0x5293aa;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  if ((DAT_0071d1fe == '\0') && ((*(byte *)(param_1 + 0x28) & 4) == 0)) {
    uStack_bc = 1;
  }
  else {
    uStack_bc = 0;
  }
  uStack_c0 = 0xf;
  piStack_c4 = DAT_0071d174;
  uStack_c8 = 0x5293d3;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_c8 = 0x7f;
  uStack_cc = 0x18;
  piStack_d0 = DAT_0071d174;
  uStack_d4 = 0x5293e5;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_d4 = (uint)(DAT_0069c6a8 != '\0');
  uStack_d8 = 0x1c;
  piStack_dc = DAT_0071d174;
  fStack_e0 = 7.583607e-39;
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
  piStack_f0 = (int *)((float)DAT_0071d1f0[0x32] * *(float *)(param_1 + 0xa0));
  piStack_e8 = (int *)0x0;
  uStack_ec = 0;
  fStack_f4 = (float)DAT_0071d1f0[0x31] * *(float *)(param_1 + 0x9c);
  piStack_f8 = (int *)0x52951f;
  shader_texture_animation_evaluate();
  if ((*(byte *)(param_1 + 0x28) & 2) != 0) {
    fStack_e0 = 1.4013e-45;
    uStack_e4 = 0x16;
    piStack_e8 = DAT_0071d174;
    uStack_ec = 0x52953b;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  piVar11 = DAT_0071d174;
  iVar3 = *DAT_0071d174;
  fStack_e0 = 7.584103e-39;
  iVar6 = __ftol();
  fStack_e0 = 7.584131e-39;
  uVar7 = __ftol();
  fStack_e0 = 7.584166e-39;
  uVar8 = __ftol();
  fStack_e0 = 7.584201e-39;
  uVar9 = __ftol();
  fStack_e0 = (float)(((iVar6 << 8 | uVar7 & 0xff) << 8 | uVar8 & 0xff) << 8 | uVar9 & 0xff);
  uStack_e4 = 0x3c;
  piStack_e8 = piVar11;
  uStack_ec = 0x5295bc;
  (**(code **)(iVar3 + 0xe4))();
  if ((*DAT_0071d1f0 & 0x200) == 0) {
    uStack_d8 = *puStack_6c;
    uStack_d4 = puStack_6c[1];
    piStack_d0 = (int *)puStack_6c[2];
    uStack_cc = puStack_6c[3];
    uStack_c8 = puStack_6c[4];
    uStack_ec = DAT_006e1ac0;
    piStack_f0 = DAT_0071d174;
    fStack_f4 = 7.584759e-39;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    fStack_f4 = (float)DAT_0069e428;
    piStack_f8 = DAT_0071d174;
    uStack_fc = 0x52974e;
    (**(code **)(*DAT_0071d174 + 0x170))();
    piVar11 = piStack_88;
    if (piStack_88 == (int *)0x0) {
      uStack_d8 = 0;
    }
    else {
      uStack_fc = 0x52975b;
      uStack_d8 = FUN_0051c790();
    }
    uStack_fc = 0;
    piStack_100 = DAT_0071d174;
    piStack_e8 = (int *)CONCAT22(piStack_e8._2_2_,0xf);
    uStack_104 = 0x52977a;
    (**(code **)(*DAT_0071d174 + 0x170))();
    uStack_104 = DAT_006e1b44;
    piStack_108 = DAT_0071d174;
    puStack_10c = (undefined4 *)0x52978f;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    puStack_10c = &uStack_e4;
    uStack_110 = 0x10;
    piStack_114 = DAT_0071d174;
    uStack_118 = 0x5297a4;
    (**(code **)(*DAT_0071d174 + 0xb0))();
    uStack_118 = 2;
    uStack_11c = 0x18;
    uStack_120 = 0;
    piStack_124 = DAT_0071d174;
    ppiStack_128 = (int **)0x5297b8;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    if ((0 < *(short *)(param_1 + 0x4c)) && (*(short *)(param_1 + 0x4c) != 2)) {
      ppiStack_128 = (int **)0x0;
      uStack_12c = 0xb;
      uStack_130 = 1;
      piStack_134 = DAT_0071d174;
      puStack_138 = (undefined4 *)0x5297e3;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      puStack_138 = &uStack_110;
      uStack_13c = 0x11;
      piStack_140 = DAT_0071d174;
      piStack_144 = (int *)0x5297f8;
      (**(code **)(*DAT_0071d174 + 0xb0))();
      piStack_144 = (int *)0x2;
      uStack_148 = 0x18;
      uStack_14c = 1;
      piStack_150 = DAT_0071d174;
      piStack_154 = (int *)0x52980c;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      piStack_154 = (int *)0xff000000;
      uStack_158 = 0x22;
      piStack_15c = DAT_0071d174;
      piStack_160 = (int *)0x529821;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      piVar5 = piStack_f0;
      piStack_160 = piStack_f0;
      piStack_164 = (int *)0x1;
      piStack_168 = (int *)0x0;
      uStack_16c = 0;
      piStack_170 = (int *)0x529837;
      chimera__rasterizer_set_texture();
      piStack_170 = piVar5;
      piStack_174 = (int *)0x1;
      uStack_178 = 0;
      uStack_17c = 1;
      piStack_180 = (int *)0x529849;
      chimera__rasterizer_set_texture();
      piStack_160 = (int *)0x1;
      piStack_164 = (int *)0x1b;
      piStack_168 = DAT_0071d174;
      uStack_16c = 0x52985e;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_16c = 0;
      piStack_170 = (int *)0xf;
      piStack_174 = DAT_0071d174;
      uStack_178 = 0x529870;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_178 = 5;
      uStack_17c = 0x13;
      piStack_180 = DAT_0071d174;
      if (DAT_0071d1fe == '\0') {
        piStack_184 = (int *)0x52999d;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        piStack_184 = (int *)0x1;
        uStack_188 = 0x14;
        piStack_18c = DAT_0071d174;
        uStack_190 = 0x5299af;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_190 = 1;
        piStack_194 = (int *)0xab;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        (**(code **)(*DAT_0071d174 + 0x10c))();
        piStack_1b4 = (int *)0x0;
        piStack_1b8 = DAT_0071d174;
        uStack_1bc = 0x5299ec;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1bc = 2;
        uStack_1c0 = 3;
        uStack_1c4 = 0;
        piStack_1c8 = DAT_0071d174;
        uStack_1cc = 0x529a00;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1cc = 4;
        uStack_1d0 = 4;
        uStack_1d4 = 0;
        piStack_1d8 = DAT_0071d174;
        uStack_1dc = 0x529a14;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1dc = 0;
        uStack_1e0 = 5;
        uStack_1e4 = 0;
        piStack_1e8 = DAT_0071d174;
        uStack_1ec = 0x529a28;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1ec = 2;
        uStack_1f0 = 5;
        uStack_1f4 = 0;
        piStack_1f8 = DAT_0071d174;
        uStack_1fc = 0x529a3c;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1fc = 4;
        uStack_200 = 1;
        uStack_204 = 1;
        piStack_208 = DAT_0071d174;
        uStack_20c = 0x529a50;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_20c = 1;
        uStack_210 = 2;
        uStack_214 = 1;
        piStack_218 = DAT_0071d174;
        uStack_21c = 0x529a64;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_21c = 3;
        uStack_220 = 3;
        uStack_224 = 1;
        piStack_228 = DAT_0071d174;
        uStack_22c = 0x529a78;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_22c = 4;
        uStack_230 = 4;
        uStack_234 = 1;
        piStack_238 = DAT_0071d174;
        uStack_23c = 0x529a8c;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_23c = 1;
        uStack_240 = 5;
        uStack_244 = 1;
        piStack_248 = DAT_0071d174;
        uStack_24c = 0x529aa0;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_24c = 2;
        uStack_250 = 6;
        uStack_254 = 1;
        piStack_258 = DAT_0071d174;
        (**(code **)(*DAT_0071d174 + 0x10c))();
      }
      else {
        piStack_184 = (int *)0x52988f;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        piStack_184 = (int *)&DAT_00000006;
        uStack_188 = 0x14;
        piStack_18c = DAT_0071d174;
        uStack_190 = 0x5298a1;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_190 = 1;
        piStack_194 = (int *)0xab;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        (**(code **)(*DAT_0071d174 + 0x10c))();
        piStack_1b4 = (int *)0x0;
        piStack_1b8 = DAT_0071d174;
        uStack_1bc = 0x5298de;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1bc = 2;
        uStack_1c0 = 3;
        uStack_1c4 = 0;
        piStack_1c8 = DAT_0071d174;
        uStack_1cc = 0x5298f2;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1cc = 4;
        uStack_1d0 = 4;
        uStack_1d4 = 0;
        piStack_1d8 = DAT_0071d174;
        uStack_1dc = 0x529906;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1dc = 3;
        uStack_1e0 = 5;
        uStack_1e4 = 0;
        piStack_1e8 = DAT_0071d174;
        uStack_1ec = 0x52991a;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1ec = 2;
        uStack_1f0 = 6;
        uStack_1f4 = 0;
        piStack_1f8 = DAT_0071d174;
        uStack_1fc = 0x52992e;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_1fc = 4;
        uStack_200 = 1;
        uStack_204 = 1;
        piStack_208 = DAT_0071d174;
        uStack_20c = 0x529942;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_20c = 1;
        uStack_210 = 2;
        uStack_214 = 1;
        piStack_218 = DAT_0071d174;
        uStack_21c = 0x529956;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_21c = 3;
        uStack_220 = 3;
        uStack_224 = 1;
        piStack_228 = DAT_0071d174;
        uStack_22c = 0x52996a;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_22c = 2;
        uStack_230 = 4;
        uStack_234 = 1;
        piStack_238 = DAT_0071d174;
        uStack_23c = 0x52997e;
        (**(code **)(*DAT_0071d174 + 0x10c))();
        uStack_23c = 1;
        uStack_240 = 5;
        uStack_244 = 1;
        piStack_248 = DAT_0071d174;
        uStack_24c = 0x529992;
        (**(code **)(*DAT_0071d174 + 0x10c))();
      }
      uStack_24c = 1;
      uStack_250 = 1;
      uStack_254 = 2;
      piStack_258 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uVar4 = uStack_1f4;
      rasterizer_dynamic_geometry_draw_dispatch((int)piVar11,uStack_1f4,(int)&piStack_258);
      uStack_27c = 0x529b08;
      color_rgb_float_to_int((float *)&DAT_007c140c);
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_27c = 0x13;
      piStack_280 = DAT_0071d174;
      uStack_284 = 0x529b2f;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_284 = 2;
      uStack_288 = 0x14;
      piStack_28c = DAT_0071d174;
      uStack_290 = 0x529b41;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_290 = 1;
      uStack_294 = 0xab;
      piStack_298 = DAT_0071d174;
      uStack_29c = 0x529b56;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_29c = 0;
      uStack_2a0 = 0xe;
      piStack_2a4 = DAT_0071d174;
      uStack_2a8 = 0x529b68;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_2a8 = 3;
      uStack_2ac = 0x17;
      piStack_2b0 = DAT_0071d174;
      uStack_2b4 = 0x529b7a;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_2b4 = 4;
      uStack_2b8 = 1;
      uStack_2bc = 0;
      piStack_2c0 = DAT_0071d174;
      uStack_2c4 = 0x529b8e;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_2c4 = 2;
      uStack_2c8 = 2;
      uStack_2cc = 0;
      piStack_2d0 = DAT_0071d174;
      uStack_2d4 = 0x529ba2;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_2d4 = 0;
      uStack_2d8 = 3;
      uStack_2dc = 0;
      piStack_2e0 = DAT_0071d174;
      uStack_2e4 = 0x529bb6;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_2e4 = 4;
      uStack_2e8 = 4;
      uStack_2ec = 0;
      piStack_2f0 = DAT_0071d174;
      uStack_2f4 = 0x529bca;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_2f4 = 3;
      uStack_2f8 = 5;
      uStack_2fc = 0;
      piStack_300 = DAT_0071d174;
      uStack_304 = 0x529bde;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_304 = 2;
      uStack_308 = 6;
      uStack_30c = 0;
      piStack_310 = DAT_0071d174;
      uStack_314 = 0x529bf2;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_314 = 4;
      uStack_318 = 1;
      uStack_31c = 1;
      piStack_320 = DAT_0071d174;
      uStack_324 = 0x529c06;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_324 = 0x32;
      uStack_328 = 2;
      uStack_32c = 1;
      piStack_330 = DAT_0071d174;
      uStack_334 = 0x529c1a;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_334 = 1;
      uStack_338 = 3;
      uStack_33c = 1;
      piStack_340 = DAT_0071d174;
      uStack_344 = 0x529c2e;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_344 = 2;
      uStack_348 = 4;
      uStack_34c = 1;
      piStack_350 = DAT_0071d174;
      uStack_354 = 0x529c42;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_354 = 1;
      uStack_358 = 5;
      uStack_35c = 1;
      piStack_360 = DAT_0071d174;
      uStack_364 = 0x529c56;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_364 = 1;
      uStack_368 = 1;
      uStack_36c = 2;
      piStack_370 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,4,1);
      rasterizer_dynamic_geometry_draw_dispatch((int)piStack_310,uVar4,(int)&piStack_370);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,0xb,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,0x18,0);
      goto LAB_00529d9e;
    }
    ppiStack_128 = (int **)piStack_b8;
    uStack_12c = 1;
    uStack_130 = 0;
    piStack_134 = (int *)0x0;
    puStack_138 = (undefined4 *)0x529cd8;
    chimera__rasterizer_set_texture();
    ppiStack_128 = (int **)&DAT_00000004;
    uStack_12c = 1;
    uStack_130 = 0;
    piStack_134 = DAT_0071d174;
    puStack_138 = (undefined4 *)0x529cef;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    puStack_138 = (undefined4 *)0x2;
    uStack_13c = 2;
    piStack_140 = (int *)0x0;
    piStack_144 = DAT_0071d174;
    uStack_148 = 0x529d03;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_148 = 0;
    uStack_14c = 3;
    piStack_150 = (int *)0x0;
    piStack_154 = DAT_0071d174;
    uStack_158 = 0x529d17;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_158 = 4;
    piStack_15c = (int *)&DAT_00000004;
    piStack_160 = (int *)0x0;
    piStack_164 = DAT_0071d174;
    piStack_168 = (int *)0x529d2b;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    piStack_168 = (int *)0x2;
    uStack_16c = 5;
    piStack_170 = (int *)0x0;
    piStack_174 = DAT_0071d174;
    uStack_178 = 0x529d3f;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_178 = 3;
    uStack_17c = 6;
    piStack_180 = (int *)0x0;
    piStack_184 = DAT_0071d174;
    uStack_188 = 0x529d53;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_188 = 1;
    piStack_18c = (int *)0x1;
    uStack_190 = 1;
    piStack_194 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    (**(code **)(*DAT_0071d174 + 0x10c))();
    is_static = &piStack_194;
  }
  else {
    uStack_ec = 0;
    piStack_f0 = DAT_0071d174;
    fStack_f4 = 7.584272e-39;
    (**(code **)(*DAT_0071d174 + 0x170))();
    fStack_f4 = (float)DAT_006e1b38;
    piStack_f8 = DAT_0071d174;
    uStack_fc = 0x5295f2;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_fc = 2;
    piStack_100 = (int *)&DAT_00000018;
    uStack_104 = 0;
    piStack_108 = DAT_0071d174;
    puStack_10c = (undefined4 *)0x529606;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    puStack_10c = (undefined4 *)uStack_9c;
    uStack_110 = 1;
    piStack_114 = (int *)0x0;
    uStack_118 = 0;
    uStack_11c = 0x52961c;
    chimera__rasterizer_set_texture();
    puStack_10c = &uStack_e4;
    uStack_110 = 0x10;
    piStack_114 = DAT_0071d174;
    uStack_118 = 0x529634;
    (**(code **)(*DAT_0071d174 + 0xb0))();
    uStack_118 = 2;
    uStack_11c = 0x18;
    uStack_120 = 0;
    piStack_124 = DAT_0071d174;
    ppiStack_128 = (int **)0x529648;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    ppiStack_128 = (int **)&DAT_00000004;
    uStack_12c = 1;
    uStack_130 = 0;
    piStack_134 = DAT_0071d174;
    puStack_138 = (undefined4 *)0x52965c;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    puStack_138 = (undefined4 *)0x2;
    uStack_13c = 2;
    piStack_140 = (int *)0x0;
    piStack_144 = DAT_0071d174;
    uStack_148 = 0x529670;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_148 = 0;
    uStack_14c = 3;
    piStack_150 = (int *)0x0;
    piStack_154 = DAT_0071d174;
    uStack_158 = 0x529684;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_158 = 4;
    piStack_15c = (int *)&DAT_00000004;
    piStack_160 = (int *)0x0;
    piStack_164 = DAT_0071d174;
    piStack_168 = (int *)0x529698;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    piStack_168 = (int *)0x2;
    uStack_16c = 5;
    piStack_170 = (int *)0x0;
    piStack_174 = DAT_0071d174;
    uStack_178 = 0x5296ac;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_178 = 3;
    uStack_17c = 6;
    piStack_180 = (int *)0x0;
    piStack_184 = DAT_0071d174;
    uStack_188 = 0x5296c0;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_188 = 1;
    piStack_18c = (int *)0x1;
    uStack_190 = 1;
    piStack_194 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    (**(code **)(*DAT_0071d174 + 0x10c))();
    piVar11 = piStack_134;
    is_static = ppiStack_128;
  }
  piStack_1b4 = (int *)0x529d9b;
  rasterizer_dynamic_geometry_draw_dispatch((int)piVar11,uStack_130,(int)is_static);
LAB_00529d9e:
  piStack_1b4 = DAT_0071d174;
  piStack_1b8 = (int *)0x529db2;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    piStack_1b4 = (int *)0x529dd7;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    piStack_1b4 = (int *)0x529df8;
    (**(code **)(*DAT_0071d174 + 0xe4))();
  }
  return;
}
#endif
