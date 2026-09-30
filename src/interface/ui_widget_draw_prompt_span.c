// ui_widget_draw_prompt_span  (Ghidra: FUN_0049ad30, unnamed)
// address 0x49ad30, size 106 bytes
// name confidence: 0.35   rewrite confidence: 0.8
// evidence: sole caller is ui_widget_draw_formatted_prompt_string @0x49ade0
// (out/phase2/interface/02.md), which inlines this exact sequence for its own first text span
// (set the clip globals from the x-position delta, measure with text_measure_string_extents, draw, retreat the
// cursor by 3, write the top back) before calling this address for every later span.
// register convention: text in the recognized stack parameter; running cursor rect in EAX and
// the caller origin rect in ECX.
// blam-cc: EAX -> cursor, ECX -> origin; text is an ordinary cdecl stack parameter
// Review pass (phase 4), from the disassembly (0x49ad30..0x49ad99): both records are full
// Rectangle2Ds (the earlier {color, x} reading was their top and left halves). The span is
// measured by text_measure_string_extents with the origin in EBX, the cursor in ESI (read and rewritten) and a
// local output rect in EDI; the output left edge is then pinned to origin->left and
// chimera__draw_16_bit_text draws into it (ECX) with EAX = NULL. 0x6e4770 is a word.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern int16_t ui_prompt_clip_y; // 0x006e4770, word store
extern int16_t ui_prompt_clip_x; // 0x006e476e, clamped to >= 0

extern void text_measure_string_extents(Rectangle2D *origin, Rectangle2D *cursor, Rectangle2D *out_bounds,
                         const uint16_t *text); // 0x5562d0, measure a span
    // blam-cc: EBX -> origin, ESI -> cursor (read and rewritten), EDI -> out_bounds, stack -> text
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text); // 0x514ab0
    // blam-cc: EAX -> clip (NULL here), ECX -> bounds

// blam-cc: EAX -> cursor, ECX -> origin, stack -> text
// Draws one span of a button-prompt caption: sets the clip offset to the non-negative distance
// the cursor has moved right of the origin, measures the span, retreats the cursor by 3, draws
// the span with its left edge at origin->left, and copies the cursor top back to the origin.
void ui_widget_draw_prompt_span(const uint16_t *text, Rectangle2D *cursor, Rectangle2D *origin)
{
    Rectangle2D bounds;
    int16_t delta = (int16_t)(cursor->left - origin->left);

    ui_prompt_clip_y = 0;
    ui_prompt_clip_x = (delta < 0) ? 0 : delta;
    text_measure_string_extents(origin, cursor, &bounds, text);
    cursor->left = (int16_t)(cursor->left - 3);
    bounds.left = origin->left;
    chimera__draw_16_bit_text((Rectangle2D *)0, &bounds, 0, 0, text);
    origin->top = cursor->top;
}

#if 0
Original Ghidra decompilation (0x49ad30):

void FUN_0049ad30(undefined4 param_1)

{
  undefined2 *in_EAX;
  undefined2 *in_ECX;

  DAT_006e4770 = 0;
  DAT_006e476e = in_EAX[1] - in_ECX[1] & ((short)(in_EAX[1] - in_ECX[1]) < 0) - 1;
  FUN_005562d0(param_1);
  in_EAX[1] = in_EAX[1] + -3;
  chimera__draw_16_bit_text(0,0,param_1);
  *in_ECX = *in_EAX;
  return;
}
#endif
