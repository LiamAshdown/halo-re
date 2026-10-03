// rasterizer_render_target_capture_frame  (Ghidra: FUN_00519b00, unnamed)
// address 0x519b00, size 1123 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: called once, from rasterizer_transparent_geometry_group_draw 0x533850 (0x533d7c),
//   before the transparent geometry that samples the frame. With pixel shaders and the
//   render target pool available (0x00689421 set, flags 688/68a clear, ps_1_1 or better) it binds
//   rasterizer_render_targets[2].surface (0x0069d38c), resets the viewport to that surface and,
//   when 0x0071d1b1 was raised this frame (0x52b130 raises it, rasterizer_begin_frame clears it),
//   draws render target 1 (0x0069d37c) into it as a full screen fan through vertex shader 35
//   (0x0069e468) and the dynamic screen vertex declaration (type 8, 0x006e1af0/0x006e1af8).
//   Afterwards it rebinds the window's own target through rasterizer_render_target_set_active and latches 0x0071d1b2.
//   Ghidra lost every argument list of the device calls and mixed the constant block with the
//   argument pushes; the arguments, the five vec4 at register 0xd and the quad at 0x006d9878
//   are taken from the raw code (0x519b45..0x519f5b).
// register convention: none, __cdecl with no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                                     // 0x0071d174
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern int16_t rasterizer_active_render_target;                     // 0x0069d350
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern uint8_t rasterizer_caps_flag_688;                            // 0x0069c688
extern uint8_t rasterizer_caps_flag_68a;                            // 0x0069c68a
extern uint8_t console_debug_toggle_689421;                         // 0x00689421 render target capture enable
extern uint8_t console_debug_toggle_689422;                         // 0x00689422 keep the request latched
extern uint8_t rasterizer_render_target_capture_requested;          // 0x0071d1b1
extern uint8_t rasterizer_render_target_capture_done;               // 0x0071d1b2
extern float rasterizer_screen_quad_vertices[4][6];                 // 0x006d9878, type 8 vertices (stride 0x18)

extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200, blam-cc: AX mode
// blam-cc: AX target_index, clear color and clear flag on the stack
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear_target); // 0x52ccc0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_render_target_fn)(void *self, uint32_t index, void *surface);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *self, d3d_surface_desc *desc);
typedef int32_t (__stdcall *d3d_set_viewport_fn)(void *self, const d3d_viewport *viewport);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_texture_fn)(void *self, uint32_t stage, void *texture);
typedef int32_t (__stdcall *d3d_set_vertex_shader_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t primitive_count, const void *data, uint32_t stride);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

static void rasterizer_set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, stage, type, value);
}

static void rasterizer_set_texture_stage_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, sampler, type, value);
}

static void rasterizer_set_screen_quad_vertex(int32_t index, float x, float y, float u, float v)
{
    float *vertex = rasterizer_screen_quad_vertices[index];

    vertex[0] = x;
    vertex[1] = y;
    vertex[2] = 0.0f;
    vertex[3] = 0.0f;
    vertex[4] = u;
    vertex[5] = v;
}

void rasterizer_render_target_capture_frame(void)
{
    void *surface;
    d3d_surface_desc desc;
    d3d_viewport viewport;
    float constants[20];
    float width;
    float height;
    float inverse_width;
    float inverse_height;

    if (console_debug_toggle_689421 == 0 || rasterizer_caps_flag_688 != 0 || rasterizer_caps_flag_68a != 0 ||
        rasterizer_caps.pixel_shader_version < 0xffff0101) {
        return;
    }

    surface = (void *)rasterizer_render_targets[2].surface;
    ((d3d_set_render_target_fn)device_vtable()[0x94 / 4])(rasterizer_device, 0, surface);
    rasterizer_active_render_target = 2;
    ((d3d_get_desc_fn)(*(void ***)surface)[0x30 / 4])(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    ((d3d_set_viewport_fn)device_vtable()[0xbc / 4])(rasterizer_device, &viewport);
    rasterizer_set_shader_stage_config(0);

    if (rasterizer_render_target_capture_requested != 0) {
        rasterizer_vertex_declaration *declaration = &rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen];

        ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)declaration->declaration);
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                                   ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                    declaration->usage) & 0x10);
        ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device, (void *)rasterizer_vertex_shaders[35].shader);
        ((d3d_set_pointer_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);            // SetPixelShader(NULL)
        ((d3d_set_texture_fn)device_vtable()[0x104 / 4])(rasterizer_device, 0, (void *)rasterizer_render_targets[1].texture);
        rasterizer_set_sampler_state(0, 1, 3);
        rasterizer_set_sampler_state(0, 2, 3);
        rasterizer_set_sampler_state(0, 5, 2);
        rasterizer_set_sampler_state(0, 6, 2);
        rasterizer_set_sampler_state(0, 7, 1);
        rasterizer_set_render_state(0x16, 3);                   // D3DRS_CULLMODE
        rasterizer_set_render_state(0xa8, 7);                   // D3DRS_COLORWRITEENABLE rgb
        rasterizer_set_render_state(0x1b, 0);                   // D3DRS_ALPHABLENDENABLE
        rasterizer_set_render_state(0xf, 0);                    // D3DRS_ALPHATESTENABLE
        rasterizer_set_render_state(7, 0);                      // D3DRS_ZENABLE
        rasterizer_set_render_state(0x1c, 0);                   // D3DRS_FOGENABLE
        rasterizer_set_texture_stage_state(0, 1, 2);
        rasterizer_set_texture_stage_state(0, 2, 2);
        rasterizer_set_texture_stage_state(0, 4, 2);
        rasterizer_set_texture_stage_state(0, 5, 2);
        rasterizer_set_texture_stage_state(1, 1, 1);
        rasterizer_set_texture_stage_state(1, 4, 1);

        width = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
        height = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
        inverse_width = 1.0f / width;
        inverse_height = 1.0f / height;

        // c13, c14: pixel to clip space with the half texel shift
        constants[0] = inverse_width + inverse_width;
        constants[1] = 0.0f;
        constants[2] = 0.0f;
        constants[3] = -1.0f - inverse_width;
        constants[4] = 0.0f;
        constants[5] = inverse_height * -2.0f;
        constants[6] = 0.0f;
        constants[7] = inverse_height + 1.0f;
        // c15..c17
        constants[8] = 0.0f;
        constants[9] = 0.0f;
        constants[10] = 0.0f;
        constants[11] = 0.5f;
        constants[12] = 0.0f;
        constants[13] = 0.0f;
        constants[14] = 0.0f;
        constants[15] = 1.0f;
        constants[16] = 1.0f;
        constants[17] = 1.0f;
        constants[18] = 0.0f;
        constants[19] = 1.0f;
        ((d3d_set_vertex_shader_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, constants, 5);

        rasterizer_set_screen_quad_vertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
        rasterizer_set_screen_quad_vertex(1, width, 0.0f, 1.0f, 0.0f);
        rasterizer_set_screen_quad_vertex(2, width, height, 1.0f, 1.0f);
        rasterizer_set_screen_quad_vertex(3, 0.0f, height, 0.0f, 1.0f);
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, rasterizer_screen_quad_vertices, 0x18);
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
    }

    // eax is the whole first dword of the window block; only its low word (type) is used as
    // the target index
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    rasterizer_set_shader_stage_config(2);
    if (console_debug_toggle_689422 == 0) {
        rasterizer_render_target_capture_requested = 0;
    }
    rasterizer_render_target_capture_done = 1;
}

#if 0
Original Ghidra decompilation (0x519b00):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00519b00(void)

{
  int *piVar1;
  float afStack_1b0 [2];
  int *piStack_1a8;
  float fStack_1a4;
  undefined4 uStack_1a0;
  float fStack_19c;
  int *piStack_198;
  float fStack_194;
  undefined4 uStack_190;
  undefined4 uStack_18c;
  int *piStack_188;
  undefined4 uStack_184;
  undefined4 uStack_180;
  undefined4 uStack_17c;
  int *piStack_178;
  undefined4 uStack_174;
  undefined4 uStack_170;
  int *piStack_16c;
  undefined4 uStack_168;
  undefined4 uStack_164;
  int *piStack_160;
  undefined4 uStack_15c;
  undefined4 uStack_158;
  int *piStack_154;
  undefined4 uStack_150;
  undefined4 uStack_14c;
  int *piStack_148;
  undefined4 uStack_144;
  undefined4 uStack_140;
  int *piStack_13c;
  undefined4 uStack_138;
  undefined4 uStack_134;
  int *piStack_130;
  undefined4 uStack_12c;
  undefined4 uStack_128;
  undefined4 uStack_124;
  int *piStack_120;
  undefined4 uStack_11c;
  undefined4 uStack_118;
  undefined4 uStack_114;
  int *piStack_110;
  undefined4 uStack_10c;
  undefined4 uStack_108;
  undefined4 uStack_104;
  int *piStack_100;
  undefined4 uStack_fc;
  undefined4 uStack_f8;
  undefined4 uStack_f4;
  int *piStack_f0;
  undefined4 uStack_ec;
  undefined4 uStack_e8;
  undefined4 uStack_e4;
  int *piStack_e0;
  undefined4 uStack_dc;
  undefined4 uStack_d8;
  int *piStack_d4;
  undefined4 uStack_d0;
  int *piStack_cc;
  undefined4 uStack_c8;
  int *piStack_c4;
  uint uStack_c0;
  int *piStack_bc;
  undefined4 uStack_b8;
  int *piStack_b4;
  int **ppiStack_b0;
  int *piStack_ac;
  undefined1 *puStack_a8;
  int *piStack_a4;
  undefined4 uStack_a0;
  int *piStack_9c;
  undefined1 auStack_2c [44];
  
  piVar1 = DAT_0069d38c;
  if ((((DAT_00689421 != '\0') && (DAT_0069c688 == '\0')) && (DAT_0069c68a == '\0')) &&
     (0xffff0100 < DAT_007c118c)) {
    piStack_9c = DAT_0069d38c;
    uStack_a0 = 0;
    piStack_a4 = DAT_0071d174;
    puStack_a8 = (undefined1 *)0x519b55;
    (**(code **)(*DAT_0071d174 + 0x94))();
    puStack_a8 = auStack_2c;
    piStack_ac = piVar1;
    _DAT_0069d350 = 2;
    ppiStack_b0 = (int **)0x519b69;
    (**(code **)(*piVar1 + 0x30))();
    ppiStack_b0 = &piStack_9c;
    piStack_9c = (int *)0x0;
    piStack_b4 = DAT_0071d174;
    uStack_b8 = 0x519ba6;
    (**(code **)(*DAT_0071d174 + 0xbc))();
    uStack_b8 = 0x519bad;
    rasterizer_set_shader_stage_config();
    if (DAT_0071d1b1 != '\0') {
      uStack_b8 = DAT_006e1af0;
      piStack_bc = DAT_0071d174;
      uStack_c0 = 0x519bce;
      (**(code **)(*DAT_0071d174 + 0x15c))();
      uStack_c0 = -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10;
      piStack_c4 = DAT_0071d174;
      uStack_c8 = 0x519bf5;
      (**(code **)(*DAT_0071d174 + 0x134))();
      uStack_c8 = DAT_0069e468;
      piStack_cc = DAT_0071d174;
      uStack_d0 = 0x519c0a;
      (**(code **)(*DAT_0071d174 + 0x170))();
      uStack_d0 = 0;
      piStack_d4 = DAT_0071d174;
      uStack_d8 = 0x519c19;
      (**(code **)(*DAT_0071d174 + 0x1ac))();
      uStack_d8 = DAT_0069d37c;
      uStack_dc = 0;
      piStack_e0 = DAT_0071d174;
      uStack_e4 = 0x519c2f;
      (**(code **)(*DAT_0071d174 + 0x104))();
      uStack_e4 = 3;
      uStack_e8 = 1;
      uStack_ec = 0;
      piStack_f0 = DAT_0071d174;
      uStack_f4 = 0x519c42;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_f4 = 3;
      uStack_f8 = 2;
      uStack_fc = 0;
      piStack_100 = DAT_0071d174;
      uStack_104 = 0x519c55;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_104 = 2;
      uStack_108 = 5;
      uStack_10c = 0;
      piStack_110 = DAT_0071d174;
      uStack_114 = 0x519c68;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_114 = 2;
      uStack_118 = 6;
      uStack_11c = 0;
      piStack_120 = DAT_0071d174;
      uStack_124 = 0x519c7b;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_124 = 1;
      uStack_128 = 7;
      uStack_12c = 0;
      piStack_130 = DAT_0071d174;
      uStack_134 = 0x519c8e;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_134 = 3;
      uStack_138 = 0x16;
      piStack_13c = DAT_0071d174;
      uStack_140 = 0x519ca0;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_140 = 7;
      uStack_144 = 0xa8;
      piStack_148 = DAT_0071d174;
      uStack_14c = 0x519cb5;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_14c = 0;
      uStack_150 = 0x1b;
      piStack_154 = DAT_0071d174;
      uStack_158 = 0x519cc6;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_158 = 0;
      uStack_15c = 0xf;
      piStack_160 = DAT_0071d174;
      uStack_164 = 0x519cd7;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_164 = 0;
      uStack_168 = 7;
      piStack_16c = DAT_0071d174;
      uStack_170 = 0x519ce8;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_170 = 0;
      uStack_174 = 0x1c;
      piStack_178 = DAT_0071d174;
      uStack_17c = 0x519cf9;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_17c = 2;
      uStack_180 = 1;
      uStack_184 = 0;
      piStack_188 = DAT_0071d174;
      uStack_18c = 0x519d0c;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_18c = 2;
      uStack_190 = 2;
      fStack_194 = 0.0;
      piStack_198 = DAT_0071d174;
      fStack_19c = 7.49504e-39;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      fStack_19c = 2.8026e-45;
      uStack_1a0 = 4;
      fStack_1a4 = 0.0;
      piStack_1a8 = DAT_0071d174;
      afStack_1b0[1] = 7.495066e-39;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      afStack_1b0[1] = 2.8026e-45;
      afStack_1b0[0] = 7.00649e-45;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      fStack_1a4 = 1.0 / (float)(int)(short)(DAT_007c1258._2_2_ - DAT_007c1254._2_2_);
      afStack_1b0[0] = fStack_1a4 + fStack_1a4;
      fStack_1a4 = -1.0 - fStack_1a4;
      afStack_1b0[1] = 0.0;
      piStack_1a8 = (int *)0x0;
      uStack_1a0 = 0;
      piStack_198 = (int *)0x0;
      uStack_190 = 0;
      uStack_18c = 0;
      fStack_194 = 1.0 / (float)(int)(short)((short)DAT_007c1258 - (short)DAT_007c1254);
      piStack_188 = (int *)0x0;
      uStack_184 = 0x3f000000;
      uStack_180 = 0;
      uStack_17c = 0;
      piStack_178 = (int *)0x0;
      uStack_174 = 0x3f800000;
      uStack_170 = 0x3f800000;
      piStack_16c = (int *)0x3f800000;
      uStack_168 = 0;
      uStack_164 = 0x3f800000;
      fStack_19c = fStack_194 * -2.0;
      fStack_194 = fStack_194 + 1.0;
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd);
      _DAT_006d9890 = 5;
      _DAT_006d98a8 = 5;
      _DAT_006d9884 = 0;
      _DAT_006d9888 = 0;
      _DAT_006d988c = 0;
      _DAT_006d9878 = 0;
      _DAT_006d987c = 0;
      _DAT_006d989c = 0;
      _DAT_006d98a0 = 0x3f800000;
      _DAT_006d98a4 = 0;
      _DAT_006d9894 = 0;
      _DAT_006d98b4 = 0;
      _DAT_006d98b8 = 0x3f800000;
      _DAT_006d98bc = 0x3f800000;
      _DAT_006d98cc = 0;
      _DAT_006d98d0 = 0;
      _DAT_006d98d4 = 0x3f800000;
      _DAT_006d98c0 = 0;
      _DAT_006d98c8 = 0;
      _DAT_006d98b0 = 0;
      _DAT_006d9898 = 0;
      _DAT_006d9880 = 0;
      _DAT_006d98ac = afStack_1b0;
      _DAT_006d98c4 = afStack_1b0;
      (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006d9878,0x18);
      (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
    }
    uStack_b8 = 0;
    piStack_bc = (int *)0x0;
    uStack_c0 = 0x519f38;
    FUN_0052ccc0();
    uStack_b8 = 0x519f45;
    rasterizer_set_shader_stage_config();
    if (DAT_00689422 == '\0') {
      DAT_0071d1b1 = '\0';
    }
    DAT_0071d1b2 = 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
