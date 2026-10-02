// rasterizer_sun_glow_capture  (Ghidra: FUN_00525320; the earlier rewrite called it
//   rasterizer_light_shadow_render_target_begin)
// address 0x525320, size 1021 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). Its only caller is rasterizer_sun_glow_render
//   0x525ab0, twice, with the 64 pixel box around the projected sun in ESI and render target 6
//   or 7 pushed. The earlier file ignored the target argument (it activated target 0), missed the
//   ZENABLE state and the c20.y entry, and filled the static quad with shifted values.
// What it does: binds render target 1 (the frame, 0x0069d37c = rasterizer_render_targets[1]
//   .texture) on stage 0 and the GlobalsRasterizerData glow bitmap on stage 1, maps the box to
//   texture coordinates of the window (in texels when 0x00722b30 is set) as c13/c14 with identity
//   rows c15..c20, activates the target and, when effect 76 is loaded, draws the full target quad
//   through it, then gives the window target back.
// register convention: ESI -> rect (left, right, top, bottom in window pixels), stack ->
//   target_index.
// blam-cc: ESI -> rect, stack -> target_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern GlobalsRasterizerData *rasterizer_globals_data;      // 0x0071d164
extern uint8_t rasterizer_software_vertex_processing;       // 0x0069c680
extern uint32_t config_linear_texture_addressing_sun;                           // 0x00722b30 UNSURE: render target textures addressed in texels
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_dynamic_screen_vertex rasterizer_shadow_screen_quad[4]; // 0x006e1720

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame); // 0x518770
// blam-cc: EAX -> target_index, stack -> (clear_color, clear)
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear); // 0x52ccc0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_quad_vertex(int i, float x, float y, float u, float v)
{
    rasterizer_shadow_screen_quad[i].x = x;
    rasterizer_shadow_screen_quad[i].y = y;
    rasterizer_shadow_screen_quad[i].z = 0.0f;
    rasterizer_shadow_screen_quad[i].color = 0xffffffff;
    rasterizer_shadow_screen_quad[i].u = u;
    rasterizer_shadow_screen_quad[i].v = v;
}

void rasterizer_sun_glow_capture(const float *rect, int16_t target_index)
{
    float constants[8][4];
    float width, height;
    void *effect;
    int i, j;

    ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, 0, rasterizer_render_targets[1].texture); // SetTexture
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, 0, 7, 1);    // MIPFILTER POINT
    chimera__rasterizer_set_texture_direct_d3d9(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0x6c), 1, 0); // glow
    set_render_state(0x16, 3);
    set_render_state(0xa8, 0xf);      // COLORWRITEENABLE rgba
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

    width = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 4; j++) {
            constants[i][j] = 0.0f;
        }
    }
    for (i = 2; i < 8; i++) {
        constants[i][i & 1] = 1.0f;   // c15 (1,0,0,0), c16 (0,1,0,0), ... c20 (0,1,0,0)
    }
    constants[0][0] = (rect[1] - rect[0]) / width;
    constants[0][3] = rect[0] / width;
    constants[1][1] = (rect[3] - rect[2]) / height;
    constants[1][3] = rect[2] / height;
    if (config_linear_texture_addressing_sun) {
        constants[0][0] *= width;
        constants[0][3] = width * constants[0][3];
        constants[1][1] = height * constants[1][1];
        constants[1][3] = height * constants[1][3];
    }
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &constants[0][0], 8);
    rasterizer_render_target_set_active(target_index, 0, 0);

    effect = (void *)(uintptr_t)rasterizer_effects[76].effect;
    if (effect != NULL) {
        uint32_t passes;

        set_quad_vertex(0, -1.015625f, 1.015625f, 0.0f, 0.0f);
        set_quad_vertex(1, 0.984375f, 1.015625f, 1.0f, 0.0f);
        set_quad_vertex(2, 0.984375f, -0.984375f, 1.0f, 1.0f);
        set_quad_vertex(3, -1.015625f, -0.984375f, 0.0f, 1.0f);
        ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
        effect = (void *)(uintptr_t)rasterizer_effects[76].effect;
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, 0);
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, rasterizer_shadow_screen_quad,
                                                               sizeof(rasterizer_dynamic_screen_vertex));
        effect = (void *)(uintptr_t)rasterizer_effects[76].effect;
        ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x525320):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00525320(void)

{
  float fVar1;
  float *unaff_ESI;
  int *piStack_110;
  undefined4 uStack_10c;
  int **ppiStack_108;
  int *piStack_104;
  float fStack_100;
  int *piStack_fc;
  uint uStack_f8;
  int *piStack_f4;
  float fStack_f0;
  int *piStack_ec;
  float fStack_e8;
  undefined4 uStack_e4;
  int *piStack_e0;
  undefined4 uStack_dc;
  undefined4 uStack_d8;
  int *piStack_d4;
  undefined4 uStack_d0;
  undefined4 uStack_cc;
  int *piStack_c8;
  undefined4 uStack_c4;
  undefined4 uStack_c0;
  int *piStack_bc;
  undefined4 uStack_b8;
  undefined4 uStack_b4;
  int *piStack_b0;
  undefined4 uStack_ac;
  undefined4 uStack_a8;
  int *piStack_a4;
  undefined4 uStack_a0;
  undefined4 uStack_9c;
  undefined4 uStack_98;
  int *piStack_94;
  undefined4 uStack_90;
  undefined4 uStack_8c;
  
  uStack_8c = DAT_0069d37c;
  uStack_90 = 0;
  piStack_94 = DAT_0071d174;
  uStack_98 = 0x52533e;
  (**(code **)(*DAT_0071d174 + 0x104))();
  uStack_98 = 1;
  uStack_9c = 7;
  uStack_a0 = 0;
  piStack_a4 = DAT_0071d174;
  uStack_a8 = 0x525352;
  (**(code **)(*DAT_0071d174 + 0x114))();
  uStack_a8 = 0;
  uStack_ac = 1;
  piStack_b0 = (int *)0x525364;
  chimera__rasterizer_set_texture_direct_d3d9();
  uStack_a8 = 3;
  uStack_ac = 0x16;
  piStack_b0 = DAT_0071d174;
  uStack_b4 = 0x525379;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_b4 = 0xf;
  uStack_b8 = 0xa8;
  piStack_bc = DAT_0071d174;
  uStack_c0 = 0x52538e;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_c0 = 0;
  uStack_c4 = 0x1b;
  piStack_c8 = DAT_0071d174;
  uStack_cc = 0x5253a0;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_cc = 0;
  uStack_d0 = 0xf;
  piStack_d4 = DAT_0071d174;
  uStack_d8 = 0x5253b2;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_d8 = 0;
  uStack_dc = 7;
  piStack_e0 = DAT_0071d174;
  uStack_e4 = 0x5253c4;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_e4 = 0;
  fStack_e8 = 3.92364e-44;
  piStack_ec = DAT_0071d174;
  fStack_f0 = 7.560585e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_f0 = (float)DAT_006e1af0;
  piStack_f4 = DAT_0071d174;
  uStack_f8 = 0x5253eb;
  (**(code **)(*DAT_0071d174 + 0x15c))();
  uStack_f8 = -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10;
  piStack_fc = DAT_0071d174;
  fStack_100 = 7.56067e-39;
  (**(code **)(*DAT_0071d174 + 0x134))();
  fStack_100 = (float)DAT_0069e350;
  piStack_104 = DAT_0071d174;
  ppiStack_108 = (int **)0x525427;
  (**(code **)(*DAT_0071d174 + 0x170))();
  fVar1 = (float)(int)(short)(DAT_007c1258._2_2_ - DAT_007c1254._2_2_);
  piStack_fc = (int *)((unaff_ESI[1] - *unaff_ESI) / fVar1);
  uStack_f8 = 0;
  piStack_f4 = (int *)0x0;
  piStack_ec = (int *)0x0;
  uStack_e4 = 0;
  uStack_dc = 0x3f800000;
  uStack_d8 = 0;
  piStack_d4 = (int *)0x0;
  uStack_d0 = 0;
  uStack_cc = 0;
  piStack_c8 = (int *)0x3f800000;
  uStack_c4 = 0;
  uStack_c0 = 0;
  piStack_bc = (int *)0x3f800000;
  uStack_b8 = 0;
  uStack_b4 = 0;
  piStack_b0 = (int *)0x0;
  uStack_ac = 0;
  uStack_a8 = 0x3f800000;
  piStack_a4 = (int *)0x0;
  uStack_a0 = 0;
  uStack_9c = 0x3f800000;
  uStack_98 = 0;
  piStack_94 = (int *)0x0;
  uStack_90 = 0;
  uStack_8c = 0;
  fStack_f0 = *unaff_ESI / fVar1;
  fStack_100 = (float)(int)(short)((short)DAT_007c1258 - (short)DAT_007c1254);
  fStack_e8 = (unaff_ESI[3] - unaff_ESI[2]) / fStack_100;
  piStack_e0 = (int *)(unaff_ESI[2] / fStack_100);
  if (DAT_00722b30 != 0) {
    piStack_fc = (int *)((float)piStack_fc * fVar1);
    fStack_f0 = fVar1 * fStack_f0;
    fStack_e8 = fStack_100 * fStack_e8;
    piStack_e0 = (int *)(fStack_100 * (float)piStack_e0);
  }
  piStack_104 = (int *)0x8;
  ppiStack_108 = &piStack_fc;
  uStack_10c = 0xd;
  piStack_110 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x178))();
  FUN_0052ccc0(0,0);
  if (DAT_0069dd90 != (int *)0x0) {
    _DAT_006e1730 = 0;
    _DAT_006e1734 = 0;
    _DAT_006e174c = 0;
    _DAT_006e1778 = 0;
    _DAT_006e1770 = 0;
    _DAT_006e1758 = 0;
    _DAT_006e1740 = 0;
    _DAT_006e1728 = 0;
    _DAT_006e172c = 0xffffffff;
    _DAT_006e1720 = 0xbf820000;
    _DAT_006e1724 = 0x3f820000;
    _DAT_006e1744 = 0xffffffff;
    _DAT_006e1748 = 0x3f800000;
    _DAT_006e1738 = 0x3f7c0000;
    _DAT_006e173c = 0x3f820000;
    _DAT_006e175c = 0xffffffff;
    _DAT_006e1760 = 0x3f800000;
    _DAT_006e1764 = 0x3f800000;
    _DAT_006e1750 = 0x3f7c0000;
    _DAT_006e1754 = 0xbf7c0000;
    _DAT_006e1774 = 0xffffffff;
    _DAT_006e177c = 0x3f800000;
    _DAT_006e1768 = 0xbf820000;
    _DAT_006e176c = 0xbf7c0000;
    (**(code **)(*DAT_0069dd90 + 0x100))(DAT_0069dd90,&piStack_110,3);
    (**(code **)(*DAT_0069dd90 + 0x104))(DAT_0069dd90,0);
    (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1720,0x18);
    (**(code **)(*DAT_0069dd90 + 0x108))(DAT_0069dd90);
  }
  FUN_0052ccc0(0,0);
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
