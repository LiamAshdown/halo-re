// rasterizer_object_shadow_structure_draw  (Ghidra: FUN_00531570; the phase 3 rewrite called it
//   rasterizer_motion_sensor_hud_render)
// address 0x531570, size 1337 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: raw disassembly (phase 4 review). Only caller is the thunk 0x511f50, which
//   shadow_compute_bounding_box_and_register 0x50f980 hands (via 0x552b40 / 0x552de0) to the
//   structure BSP polygon walk as the per-batch callback; the thunk forwards its third to fifth
//   stack arguments and puts the sixth in EAX. So this draws one batch of structure triangles
//   that the current object shadow falls on.
// What it does: once per shadow (0x0071d264 clear): optionally blurs target 3 into target 4
//   (rasterizer_object_shadow_blur 0x530830, when 0x0068941e is set), binds that target to
//   stage 0 and the linear corner fade map of the rasterizer globals to stage 1, both clamped
//   with linear filtering; multiply blending (ZERO x INVSRCCOLOR), alpha test with ref 0,
//   z test EQUAL without z writes, no fog; uploads the projection c13..c17 built from the
//   recorded shadow matrix and radius and c0 = {1 - color, 1} as the pixel constant, then marks
//   the shadow prepared. Every call: restores the window target once per shadow, selects shader
//   stage config 2, the vertex declaration of the vertex buffer and vertex shader 19, and draws
//   the batch with dynamic indices through every pass of effect 47.
// register convention: EAX -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive,
//   primitive_count).
// blam-cc: EAX -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive, primitive_count)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern GlobalsRasterizerData *rasterizer_globals_data;      // 0x0071d164
extern uint8_t unknown_0069c689;                            // 0x0069c689
extern uint8_t console_debug_toggle_6893f2;                 // 0x006893f2 object shadows enabled
extern uint8_t console_debug_toggle_68941e;                 // 0x0068941e object shadow blur enabled
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern ColorRGB rasterizer_object_shadow_color;             // 0x006e1cc0
extern float rasterizer_object_shadow_radius;               // 0x006e1ccc
extern real_matrix4x3 rasterizer_object_shadow_projection;  // 0x006e1cd0
extern uint8_t rasterizer_object_shadow_prepared;           // 0x0071d264
extern uint8_t rasterizer_object_shadow_window_restored;    // 0x0069e550

extern void rasterizer_object_shadow_blur(void); // 0x530830
// blam-cc: AX -> target_index, DX -> stage
extern void *rasterizer_render_target_bind_texture_stage(int16_t target_index, int16_t stage); // 0x52cdd0
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage,
                                                           int16_t frame); // 0x518770
// blam-cc: EAX -> target_index, stack -> (clear_color, clear)
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear); // 0x52ccc0
// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                       int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

typedef int32_t (*d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (*d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (*d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (*d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (*d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (*d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (*d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_clamped_linear_sampler(uint32_t sampler)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 1, 3);  // ADDRESSU CLAMP
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 2, 3);  // ADDRESSV CLAMP
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 5, 2);  // MAGFILTER LINEAR
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 6, 2);  // MINFILTER LINEAR
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, 7, 2);  // MIPFILTER LINEAR
}
static float dot_position(const real_vector3d *v)
{
    const real_point3d *p = &rasterizer_object_shadow_projection.position;
    return p->x * v->i + p->y * v->j + p->z * v->k;
}

void rasterizer_object_shadow_structure_draw(rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot,
                                             int32_t first_primitive, int32_t primitive_count)
{
    const real_matrix4x3 *m = &rasterizer_object_shadow_projection;
    void *effect;
    uint32_t passes;
    uint32_t pass;

    if (rasterizer_window.type != 1 || unknown_0069c689 != 0 || console_debug_toggle_6893f2 == 0) {
        return;
    }
    if (rasterizer_effects[47].effect == 0) {
        return;
    }
    if (!rasterizer_object_shadow_prepared) {
        float inverse_radius = 1.0f / rasterizer_object_shadow_radius;
        float quarter_inverse = 1.0f / (rasterizer_object_shadow_radius * 4.0f);
        float double_inverse = 1.0f / (rasterizer_object_shadow_radius * 0.5f);
        float up_dot;
        float vs[5][4];
        float ps[4];

        if (console_debug_toggle_68941e) {
            rasterizer_object_shadow_blur();
        }
        rasterizer_render_target_bind_texture_stage((int16_t)(console_debug_toggle_68941e ? 4 : 3), 0);
        set_clamped_linear_sampler(0);
        chimera__rasterizer_set_texture_direct_d3d9(*(const uint32_t *)&rasterizer_globals_data->linear_corner_fade.tag_id, 1, 0);
        set_clamped_linear_sampler(1);
        set_render_state(0x16, 3);       // CULLMODE CCW
        set_render_state(0xa8, 0xf);     // COLORWRITEENABLE rgba
        set_render_state(0x1b, 1);       // ALPHABLENDENABLE
        set_render_state(0x13, 1);       // SRCBLEND ZERO
        set_render_state(0x14, 4);       // DESTBLEND INVSRCCOLOR
        set_render_state(0xab, 1);       // BLENDOP ADD
        set_render_state(0x0f, 1);       // ALPHATESTENABLE
        set_render_state(0x18, 0);       // ALPHAREF
        set_render_state(0x07, 1);       // ZENABLE
        set_render_state(0x17, 3);       // ZFUNC EQUAL
        set_render_state(0x0e, 0);       // ZWRITEENABLE
        set_render_state(0x1c, 0);       // FOGENABLE

        // c13, c14: forward / left projected into [0, 1] texture space over the radius
        vs[0][0] = m->forward.i * inverse_radius * 0.5f;
        vs[0][1] = m->forward.j * inverse_radius * 0.5f;
        vs[0][2] = m->forward.k * inverse_radius * 0.5f;
        vs[0][3] = (1.0f - dot_position(&m->forward) * inverse_radius) * 0.5f;
        vs[1][0] = m->left.i * inverse_radius * -0.5f;
        vs[1][1] = m->left.j * inverse_radius * -0.5f;
        vs[1][2] = m->left.k * inverse_radius * -0.5f;
        vs[1][3] = (dot_position(&m->left) * inverse_radius + 1.0f) * 0.5f;
        // c15: depth along up over four radii; c16: the same over half a radius, negated
        up_dot = dot_position(&m->up);
        vs[2][0] = m->up.i * quarter_inverse;
        vs[2][1] = m->up.j * quarter_inverse;
        vs[2][2] = m->up.k * quarter_inverse;
        vs[2][3] = -(up_dot * quarter_inverse);
        vs[3][0] = -(m->up.i * double_inverse);
        vs[3][1] = -(m->up.j * double_inverse);
        vs[3][2] = -(m->up.k * double_inverse);
        vs[3][3] = up_dot * double_inverse;
        // c17: the raw up vector
        vs[4][0] = m->up.i;
        vs[4][1] = m->up.j;
        vs[4][2] = m->up.k;
        vs[4][3] = 0.0f;
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &vs[0][0], 5);
        ps[0] = 1.0f - rasterizer_object_shadow_color.red;
        ps[1] = 1.0f - rasterizer_object_shadow_color.green;
        ps[2] = 1.0f - rasterizer_object_shadow_color.blue;
        ps[3] = 1.0f;
        ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, ps, 1);
        rasterizer_object_shadow_prepared = 1;
    }
    if (!rasterizer_object_shadow_window_restored) {
        rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
        rasterizer_object_shadow_window_restored = 1;
    }
    rasterizer_set_shader_stage_config(2);
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[vertex_buffer->type].declaration);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[19].shader);
    effect = (void *)(uintptr_t)rasterizer_effects[47].effect;
    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        effect = (void *)(uintptr_t)rasterizer_effects[47].effect;
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot,
                                                                   first_primitive);
    }
    effect = (void *)(uintptr_t)rasterizer_effects[47].effect;
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x531570): phase 3 file rasterizer_motion_sensor_hud_render replaced

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00531570(void)

{
  float fVar1;
  float fVar2;
  short *in_EAX;
  int *piVar3;
  float fStack_1a0;
  float fStack_19c;
  int *piStack_198;
  undefined4 uStack_194;
  undefined4 uStack_190;
  int *piStack_18c;
  undefined4 uStack_188;
  undefined4 uStack_184;
  int *piStack_180;
  float fStack_17c;
  float fStack_178;
  int *piStack_174;
  float fStack_170;
  float fStack_16c;
  int *piStack_168;
  float fStack_164;
  float fStack_160;
  int *piStack_15c;
  float fStack_158;
  float fStack_154;
  int *piStack_150;
  float fStack_14c;
  float fStack_148;
  int *piStack_144;
  float fStack_140;
  float fStack_13c;
  int *piStack_138;
  undefined4 uStack_134;
  undefined4 uStack_130;
  int *piStack_12c;
  undefined4 uStack_128;
  undefined4 uStack_124;
  int *piStack_120;
  undefined4 uStack_11c;
  undefined4 uStack_118;
  int *piStack_114;
  undefined4 uStack_110;
  undefined4 uStack_10c;
  undefined4 uStack_108;
  int *piStack_104;
  undefined4 uStack_100;
  undefined4 uStack_fc;
  undefined4 uStack_f8;
  int *piStack_f4;
  undefined4 uStack_f0;
  undefined4 uStack_ec;
  undefined4 uStack_e8;
  int *piStack_e4;
  undefined4 uStack_e0;
  undefined4 uStack_dc;
  undefined4 uStack_d8;
  int *piStack_d4;
  undefined4 uStack_d0;
  undefined4 uStack_cc;
  undefined4 uStack_c8;
  int *piStack_c4;
  undefined4 uStack_c0;
  undefined4 uStack_bc;
  undefined4 uStack_b8;
  int *piStack_b4;
  undefined4 uStack_b0;
  undefined4 uStack_ac;
  undefined4 uStack_a8;
  int *piStack_a4;
  undefined4 uStack_a0;
  int *piStack_9c;
  int *piStack_98;
  int *piStack_94;
  undefined1 *puStack_90;
  undefined4 uStack_8c;
  int *piStack_88;
  int *piStack_84;
  int *piStack_80;
  undefined4 uStack_7c;
  
  if (((((short)DAT_007c1220 == 1) && (DAT_0069c689 == '\0')) && (DAT_006893f2 != '\0')) &&
     (DAT_0069d9f0 != (int *)0x0)) {
    if (DAT_0071d264 == '\0') {
      if (DAT_0068941e != '\0') {
        FUN_00530830();
      }
      FUN_0052cdd0();
      uStack_7c = 1;
      piStack_80 = (int *)0x0;
      piStack_84 = DAT_0071d174;
      piStack_88 = (int *)0x531603;
      (**(code **)(*DAT_0071d174 + 0x114))();
      piStack_88 = (int *)0x3;
      uStack_8c = 2;
      puStack_90 = (undefined1 *)0x0;
      piStack_94 = DAT_0071d174;
      piStack_98 = (int *)0x531617;
      (**(code **)(*DAT_0071d174 + 0x114))();
      piStack_98 = (int *)0x2;
      piStack_9c = (int *)0x5;
      uStack_a0 = 0;
      piStack_a4 = DAT_0071d174;
      uStack_a8 = 0x53162b;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_a8 = 2;
      uStack_ac = 6;
      uStack_b0 = 0;
      piStack_b4 = DAT_0071d174;
      uStack_b8 = 0x53163f;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_b8 = 2;
      uStack_bc = 7;
      uStack_c0 = 0;
      piStack_c4 = DAT_0071d174;
      uStack_c8 = 0x531653;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_c8 = 0;
      uStack_cc = 1;
      uStack_d0 = 0x531665;
      chimera__rasterizer_set_texture_direct_d3d9();
      uStack_c8 = 3;
      uStack_cc = 1;
      uStack_d0 = 1;
      piStack_d4 = DAT_0071d174;
      uStack_d8 = 0x53167c;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_d8 = 3;
      uStack_dc = 2;
      uStack_e0 = 1;
      piStack_e4 = DAT_0071d174;
      uStack_e8 = 0x531690;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_e8 = 2;
      uStack_ec = 5;
      uStack_f0 = 1;
      piStack_f4 = DAT_0071d174;
      uStack_f8 = 0x5316a4;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_f8 = 2;
      uStack_fc = 6;
      uStack_100 = 1;
      piStack_104 = DAT_0071d174;
      uStack_108 = 0x5316b8;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_108 = 2;
      uStack_10c = 7;
      uStack_110 = 1;
      piStack_114 = DAT_0071d174;
      uStack_118 = 0x5316cc;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_118 = 3;
      uStack_11c = 0x16;
      piStack_120 = DAT_0071d174;
      uStack_124 = 0x5316de;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_124 = 0xf;
      uStack_128 = 0xa8;
      piStack_12c = DAT_0071d174;
      uStack_130 = 0x5316f3;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_130 = 1;
      uStack_134 = 0x1b;
      piStack_138 = DAT_0071d174;
      fStack_13c = 7.630604e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_13c = 1.4013e-45;
      fStack_140 = 2.66247e-44;
      piStack_144 = DAT_0071d174;
      fStack_148 = 7.630629e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_148 = 5.60519e-45;
      fStack_14c = 2.8026e-44;
      piStack_150 = DAT_0071d174;
      fStack_154 = 7.630654e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_154 = 1.4013e-45;
      fStack_158 = 2.39622e-43;
      piStack_15c = DAT_0071d174;
      fStack_160 = 7.630684e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_160 = 1.4013e-45;
      fStack_164 = 2.10195e-44;
      piStack_168 = DAT_0071d174;
      fStack_16c = 7.630709e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_16c = 0.0;
      fStack_170 = 3.36312e-44;
      piStack_174 = DAT_0071d174;
      fStack_178 = 7.630734e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_178 = 1.4013e-45;
      fStack_17c = 9.80909e-45;
      piStack_180 = DAT_0071d174;
      uStack_184 = 0x531774;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_184 = 3;
      uStack_188 = 0x17;
      piStack_18c = DAT_0071d174;
      uStack_190 = 0x531786;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_190 = 0;
      uStack_194 = 0xe;
      piStack_198 = DAT_0071d174;
      fStack_19c = 7.63081e-39;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      fStack_19c = 0.0;
      fStack_1a0 = 3.92364e-44;
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174);
      fVar1 = 1.0 / _DAT_006e1ccc;
      fVar2 = 1.0 / (_DAT_006e1ccc * 4.0);
      fStack_19c = 1.0 / (_DAT_006e1ccc * 0.5);
      piStack_180 = (int *)(DAT_006e1cd4 * fVar1 * 0.5);
      fStack_17c = _DAT_006e1cd8 * fVar1 * 0.5;
      fStack_178 = _DAT_006e1cdc * fVar1 * 0.5;
      piStack_174 = (int *)((1.0 - (_DAT_006e1cf8 * DAT_006e1cd4 +
                                   _DAT_006e1cfc * _DAT_006e1cd8 + _DAT_006e1d00 * _DAT_006e1cdc) *
                                   fVar1) * 0.5);
      fStack_170 = _DAT_006e1ce0 * fVar1 * -0.5;
      fStack_16c = _DAT_006e1ce4 * fVar1 * -0.5;
      piStack_168 = (int *)(_DAT_006e1ce8 * fVar1 * -0.5);
      fStack_164 = ((_DAT_006e1cf8 * _DAT_006e1ce0 +
                    _DAT_006e1cfc * _DAT_006e1ce4 + _DAT_006e1d00 * _DAT_006e1ce8) * fVar1 + 1.0) *
                   0.5;
      fStack_160 = DAT_006e1cec * fVar2;
      piStack_15c = (int *)(DAT_006e1cf0 * fVar2);
      fStack_158 = DAT_006e1cf4 * fVar2;
      piStack_198 = (int *)(_DAT_006e1cf8 * DAT_006e1cec +
                           _DAT_006e1cfc * DAT_006e1cf0 + _DAT_006e1d00 * DAT_006e1cf4);
      fStack_154 = -((float)piStack_198 * fVar2);
      piStack_150 = (int *)-(DAT_006e1cec * fStack_19c);
      piStack_138 = (int *)DAT_006e1cf4;
      fStack_14c = -(DAT_006e1cf0 * fStack_19c);
      fStack_13c = DAT_006e1cf0;
      fStack_148 = -(DAT_006e1cf4 * fStack_19c);
      fStack_140 = DAT_006e1cec;
      uStack_134 = 0;
      piStack_144 = (int *)((float)piStack_198 * fStack_19c);
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&piStack_180,5);
      fStack_1a0 = 1.0 - _DAT_006e1cc0;
      fStack_19c = 1.0 - _DAT_006e1cc4;
      uStack_194 = 0x3f800000;
      piStack_198 = (int *)(1.0 - _DAT_006e1cc8);
      (**(code **)(*DAT_0071d174 + 0x1b4))(DAT_0071d174,0,&fStack_1a0,1);
      DAT_0071d264 = '\x01';
    }
    if (DAT_0069e550 == '\0') {
      uStack_7c = 0;
      piStack_80 = (int *)0x5319ff;
      FUN_0052ccc0();
      DAT_0069e550 = '\x01';
    }
    uStack_7c = 0x531a14;
    rasterizer_set_shader_stage_config();
    uStack_7c = (&DAT_006e1a90)[*in_EAX * 3];
    piStack_80 = DAT_0071d174;
    piStack_84 = (int *)0x531a31;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    piStack_84 = (int *)DAT_0069e3e8;
    piStack_88 = DAT_0071d174;
    uStack_8c = 0x531a46;
    (**(code **)(*DAT_0071d174 + 0x170))();
    uStack_8c = 3;
    puStack_90 = &stack0xffffff8c;
    piStack_94 = DAT_0069d9f0;
    piStack_98 = (int *)0x531a5b;
    (**(code **)(*DAT_0069d9f0 + 0x100))();
    piVar3 = (int *)0x0;
    if (piStack_80 != (int *)0x0) {
      do {
        piStack_9c = DAT_0069d9f0;
        uStack_a0 = 0x531a74;
        piStack_98 = piVar3;
        (**(code **)(*DAT_0069d9f0 + 0x104))();
        uStack_a0 = 0x531a88;
        chimera__rasterizer_draw_dynamic_triangles_static_vertices();
        piVar3 = (int *)((int)piVar3 + 1);
      } while (piVar3 < piStack_80);
    }
    piStack_98 = DAT_0069d9f0;
    piStack_9c = (int *)0x531aa2;
    (**(code **)(*DAT_0069d9f0 + 0x108))();
  }
  return;
}
#endif
