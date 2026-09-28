// text_language_initialize_from_string_list  (Ghidra: text_language_initialize_from_string_list, already named)
// address 0x5561b0, size 161 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/text_types_notes.md. Reads Globals.interface_bitmaps (+0x140, a
//   TagReflexive) and, from its first (and only) GlobalsInterfaceBitmaps element, the
//   localization string_list dependency's tag_id (+0xac) into text_localization_strings.
//   atol()s string 0 ("the encoding id") of that string list into text_encoding, clamped
//   to 0..5, then resets the draw-state globals that the tokenizer (0x556bb0) and the
//   wrap loops (0x556400 / 0x556780) depend on: hud_text_draw_font_tag_id, hud_text_draw_background_mode,
//   text_flags, text_justification (only the high half of the DAT_006e4734 dword --
//   text_style is left untouched), ui_prompt_clip_x, ui_prompt_clip_y.
// register convention: no register-passed arguments; the function only reads and writes
//   file-scope globals and one tag lookup.
// UNSURE: when Globals.interface_bitmaps.count is 0, the original still dereferences
//   NULL + 0xac for the tag_id (iVar2 stays 0 and DAT_006e4728 = *(uint *)(iVar2 + 0xac)).
//   Preserved as-is; in practice interface_bitmaps always has exactly one element in
//   retail so this path is not believed to be reachable.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "text.h"
#include <stdlib.h>

// text_encoding, text_justification and text_flags are already the names of this
// module's enum typedefs (types/text.h), so the file-scope globals that hold their
// current values are named with a "_state" suffix instead to avoid redeclaring them.
extern Globals *global_globals;                     // 0x00746fa0
extern tag_instance *tag_instances;                 // 0x0087bc14

extern datum_index text_localization_strings;       // 0x006e4728
extern int16_t text_encoding_state;                  // 0x006e4800
extern datum_index hud_text_draw_font_tag_id;                        // 0x006e472c
extern int16_t hud_text_draw_background_mode;                  // 0x006e4748
extern uint32_t hud_text_draw_unknown_4730;                    // 0x006e4730
extern int16_t hud_text_draw_column;             // 0x006e4736
extern int16_t ui_prompt_clip_x;               // 0x006e476e
extern int16_t ui_prompt_clip_y;             // 0x006e4770
extern char missing_string[17];                      // 0x00671fd0, "<missing string>"

void text_language_initialize_from_string_list(void)
{
    GlobalsInterfaceBitmaps *bitmaps;
    StringList *localization;
    char *encoding_string;

    bitmaps = (global_globals->interface_bitmaps.count == 0) ? (GlobalsInterfaceBitmaps *)0
        : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    text_localization_strings = *(datum_index *)&bitmaps->localization.tag_id;

    if (text_localization_strings != (datum_index)k_datum_index_none) {
        localization = (StringList *)tag_instances[text_localization_strings & 0xffff].data;
        encoding_string = missing_string;
        if (localization->strings.count > 0) {
            StringListString *first = (StringListString *)localization->strings.pointer;
            if ((int32_t)first->string.size > 0) {
                encoding_string = (char *)first->string.pointer;
                encoding_string[first->string.size - 1] = '\0';
            }
        }
        text_encoding_state = (int16_t)atol(encoding_string);
        if (text_encoding_state < 0 || text_encoding_state > 5) {
            text_encoding_state = 0;
        }

        hud_text_draw_font_tag_id = (datum_index)k_datum_index_none;
        hud_text_draw_background_mode = 0;
        hud_text_draw_unknown_4730 = 0;
        hud_text_draw_column = 0;
        ui_prompt_clip_x = 0;
        ui_prompt_clip_y = 0;
    }
}

#if 0
Original Ghidra decompilation (0x5561b0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void text_language_initialize_from_string_list(void)

{
  int *piVar1;
  int iVar2;
  long lVar3;
  char *_Str;

  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar2 = 0;
  }
  else {
    iVar2 = *(int *)(DAT_00746fa0 + 0x144);
  }
  DAT_006e4728 = *(uint *)(iVar2 + 0xac);
  if (DAT_006e4728 != 0xffffffff) {
    piVar1 = *(int **)((DAT_006e4728 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    _Str = "<missing string>";
    if (0 < *piVar1) {
      piVar1 = (int *)piVar1[1];
      iVar2 = *piVar1;
      if (0 < iVar2) {
        _Str = (char *)piVar1[3];
        _Str[iVar2 + -1] = '\0';
      }
    }
    lVar3 = _atol(_Str);
    DAT_006e4800 = (short)lVar3;
    if ((DAT_006e4800 < 0) || (5 < DAT_006e4800)) {
      DAT_006e4800 = 0;
    }
    DAT_006e472c = 0xffffffff;
    DAT_006e4748 = 0;
    _DAT_006e4730 = 0;
    DAT_006e4734._2_2_ = 0;
    DAT_006e476e = 0;
    DAT_006e4770 = 0;
  }
  return;
}
#endif
