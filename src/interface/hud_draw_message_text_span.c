// hud_draw_message_text_span  (Ghidra: FUN_004ad8e0, renamed in the phase-4 review)
// address 0x4ad8e0, size 144 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: objdump 0x4ad8e0..0x4ad96f; the only caller is hud_messaging_update (0x4ae550). EAX
// is the running cursor rectangle, ECX the origin rectangle of the message line. The clip x
// at 0x006e476e is cursor.left - origin.left; text_measure_string_extents measures the span (EBX origin, ESI
// cursor, EDI out bounds); cursor.left moves back 3 pixels; the out bounds start at
// origin.left; the span is drawn as a formatted button prompt string (0x49ade0, EDX text) when
// the caller asks for it and a multiplayer game engine is running, otherwise as plain text
// (0x514ab0); origin.top follows cursor.top. The first rewrite passed no rectangles on and
// read 0x006f1d20 (current_game_engine) as a flag byte.
// register convention: EAX cursor, ECX origin; two stack arguments.
//   // blam-cc: cursor -> EAX, origin -> ECX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t ui_prompt_clip_x;                 // 0x006e476e
extern int16_t ui_prompt_clip_y;                 // 0x006e4770
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void text_measure_string_extents(Rectangle2D *origin, Rectangle2D *cursor, Rectangle2D *out_bounds,
                         const uint16_t *text); // 0x5562d0, measure a span, blam-cc: EBX origin, ESI cursor, EDI out_bounds
extern void ui_widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color,
                                                   const uint16_t *text); // 0x49ade0, blam-cc: EDX text
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text); // 0x514ab0; blam-cc: EAX clip, ECX bounds

// blam-cc: cursor -> EAX, origin -> ECX
// Draws one text span of a HUD message line at the cursor and advances the cursor past it.
void hud_draw_message_text_span(Rectangle2D *cursor, Rectangle2D *origin, const uint16_t *text,
                                uint8_t allow_button_prompts)
{
    Rectangle2D bounds;

    ui_prompt_clip_x = (int16_t)(cursor->left - origin->left);
    ui_prompt_clip_y = 0;
    text_measure_string_extents(origin, cursor, &bounds, text);
    cursor->left = (int16_t)(cursor->left - 3);
    bounds.left = origin->left;
    if (allow_button_prompts != 0 && current_game_engine != 0) {
        ui_widget_draw_formatted_prompt_string(&bounds, 1, text);
    } else {
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text);
    }
    origin->top = cursor->top;
}

#if 0
Original Ghidra decompilation (0x4ad8e0):

void FUN_004ad8e0(undefined4 param_1,char param_2)

{
  undefined2 *in_EAX;
  undefined2 *in_ECX;
  undefined1 local_8 [2];
  undefined2 local_6;

  DAT_006e476e = in_EAX[1] - in_ECX[1];
  DAT_006e4770 = 0;
  FUN_005562d0(param_1);
  in_EAX[1] = in_EAX[1] + -3;
  local_6 = in_ECX[1];
  if ((param_2 != '\0') && (DAT_006f1d20 != 0)) {
    ui_widget_draw_formatted_prompt_string(local_8,1);
    *in_ECX = *in_EAX;
    return;
  }
  chimera__draw_16_bit_text(0,0,param_1);
  *in_ECX = *in_EAX;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
