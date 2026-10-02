// chimera__cinematic_screen_effect  (Ghidra: chimera__cinematic_screen_effect, already named --
// Chimera name, hint only)
// address 0x517470, size 139 bytes
// name confidence: 0.45  rewrite confidence: 0.45
// evidence: writes rasterizer_default_z_near (0x0069c65c, types/rasterizer.h) from the
//   cinematic screen effect block's +0x74 near_clip_distance field when positive (same block
//   decal_and_font_system_reset.c resets), copies four dwords into rasterizer_time
//   (0x007c1200), latches a pixel-shader-version flag, and drives the lens flare visibility
//   smoothing pass (lens_flare_update_visibility, Ghidra: decal_shadow_value_update) plus two
//   frame counters outside this module.
// register convention: source time block in in_ECX. // blam-cc: ECX -> time_source
// UNSURE: DAT_0071d275/DAT_0071d276/DAT_006ac540/DAT_0071d1c0/DAT_00689421-adjacent toggle
//   (0x006893f5) are not documented in types/rasterizer.h.
// reconciled: R11 rasterizer_letterbox_height -> rasterizer_default_z_near; R80 0x0071cfc4 uint32_t* cinematic_globals -> render.h cinematic_screen_effect_globals *cinematic_screen_effect_state (+0x74 near_clip_distance)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern cinematic_screen_effect_globals *cinematic_screen_effect_state; // 0x0071cfc4, render.h (0x78 bytes)
extern float rasterizer_default_z_near; // 0x0069c65c, rasterizer.h
extern rasterizer_frame_time rasterizer_time; // 0x007c1200
extern d3d_caps9 rasterizer_caps; // 0x007c10c0
extern uint8_t unknown_0071d275; // 0x0071d275 UNSURE
extern uint8_t unknown_0071d276;                                    // 0x0071d276 UNSURE
extern uint8_t *texture_cache; // 0x006ac540 UNSURE: some HUD/cinematic object, +0x30 frame counter
extern uint8_t decals_for_all_responses;                         // 0x006893f5
extern uint8_t *rasterizer_decal_vertex_cache_handle;               // 0x0071d1c0 UNSURE: +0x2c shift, +0x3c data_array

extern void lens_flare_update_visibility(void); // 0x513780 (this session)
extern void decals_update_fade(void); // 0x44e2b0

// blam-cc: ECX -> time_source
// Per-frame update of the default near clip distance (cinematic override), latches the current frame time into
// rasterizer_time, and drives the lens flare visibility smoothing pass plus two external frame
// counters (the second only when a debug toggle is set).
void chimera__cinematic_screen_effect(rasterizer_frame_time *time_source)
{
    rasterizer_default_z_near = 0.0625f; // 0x3d800000

    if (cinematic_screen_effect_state != (cinematic_screen_effect_globals *)0) {
        float near_clip = cinematic_screen_effect_state->near_clip_distance; // +0x74
        if (near_clip > 0.0f) {
            rasterizer_default_z_near = near_clip;
        }
    }

    rasterizer_time = *time_source;

    unknown_0071d275 = (uint8_t)(1 - (rasterizer_caps.pixel_shader_version < 0xffff0101));
    unknown_0071d276 = 0;

    lens_flare_update_visibility();

    *(int32_t *)(texture_cache + 0x30) = *(int32_t *)(texture_cache + 0x30) + 1;
    if (decals_for_all_responses != 0) {
        *(int32_t *)(rasterizer_decal_vertex_cache_handle + 0x30) = *(int32_t *)(rasterizer_decal_vertex_cache_handle + 0x30) + 1;
        decals_update_fade();
    }
}

#if 0
Original Ghidra decompilation (0x517470):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chimera__cinematic_screen_effect(void)

{
  undefined4 *in_ECX;

  DAT_0069c65c = 0x3d800000;
  if ((DAT_0071cfc4 != 0) && (DAT_0069c65c = 0x3d800000, 0.0 < *(float *)(DAT_0071cfc4 + 0x74))) {
    DAT_0069c65c = *(undefined4 *)(DAT_0071cfc4 + 0x74);
  }
  _DAT_007c1200 = *in_ECX;
  _DAT_007c1204 = in_ECX[1];
  _DAT_007c1208 = in_ECX[2];
  _DAT_007c120c = in_ECX[3];
  DAT_0071d275 = '\x01' - (DAT_007c118c < 0xffff0101);
  DAT_0071d276 = 0;
  decal_shadow_value_update();
  *(int *)(DAT_006ac540 + 0x30) = *(int *)(DAT_006ac540 + 0x30) + 1;
  if (DAT_006893f5 != '\0') {
    *(int *)(DAT_0071d1c0 + 0x30) = *(int *)(DAT_0071d1c0 + 0x30) + 1;
    chimera__decal_table();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
