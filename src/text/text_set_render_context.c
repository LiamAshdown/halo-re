// text_set_render_context  (Ghidra: text_set_render_context, already named)
// address 0x5563b0, size 74 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/text_types_notes.md. Copies a font datum, a ColorARGB and two
//   int16 codes into the module's draw-state globals, the same globals
//   text_parse_state_initialize (0x556b00) reads on every wrap/draw call.
// register convention: ECX = font datum, EAX = ColorARGB*, stack = (int16 style,
//   int16 justification, uint32_t flags).
//   // blam-cc: ECX -> font, EAX -> color, stack -> style, justification, flags

#include "tags.h"
#include "memory.h"
#include "text.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// text_encoding, text_justification and text_flags are already the names of this
// module's enum typedefs (types/text.h); the file-scope globals use a "_state" suffix
// instead (see text_language_initialize_from_string_list.c).
extern datum_index hud_text_draw_font_tag_id;                        // 0x006e472c
extern ColorARGB hud_text_draw_color_a;                          // 0x006e4738
extern int16_t hud_text_draw_color_or_flags;                     // 0x006e4734
extern int16_t hud_text_draw_column;              // 0x006e4736
extern uint32_t hud_text_draw_unknown_4730;                     // 0x006e4730

// blam-cc: ECX -> font, EAX -> color, stack -> style, justification, flags
void text_set_render_context(datum_index font, ColorARGB *color, int16_t style,
    int16_t justification, uint32_t flags)
{
    hud_text_draw_font_tag_id = font;
    hud_text_draw_color_a = *color;
    hud_text_draw_color_or_flags = style;
    hud_text_draw_column = justification;
    hud_text_draw_unknown_4730 = flags;
}

#if 0
Original Ghidra decompilation (0x5563b0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void text_set_render_context(undefined2 param_1,undefined2 param_2,undefined4 param_3)

{
  undefined4 *in_EAX;
  undefined4 in_ECX;

  DAT_006e472c = in_ECX;
  DAT_006e4738 = *in_EAX;
  DAT_006e473c = in_EAX[1];
  DAT_006e4740 = in_EAX[2];
  DAT_006e4744 = in_EAX[3];
  DAT_006e4734._0_2_ = param_1;
  DAT_006e4734._2_2_ = param_2;
  _DAT_006e4730 = param_3;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
