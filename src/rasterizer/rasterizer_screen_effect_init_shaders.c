// rasterizer_screen_effect_init_shaders  (Ghidra: already named)
// address 0x52d740, size 341 bytes
// name confidence: 0.85   rewrite confidence: 0.9
// evidence: fills screen_effect_techniques[11] (0x0071d210, types/rasterizer.h) one entry per
//   named technique of the 'video' (old-TV/convolution) screen effect shader, via
//   rasterizer_shader_technique_for_name (0x530120, declared in this module elsewhere), stopping
//   and returning failure the first time a name fails to resolve.
// Phase 4 review fix: every lookup runs against effect 114 (EDI = 0x0069e250), which the earlier
//   file dropped; the result is a byte (mov al, 1).
// register convention: none -- no parameters, __cdecl.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern uint32_t screen_effect_techniques[k_rasterizer_screen_effect_techniques]; // 0x0071d210

extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
// blam-cc: EDI -> effect, stack -> name
extern void *rasterizer_shader_technique_for_name(void *effect, const char *name); // 0x530120

static const char *const k_video_technique_names[k_rasterizer_screen_effect_techniques] = {
    "VideoOn",
    "VideoOffNonConvolved",
    "VideoOffConvolvedMask",
    "VideoOffConvolvedMaskThreeStage",
    "VideoOffConvolvedMaskFilterLightAndDesaturation",
    "VideoOffConvolvedMaskFilterLight",
    "VideoOffConvolvedMaskFilterDesaturation",
    "VideoOffConvolved",
    "VideoOffConvolvedFilterLightAndDesaturation",
    "VideoOffConvolvedFilterLight",
    "VideoOffConvolvedFilterDesaturation",
};

// Looks up and caches the effect-technique handles for all variants of the 'video' (old-TV/
// convolution) screen effect shader, returning success only if every technique was found.
uint8_t rasterizer_screen_effect_init_shaders(void)
{
    int i;

    for (i = 0; i < k_rasterizer_screen_effect_techniques; i++) {
        screen_effect_techniques[i] = (int32_t)(uintptr_t)rasterizer_shader_technique_for_name(
            (void *)(uintptr_t)rasterizer_effects[114].effect, k_video_technique_names[i]);   // EDI = effect 114
        if (screen_effect_techniques[i] == 0) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x52d740):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl rasterizer_screen_effect_init_shaders(void)

{
  DAT_0071d210 = rasterizer_shader_technique_for_name("VideoOn");
  if (DAT_0071d210 != 0) {
    _DAT_0071d214 = rasterizer_shader_technique_for_name("VideoOffNonConvolved");
    if (_DAT_0071d214 != 0) {
      DAT_0071d218 = rasterizer_shader_technique_for_name("VideoOffConvolvedMask");
      if (DAT_0071d218 != 0) {
        DAT_0071d21c = rasterizer_shader_technique_for_name("VideoOffConvolvedMaskThreeStage");
        if (DAT_0071d21c != 0) {
          DAT_0071d220 = rasterizer_shader_technique_for_name
                                   ("VideoOffConvolvedMaskFilterLightAndDesaturation");
          if (DAT_0071d220 != 0) {
            DAT_0071d224 = rasterizer_shader_technique_for_name("VideoOffConvolvedMaskFilterLight");
            if (DAT_0071d224 != 0) {
              DAT_0071d228 = rasterizer_shader_technique_for_name
                                       ("VideoOffConvolvedMaskFilterDesaturation");
              if (DAT_0071d228 != 0) {
                DAT_0071d22c = rasterizer_shader_technique_for_name("VideoOffConvolved");
                if (DAT_0071d22c != 0) {
                  DAT_0071d230 = rasterizer_shader_technique_for_name
                                           ("VideoOffConvolvedFilterLightAndDesaturation");
                  if (DAT_0071d230 != 0) {
                    DAT_0071d234 = rasterizer_shader_technique_for_name
                                             ("VideoOffConvolvedFilterLight");
                    if (DAT_0071d234 != 0) {
                      DAT_0071d238 = rasterizer_shader_technique_for_name
                                               ("VideoOffConvolvedFilterDesaturation");
                      if (DAT_0071d238 != 0) {
                        return CONCAT31((int3)((uint)DAT_0071d238 >> 8),1);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}
#endif
