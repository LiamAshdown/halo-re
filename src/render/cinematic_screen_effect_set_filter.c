// cinematic_screen_effect_set_filter  (Ghidra: FUN_00512230; named from
// out/phase4/render_types_notes.md: "0x481220 -> 0x512230 set_filter")
// address 0x512230, size 97 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/render.h cinematic_screen_effect_globals field offsets match exactly:
//   filter_light_enhancement_intensity_lower/upper_bound +0x4c/+0x50, filter_desaturation_
//   intensity_lower/upper_bound +0x54/+0x58, filter_desaturation_is_additive +0x20, unknown_21/
//   unknown_22 cleared, filter_start_time +0x5c (game ticks / 30), filter_end_time +0x60
//   (start + duration); also clears video_enabled/video_overbright_mode/video_scanline_map/
//   video_noise_intensity/unknown_30/video_noise_map (+0x23,+0x24,+0x28..+0x34), matching
//   cinematic_screen_effect_set_convolution 0x5121d0.
// register convention: all six arguments are on the stack (objdump: no incoming register is
//   read before being freshly loaded).
//   // blam-cc: stack -> light_lower, light_upper, desat_lower, desat_upper, is_additive, duration

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "rasterizer.h"
#include "render.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern cinematic_screen_effect_globals *cinematic_screen_effect_state; // 0x0071cfc4 (named _state;
                                             // a variable cannot share the typedef's own name in C)
extern game_time_globals *game_time; // 0x006f1d6c

// Starts (or restarts) the light-enhancement/desaturation filter screen effect: sets the four
// intensity bounds, the additive-blend flag, clears the unrelated video fields, and sets the
// [start, start+duration] time window the per-frame update interpolates over.
void cinematic_screen_effect_set_filter(float light_enhancement_lower, float light_enhancement_upper,
                                         float desaturation_lower, float desaturation_upper,
                                         uint8_t is_additive, float duration)
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    float start_time;

    if (g == 0) {
        return;
    }

    g->filter_light_enhancement_intensity_lower_bound = light_enhancement_lower;
    g->filter_light_enhancement_intensity_upper_bound = light_enhancement_upper;
    g->filter_desaturation_intensity_lower_bound = desaturation_lower;
    g->filter_desaturation_intensity_upper_bound = desaturation_upper;

    g->video_enabled = 0;
    g->video_overbright_mode = 0;
    g->video_scanline_map = 0;
    g->video_noise_intensity = 0.0f;
    g->unknown_30 = 0.0f;
    g->video_noise_map = 0;

    g->filter_desaturation_is_additive = is_additive;

    start_time = (float)game_time->game_time * 0.033333335f;
    g->night_vision_masked = 0;
    g->desaturation_masked = 0;
    g->filter_start_time = start_time;
    g->filter_end_time = start_time + duration;
}

#if 0
Original Ghidra decompilation (0x512230):

void FUN_00512230(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined1 param_5,float param_6)

{
  int iVar1;
  float fVar2;
  int iVar3;

  iVar3 = DAT_0071cfc4;
  if (DAT_0071cfc4 != 0) {
    *(undefined4 *)(DAT_0071cfc4 + 0x4c) = param_1;
    *(undefined4 *)(iVar3 + 0x50) = param_2;
    *(undefined4 *)(iVar3 + 0x54) = param_3;
    *(undefined4 *)(iVar3 + 0x58) = param_4;
    iVar1 = DAT_006f1d6c;
    *(undefined1 *)(iVar3 + 0x23) = 0;
    *(undefined2 *)(iVar3 + 0x24) = 0;
    *(undefined4 *)(iVar3 + 0x28) = 0;
    *(undefined4 *)(iVar3 + 0x2c) = 0;
    *(undefined4 *)(iVar3 + 0x30) = 0;
    *(undefined4 *)(iVar3 + 0x34) = 0;
    iVar1 = *(int *)(iVar1 + 0xc);
    *(undefined1 *)(iVar3 + 0x20) = param_5;
    fVar2 = (float)iVar1 * 0.033333335;
    *(undefined1 *)(iVar3 + 0x21) = 0;
    *(undefined1 *)(iVar3 + 0x22) = 0;
    *(float *)(iVar3 + 0x5c) = fVar2;
    *(float *)(iVar3 + 0x60) = fVar2 + param_6;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
