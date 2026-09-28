// rasterizer_motion_sensor_begin  (Ghidra: FUN_0052b690, unnamed; the earlier placeholder called
//   it rasterizer_debug_marker_setup)
// address 0x52b690, size 1081 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: called once, first, by motion_sensor_render 0x4b4120 before the per blip helper
//   FUN_004b37a0 (which reaches rasterizer_motion_sensor_blip_draw 0x52bad0) and before
//   rasterizer_motion_sensor_end 0x52bc40. It resolves Globals.interface_bitmaps[0]
//   (Globals +0x140/+0x144) motion_sensor_blip_bitmap (+0xcc tag id) and interface_goo_map1
//   (+0xdc tag id), makes sure both are resident, then points the device at render target 5
//   (0x0069d3c8 is rasterizer_render_targets[5].surface), clears it and sets additive blip
//   states. Rebuilt from the raw disassembly (Ghidra lost every device call argument).
// register convention: none.
// blam-cc: none
// UNSURE: 0x0069c689 is only known as a byte that disables the motion sensor path when set.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                    // 0x0071d174
extern Globals *global_globals;                    // 0x00746fa0
extern tag_instance *tag_instances;                // 0x0087bc14
extern uint8_t rasterizer_caps_flag_689;                   // 0x0069c689
extern uint8_t console_debug_toggle_689403;        // 0x00689403 motion sensor rendering enabled
extern uint8_t rasterizer_motion_sensor_ready;     // 0x0071d205
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern int16_t rasterizer_active_render_target;    // 0x0069d350
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

// blam-cc: EAX -> bitmap, stack -> (wait, allocate_if_missing)
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550
// blam-cc: ESI -> bitmap, stack -> stage
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *surface, d3d_surface_desc *desc);
typedef int32_t (__stdcall *d3d_set_viewport_fn)(void *self, const d3d_viewport *viewport);
typedef int32_t (__stdcall *d3d_clear_fn)(void *self, uint32_t count, const void *rects, uint32_t flags, uint32_t color,
                                float z, uint32_t stencil);

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

// First BitmapData of the bitmap tag named by a tag id, or NULL.
static BitmapData *first_bitmap_data(uint32_t tag_id)
{
    uint8_t *bitmap = (uint8_t *)tag_instances[tag_id & 0xffff].data;

    if (bitmap != NULL && *(int32_t *)(bitmap + 0x60) > 0) {      // Bitmap.bitmap_data.count
        return (BitmapData *)(uintptr_t)*(uint32_t *)(bitmap + 0x64);
    }
    return NULL;
}

void rasterizer_motion_sensor_begin(void)
{
    uint8_t *interface_bitmaps;
    BitmapData *blip_bitmap;
    BitmapData *goo_bitmap;
    void *surface;
    d3d_surface_desc desc;
    d3d_viewport viewport;
    float constants[5][4];
    int i, j;

    interface_bitmaps = global_globals->interface_bitmaps.count != 0
                            ? (uint8_t *)(uintptr_t)global_globals->interface_bitmaps.pointer
                            : NULL;
    blip_bitmap = first_bitmap_data(*(uint32_t *)(interface_bitmaps + 0xcc)); // motion_sensor_blip_bitmap
    goo_bitmap = first_bitmap_data(*(uint32_t *)(interface_bitmaps + 0xdc));  // interface_goo_map1

    rasterizer_motion_sensor_ready = 0;
    if (rasterizer_caps_flag_689 || !console_debug_toggle_689403) {
        return;
    }
    if (texture_cache_get(blip_bitmap, 0, 1) == NULL) {
        return;
    }
    if (texture_cache_get(goo_bitmap, 0, 1) == NULL) {
        return;
    }

    surface = (void *)(uintptr_t)rasterizer_render_targets[5].surface;
    rasterizer_motion_sensor_ready = 1;
    ((d3d_call2_fn)device_vtable()[0x94 / 4])(rasterizer_device, 0, (uint32_t)(uintptr_t)surface); // SetRenderTarget
    rasterizer_active_render_target = 5;
    ((d3d_get_desc_fn)(*(void ***)surface)[0x30 / 4])(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    ((d3d_set_viewport_fn)device_vtable()[0xbc / 4])(rasterizer_device, &viewport);
    ((d3d_clear_fn)device_vtable()[0xac / 4])(rasterizer_device, 0, NULL, 1, 0, 1.0f, 0); // TARGET, black

    rasterizer_bind_texture_d3d9(0, blip_bitmap);
    set_sampler_state(0, 1, 3);   // ADDRESSU CLAMP
    set_sampler_state(0, 2, 3);   // ADDRESSV CLAMP
    set_sampler_state(0, 5, 2);   // MAGFILTER LINEAR
    set_sampler_state(0, 6, 2);   // MINFILTER LINEAR
    set_sampler_state(0, 7, 1);   // MIPFILTER POINT
    set_render_state(0x16, 3);    // CULLMODE CCW
    set_render_state(0xa8, 7);    // COLORWRITEENABLE rgb
    set_render_state(0x1b, 1);    // ALPHABLENDENABLE
    set_render_state(0x13, 2);    // SRCBLEND ONE
    set_render_state(0x14, 2);    // DESTBLEND ONE
    set_render_state(0xab, 1);    // BLENDOP ADD
    set_render_state(0x0f, 0);    // ALPHATESTENABLE
    set_render_state(0x07, 0);    // ZENABLE
    set_render_state(0x1c, 0);    // FOGENABLE
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
                                               rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    // the usage bits come from declaration 6, not 8, in the binary (0x006e1ae0)
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                               ((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[6].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[35].shader);
    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0); // SetPixelShader(NULL)

    // c13..c16 identity, c17 (1, 1, 0, 1)
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            constants[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    constants[4][0] = 1.0f;
    constants[4][1] = 1.0f;
    constants[4][2] = 0.0f;
    constants[4][3] = 1.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &constants[0][0], 5);
    set_texture_stage_state(0, 1, 4);   // COLOROP MODULATE
    set_texture_stage_state(0, 2, 2);   // COLORARG1 TEXTURE
    set_texture_stage_state(0, 3, 0);   // COLORARG2 DIFFUSE
    set_texture_stage_state(0, 4, 4);   // ALPHAOP MODULATE
    set_texture_stage_state(0, 5, 2);   // ALPHAARG1 TEXTURE
    set_texture_stage_state(0, 6, 0);   // ALPHAARG2 DIFFUSE
    set_texture_stage_state(1, 1, 1);   // stage 1 COLOROP DISABLE
    set_texture_stage_state(1, 4, 1);   // stage 1 ALPHAOP DISABLE
}

#if 0
Original Ghidra decompilation (0x52b690): (device call arguments lost; the rewrite follows the disassembly)

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0052b690(void)

{
  int iVar1;
  undefined4 uStack_184;
  undefined4 uStack_180;
  int *piStack_17c;
  undefined4 uStack_178;
  undefined4 uStack_174;
  int *piStack_170;
  undefined4 uStack_16c;
  undefined4 uStack_168;
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
  int *piStack_11c;
  undefined4 uStack_118;
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
  undefined4 uStack_c0;
  undefined4 uStack_bc;
  undefined4 uStack_b8;
  undefined4 uStack_b4;
  int *piStack_b0;
  undefined4 *puStack_ac;
  int *piStack_a8;
  undefined1 *puStack_a4;
  int *piStack_a0;
  undefined4 uStack_9c;
  int *piStack_98;
  undefined1 auStack_2c [44];
  
  DAT_0071d205 = 0;
  if ((DAT_0069c689 == '\0') && (DAT_00689403 != '\0')) {
    piStack_98 = (int *)0x1;
    uStack_9c = 0;
    piStack_a0 = (int *)0x52b738;
    iVar1 = texture_cache_get();
    if (iVar1 != 0) {
      piStack_98 = (int *)0x1;
      uStack_9c = 0;
      piStack_a0 = (int *)0x52b74e;
      iVar1 = texture_cache_get();
      piStack_a8 = DAT_0069d3c8;
      if (iVar1 != 0) {
        piStack_98 = DAT_0069d3c8;
        uStack_9c = 0;
        piStack_a0 = DAT_0071d174;
        DAT_0071d205 = 1;
        puStack_a4 = (undefined1 *)0x52b777;
        (**(code **)(*DAT_0071d174 + 0x94))();
        puStack_a4 = auStack_2c;
        _DAT_0069d350 = 5;
        puStack_ac = (undefined4 *)0x52b78b;
        (**(code **)(*piStack_a8 + 0x30))();
        puStack_ac = &uStack_9c;
        uStack_9c = 0;
        piStack_98 = (int *)0x0;
        piStack_b0 = DAT_0071d174;
        uStack_b4 = 0x52b7d4;
        (**(code **)(*DAT_0071d174 + 0xbc))();
        uStack_b4 = 0;
        uStack_b8 = 0x3f800000;
        uStack_bc = 0;
        uStack_c0 = 1;
        uStack_c4 = 0;
        uStack_c8 = 0;
        piStack_cc = DAT_0071d174;
        uStack_d0 = 0x52b7f1;
        (**(code **)(*DAT_0071d174 + 0xac))();
        uStack_d0 = 0;
        uStack_d4 = 0x52b7f8;
        FUN_00518680();
        uStack_d0 = 3;
        uStack_d4 = 1;
        uStack_d8 = 0;
        piStack_dc = DAT_0071d174;
        uStack_e0 = 0x52b80f;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_e0 = 3;
        uStack_e4 = 2;
        uStack_e8 = 0;
        piStack_ec = DAT_0071d174;
        uStack_f0 = 0x52b823;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_f0 = 2;
        uStack_f4 = 5;
        uStack_f8 = 0;
        piStack_fc = DAT_0071d174;
        uStack_100 = 0x52b837;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_100 = 2;
        uStack_104 = 6;
        uStack_108 = 0;
        piStack_10c = DAT_0071d174;
        uStack_110 = 0x52b84b;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_110 = 1;
        uStack_114 = 7;
        uStack_118 = 0;
        piStack_11c = DAT_0071d174;
        uStack_120 = 0x52b85f;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_120 = 3;
        uStack_124 = 0x16;
        piStack_128 = DAT_0071d174;
        uStack_12c = 0x52b871;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_12c = 7;
        uStack_130 = 0xa8;
        piStack_134 = DAT_0071d174;
        uStack_138 = 0x52b886;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_138 = 1;
        uStack_13c = 0x1b;
        piStack_140 = DAT_0071d174;
        uStack_144 = 0x52b898;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_144 = 2;
        uStack_148 = 0x13;
        piStack_14c = DAT_0071d174;
        uStack_150 = 0x52b8aa;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_150 = 2;
        uStack_154 = 0x14;
        piStack_158 = DAT_0071d174;
        uStack_15c = 0x52b8bc;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_15c = 1;
        uStack_160 = 0xab;
        piStack_164 = DAT_0071d174;
        uStack_168 = 0x52b8d1;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_168 = 0;
        uStack_16c = 0xf;
        piStack_170 = DAT_0071d174;
        uStack_174 = 0x52b8e3;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_174 = 0;
        uStack_178 = 7;
        piStack_17c = DAT_0071d174;
        uStack_180 = 0x52b8f5;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_180 = 0;
        uStack_184 = 0x1c;
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174);
        (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1af0);
        (**(code **)(*DAT_0071d174 + 0x134))
                  (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1ae0 & 0x10);
        (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e468);
        (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
        uStack_184 = 0x3f800000;
        uStack_180 = 0;
        piStack_17c = (int *)0x0;
        uStack_178 = 0;
        uStack_174 = 0;
        piStack_170 = (int *)0x3f800000;
        uStack_16c = 0;
        uStack_168 = 0;
        piStack_164 = (int *)0x0;
        uStack_160 = 0;
        uStack_15c = 0x3f800000;
        piStack_158 = (int *)0x0;
        uStack_154 = 0;
        uStack_150 = 0;
        piStack_14c = (int *)0x0;
        uStack_148 = 0x3f800000;
        uStack_144 = 0x3f800000;
        piStack_140 = (int *)0x3f800000;
        uStack_13c = 0;
        uStack_138 = 0x3f800000;
        (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&uStack_184,5);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      }
    }
  }
  return;
}
#endif
