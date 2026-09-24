# text module: type recovery notes

Target: retail Halo PC `halo.exe` 1.0.10, module `text` (22 Ghidra functions, 0x5561b0..0x5579d2).
Header: `types/text.h`. Smoke test: `out/phase4/text_smoke.c`:

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -Wall -Wextra -I types out/phase4/text_smoke.c
```

It passes on the host compiler and with `-m32`. It checks the size of `text_parse_state` and every field offset,
the tags.h `Font` / `FontCharacter` / `StringListString` / `Globals` / `GlobalsInterfaceBitmaps` offsets that the
module reads by raw arithmetic, the global run ends (`0x006e4738 + sizeof(ColorARGB) == 0x006e4748`, the tab-stop
array ending at `0x006e476a`, `Rectangle2D` at `0x006e4714` ending at the `Font*` at `0x006e471c`), and the enum
values the code compares against. I also built a translation unit that includes every header in `types/` after
`tags.h memory.h math.h`. None of its diagnostics mention `text.h`, so no names collide. The errors it does report
come from forward references that were already there (effects.h and input.h).

The disassembly used is `objdump -d -M intel` of `bin/halo.exe` over 0x556150..0x5579e0.

## Structs

### text_parse_state (0x1c), new

| off | field | established by |
|---|---|---|
| 0x00 | `datum_index font` | 0x556b00 `mov [esi],eax` (stack arg 1, always 0x006e472c). The style-resolve code in 0x556bb0 reads it again. |
| 0x04 | `Font* font_definition` (uint32) | 0x556b00 `mov [esi+0x4],ecx` after the style resolve. Also rewritten on a style token in 0x556bb0 (`unaff_EDI[1]`). Every glyph lookup reads +0x34 / +0x80 through it. |
| 0x08 | `string` (uint32: char* or uint16_t*) | 0x556b00 `mov [esi+0x8],ecx` (ECX is arg 6 of the wrap loops, arg 5 of the draw loops). 0x556bb0 reads it as bytes; 0x556f10 reads it as `[esi+edx*2]`. |
| 0x0c | `int16 position` | 0x556b00 writes 0. Both tokenizers advance it by 1 or 2. 0x557030 / 0x5572b0 seed it with the span start (`mov [esp+0x34],ax`). 0x557530 returns it as the break column. |
| 0x0e | `int16 style` | 0x556b00 `mov [esi+0xe],bx`. 0x556bb0 writes 0/1/2/3/-1 on the b/i/k/u/p codes. |
| 0x10 | `int16 justification` | 0x556b00 `mov [esi+0x10],dx`. 0x556bb0 writes 0/1/2 on l/r/c. The wrap loops test it (local_30 == 1 / 2). |
| 0x12 | `uint16 character` | Both tokenizers store the code they read. The wrap, draw and measure loops split it into page (>>8) and index (&0xff). |
| 0x14 | `int16 token` | Both tokenizers store their return value. The measure loop switches on it. |
| 0x16 | `unknown_16` | Nothing writes or reads it. Alignment before the dword. |
| 0x18 | `uint32 color` | 0x556b00: four `fld [edi+n]; fmul 255.0; __ftol` steps packed as a<<24 \| r<<16 \| g<<8 \| b. The draw loops pass it as the callback colour and XOR it with 0xffffff inside the highlight. |

The size comes from the stack frames. In 0x556400 / 0x556780 (frame 0x68) the record sits at esp+0x4c, which leaves
exactly 0x1c bytes. In 0x557530 (frame 0x30) it sits at esp+0x14, again 0x1c bytes. The draw loops pass `&state`
as callback argument 1 (`lea ecx,[esp+0x4c]` after 9 pushes, equal to steady esp+0x28, the record base).

Register conventions confirmed from call sites and prologues:
- 0x556b00: ESI = state, stack = (font datum, ColorARGB*), ECX = string, DX = justification, BX = style.
- 0x556bb0: EDI = state. 0x556f10: EAX = state.
- 0x557030 / 0x5572b0: EAX = line bounds `Rectangle2D*` (the wrap's local rect copy after tab/indent adjustment).
  The stack args are (callback, `Point2DInt*` pen, clip `Rectangle2D*`, color, string, start, end). Ghidra drops the
  string (param_5) because it only reaches 0x556b00 through ECX.
- 0x556400 / 0x556780 stack: (callback, bounds `Rectangle2D*`, out `Point2DInt*` final pen, clip `Rectangle2D*`,
  int16 extra line spacing, string).
- 0x5562d0: EBX = origin `Rectangle2D*`, ESI = out cursor rect (y-ascent, x, y+descent, x+1), EDI = out extents
  rect, stack = UTF-16 string. It calls 0x556780 with callback 0x556260.
- 0x5563b0: ECX = font datum, EAX = ColorARGB*, stack = (int16 style, int16 justification, uint32 flags).
- 0x557530: ECX = UTF-16 string (passed straight through to 0x556b00), stack = `int *max_width`.

### Foreign layouts confirmed (not redefined)

- `Font` (tags.h): +0x04 ascending, +0x06 descending, +0x08 leading_height, +0x0a leading_width. The line pitch is
  asc+desc+leading+extra. +0x34 `character_tables.pointer`: 0x0c stride, and a page is used only when its count is
  0x100. +0x48 + style*0x10 is the tag_id of the bold/italic/condense/underline dependencies. +0x80
  `characters.pointer` has a 0x14 stride.
- `FontCharacter` (tags.h): +0x02 advance, +0x04/+0x06 bitmap size, +0x08/+0x0a bitmap origin. The draw loops
  subtract the origin from the pen.
- `StringListString` / `UnicodeStringListString`: 0x14 stride, size at +0, pointer at +0x0c. 0x5578c0 terminates
  at `(size & ~1) - 2` as a 16-bit store, so it reads the **unicode** string list.
- `Globals.interface_bitmaps` at +0x140/+0x144. `GlobalsInterfaceBitmaps.localization.tag_id` is at +0xac, which
  gives the datum cached in 0x006e4728.
- `ColorARGB` at 0x006e4738: alpha first. Callers store 1.0f in the first dword (decompile line 58189). Note for
  **types/game.h**: `hud_world_text_params` calls its +0x00 an opaque dword and its +0x04..+0x0c r/g/b. The whole
  0x10 block is a ColorARGB, and its +0x00 is the alpha.
- `Rectangle2D` (top, left, bottom, right): the clip intersection in 0x557030 compares [1]/[3] against x and
  [0]/[2] against y. The pen is `Point2DInt` (x low word, y high word).

## Enums and constants

- `text_justification` comes from the l/r/c codes and the right/centre arithmetic in the wrap loops.
- `text_style` comes from the b/i/k/u/p codes and the +0x48 + style*0x10 lookup.
- `text_token` comes from both tokenizers and the switch cases in the wrap and measure loops. No tokenizer produces
  5. 0x556bb0 skips 5 and 7 internally.
- `text_flags`: bit 0 (`test byte [0x6e4730],1`) turns on the word-wrap back-up. Bit 1 draws lines whose baseline
  is at or below the bounds bottom. One caller writes value 8 (bit 3), and nothing in the module reads it.
- `text_encoding`: the five DBCS range tables in 0x557750. The code-page names are inferred from the byte ranges.
- `text_localization_string`: strings 0, 4, 5, 6 as read by 0x5561b0 and 0x556bb0.
- `k_text_maximum_tab_stops = 16` is inferred from the .bss gap, not from the code (see below).

## Globals

This module owns 0x006e4714..0x006e471f (the extents probe), 0x006e4728..0x006e4771 (the draw state) and
0x006e4800 (the encoding). The .rdata strings are 0x00671fa0 "ibukprlctn" and 0x00671fd0 "<missing string>". The
full list with types is at the bottom of text.h. The draw state is deliberately kept as separate globals, not one
struct. The fields are independent file-scope variables that about 17 callers outside the module write directly,
because LTCG inlined the setters. Nothing takes the address of the run as a whole.

## Unresolved

- `text_parse_state.unknown_16` (+0x16, 2 bytes): never touched. It is taken to be padding.
- `text_tab_stops` capacity: the most any caller writes is a count of 7 (0x006e474a..0x006e4757). The declared 16
  is only the space before 0x006e476a.
- `text_highlight_start` / `text_highlight_end` (0x006e476a / 0x006e476c): the only writes are the zeroing at the
  end of both wrap loops. No setter was found, so the highlight-invert path may be dead in retail.
- `_text_flag_unknown_bit3` (0x08): written once (decompile line 61238), never tested here.
- Localization strings 1..3: not read by this module.
- `text_encoding` value 2: GB2312 and EUC-KR share the tested ranges, so the language is ambiguous.
- 0x006e4720..0x006e4727 and 0x006e4772..0x006e47ff: no references anywhere in the decompile.

## Function attribution and naming notes

- 0x557910 `string_format_wide_va_bounded` and 0x557930 `string_format_wide_va` are thin engine wrappers around
  the CRT `_vsnwprintf` / `_vswprintf` (0x557910 takes the count in EDX, 0x557930 takes the buffer in EDX). They are
  engine code in the text directory, not library code, but they operate on no struct.
- 0x557950 (wide to narrow, non-ASCII becomes space; EDI src, ESI dst, stack capacity) and 0x557990 (narrow to wide;
  EBX src, EAX dst, EDI capacity in bytes) touch no struct. The phase 2 summary of 0x557990 says
  "length-prefixed". That is wrong: `in_EAX + 2 + (i-1)*2` is just `dst[i]`, a plain NUL-terminated copy.
- The phase 2 names "styled" and "plain" for 0x556400/0x556bb0/0x557030 and 0x556780/0x556f10/0x5572b0 are really
  **narrow (8-bit DBCS with markup)** versus **wide (UTF-16)**. The "plain" tokenizer reads 16-bit code units.
- 0x5578c0 `text_string_list_get_string` reads a UnicodeStringList and returns `uint16_t*`, with the fallback
  0x00671fac L"<missing string>". The CEA hint `unicode_string_list_get_string` fits.
- 0x5561b0 caches the Globals localization string list and sets the DBCS encoding. "language" in its name really
  means that encoding id.
- 0x556260 (the glyph callback that 0x5562d0 passes to 0x556780) is not a Ghidra function. It owns the writes to
  0x006e4714..0x006e471f.
- No function in the range belongs to another module.
