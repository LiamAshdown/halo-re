// rasterizer_shader_environment_set_lightmap  (Ghidra: FUN_00520910; the phase 4 rewriter called it
// rasterizer_shader_environment_technique_enable)
// address 0x520910, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: out/phase2/results/rasterizer_01.json ("Enables or disables the shader_environment
// technique selected by 0x520790 based on a caller-supplied flag."). No register ambiguity: the
// sole parameter arrives in EAX.
// register convention: lightmap BitmapData* in EAX.

// Spot-check fix (phase 4 review, raw code 0x520910..0x52096a): EAX is the lightmap BitmapData,
//   bound as stage 0 of the active environment effect (ESI bitmap, EDI effect slot) through
//   rasterizer_bind_texture_d3dx; 0x006e0a0c is cleared when a lightmap was bound and set when
//   EAX is NULL, and rasterizer_shader_environment_technique_draw 0x520970 skips while it is set.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f8; // 0x006893f8
extern uint8_t console_debug_toggle_6893fa; // 0x006893fa
extern int16_t render_force_flag;                             // 0x0069c67c UNSURE (read as a word)
extern rasterizer_effect_slot *rasterizer_active_environment_effect; // 0x0071d1d0, &rasterizer_effects[36] or NULL
extern uint8_t rasterizer_environment_lightmap_missing;           // 0x006e0a0c 1 when no lightmap was bound

// blam-cc: ESI -> bitmap, EDI -> effect_slot, stack -> stage
extern uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot); // 0x5186c0

// blam-cc: EAX = lightmap
void rasterizer_shader_environment_set_lightmap(BitmapData *lightmap)
{
    if (console_debug_toggle_6893e4 == 0 && console_debug_toggle_6893f8 != 0 &&
        console_debug_toggle_6893fa != 0 && render_force_flag == 0 &&
        rasterizer_active_environment_effect != 0 &&
        rasterizer_active_environment_effect->effect != 0) {
        if (lightmap != 0) {
            rasterizer_bind_texture_d3dx(0, lightmap, rasterizer_active_environment_effect);
            rasterizer_environment_lightmap_missing = 0;
            return;
        }
        rasterizer_environment_lightmap_missing = 1;
    }
}

#if 0
Original Ghidra decompilation (0x520910):

void FUN_00520910(void)

{
  int in_EAX;

  if ((((DAT_006893e4 == 0) && (DAT_006893f8 != '\0')) && (DAT_006893fa != '\0')) &&
     (((DAT_0069c67c == 0 && (DAT_0071d1d0 != (int *)0x0)) && (*DAT_0071d1d0 != 0)))) {
    if (in_EAX != 0) {
      FUN_005186c0(0);
      DAT_006e0a0c = 0;
      return;
    }
    DAT_006e0a0c = 1;
  }
  return;
}
#endif
