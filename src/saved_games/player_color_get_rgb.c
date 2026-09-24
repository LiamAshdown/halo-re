// player_color_get_rgb  (Ghidra: player_color_get_rgb, already named)
// address 0x539c20, size 132 bytes
// name confidence: 0.65   rewrite confidence: 0.9
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md
// "player_color_table[18]" global and "18 player colors (0x539c20 clamp)" enum note. The clamp
// expression `(((int)index < 0) - 1 & index)` is a branchless clamp-to-0-if-negative; combined
// with the earlier `if (index > 16) index = 17;` it clamps to 0..17
// (k_player_color_count - 1). 0x00672ad4 (listed but unused directly) is presumably the source
// of the 1/255 float literal.
// Phase 4 review: matched objdump 0x539c20..0x539ca3; 0x00672ad4 is 0x3b808081 (1/255).
// register convention: out_rgb in EAX, color_index in ECX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint32_t player_color_table[k_player_color_count]; // 0x0069e7f0

// blam-cc: out_rgb in EAX, color_index in ECX
// Looks up a player color index (clamped to 0..k_player_color_count-1) in player_color_table
// and converts the packed 0x00RRGGBB entry to a normalized (0..1) float RGB triple.
// EAX (out_rgb) is never overwritten (mov ecx,eax at 0x539c77), so callers such as
// src/game/game_engine_get_player_color.c use it as the return value.
float *player_color_get_rgb(float *out_rgb, int32_t color_index)
{
    uint32_t packed;

    if (color_index > 0x10) {
        color_index = 0x11;
    }
    if (color_index < 0) {
        color_index = 0;
    }
    packed = player_color_table[color_index];

    out_rgb[0] = (float)((packed >> 0x10) & 0xff) * 0.003921569f;
    out_rgb[1] = (float)((packed >> 8) & 0xff) * 0.003921569f;
    out_rgb[2] = (float)(packed & 0xff) * 0.003921569f; // 0x3b808081, 1/255
    return out_rgb;
}

#if 0
Original Ghidra decompilation (0x539c20):

void player_color_get_rgb(void)

{
  uint uVar1;
  float *in_EAX;
  uint in_ECX;

  if (0x10 < (int)in_ECX) {
    in_ECX = 0x11;
  }
  uVar1 = *(uint *)(&DAT_0069e7f0 + (((int)in_ECX < 0) - 1 & in_ECX) * 4);
  *in_EAX = (float)((int)uVar1 >> 0x10 & 0xff) * 0.003921569;
  in_EAX[1] = (float)((int)uVar1 >> 8 & 0xff) * 0.003921569;
  in_EAX[2] = (float)(uVar1 & 0xff) * 0.003921569;
  return;
}
#endif
