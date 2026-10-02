// cinematic_screen_effect_set_video  (Ghidra: FUN_005122a0; named from
// out/phase4/render_types_notes.md: "0x4812f0 set_video -> 0x5122a0")
// address 0x5122a0, size 177 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/render.h cinematic_screen_effect_globals field offsets match exactly:
//   clears the 0xe leading dwords (+0x00..+0x37) and +0x3c..+0x60, sets video_overbright_mode
//   +0x24, video_enabled +0x23 = 1, video_scanline_map +0x28 and video_noise_map +0x34 from the
//   BitmapData (tag data + 0x64) of GlobalsRasterizerData's video_scanline_map/video_noise_map
//   tags (+0x128/+0x138, matching types/tags.h's TagDependency layout), video_noise_intensity
//   +0x2c and unknown_30 +0x30 = 1.0. The whole function is a no-op when either tag id is -1.
// register convention: both arguments are on the stack.
//   // blam-cc: stack -> overbright_mode, noise_intensity

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "render.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern cinematic_screen_effect_globals *cinematic_screen_effect_state; // 0x0071cfc4 (named _state;
                                             // a variable cannot share the typedef's own name in C)
extern GlobalsRasterizerData *rasterizer_globals_data;                  // 0x0071d164
extern tag_instance *tag_instances;                                     // 0x0087bc14

// Starts (or restarts) the video screen effect (overbright mode plus scanline/noise maps),
// provided both bitmap tags are actually set; otherwise leaves the block untouched.
void cinematic_screen_effect_set_video(int16_t overbright_mode, float noise_intensity)
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    tag_instance *scanline_tag;
    tag_instance *noise_tag;
    uint32_t *block;
    int i;

    if (g == 0) {
        return;
    }
    if (*(int32_t *)&rasterizer_globals_data->video_scanline_map.tag_id == -1) {
        return;
    }
    if (*(int32_t *)&rasterizer_globals_data->video_noise_map.tag_id == -1) {
        return;
    }

    block = (uint32_t *)g;
    for (i = 0; i < 0xe; i++) {
        block[i] = 0;
    }

    g->video_overbright_mode = overbright_mode;

    g->convolution_radius_lower_bound = 0.0f;
    g->convolution_radius_upper_bound = 0.0f;
    g->convolution_start_time = 0.0f;
    g->convolution_end_time = 0.0f;
    g->filter_light_enhancement_intensity_lower_bound = 0.0f;
    g->filter_light_enhancement_intensity_upper_bound = 0.0f;
    g->filter_desaturation_intensity_lower_bound = 0.0f;
    g->filter_desaturation_intensity_upper_bound = 0.0f;
    g->filter_start_time = 0.0f;
    g->filter_end_time = 0.0f;

    g->video_enabled = 1;

    scanline_tag = &tag_instances[(uint16_t)*(int32_t *)&rasterizer_globals_data->video_scanline_map.tag_id];
    g->video_scanline_map = *(uint32_t *)((uint8_t *)scanline_tag->data + 0x64);

    g->video_noise_intensity = noise_intensity;
    g->unknown_30 = 1.0f;

    noise_tag = &tag_instances[(uint16_t)*(int32_t *)&rasterizer_globals_data->video_noise_map.tag_id];
    g->video_noise_map = *(uint32_t *)((uint8_t *)noise_tag->data + 0x64);
}

#if 0
Original Ghidra decompilation (0x5122a0):

void FUN_005122a0(undefined2 param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;

  iVar2 = DAT_0071d164;
  puVar1 = DAT_0071cfc4;
  if (((DAT_0071cfc4 != (undefined4 *)0x0) && (*(int *)(DAT_0071d164 + 0x128) != -1)) &&
     (*(int *)(DAT_0071d164 + 0x138) != -1)) {
    puVar4 = DAT_0071cfc4;
    for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *(undefined2 *)(puVar1 + 9) = param_1;
    iVar3 = DAT_0087bc14;
    puVar1[0xf] = 0;
    puVar1[0x10] = 0;
    puVar1[0x11] = 0;
    puVar1[0x12] = 0;
    puVar1[0x13] = 0;
    puVar1[0x14] = 0;
    puVar1[0x15] = 0;
    puVar1[0x16] = 0;
    puVar1[0x17] = 0;
    puVar1[0x18] = 0;
    *(undefined1 *)((int)puVar1 + 0x23) = 1;
    puVar1[10] = *(undefined4 *)
                  (*(int *)((*(uint *)(iVar2 + 0x128) & 0xffff) * 0x20 + 0x14 + iVar3) + 100);
    puVar1[0xb] = param_2;
    puVar1[0xc] = 0x3f800000;
    puVar1[0xd] = *(undefined4 *)
                   (*(int *)((*(uint *)(iVar2 + 0x138) & 0xffff) * 0x20 + 0x14 + iVar3) + 100);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
