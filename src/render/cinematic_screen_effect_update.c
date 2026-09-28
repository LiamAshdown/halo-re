// cinematic_screen_effect_update  (Ghidra: screen_effect_update, already named; renamed to match
// the module's own convention of prefixing this record's accessors with cinematic_screen_effect_,
// see types/render.h's comment on cinematic_screen_effect_globals; CEA
// rasterizer_screen_effect_get_cinematic_parameters, hint only)
// address 0x512360, size 464 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x512360..0x51252f, re-traced in the phase-4 review.
//   types/render.h cinematic_screen_effect_globals field offsets: active +0x38 (early out),
//   convolution_start_time/end_time +0x44/+0x48 (progress, 1.0 when equal, else clamped to
//   [0, 1]; game ticks * 1/30 from 0x672acc), convolution_radius +0x04 = (1 - p) * lower +0x3c +
//   p * upper +0x40, filter_start_time/end_time +0x5c/+0x60 (a second progress),
//   filter_light_enhancement_intensity +0x0c and filter_desaturation_intensity +0x10 through
//   real_lerp_clamped 0x4cd900 (ECX out, stack (lower, upper, t)) against +0x4c/+0x50 and
//   +0x54/+0x58, filter_desaturation_tint +0x14 replaced by *global_real_rgb_green_pointer while it
//   equals *default_axis_b (a dword compare, repz cmpsd), and the two clears (radius
//   <= 0.0001; both intensities <= 0.0001 once the filter progress reached 1.0).
//   - EAX in and out: both callers (rasterizer_screen_effect_render 0x52d8a0 and the fixed
//     function variant 0x52e2d0) load EAX with their own screen effect parameter block before
//     the call and use EAX afterwards; this function only replaces it (mov eax,edx at 0x51252b)
//     when a cinematic effect block exists and is active, so the caller's block passes through
//     otherwise. The caller block is types/interface.h weapon_screen_effect_parameters, whose
//     0x38 bytes share this record's layout (mask bitmap +0x08, the masked flags +0x21 / +0x22).
// review fix (phase-4 gate): the first draft returned nothing (the EAX pass-through is part of
//   the interface), read 0x00686b0c / 0x00686b14 as colours instead of pointers to the black
//   {0,0,0} (0x0065515c) and green {0,1,0} (0x0065517c) constants, and used < instead of <= in
//   the two 0.0001 clears.
// register convention: EAX = the caller's screen effect parameters, returned in EAX.
//   // blam-cc: EAX -> input, returns EAX
// UNSURE: none of the arithmetic; the common name of the two block layouts is open.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "rasterizer.h"
#include "render.h"

extern cinematic_screen_effect_globals *cinematic_screen_effect_state; // 0x0071cfc4 (named _state;
                                             // a variable cannot share the typedef's own name in C)
extern game_time_globals *game_time; // 0x006f1d6c
extern ColorRGB *default_axis_b; // 0x00686b0c -> {0,0,0} at 0x0065515c
extern ColorRGB *global_real_rgb_green_pointer; // 0x00686b14 -> {0,1,0} at 0x0065517c

extern void real_lerp_clamped(real *out, real a, real b, real t); // 0x4cd900, math module;
                                                                  // blam-cc: ECX -> out

static float progress(float start_time, float end_time)
{
    float t;

    if (end_time == start_time) {
        return 1.0f;
    }
    t = ((float)game_time->game_time * 0.033333335f - start_time) / (end_time - start_time);
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t > 1.0f) {
        return 1.0f;
    }
    return t;
}

// Recomputes the interpolated convolution radius and filter intensities / tint of the active
// cinematic screen effect from the elapsed game time and clears each part once it has decayed.
// Returns the cinematic block when one is active, else hands the caller's own block back.
cinematic_screen_effect_globals *cinematic_screen_effect_update(cinematic_screen_effect_globals *input)
    // blam-cc: EAX -> input, returns EAX
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;
    float convolution_progress;
    float filter_progress;
    uint32_t *tint;
    uint32_t *black;

    if (g == 0 || g->active == 0) {
        return input;
    }

    convolution_progress = progress(g->convolution_start_time, g->convolution_end_time);
    filter_progress = progress(g->filter_start_time, g->filter_end_time);

    g->convolution_radius = (1.0f - convolution_progress) * g->convolution_radius_lower_bound +
                            convolution_progress * g->convolution_radius_upper_bound;
    real_lerp_clamped(&g->filter_light_enhancement_intensity,
                      g->filter_light_enhancement_intensity_lower_bound,
                      g->filter_light_enhancement_intensity_upper_bound, filter_progress);
    real_lerp_clamped(&g->filter_desaturation_intensity,
                      g->filter_desaturation_intensity_lower_bound,
                      g->filter_desaturation_intensity_upper_bound, filter_progress);

    tint = (uint32_t *)&g->filter_desaturation_tint;
    black = (uint32_t *)default_axis_b;
    if (tint[0] == black[0] && tint[1] == black[1] && tint[2] == black[2]) {
        g->filter_desaturation_tint = *global_real_rgb_green_pointer;
    }

    if (g->convolution_radius <= 0.0001f) {
        g->convolution_radius = 0.0f;
        g->convolution_type = 0;
        g->convolution_extra_passes = 0;
    }
    if (g->filter_light_enhancement_intensity <= 0.0001f &&
        g->filter_desaturation_intensity <= 0.0001f && filter_progress >= 1.0f) {
        g->filter_light_enhancement_intensity = 0.0f;
        g->filter_desaturation_intensity = 0.0f;
    }
    return g;
}

#if 0
Original Ghidra decompilation (0x512360):

void __cdecl screen_effect_update(void)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  undefined *puVar4;
  int iVar5;
  int extraout_EDX;
  undefined2 *extraout_EDX_00;
  int *piVar6;
  int *piVar7;
  bool bVar8;
  float local_4;

  iVar5 = DAT_0071cfc4;
  if (DAT_0071cfc4 == 0) {
    return;
  }
  if (*(char *)(DAT_0071cfc4 + 0x38) == '\0') {
    return;
  }
  if (*(float *)(DAT_0071cfc4 + 0x48) == *(float *)(DAT_0071cfc4 + 0x44)) {
LAB_005123e6:
    fVar2 = 1.0;
  }
  else {
    fVar2 = (float)*(int *)(DAT_006f1d6c + 0xc) * 0.033333335;
    if (0.0 <= (fVar2 - *(float *)(DAT_0071cfc4 + 0x44)) /
               (*(float *)(DAT_0071cfc4 + 0x48) - *(float *)(DAT_0071cfc4 + 0x44))) {
      if (1.0 < (fVar2 - *(float *)(DAT_0071cfc4 + 0x44)) /
                (*(float *)(DAT_0071cfc4 + 0x48) - *(float *)(DAT_0071cfc4 + 0x44)))
      goto LAB_005123e6;
      fVar2 = (fVar2 - *(float *)(DAT_0071cfc4 + 0x44)) /
              (*(float *)(DAT_0071cfc4 + 0x48) - *(float *)(DAT_0071cfc4 + 0x44));
    }
    else {
      fVar2 = 0.0;
    }
  }
  if (*(float *)(DAT_0071cfc4 + 0x60) != *(float *)(DAT_0071cfc4 + 0x5c)) {
    fVar3 = (float)*(int *)(DAT_006f1d6c + 0xc) * 0.033333335;
    if ((fVar3 - *(float *)(DAT_0071cfc4 + 0x5c)) /
        (*(float *)(DAT_0071cfc4 + 0x60) - *(float *)(DAT_0071cfc4 + 0x5c)) < 0.0) {
      local_4 = 0.0;
      goto LAB_0051245d;
    }
    if ((fVar3 - *(float *)(DAT_0071cfc4 + 0x5c)) /
        (*(float *)(DAT_0071cfc4 + 0x60) - *(float *)(DAT_0071cfc4 + 0x5c)) <= 1.0) {
      local_4 = (fVar3 - *(float *)(DAT_0071cfc4 + 0x5c)) /
                (*(float *)(DAT_0071cfc4 + 0x60) - *(float *)(DAT_0071cfc4 + 0x5c));
      goto LAB_0051245d;
    }
  }
  local_4 = 1.0;
LAB_0051245d:
  pfVar1 = (float *)(DAT_0071cfc4 + 0xc);
  *(float *)(DAT_0071cfc4 + 4) =
       fVar2 * *(float *)(DAT_0071cfc4 + 0x40) + (1.0 - fVar2) * *(float *)(DAT_0071cfc4 + 0x3c);
  real_lerp_clamped(*(undefined4 *)(iVar5 + 0x4c),*(undefined4 *)(iVar5 + 0x50),local_4);
  real_lerp_clamped(*(undefined4 *)(extraout_EDX + 0x54),*(undefined4 *)(extraout_EDX + 0x58),
                    local_4);
  puVar4 = PTR_DAT_00686b14;
  iVar5 = 3;
  bVar8 = true;
  piVar6 = (int *)(extraout_EDX_00 + 10);
  piVar7 = (int *)PTR_DAT_00686b0c;
  do {
    if (iVar5 == 0) break;
    iVar5 = iVar5 + -1;
    bVar8 = *piVar6 == *piVar7;
    piVar6 = piVar6 + 1;
    piVar7 = piVar7 + 1;
  } while (bVar8);
  if (bVar8) {
    *(int *)(extraout_EDX_00 + 10) = *(int *)PTR_DAT_00686b14;
    *(undefined4 *)(extraout_EDX_00 + 0xc) = *(undefined4 *)(puVar4 + 4);
    *(undefined4 *)(extraout_EDX_00 + 0xe) = *(undefined4 *)(puVar4 + 8);
  }
  if (*(float *)(extraout_EDX_00 + 2) < 0.0001 != (*(float *)(extraout_EDX_00 + 2) == 0.0001)) {
    *(undefined4 *)(extraout_EDX_00 + 2) = 0;
    extraout_EDX_00[1] = 0;
    *extraout_EDX_00 = 0;
  }
  fVar2 = *pfVar1;
  if (((fVar2 < 0.0001 != (fVar2 == 0.0001)) &&
      (*(float *)(extraout_EDX_00 + 8) < 0.0001 != (*(float *)(extraout_EDX_00 + 8) == 0.0001))) &&
     (1.0 <= local_4)) {
    *pfVar1 = 0.0;
    *(undefined4 *)(extraout_EDX_00 + 8) = 0;
  }
  return;
}

Confirmed via objdump (0x512360..0x51252f) that the two real_lerp_clamped calls' ECX (out
pointer) arguments are &g->filter_light_enhancement_intensity (edx+0xc) and
&g->filter_desaturation_intensity (edx+0x10) respectively:

  512476: lea    ebp,0xc(%edx)
  512486: mov    ecx,ebp
  512488: call   0x4cd900
  512495: lea    ecx,0x10(%edx)
  512499: call   0x4cd900
#endif
