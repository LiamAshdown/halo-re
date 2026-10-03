// rasterizer_screen_flash_init_shaders  (Ghidra: rasterizer_screen_flash_init_shaders, already named)
// address 0x52ec40, size 181 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: cea-pdb match on the string set {"FlashLighten","FlashDarken","FlashMax","FlashMin",
// "FlashInvert","FlashTint"}; the six DAT_ globals it writes are the elements of
// screen_flash_techniques[6] at 0x0071d23c (types/rasterizer.h global notes).
// register convention: none, __cdecl, no arguments; passes rasterizer_screen_flash_effect to
//   rasterizer_shader_technique_for_name's EDI parameter (its only other caller with a knowable
//   effect object is rasterizer_screen_flash_render.c, which reads the same global).

#include "tags.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *screen_flash_techniques[6]; // 0x0071d23c, FlashLighten .. FlashTint
extern void *rasterizer_screen_flash_effect; // 0x0069e270

// blam-cc: EDI -> effect, stack -> name
extern void *rasterizer_shader_technique_for_name(void *effect, const char *name); // 0x530120

// Looks up and caches the effect-technique handle for each of the six full-screen flash blend
// modes (Lighten, Darken, Max, Min, Invert, Tint). Stops at the first failure, so a lookup
// failure partway through leaves the later slots unset from a previous call, if any.
int rasterizer_screen_flash_init_shaders(void)
{
    screen_flash_techniques[0] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashLighten");
    if (screen_flash_techniques[0] == 0) {
        return 0;
    }
    screen_flash_techniques[1] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashDarken");
    if (screen_flash_techniques[1] == 0) {
        return 0;
    }
    screen_flash_techniques[2] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashMax");
    if (screen_flash_techniques[2] == 0) {
        return 0;
    }
    screen_flash_techniques[3] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashMin");
    if (screen_flash_techniques[3] == 0) {
        return 0;
    }
    screen_flash_techniques[4] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashInvert");
    if (screen_flash_techniques[4] == 0) {
        return 0;
    }
    screen_flash_techniques[5] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashTint");
    if (screen_flash_techniques[5] == 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x52ec40):

int __cdecl rasterizer_screen_flash_init_shaders(void)

{
  DAT_0071d23c = rasterizer_shader_technique_for_name("FlashLighten");
  if (DAT_0071d23c != 0) {
    DAT_0071d240 = rasterizer_shader_technique_for_name("FlashDarken");
    if (DAT_0071d240 != 0) {
      DAT_0071d244 = rasterizer_shader_technique_for_name("FlashMax");
      if (DAT_0071d244 != 0) {
        DAT_0071d248 = rasterizer_shader_technique_for_name("FlashMin");
        if (DAT_0071d248 != 0) {
          DAT_0071d24c = rasterizer_shader_technique_for_name("FlashInvert");
          if (DAT_0071d24c != 0) {
            DAT_0071d250 = rasterizer_shader_technique_for_name("FlashTint");
            if (DAT_0071d250 != 0) {
              return CONCAT31((int3)((uint)DAT_0071d250 >> 8),1);
            }
          }
        }
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
