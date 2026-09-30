// text_measure_glyph_callback  (not a Ghidra function: no entry at 0x556260 in the project)
// address 0x556260, size 108 bytes (0x556260..0x5562cb, int3 padding to 0x5562d0)
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: its only reference is the push 0x556260 at 0x556347 in
//   text_measure_string_extents (0x5562d0), which passes it as the glyph callback of
//   text_wrap_and_draw_wide (0x556780). It draws nothing: it grows text_measure_bounds
//   (0x006e4714, reset to 0x7fff/0x7fff/0x8000/0x8000 by the caller) to cover every
//   clipped glyph rectangle it is handed, and stores the glyph font in
//   text_measure_font (0x006e471c). out/phase4/text_types_notes.md: "0x556260 ... owns
//   the writes to 0x006e4714..0x006e471f".
// register convention: plain cdecl, the text_glyph_draw_proc shape (types/text.h). The
//   draw loops push ten dwords and pop 0x28 themselves (0x557287 / 0x557507). Only
//   x (+0x14), y (+0x18), width (+0x20), height (+0x24) and font (+0x08) are read,
//   each as its low word except font.
// Ghidra has no decompilation of this address, so the block at the end holds the
// objdump listing instead.

#include "tags.h"
#include "memory.h"
#include "text.h"
#include "fn_text.h"

extern Rectangle2D text_measure_bounds;             // 0x006e4714, top, left, bottom, right
extern uint32_t text_measure_font;                  // 0x006e471c, Font*

// cdecl, all ten arguments on the stack
void text_measure_glyph_callback(text_parse_state *state, void *font, void *character,
    uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y,
    int16_t width, int16_t height)
{
    int16_t right;
    int16_t bottom;

    right = (int16_t)(x + width);
    bottom = (int16_t)(y + height);

    if (x < text_measure_bounds.left) {
        text_measure_bounds.left = x;
    }
    if (y < text_measure_bounds.top) {
        text_measure_bounds.top = y;
    }
    if (right > text_measure_bounds.right) {
        text_measure_bounds.right = right;
    }
    if (bottom > text_measure_bounds.bottom) {
        text_measure_bounds.bottom = bottom;
    }
    // both exits store the font; the compiler duplicated the tail
    text_measure_font = (uint32_t)font;
}

#if 0
objdump -d -M intel bin/halo.exe, 0x556260..0x5562cc (no Ghidra function at this address):

  556260:	mov    eax,DWORD PTR [esp+0x14]      ; x
  556264:	mov    ecx,DWORD PTR [esp+0x24]      ; width
  556268:	push   esi
  556269:	mov    esi,DWORD PTR [esp+0x2c]      ; height
  55626d:	lea    edx,[eax+ecx*1]               ; x + width
  556270:	mov    ecx,DWORD PTR [esp+0x1c]      ; y
  556274:	add    esi,ecx                       ; y + height
  556276:	cmp    ax,WORD PTR ds:0x6e4716
  55627d:	jge    0x556285
  55627f:	mov    ds:0x6e4716,ax
  556285:	cmp    cx,WORD PTR ds:0x6e4714
  55628c:	jge    0x556295
  55628e:	mov    WORD PTR ds:0x6e4714,cx
  556295:	cmp    dx,WORD PTR ds:0x6e471a
  55629c:	jle    0x5562a5
  55629e:	mov    WORD PTR ds:0x6e471a,dx
  5562a5:	cmp    si,WORD PTR ds:0x6e4718
  5562ac:	jle    0x5562c1
  5562ae:	mov    edx,DWORD PTR [esp+0xc]       ; font
  5562b2:	mov    WORD PTR ds:0x6e4718,si
  5562b9:	mov    DWORD PTR ds:0x6e471c,edx
  5562bf:	pop    esi
  5562c0:	ret
  5562c1:	mov    eax,DWORD PTR [esp+0xc]       ; font
  5562c5:	mov    ds:0x6e471c,eax
  5562ca:	pop    esi
  5562cb:	ret
#endif
