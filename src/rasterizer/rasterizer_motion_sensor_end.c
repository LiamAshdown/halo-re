// rasterizer_motion_sensor_end  (Ghidra: FUN_0052bc40, unnamed; the earlier draft called it
//   rasterizer_debug_marker_draw_textured)
// address 0x52bc40, size 2141 bytes
// VERIFIED against disassembly 0x52bc40..0x52c49d (2026-09-30): exit paths, the additive sweep quad (sweep <= 2.75), the mask multiply quad, the window target restore and the composite quad (32 / 42 pixel half size) with c13..c17
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: the last call of motion_sensor_render 0x4b4120, made with EBX = 0x00873d38 (the
//   sensor screen position the caller just stored as two floats) and the sweep value
//   0x0071943c on the stack. While render target 5 is still bound it adds the sweep
//   (Globals.interface_bitmaps[0] motion_sensor_sweep_bitmap, +0x7c tag id) as a full target
//   quad when sweep <= 2.75, multiplies by the sweep mask (+0x8c tag id), restores the window
//   render target through rasterizer_render_target_set_active 0x52ccc0, and composites render
//   target 5 additively as a 32 (split screen) or 42 pixel half size quad at the sensor
//   position, with c13..c16 mapping 640x480 pixels to clip space. Rebuilt from the raw
//   disassembly (Ghidra lost the stack argument, EBX and every device call argument).
// register convention: EBX -> position (float[2], 640x480 screen pixels), stack -> sweep.
// blam-cc: EBX -> position, stack -> sweep
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                    // 0x0071d174
extern Globals *global_globals;                    // 0x00746fa0
extern tag_instance *tag_instances;                // 0x0087bc14
extern player_globals *local_player_globals;       // 0x0087a478
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220
extern uint8_t console_debug_toggle_689403;        // 0x00689403 motion sensor rendering enabled
extern uint8_t rasterizer_motion_sensor_ready;     // 0x0071d205
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern const float rasterizer_identity_vertex_constants[5][4]; // 0x0065e118 .rdata: identity, then (1, 1, 0, 1)

// blam-cc: EAX -> bitmap, stack -> (wait, allocate_if_missing)
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550
// blam-cc: ESI -> bitmap, stack -> stage
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680
// blam-cc: EAX -> target_index, stack -> (clear_color, clear)
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear); // 0x52ccc0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
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
static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}
static void draw_fan(const rasterizer_dynamic_screen_vertex *vertices)
{
    ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, vertices,
                                                           sizeof(rasterizer_dynamic_screen_vertex));
}
static void set_vertex(rasterizer_dynamic_screen_vertex *vertex, float x, float y, uint32_t color, float u, float v)
{
    vertex->x = x;
    vertex->y = y;
    vertex->z = 0.0f;
    vertex->color = color;
    vertex->u = u;
    vertex->v = v;
}

// First BitmapData of the bitmap tag named by a tag id, or NULL.
static BitmapData *first_bitmap_data(uint32_t tag_id)
{
    uint8_t *bitmap = (uint8_t *)tag_instances[tag_id & 0xffff].data;

    if (bitmap != NULL && *(int32_t *)(bitmap + 0x60) > 0) {      // Bitmap.bitmap_data.count
        return (BitmapData *)(uintptr_t)*(uint32_t *)(bitmap + 0x64);
    }
    return NULL;
}

void rasterizer_motion_sensor_end(const float *position, float sweep)
{
    uint8_t *interface_bitmaps;
    BitmapData *sweep_bitmap;
    BitmapData *mask_bitmap;
    rasterizer_dynamic_screen_vertex vertices[4];
    float constants[5][4];
    float half_size;

    interface_bitmaps = global_globals->interface_bitmaps.count != 0
                            ? (uint8_t *)(uintptr_t)global_globals->interface_bitmaps.pointer
                            : NULL;
    sweep_bitmap = first_bitmap_data(*(uint32_t *)(interface_bitmaps + 0x7c)); // motion_sensor_sweep_bitmap
    mask_bitmap = first_bitmap_data(*(uint32_t *)(interface_bitmaps + 0x8c));  // motion_sensor_sweep_bitmap_mask

    if (!console_debug_toggle_689403) {
        return;
    }
    if (!rasterizer_motion_sensor_ready ||
        texture_cache_get(sweep_bitmap, 0, 1) == NULL ||
        texture_cache_get(mask_bitmap, 0, 1) == NULL) {
        // begin redirected rendering but a bitmap is missing: only give the window target back
        if (console_debug_toggle_689403 && rasterizer_motion_sensor_ready) {
            rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
        }
        return;
    }

    // additive sweep into render target 5
    rasterizer_bind_texture_d3d9(0, sweep_bitmap);
    set_sampler_state(0, 1, 3);
    set_sampler_state(0, 2, 3);
    set_sampler_state(0, 5, 2);
    set_sampler_state(0, 6, 2);
    set_sampler_state(0, 7, 0);   // MIPFILTER NONE
    set_render_state(0x16, 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x13, 2);    // SRCBLEND ONE
    set_render_state(0x14, 5);    // DESTBLEND SRCALPHA
    set_render_state(0xab, 1);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 0);
    set_render_state(0x1c, 0);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd,
                                                        &rasterizer_identity_vertex_constants[0][0], 5);
    set_texture_stage_state(0, 1, 4);   // COLOROP MODULATE
    set_texture_stage_state(0, 2, 2);   // COLORARG1 TEXTURE
    set_texture_stage_state(0, 3, 0);   // COLORARG2 DIFFUSE
    set_texture_stage_state(0, 4, 2);   // ALPHAOP SELECTARG1
    set_texture_stage_state(0, 5, 0);   // ALPHAARG1 DIFFUSE
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
    if (sweep <= 2.75f) {
        float t = sweep * 0.5f;
        float near_u = t + 0.5f;
        float far_u = 0.5f - t;

        set_vertex(&vertices[0], -1.015625f, 1.046875f, 0xff74b9ff, near_u, far_u);
        set_vertex(&vertices[1], 1.046875f, 1.046875f, 0xff74b9ff, far_u, far_u);
        set_vertex(&vertices[2], 1.046875f, -1.015625f, 0xff74b9ff, far_u, near_u);
        set_vertex(&vertices[3], -1.015625f, -1.015625f, 0xff74b9ff, near_u, near_u);
        draw_fan(vertices);
    }

    // multiply by the sweep mask
    set_sampler_state(0, 7, 1);
    rasterizer_bind_texture_d3d9(0, mask_bitmap);
    set_render_state(0x1b, 1);
    set_render_state(0x13, 1);    // SRCBLEND ZERO
    set_render_state(0x14, 5);    // DESTBLEND SRCALPHA
    set_render_state(0x1c, 0);
    set_texture_stage_state(0, 1, 2);   // COLOROP SELECTARG1
    set_texture_stage_state(0, 2, 2);   // COLORARG1 TEXTURE
    set_texture_stage_state(0, 4, 2);   // ALPHAOP SELECTARG1
    set_texture_stage_state(0, 5, 2);   // ALPHAARG1 TEXTURE
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
    set_vertex(&vertices[0], -1.015625f, 1.046875f, 0xff66cc66, 1.0f, 0.0f);
    set_vertex(&vertices[1], 1.046875f, 1.046875f, 0xff66cc66, 0.0f, 0.0f);
    set_vertex(&vertices[2], 1.046875f, -1.015625f, 0xff66cc66, 0.0f, 1.0f);
    set_vertex(&vertices[3], -1.015625f, -1.015625f, 0xff66cc66, 1.0f, 1.0f);
    draw_fan(vertices);

    // back to the window target, then add render target 5 at the sensor position
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, 0, rasterizer_render_targets[5].texture); // SetTexture
    set_sampler_state(0, 1, 3);
    set_sampler_state(0, 2, 3);
    set_sampler_state(0, 5, 2);
    set_sampler_state(0, 6, 2);
    set_sampler_state(0, 7, 1);
    set_render_state(0x16, 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x13, 2);    // ONE
    set_render_state(0x14, 2);    // ONE
    set_render_state(0xab, 1);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 0);
    set_render_state(0x1c, 0);
    // pixels (640x480, half pixel offset) to clip space
    constants[0][0] = 0.003125f;      constants[0][1] = 0.0f;           constants[0][2] = 0.0f; constants[0][3] = -1.0015625f;
    constants[1][0] = 0.0f;           constants[1][1] = -0.004166667f;  constants[1][2] = 0.0f; constants[1][3] = 1.0020833f;
    constants[2][0] = 0.0f;           constants[2][1] = 0.0f;           constants[2][2] = 0.0f; constants[2][3] = 0.5f;
    constants[3][0] = 0.0f;           constants[3][1] = 0.0f;           constants[3][2] = 0.0f; constants[3][3] = 1.0f;
    constants[4][0] = 1.0f;           constants[4][1] = 1.0f;           constants[4][2] = 0.0f; constants[4][3] = 1.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &constants[0][0], 5);
    half_size = (local_player_globals->local_player_count > 1) ? 32.0f : 42.0f;
    set_vertex(&vertices[0], position[0] - half_size, position[1] - half_size, 0xffffffff, 0.0f, 0.0f);
    set_vertex(&vertices[1], position[0] + half_size, position[1] - half_size, 0xffffffff, 1.0f, 0.0f);
    set_vertex(&vertices[2], position[0] + half_size, position[1] + half_size, 0xffffffff, 1.0f, 1.0f);
    set_vertex(&vertices[3], position[0] - half_size, position[1] + half_size, 0xffffffff, 0.0f, 1.0f);
    draw_fan(vertices);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x52bc40): (stack argument, EBX and device call arguments lost; the rewrite follows the disassembly)

void FUN_0052bc40(void)

{
  float fVar1;
  int iVar2;
  float *unaff_EBX;
  float fStack_378;
  int *piStack_374;
  undefined4 uStack_370;
  undefined4 uStack_36c;
  int *piStack_368;
  undefined4 uStack_364;
  float fStack_360;
  int *piStack_35c;
  undefined4 uStack_358;
  undefined4 uStack_354;
  int *piStack_350;
  undefined4 uStack_34c;
  float fStack_348;
  int *piStack_344;
  undefined4 uStack_340;
  undefined4 uStack_33c;
  int *piStack_338;
  undefined4 uStack_334;
  float fStack_330;
  int *piStack_32c;
  undefined4 uStack_328;
  undefined4 uStack_324;
  int *piStack_320;
  undefined4 uStack_31c;
  undefined4 uStack_318;
  int *piStack_314;
  undefined4 uStack_310;
  undefined4 uStack_30c;
  int *piStack_308;
  undefined4 uStack_304;
  undefined4 uStack_300;
  undefined4 uStack_2fc;
  int *piStack_2f8;
  undefined4 uStack_2f4;
  undefined4 uStack_2f0;
  undefined4 uStack_2ec;
  int *piStack_2e8;
  undefined4 uStack_2e4;
  undefined4 uStack_2e0;
  undefined4 uStack_2dc;
  int *piStack_2d8;
  undefined4 uStack_2d4;
  undefined4 uStack_2d0;
  undefined4 uStack_2cc;
  int *piStack_2c8;
  undefined4 uStack_2c4;
  undefined4 uStack_2c0;
  undefined4 uStack_2bc;
  int *piStack_2b8;
  undefined4 uStack_2b4;
  undefined4 uStack_2b0;
  int *piStack_2ac;
  undefined4 uStack_2a8;
  undefined4 uStack_2a4;
  undefined4 *puStack_2a0;
  undefined4 uStack_29c;
  int *piStack_298;
  undefined4 uStack_294;
  undefined4 uStack_290;
  undefined4 uStack_28c;
  int *piStack_288;
  undefined4 uStack_284;
  undefined4 uStack_280;
  undefined4 uStack_27c;
  int *piStack_278;
  undefined4 uStack_274;
  undefined4 uStack_270;
  undefined4 uStack_26c;
  int *piStack_268;
  undefined4 uStack_264;
  undefined4 uStack_260;
  undefined4 uStack_25c;
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
  int *piStack_22c;
  undefined4 uStack_228;
  undefined4 uStack_224;
  int *piStack_220;
  undefined4 uStack_21c;
  undefined4 uStack_218;
  int *piStack_214;
  undefined4 uStack_210;
  int *piStack_20c;
  int *piStack_208;
  undefined4 uStack_204;
  undefined4 *puStack_200;
  undefined4 uStack_1fc;
  int *piStack_1f8;
  undefined4 uStack_1f4;
  float fStack_1f0;
  undefined4 uStack_1ec;
  int *piStack_1e8;
  undefined4 uStack_1e4;
  undefined4 uStack_1e0;
  int *piStack_1dc;
  int *piStack_1d8;
  undefined4 uStack_1d4;
  undefined4 uStack_1d0;
  undefined4 uStack_1cc;
  int *piStack_1c8;
  int *piStack_1c4;
  int *piStack_1c0;
  undefined4 uStack_1bc;
  int *piStack_1b8;
  undefined4 uStack_1b4;
  undefined4 uStack_1b0;
  int *piStack_1ac;
  int *piStack_1a8;
  undefined4 uStack_1a4;
  undefined4 uStack_1a0;
  undefined4 uStack_19c;
  int *piStack_198;
  int *piStack_194;
  int *piStack_190;
  undefined4 uStack_18c;
  int *piStack_188;
  undefined4 uStack_184;
  undefined *puStack_180;
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
  float fStack_138;
  undefined4 uStack_134;
  int *piStack_130;
  undefined4 uStack_12c;
  undefined4 uStack_128;
  int *piStack_124;
  undefined4 uStack_120;
  undefined4 uStack_11c;
  int *piStack_118;
  undefined4 uStack_114;
  undefined4 uStack_110;
  int *piStack_10c;
  undefined4 uStack_108;
  undefined4 uStack_104;
  undefined4 uStack_100;
  int *piStack_fc;
  undefined4 uStack_f8;
  undefined4 uStack_f4;
  undefined4 uStack_f0;
  int *piStack_ec;
  undefined4 uStack_e8;
  undefined4 uStack_e4;
  undefined4 uStack_e0;
  int *piStack_dc;
  undefined4 uStack_d8;
  undefined4 uStack_d4;
  undefined4 uStack_d0;
  int *piStack_cc;
  undefined4 uStack_c8;
  undefined4 uStack_c4;
  
  if (DAT_00689403 != '\0') {
    if (DAT_0071d205 != '\0') {
      uStack_c4 = 0;
      uStack_c8 = 0x52bcdf;
      iVar2 = texture_cache_get();
      if (iVar2 != 0) {
        uStack_c4 = 0;
        uStack_c8 = 0x52bcf5;
        iVar2 = texture_cache_get();
        if (iVar2 != 0) {
          uStack_c4 = 0x52bd07;
          FUN_00518680();
          uStack_c4 = 1;
          uStack_c8 = 0;
          piStack_cc = DAT_0071d174;
          uStack_d0 = 0x52bd1e;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_d0 = 3;
          uStack_d4 = 2;
          uStack_d8 = 0;
          piStack_dc = DAT_0071d174;
          uStack_e0 = 0x52bd32;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_e0 = 2;
          uStack_e4 = 5;
          uStack_e8 = 0;
          piStack_ec = DAT_0071d174;
          uStack_f0 = 0x52bd46;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_f0 = 2;
          uStack_f4 = 6;
          uStack_f8 = 0;
          piStack_fc = DAT_0071d174;
          uStack_100 = 0x52bd5a;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_100 = 0;
          uStack_104 = 7;
          uStack_108 = 0;
          piStack_10c = DAT_0071d174;
          uStack_110 = 0x52bd6e;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_110 = 3;
          uStack_114 = 0x16;
          piStack_118 = DAT_0071d174;
          uStack_11c = 0x52bd80;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_11c = 7;
          uStack_120 = 0xa8;
          piStack_124 = DAT_0071d174;
          uStack_128 = 0x52bd95;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_128 = 1;
          uStack_12c = 0x1b;
          piStack_130 = DAT_0071d174;
          uStack_134 = 0x52bda7;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_134 = 2;
          fStack_138 = 2.66247e-44;
          piStack_13c = DAT_0071d174;
          uStack_140 = 0x52bdb9;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_140 = 5;
          uStack_144 = 0x14;
          piStack_148 = DAT_0071d174;
          uStack_14c = 0x52bdcb;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_14c = 1;
          uStack_150 = 0xab;
          piStack_154 = DAT_0071d174;
          uStack_158 = 0x52bde0;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_158 = 0;
          uStack_15c = 0xf;
          piStack_160 = DAT_0071d174;
          uStack_164 = 0x52bdf2;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_164 = 0;
          uStack_168 = 7;
          piStack_16c = DAT_0071d174;
          uStack_170 = 0x52be04;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_170 = 0;
          uStack_174 = 0x1c;
          piStack_178 = DAT_0071d174;
          uStack_17c = 0x52be16;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_17c = 5;
          puStack_180 = &DAT_0065e118;
          uStack_184 = 0xd;
          piStack_188 = DAT_0071d174;
          uStack_18c = 0x52be2d;
          (**(code **)(*DAT_0071d174 + 0x178))();
          uStack_18c = 4;
          piStack_190 = (int *)0x1;
          piStack_194 = (int *)0x0;
          piStack_198 = DAT_0071d174;
          uStack_19c = 0x52be41;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_19c = 2;
          uStack_1a0 = 2;
          uStack_1a4 = 0;
          piStack_1a8 = DAT_0071d174;
          piStack_1ac = (int *)0x52be55;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          piStack_1ac = (int *)0x0;
          uStack_1b0 = 3;
          uStack_1b4 = 0;
          piStack_1b8 = DAT_0071d174;
          uStack_1bc = 0x52be69;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_1bc = 2;
          piStack_1c0 = (int *)&DAT_00000004;
          piStack_1c4 = (int *)0x0;
          piStack_1c8 = DAT_0071d174;
          uStack_1cc = 0x52be7d;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_1cc = 0;
          uStack_1d0 = 5;
          uStack_1d4 = 0;
          piStack_1d8 = DAT_0071d174;
          piStack_1dc = (int *)0x52be91;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          piStack_1dc = (int *)0x1;
          uStack_1e0 = 1;
          uStack_1e4 = 1;
          piStack_1e8 = DAT_0071d174;
          uStack_1ec = 0x52bea5;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_1ec = 1;
          fStack_1f0 = 5.60519e-45;
          uStack_1f4 = 1;
          piStack_1f8 = DAT_0071d174;
          uStack_1fc = 0x52beb9;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          if (fStack_138 < 2.75 != (fStack_138 == 2.75)) {
            uStack_1fc = 0x18;
            fStack_1f0 = fStack_138 * 0.5;
            puStack_200 = &uStack_1ec;
            uStack_204 = 2;
            piStack_1dc = (int *)(fStack_1f0 + 0.5);
            uStack_1e0 = 0xff74b9ff;
            piStack_1c8 = (int *)0xff74b9ff;
            uStack_1b0 = 0xff74b9ff;
            piStack_198 = (int *)0xff74b9ff;
            piStack_1d8 = (int *)(0.5 - fStack_1f0);
            piStack_208 = (int *)0x6;
            uStack_1ec = 0xbf820000;
            piStack_1e8 = (int *)0x3f860000;
            uStack_1d4 = 0x3f860000;
            uStack_1d0 = 0x3f860000;
            uStack_1bc = 0x3f860000;
            piStack_1b8 = (int *)0xbf820000;
            uStack_1a4 = 0xbf820000;
            uStack_1a0 = 0xbf820000;
            uStack_19c = 0;
            uStack_1b4 = 0;
            uStack_1cc = 0;
            uStack_1e4 = 0;
            piStack_20c = DAT_0071d174;
            uStack_210 = 0x52bfa0;
            piStack_1c4 = piStack_1d8;
            piStack_1c0 = piStack_1d8;
            piStack_1ac = piStack_1d8;
            piStack_1a8 = piStack_1dc;
            piStack_194 = piStack_1dc;
            piStack_190 = piStack_1dc;
            (**(code **)(*DAT_0071d174 + 0x14c))();
          }
          uStack_1fc = 1;
          puStack_200 = (undefined4 *)0x7;
          uStack_204 = 0;
          piStack_208 = DAT_0071d174;
          piStack_20c = (int *)0x52bfb4;
          (**(code **)(*DAT_0071d174 + 0x114))();
          piStack_20c = (int *)0x0;
          uStack_210 = 0x52bfbd;
          FUN_00518680();
          piStack_20c = (int *)0x1;
          uStack_210 = 0x1b;
          piStack_214 = DAT_0071d174;
          uStack_218 = 0x52bfd2;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_218 = 1;
          uStack_21c = 0x13;
          piStack_220 = DAT_0071d174;
          uStack_224 = 0x52bfe4;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_224 = 5;
          uStack_228 = 0x14;
          piStack_22c = DAT_0071d174;
          uStack_230 = 0x52bff6;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_230 = 0;
          uStack_234 = 0x1c;
          piStack_238 = DAT_0071d174;
          uStack_23c = 0x52c008;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_23c = 2;
          uStack_240 = 1;
          uStack_244 = 0;
          piStack_248 = DAT_0071d174;
          uStack_24c = 0x52c01c;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_24c = 2;
          uStack_250 = 2;
          uStack_254 = 0;
          piStack_258 = DAT_0071d174;
          uStack_25c = 0x52c030;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_25c = 2;
          uStack_260 = 4;
          uStack_264 = 0;
          piStack_268 = DAT_0071d174;
          uStack_26c = 0x52c044;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_26c = 2;
          uStack_270 = 5;
          uStack_274 = 0;
          piStack_278 = DAT_0071d174;
          uStack_27c = 0x52c058;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_27c = 1;
          uStack_280 = 1;
          uStack_284 = 1;
          piStack_288 = DAT_0071d174;
          uStack_28c = 0x52c06c;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_28c = 1;
          uStack_290 = 4;
          uStack_294 = 1;
          piStack_298 = DAT_0071d174;
          uStack_29c = 0x52c080;
          (**(code **)(*DAT_0071d174 + 0x10c))();
          uStack_280 = 0xff66cc66;
          uStack_27c = 0x3f800000;
          piStack_278 = (int *)0x0;
          uStack_29c = 0x18;
          puStack_2a0 = &uStack_28c;
          uStack_2a4 = 2;
          piStack_268 = (int *)0xff66cc66;
          uStack_250 = 0xff66cc66;
          piStack_238 = (int *)0xff66cc66;
          uStack_2a8 = 6;
          uStack_28c = 0xbf820000;
          piStack_288 = (int *)0x3f860000;
          uStack_264 = 0;
          uStack_260 = 0;
          uStack_274 = 0x3f860000;
          uStack_270 = 0x3f860000;
          uStack_24c = 0;
          piStack_248 = (int *)0x3f800000;
          uStack_25c = 0x3f860000;
          piStack_258 = (int *)0xbf820000;
          uStack_234 = 0x3f800000;
          uStack_230 = 0x3f800000;
          uStack_244 = 0xbf820000;
          uStack_240 = 0xbf820000;
          uStack_23c = 0;
          uStack_254 = 0;
          uStack_26c = 0;
          uStack_284 = 0;
          piStack_2ac = DAT_0071d174;
          uStack_2b0 = 0x52c14e;
          (**(code **)(*DAT_0071d174 + 0x14c))();
          uStack_2b0 = 0;
          uStack_2b4 = 0;
          piStack_2b8 = (int *)0x52c15c;
          FUN_0052ccc0();
          uStack_2b0 = DAT_0069d3cc;
          uStack_2b4 = 0;
          piStack_2b8 = DAT_0071d174;
          uStack_2bc = 0x52c176;
          (**(code **)(*DAT_0071d174 + 0x104))();
          uStack_2bc = 3;
          uStack_2c0 = 1;
          uStack_2c4 = 0;
          piStack_2c8 = DAT_0071d174;
          uStack_2cc = 0x52c18a;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_2cc = 3;
          uStack_2d0 = 2;
          uStack_2d4 = 0;
          piStack_2d8 = DAT_0071d174;
          uStack_2dc = 0x52c19e;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_2dc = 2;
          uStack_2e0 = 5;
          uStack_2e4 = 0;
          piStack_2e8 = DAT_0071d174;
          uStack_2ec = 0x52c1b2;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_2ec = 2;
          uStack_2f0 = 6;
          uStack_2f4 = 0;
          piStack_2f8 = DAT_0071d174;
          uStack_2fc = 0x52c1c6;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_2fc = 1;
          uStack_300 = 7;
          uStack_304 = 0;
          piStack_308 = DAT_0071d174;
          uStack_30c = 0x52c1da;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_30c = 3;
          uStack_310 = 0x16;
          piStack_314 = DAT_0071d174;
          uStack_318 = 0x52c1ec;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_318 = 7;
          uStack_31c = 0xa8;
          piStack_320 = DAT_0071d174;
          uStack_324 = 0x52c201;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_324 = 1;
          uStack_328 = 0x1b;
          piStack_32c = DAT_0071d174;
          fStack_330 = 7.600131e-39;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          fStack_330 = 2.8026e-45;
          uStack_334 = 0x13;
          piStack_338 = DAT_0071d174;
          uStack_33c = 0x52c225;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_33c = 2;
          uStack_340 = 0x14;
          piStack_344 = DAT_0071d174;
          fStack_348 = 7.600182e-39;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          fStack_348 = 1.4013e-45;
          uStack_34c = 0xab;
          piStack_350 = DAT_0071d174;
          uStack_354 = 0x52c24c;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_354 = 0;
          uStack_358 = 0xf;
          piStack_35c = DAT_0071d174;
          fStack_360 = 7.600236e-39;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          fStack_360 = 0.0;
          uStack_364 = 7;
          piStack_368 = DAT_0071d174;
          uStack_36c = 0x52c270;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_36c = 0;
          uStack_370 = 0x1c;
          piStack_374 = DAT_0071d174;
          fStack_378 = 7.600287e-39;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          fStack_378 = 7.00649e-45;
          piStack_308 = (int *)0x3b4ccccd;
          uStack_304 = 0;
          uStack_300 = 0;
          uStack_2fc = 0xbf803333;
          piStack_2f8 = (int *)0x0;
          uStack_2f4 = 0xbb888889;
          uStack_2f0 = 0;
          uStack_2ec = 0x3f804444;
          piStack_2e8 = (int *)0x0;
          uStack_2e4 = 0;
          uStack_2e0 = 0;
          uStack_2dc = 0x3f000000;
          piStack_2d8 = (int *)0x0;
          uStack_2d4 = 0;
          uStack_2d0 = 0;
          uStack_2cc = 0x3f800000;
          piStack_2c8 = (int *)0x3f800000;
          uStack_2c4 = 0x3f800000;
          uStack_2c0 = 0;
          uStack_2bc = 0x3f800000;
          (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&piStack_308);
          if (*(short *)(DAT_0087a478 + 0xc) < 2) {
            fVar1 = 42.0;
          }
          else {
            fVar1 = 32.0;
          }
          fStack_378 = *unaff_EBX - fVar1;
          uStack_36c = 0xffffffff;
          uStack_354 = 0xffffffff;
          uStack_33c = 0xffffffff;
          uStack_324 = 0xffffffff;
          piStack_374 = (int *)(unaff_EBX[1] - fVar1);
          piStack_368 = (int *)0x0;
          fStack_360 = fVar1 + *unaff_EBX;
          uStack_364 = 0;
          piStack_350 = (int *)0x3f800000;
          uStack_34c = 0;
          piStack_338 = (int *)0x3f800000;
          uStack_334 = 0x3f800000;
          piStack_320 = (int *)0x0;
          uStack_31c = 0x3f800000;
          uStack_328 = 0;
          uStack_340 = 0;
          piStack_344 = (int *)(fVar1 + unaff_EBX[1]);
          uStack_358 = 0;
          uStack_370 = 0;
          piStack_35c = piStack_374;
          fStack_348 = fStack_360;
          fStack_330 = fStack_378;
          piStack_32c = piStack_344;
          (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&fStack_378,0x18);
          (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
          return;
        }
      }
    }
    if ((DAT_00689403 != '\0') && (DAT_0071d205 != '\0')) {
      uStack_c4 = 0;
      uStack_c8 = 0x52c491;
      FUN_0052ccc0();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
