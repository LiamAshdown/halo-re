// cinematic_screen_effect_set_convolution  (Ghidra: FUN_005121d0; named from
// out/phase4/render_types_notes.md: "0x4811c0 -> 0x5121d0 set_convolution")
// address 0x5121d0, size 82 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/render.h cinematic_screen_effect_globals field offsets match exactly:
//   convolution_type +0x02 (EDX), convolution_extra_passes +0x00, convolution_radius_lower_bound
//   +0x3c, convolution_radius_upper_bound +0x40, convolution_start_time +0x44 (game ticks / 30),
//   convolution_end_time +0x48 (start + duration); also clears video_enabled +0x23,
//   video_overbright_mode +0x24 and video_scanline_map/video_noise_intensity/unknown_30/
//   video_noise_map (+0x28..+0x34).
// register convention: EDX = convolution_type, stack = (extra_passes, radius_lower_bound,
//   radius_upper_bound, duration), confirmed by objdump (all four stack slots read directly with
//   no incoming EAX/ECX use).
//   // blam-cc: EDX -> convolution_type, stack -> extra_passes/radius_lower_bound/radius_upper_bound/duration

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
extern game_time_globals *game_time; // 0x006f1d6c (matches src/ai/actor_begin_vocalization.c)

// Starts (or restarts) the convolution screen effect: clears the unrelated video fields, then
// sets the extra-passes count, type, radius bounds and the [start, start+duration] time window
// the per-frame update (cinematic_screen_effect_update 0x512360) will interpolate over.
void cinematic_screen_effect_set_convolution(int16_t convolution_type, int16_t extra_passes,
                                              float radius_lower_bound, float radius_upper_bound,
                                              float duration) // blam-cc: EDX=convolution_type, stack=rest
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    float start_time;

    if (g == 0) {
        return;
    }

    g->video_enabled = 0;
    g->video_overbright_mode = 0;
    g->video_scanline_map = 0;
    g->video_noise_intensity = 0.0f;
    g->unknown_30 = 0.0f;
    g->video_noise_map = 0;

    g->convolution_extra_passes = extra_passes;
    g->convolution_type = convolution_type;
    g->convolution_radius_lower_bound = radius_lower_bound;
    g->convolution_radius_upper_bound = radius_upper_bound;

    start_time = (float)game_time->game_time * 0.033333335f;
    g->convolution_start_time = start_time;
    g->convolution_end_time = start_time + duration;
}

#if 0
Original Ghidra decompilation (0x5121d0):

void FUN_005121d0(undefined2 param_1,undefined4 param_2,undefined4 param_3,float param_4)

{
  float fVar1;
  int iVar2;
  undefined2 *puVar3;
  undefined2 in_DX;

  puVar3 = DAT_0071cfc4;
  if (DAT_0071cfc4 != (undefined2 *)0x0) {
    *(undefined1 *)((int)DAT_0071cfc4 + 0x23) = 0;
    puVar3[0x12] = 0;
    *(undefined4 *)(puVar3 + 0x14) = 0;
    *(undefined4 *)(puVar3 + 0x16) = 0;
    *(undefined4 *)(puVar3 + 0x18) = 0;
    *(undefined4 *)(puVar3 + 0x1a) = 0;
    *puVar3 = param_1;
    puVar3[1] = in_DX;
    *(undefined4 *)(puVar3 + 0x1e) = param_2;
    iVar2 = DAT_006f1d6c;
    *(undefined4 *)(puVar3 + 0x20) = param_3;
    fVar1 = (float)*(int *)(iVar2 + 0xc) * 0.033333335;
    *(float *)(puVar3 + 0x22) = fVar1;
    *(float *)(puVar3 + 0x24) = fVar1 + param_4;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
