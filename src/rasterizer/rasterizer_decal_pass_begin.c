// rasterizer_decal_pass_begin  (Ghidra: FUN_0051a810, unnamed; named from
// out/phase4/rasterizer_functions.md's summary: "Prepares fixed-function render states and
// texture bindings for beginning a decal rendering pass on the given stage.")
// address 0x51a810, size 562 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x51a810..0x51aa41 (DI stage); 0x6893e4 is compared as a word.)
// evidence: gates on debug toggles, resets three decal batching trackers
//   (0x006d98d8/e0/e4/0x0071d1c4, UNSURE names), resolves GlobalsRasterizerData's first fallback
//   bitmap (same tag_instances/Bitmap.bitmap_data pattern as
//   rasterizer_resolve_and_cache_submap_b.c) and caches its *second* submap's width/height, sets
//   a fixed bundle of texture stage/render states (mirroring
//   rasterizer_set_default_render_states.c's shape), applies the decal z-bias
//   (rasterizer_apply_decal_zbias), then a stage-3-specific vs. general alpha-test branch, and
//   finally binds the decal dynamic vertex cache as the current stream source.
// register convention: stage in unaff_DI. // blam-cc: unaff_DI -> stage

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220

extern uint8_t decals_for_all_responses;                         // 0x006893f5
extern int16_t rasterizer_decal_layer;                              // 0x006d98dc set by rasterizer_decal_pass_begin
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern int16_t rasterizer_decal_blend_mode;                         // 0x006d98d8 last framebuffer blend function
extern int16_t rasterizer_decal_bitmap_frame;                       // 0x006d98e4 last bound decal frame
extern uint32_t rasterizer_decal_bitmap_tag;                        // 0x006d98e0 last bound decal bitmap
extern uint8_t unknown_0071d1c4;  // 0x0071d1c4 UNSURE
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_a[2]; // 0x006d986c
extern void *rasterizer_device; // 0x0071d174
extern uint8_t console_debug_toggle_689441;                         // 0x00689441
extern void *rasterizer_decal_vertex_cache; // 0x0071d1bc

// blam-cc: ESI -> bitmap, stack -> stage

// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200


typedef int32_t (__stdcall *d3d_set_sampler_state_fn)(void *device, uint32_t sampler, uint32_t type, uint32_t value);
typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *device, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);

// blam-cc: unaff_DI -> stage
void rasterizer_decal_pass_begin(int16_t stage)
{
    void **vtable;
    uint8_t proceed = 1;

    if (decals_for_all_responses == 0 && stage != 3) {
        proceed = 0;
    }
    if (*(int16_t *)&console_debug_toggle_6893e4 != 0 || !proceed) { // 0x51a82b: WORD compare
        rasterizer_decal_layer = stage;
        return;
    }

    rasterizer_decal_blend_mode = 0xffff;
    rasterizer_decal_bitmap_frame = 0xffff;
    rasterizer_decal_bitmap_tag = 0xffffffff;
    unknown_0071d1c4 = 0;
    rasterizer_decal_layer = stage;

    {
        TagID *fallback_tag_id = (TagID *)((uint8_t *)rasterizer_globals_data + 0xb8);
        if (*(uint32_t *)fallback_tag_id != 0xffffffff) {
            Bitmap *bitmap = (Bitmap *)tag_instances[fallback_tag_id->index].data;
            if (bitmap != (Bitmap *)0 && bitmap->bitmap_data.count > 1) {
                uint8_t *first_submap = (uint8_t *)bitmap->bitmap_data.pointer;
                if (first_submap != (uint8_t *)(uint32_t)-0x30) {
                    rasterizer_bind_texture_d3d9(0, (BitmapData *)(first_submap + 0x30)); // ESI = entry 1 (0x51a8ab)
                    rasterizer_bound_bitmap_size_a[0] = *(int16_t *)(first_submap + 0x34); // second submap's width
                    rasterizer_bound_bitmap_size_a[1] = *(int16_t *)(first_submap + 0x36); // second submap's height
                }
            }
        }
    }

    vtable = *(void ***)rasterizer_device;
    {
        d3d_set_sampler_state_fn set_sampler_state = (d3d_set_sampler_state_fn)vtable[0x114 / 4]; // SetSamplerState
        set_sampler_state(rasterizer_device, 0, 1, 3);
        set_sampler_state(rasterizer_device, 0, 2, 3);
        set_sampler_state(rasterizer_device, 0, 5, 2);
        set_sampler_state(rasterizer_device, 0, 6, 2);
        set_sampler_state(rasterizer_device, 0, 7, 2);
    }
    vtable = *(void ***)rasterizer_device;
    {
        d3d_set_render_state_fn set_rs = (d3d_set_render_state_fn)vtable[0xe4 / 4];
        set_rs(rasterizer_device, 0x16, 3);
        set_rs(rasterizer_device, 0x1b, 1);
        set_rs(rasterizer_device, 7, 1);
        set_rs(rasterizer_device, 0xe, 0);
        set_rs(rasterizer_device, 0x17, 4);
        set_rs(rasterizer_device, 0x1c, 0);
    }

    rasterizer_apply_decal_zbias();

    vtable = *(void ***)rasterizer_device;
    if (stage == 3) {
        d3d_set_render_state_fn set_rs = (d3d_set_render_state_fn)vtable[0xe4 / 4];
        set_rs(rasterizer_device, 0xf, 1);
        set_rs(rasterizer_device, 0x18, 0x7f);
        rasterizer_set_shader_stage_config(4);         // AX = 4 (0x51a9b2); the earlier rewrite passed 3
    } else {
        d3d_set_render_state_fn set_rs = (d3d_set_render_state_fn)vtable[0xe4 / 4];
        if ((console_debug_toggle_689441 == 0 || rasterizer_window.fog.atmospheric_maximum_density != 1.0f) && unknown_0071d1c4 == 0) {
            set_rs(rasterizer_device, 0xf, 0);
            goto stream_source;
        }
        unknown_0071d1c4 = 1;
        set_rs(rasterizer_device, 0xf, 1);
        set_rs(rasterizer_device, 0x18, 0);
    }

stream_source:
    vtable = *(void ***)rasterizer_device;
    ((d3d_set_stream_source_fn)vtable[400 / 4])(rasterizer_device, 0, rasterizer_decal_vertex_cache, 0, 0x10);
}

#if 0
Original Ghidra decompilation (0x51a810):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0051a810(void)

{
  int iVar1;
  bool bVar2;
  short unaff_DI;

  bVar2 = true;
  if ((DAT_006893f5 == '\0') && (unaff_DI != 3)) {
    bVar2 = false;
  }
  if (DAT_006893e4 != 0) {
    DAT_006d98dc = unaff_DI;
    return;
  }
  if (!bVar2) {
    DAT_006d98dc = unaff_DI;
    return;
  }
  DAT_006d98d8 = 0xffff;
  DAT_006d98e4 = 0xffff;
  DAT_006d98e0 = 0xffffffff;
  DAT_0071d1c4 = '\0';
  DAT_006d98dc = unaff_DI;
  if ((((*(uint *)(DAT_0071d164 + 0xb8) != 0xffffffff) &&
       (iVar1 = *(int *)((*(uint *)(DAT_0071d164 + 0xb8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
       iVar1 != 0)) && (1 < *(int *)(iVar1 + 0x60))) &&
     (iVar1 = *(int *)(iVar1 + 100), iVar1 != -0x30)) {
    FUN_00518680(0);
    _DAT_006d986c = *(undefined2 *)(iVar1 + 0x34);
    _DAT_006d986e = *(undefined2 *)(iVar1 + 0x36);
  }
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,3);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,3);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,4);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  FUN_005194e0();
  if (unaff_DI == 3) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0x7f);
    rasterizer_set_shader_stage_config();
  }
  else {
    if ((DAT_00689441 == '\0') || (DAT_007c1418 != 1.0)) {
      if (DAT_0071d1c4 == '\0') {
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
        goto LAB_0051aa26;
      }
    }
    else {
      DAT_0071d1c4 = '\x01';
    }
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0);
  }
LAB_0051aa26:
  (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,DAT_0071d1bc,0,0x10);
  return;
}
#endif
