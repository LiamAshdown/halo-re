// rasterizer_underwater_tint_jitter_update  (Ghidra: FUN_0051f310)
// address 0x51f310, size 200 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase2/results/rasterizer_01.json ("Generates a per-frame pseudo-random (or
// constant) RGB jitter used to animate the underwater screen-tint noise."). No callers found by
// phase-2 analysis. The RNG is the standard Numerical-Recipes-style LCG
// (a = 0x19660d, c = 0x3c6ef35f), used identically to reseed itself between the three channels;
// each channel takes the high 16 bits of the next state and scales by 1/65536
// (1.5259022e-05 == 2^-16).
// register convention: EAX = the environment lightmap BitmapData* of the geometry about to be
//   drawn (stored into 0x006e0a08 for rasterizer_shader_environment_self_illumination_draw) and
//   reused as the LCG seed (phase 4 review: the store was previously read as a plain seed).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t console_debug_toggle_6893f1; // 0x006893f1, gates this whole function
extern int16_t render_force_flag;                             // 0x0069c67c UNSURE (read as a word)
extern BitmapData *rasterizer_environment_lightmap;                 // 0x006e0a08 set from EAX by 0x51f310
extern float renderer_unknown_68940c;       // 0x0068940c, UNSURE meaning; constant-mode fill value
extern float rasterizer_underwater_tint_jitter_r; // 0x006e09f8
extern float rasterizer_underwater_tint_jitter_g; // 0x006e09fc
extern float rasterizer_underwater_tint_jitter_b; // 0x006e0a00

// blam-cc: EAX = lightmap
void rasterizer_underwater_tint_jitter_update(BitmapData *lightmap)
{
    uint32_t state;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    rasterizer_environment_lightmap = lightmap;
    if (render_force_flag <= 0) {
        return;
    }

    if (render_force_flag == 2) {
        rasterizer_underwater_tint_jitter_b = renderer_unknown_68940c;
        rasterizer_underwater_tint_jitter_g = renderer_unknown_68940c;
        rasterizer_underwater_tint_jitter_r = renderer_unknown_68940c;
        return;
    }

    state = (uint32_t)lightmap * 0x19660d + 0x3c6ef35f;     // the pointer value seeds the LCG
    rasterizer_underwater_tint_jitter_r = (float)(state >> 0x10) * 1.5259022e-05f;
    state = state * 0x19660d + 0x3c6ef35f;
    rasterizer_underwater_tint_jitter_g = (float)(state >> 0x10) * 1.5259022e-05f;
    state = state * 0x19660d + 0x3c6ef35f;
    rasterizer_underwater_tint_jitter_b = (float)(state >> 0x10) * 1.5259022e-05f;
}

#if 0
Original Ghidra decompilation (0x51f310):

/* WARNING: Removing unreachable block (ram,0x0051f39a) */
/* WARNING: Removing unreachable block (ram,0x0051f36e) */
/* WARNING: Removing unreachable block (ram,0x0051f3c4) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0051f310(void)

{
  int in_EAX;
  uint uVar1;

  if ((DAT_006893f1 != '\0') && (DAT_006e0a08 = in_EAX, 0 < DAT_0069c67c)) {
    if (DAT_0069c67c == 2) {
      _DAT_006e0a00 = (float)_DAT_0068940c;
      _DAT_006e09fc = (float)_DAT_0068940c;
      _DAT_006e09f8 = (float)_DAT_0068940c;
      return;
    }
    uVar1 = in_EAX * 0x19660d + 0x3c6ef35f;
    _DAT_006e09f8 = (float)(uVar1 >> 0x10) * 1.5259022e-05;
    uVar1 = uVar1 * 0x19660d + 0x3c6ef35f;
    _DAT_006e09fc = (float)(uVar1 >> 0x10) * 1.5259022e-05;
    _DAT_006e0a00 = (float)(uVar1 * 0x19660d + 0x3c6ef35f >> 0x10) * 1.5259022e-05;
  }
  return;
}
#endif
