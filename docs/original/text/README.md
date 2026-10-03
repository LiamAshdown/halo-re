# `text`: bitmap-font text layout and drawing

Retail Halo PC `halo.exe` 1.0.10, `0x5561b0 .. 0x5579d2` (22 Ghidra functions, plus the glyph
callback at `0x556260`, which Ghidra has no function for). Plain C, MSVC 7.1 (cl 13.10.3077,
LTCG), x86. Each `.c` file here holds one function, rewritten against `types/text.h`. The Ghidra
decompile is kept at the bottom of each file inside `#if 0 ... #endif`. The callback has no
decompile, so its file keeps the objdump listing there instead.

Gate: `python tools/build_check.py text` gives **23 ok, 0 failed**.

## What the module contains

The engine's text renderer. It keeps a global draw state (font, style, justification, flags,
colour, tab stops, indents), runs a word-wrap loop that measures and places each line, and runs a
per-glyph draw loop that passes every visible, clipped glyph to a callback the caller supplies.
It never touches the rasterizer itself: the callback does the drawing. The only callers of
the wrap loops are the rasterizer's `0x5148b0` (narrow) and `0x514ab0` (wide), which both pass
the glyph blitter `0x514ce0`. The other callback is `0x556260` here, used for measuring.

There are two parallel pipelines that share one parse-state record:

```
narrow (8-bit, DBCS aware, |x markup)        wide (UTF-16 code units, only |n)
  text_wrap_and_draw_narrow      0x556400      text_wrap_and_draw_wide         0x556780
    text_parse_state_initialize  0x556b00        text_parse_state_initialize   0x556b00
    text_parse_next_token_narrow 0x556bb0        text_parse_next_token_wide    0x556f10
    text_draw_character_range_narrow 0x557030    text_draw_character_range_wide 0x5572b0
      -> text_glyph_draw_proc callback             -> text_glyph_draw_proc callback
```

The wide pipeline also drives `text_measure_string_fit_width` (`0x557530`, the break column for
a pixel width) and `text_measure_string_extents` (`0x5562d0`, which returns the caret rect and span
box by running the wide wrap with `text_measure_glyph_callback`).

The rest of the module:

| Group | Functions |
|---|---|
| draw state | `text_set_render_context` (`0x5563b0`). About 17 callers outside the module write the same globals directly, because LTCG inlined this setter. |
| localization | `text_language_initialize_from_string_list` (`0x5561b0`) caches the `globals` localization string list and sets the DBCS encoding from string 0. `text_string_list_get_string` (`0x5578c0`) reads a unicode string list entry. |
| DBCS helpers | `text_char_is_double_byte` (`0x557750`), `text_get_next_character`, `text_find_character_boundary`, `text_clamp_byte_length_to_character_boundary`, `text_find_character`, `text_get_character_metrics` |
| string helpers | `string_format_wide_va_bounded` / `string_format_wide_va` (`_vsnwprintf` / `_vswprintf` wrappers), `string_convert_unicode_to_ascii`, `string_convert_ascii_to_unicode` |

Byte-level behaviour worth knowing before hooking:

- The DBCS test (`0x557750`) treats a `|` followed by one of `ibukprlctn` as a two-byte unit, so
  markup is always consumed as a pair. It then checks the lead and trail byte ranges of encoding
  1..5: Shift-JIS, EUC, Big5, Korean UHC and Johab. The language names are inferred from the
  ranges; the binary only stores the numbers.
- The narrow tokenizer classifies a glyph as a break point (token 2) from localization strings 4,
  5 and 6. The no-break test (string 6) looks at the character that comes *after* the glyph, not
  the glyph itself.
- A font page is used only when `character_table.count == 0x100`. A page with a nonzero count
  other than 0x100 makes every lookup read through NULL, and this is kept as the binary has it.

## Struct layouts

All layouts are in `types/text.h`, except the tag structs, which come from `types/tags.h`.
`#pragma pack(push,1)`.

### `text_parse_state`: size `0x1c` (filled by `0x556b00`, one on the stack of each wrap, draw and measure frame)

| Off | Type | Field |
|---|---|---|
| `0x00` | `datum_index` | `font`: the base font tag (always `text_font`), never rewritten |
| `0x04` | `uint32_t` (`Font*`) | `font_definition`: the tag data of the style font. Rewritten by style tokens in `0x556bb0`. |
| `0x08` | `uint32_t` (`char*` / `uint16_t*`) | `string`: not owned |
| `0x0c` | `int16_t` | `position`: next index, in bytes (narrow) or code units (wide) |
| `0x0e` | `int16_t` | `style`: `text_style`, -1 plain, 0..3 index `Font.bold` .. `underline` |
| `0x10` | `int16_t` | `justification`: `text_justification` |
| `0x12` | `uint16_t` | `character`: last code read. A DBCS pair is `lead << 8 \| trail`. |
| `0x14` | `int16_t` | `token`: `text_token` of the last token |
| `0x16` | `int16_t` | `unknown_16`: never touched (padding) |
| `0x18` | `uint32_t` | `color`: `a<<24 \| r<<16 \| g<<8 \| b`, each `ColorARGB` channel times 255.0 through `__ftol`. The draw loops XOR it with `0xffffff` inside the highlight. |

### `text_glyph_draw_proc`: cdecl, ten dword slots, caller pops `0x28`

| Slot | Type | Argument |
|---|---|---|
| 1 | `text_parse_state *` | `state` |
| 2 | `Font *` | `font`: the current style font |
| 3 | `FontCharacter *` | `character` |
| 4 | `uint32_t` | `color`: packed, highlight-inverted |
| 5, 6 | `int16_t` | `x`, `y`: the clipped screen position of the bitmap |
| 7, 8 | `int16_t` | `source_x`, `source_y`: texels clipped off the left and top |
| 9, 10 | `int16_t` | `width`, `height`: what is left to draw, always > 0 |

### Draw-state globals (`0x006e4714 .. 0x006e4771`, `0x006e4800`)

| Address | Type | Name | Notes |
|---|---|---|---|
| `0x006e4714` | `Rectangle2D` | `text_measure_bounds` | Reset to `0x7fff,0x7fff,0x8000,0x8000` by `0x5562d0` and grown by `0x556260` |
| `0x006e471c` | `uint32_t` (`Font*`) | `text_measure_font` | The font of the last measured glyph |
| `0x006e4728` | `datum_index` | `text_localization_strings` | The StringList from `globals.interface_bitmaps[0].localization` |
| `0x006e472c` | `datum_index` | `text_font` | Set to -1 by `0x5561b0` |
| `0x006e4730` | `uint32_t` | `text_flags_state` | bit 0 word wrap, bit 1 draw past bottom, bit 3 written once and never tested |
| `0x006e4734` | `int16_t` | `text_style_state` | Passed in BX to `0x556b00` |
| `0x006e4736` | `int16_t` | `text_justification_state` | Passed in DX to `0x556b00` |
| `0x006e4738` | `ColorARGB` | `text_color` | Alpha first |
| `0x006e4748` | `int16_t` | `text_tab_stop_count` | |
| `0x006e474a` | `int16_t[16]` | `text_tab_stops` | Absolute x. The wraps read `(&count)[n]` for the left edge after tab `n`. |
| `0x006e476a` | `int16_t` | `text_highlight_start` | Columns drawn inverted, up to but not including `end` |
| `0x006e476c` | `int16_t` | `text_highlight_end` | Only written as 0 at the end of each wrap |
| `0x006e476e` | `int16_t` | `text_first_line_indent` | Added to `left` on line 0 |
| `0x006e4770` | `int16_t` | `text_wrapped_line_indent` | Added to `left` on later lines |
| `0x006e4800` | `int16_t` | `text_encoding_state` | `text_encoding` 0..5 |
| `0x00671fa0` | `char[11]` | `text_markup_codes` | `"ibukprlctn"` |
| `0x00671fd0` | `char[17]` | `missing_string` | Narrow `"<missing string>"`. The wide one is `missing_string_text` at `0x00671fac`. |

Globals that hold an enum value carry a `_state` suffix, because C does not allow a variable and a
typedef to share a name (`text_style`, `text_justification`, `text_flags` and `text_encoding` are
the enum typedefs).

### Tag layouts this module reads (`types/tags.h`, not redefined)

| Struct | Offsets used |
|---|---|
| `Font` | `+0x04` ascending, `+0x06` descending, `+0x08` leading_height, `+0x0a` leading_width, `+0x34` character_tables (stride `0x0c`), `+0x48 + style*0x10` style dependency tag_id, `+0x80` characters (stride `0x14`) |
| `FontCharacter` | `+0x02` character_width (advance), `+0x04/+0x06` bitmap width and height, `+0x08/+0x0a` bitmap origin |
| `StringListString` / `UnicodeStringListString` | stride `0x14`, `+0x00` size, `+0x0c` pointer |

## Known gaps

1. **Highlight is probably dead in retail.** Nothing writes `text_highlight_start` or
   `text_highlight_end` except the zeroing at the end of both wraps. The XOR path in the draw
   loops is kept but may never run.
2. **Word-wrap local names are inferred.** The first rewrite named the bookkeeping locals in the
   two wrap loops (`span_width`, `candidate_position`, `wrapped_sub_line_count` and so on) from
   how they are used. The control flow and arithmetic were checked instruction by instruction in
   this review. The names were not.
3. **A newline never resets `max_wrapped_sub_line_count`.** Once a long wrapped line has been
   seen, each later newline advances by that count plus one, as the binary does. This may be a
   bug in the original. It is transcribed as is.
4. **Reads through NULL, kept as in the binary:** a font page whose count is not 0x100 (all
   six lookup sites), and `0x5561b0` when `globals.interface_bitmaps` is empty.
5. **`string_convert_ascii_to_unicode` writes `dst[-1]` when capacity is under 2 bytes.** Kept as
   is. EAX comes back as `dst`, but the function is written as `void` because nothing shows the
   source returned it.
6. **`text_find_character` returns only a flag, in AL.** The phase 2 note says it returns a byte
   offset, but the binary's upper EAX bits are garbage.
7. **Not ported to other modules:** those files are outside this module's edit scope. See the
   open questions in the phase 4 summary. In short, `src/game` declares the `0x006e47xx` globals
   under its own `hud_text_draw_*` names, some of them with the wrong types. It also omits the
   EDX count of `string_format_wide_va_bounded`, and declares `0x557990` with EDI as the
   destination (it is EAX dst, EBX src, EDI capacity).

## Functions and rewrite confidence

`name` is the confidence in the symbol name, `rw` the confidence in the C rewrite, and `U` the
number of `UNSURE` markers in the file header. Every row was checked against objdump in the
phase 4 review. The wraps and draw ranges were checked line by line, and their twins by diffing
the disassembly.

| Address | Function | Bytes | name | rw | U |
|---|---|---|---|---|---|
| `0x5561b0` | `text_language_initialize_from_string_list` | 161 | 0.55 | 0.85 | 1 |
| `0x556260` | `text_measure_glyph_callback` (no Ghidra function) | 108 | 0.7 | 0.9 | 0 |
| `0x5562d0` | `text_measure_string_extents` | 222 | 0.6 | 0.8 | 0 |
| `0x5563b0` | `text_set_render_context` | 74 | 0.6 | 0.85 | 0 |
| `0x556400` | `text_wrap_and_draw_narrow` | 859 | 0.4 | 0.8 | 3 |
| `0x556780` | `text_wrap_and_draw_wide` | 863 | 0.4 | 0.8 | 2 |
| `0x556b00` | `text_parse_state_initialize` | 169 | 0.7 | 0.9 | 0 |
| `0x556bb0` | `text_parse_next_token_narrow` | 787 | 0.5 | 0.85 | 0 |
| `0x556f10` | `text_parse_next_token_wide` | 125 | 0.5 | 0.9 | 0 |
| `0x557030` | `text_draw_character_range_narrow` | 626 | 0.5 | 0.85 | 0 |
| `0x5572b0` | `text_draw_character_range_wide` | 626 | 0.5 | 0.85 | 0 |
| `0x557530` | `text_measure_string_fit_width` | 245 | 0.5 | 0.85 | 0 |
| `0x557650` | `text_get_character_metrics` | 76 | 0.5 | 0.9 | 0 |
| `0x5576a0` | `text_get_next_character` | 41 | 0.5 | 0.9 | 0 |
| `0x5576d0` | `text_find_character_boundary` | 70 | 0.4 | 0.85 | 0 |
| `0x557720` | `text_clamp_byte_length_to_character_boundary` | 40 | 0.4 | 0.85 | 0 |
| `0x557750` | `text_char_is_double_byte` | 266 | 0.55 | 0.85 | 0 |
| `0x557870` | `text_find_character` | 71 | 0.4 | 0.85 | 1 |
| `0x5578c0` | `text_string_list_get_string` | 73 | 0.5 | 0.85 | 0 |
| `0x557910` | `string_format_wide_va_bounded` | 25 | 0.5 | 0.7 | 0 |
| `0x557930` | `string_format_wide_va` | 20 | 0.5 | 0.7 | 0 |
| `0x557950` | `string_convert_unicode_to_ascii` | 60 | 0.6 | 0.85 | 0 |
| `0x557990` | `string_convert_ascii_to_unicode` | 66 | 0.4 | 0.8 | 0 |

Register conventions (confirmed from call sites and prologues, LTCG-specific):

- `0x556b00`: ESI state, ECX string, DX justification, BX style, stack (font, ColorARGB*).
- `0x556bb0`: EDI state. `0x556f10`: EAX state.
- `0x557030` / `0x5572b0`: EAX line bounds, stack (callback, pen, clip, color, string, start, end).
- `0x5562d0`: EBX origin, ESI caret rect out, EDI extents out, stack string.
- `0x5563b0`: ECX font, EAX ColorARGB*, stack (style, justification, flags).
- `0x557530`: ECX string, stack `int *max_width`.
- `0x557650`: EDI font, DX character. `0x557750`: EAX string, result in AL.
- `0x5576a0`: EAX string, ESI cursor. `0x557720`: EBX string, EDI length.
- `0x557870`: EBX character, stack string, result in AL.
- `0x5578c0`: ECX list, DX index.
- `0x557910`: EDX count, stack (dest, format, ...). `0x557930`: EDX dest, stack (format, ...).
- `0x557950`: EDI src, ESI dst, stack capacity. `0x557990`: EAX dst, EBX src, EDI capacity (bytes).
