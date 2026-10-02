// interface_draw_cursor  (Ghidra: interface_draw_cursor, already named)
// address 0x497380, size 132 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: matches the given name; disassembled directly (objdump 0x497380..0x497410) to
// recover the two register arguments ui_draw_screen_quad @0x498b20 needs, which Ghidra's
// decompile of this caller drops entirely.
// register convention: no register-passed arguments.
// The rect is a Rectangle2D: offset 0 holds ui_cursor_y (0x00718f88) and offset 2 ui_cursor_x
// (0x00718f84), offsets 4 and 6 the same plus the icon size.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index ui_cursor_bitmap; // 0x0068e67c, ui\shell\bitmaps\cursor
extern int32_t ui_cursor_x;          // 0x00718f84
extern int32_t ui_cursor_y;          // 0x00718f88

extern int32_t bitmap_group_sequence_get_bitmap_data(datum_index bitmap, int16_t sequence,
                                                     int16_t frame); // 0x43f290; blam-cc: EAX -> bitmap, EDI -> frame, stack -> sequence
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                                 int16_t *clip_rect, uint32_t vertex_color); // 0x498b20; blam-cc: EAX, ECX
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect); // 0x449780, solid rectangle fill
    // blam-cc: EAX -> packed_color, ECX -> rect (objdump call sites 0x494d28, 0x4973f9, 0x498617)

// Draws the mouse cursor bitmap (a 32x32 quad) over the interface at ui_cursor_x/y; if the
// cursor bitmap tag or its bitmap data has not loaded yet, falls back to a 16x16 solid
// translucent red rectangle. objdump 0x497380..0x497403: the rect is a Rectangle2D
// {y, x, y + size, x + size}; the first rewrite stored x and y + size in swapped slots.
void interface_draw_cursor(void)
{
    Rectangle2D rect;
    int32_t bitmap_data;

    rect.top = (int16_t)ui_cursor_y;
    rect.left = (int16_t)ui_cursor_x;

    if (ui_cursor_bitmap != (datum_index)-1) {
        bitmap_data = bitmap_group_sequence_get_bitmap_data(ui_cursor_bitmap, 0, 0);
        if (bitmap_data != 0) {
            rect.bottom = (int16_t)(ui_cursor_y + 0x20);
            rect.right = (int16_t)(ui_cursor_x + 0x20);
            ui_draw_screen_quad((int16_t *)0, (int16_t *)&rect, bitmap_data, (int16_t *)0, 0xffffffffu);
            return;
        }
    }
    rect.bottom = (int16_t)(ui_cursor_y + 0x10);
    rect.right = (int16_t)(ui_cursor_x + 0x10);
    ui_draw_filled_rectangle(0x80ff0000, &rect);
}

#if 0
Original Ghidra decompilation (0x497380):

void interface_draw_cursor(void)

{
  int iVar1;

  if (DAT_0068e67c != -1) {
    iVar1 = bitmap_group_sequence_get_bitmap_data(0);
    if (iVar1 != 0) {
      FUN_00498b20(iVar1,0,0xffffffff);
      return;
    }
  }
  FUN_00449780();
  return;
}

Disassembly (objdump -d -M intel, 0x497380..0x497410):

00497380:
  sub    esp,0x8
  mov    eax,ds:0x68e67c
  cmp    eax,0xffffffff
  push   ebx
  mov    ebx,DWORD PTR ds:0x718f88
  push   esi
  mov    esi,DWORD PTR ds:0x718f84
  mov    WORD PTR [esp+0xa],si
  mov    WORD PTR [esp+0x8],bx
  je     0x4973e0
  push   edi
  push   0x0
  xor    edi,edi
  call   0x43f290                  ; bitmap_group_sequence_get_bitmap_data(0)
  add    esp,0x4
  test   eax,eax
  pop    edi
  je     0x4973e0
  push   0xffffffff
  push   0x0
  push   eax
  add    esi,0x20
  add    ebx,0x20
  xor    eax,eax                   ; EAX (source_rect) = NULL
  lea    ecx,[esp+0x14]            ; ECX (dest_rect) = &rect
  mov    WORD PTR [esp+0x1a],si
  mov    WORD PTR [esp+0x18],bx
  call   0x498b20                  ; ui_draw_screen_quad
  add    esp,0xc
  pop    esi
  pop    ebx
  add    esp,0x8
  ret
  add    esi,0x10
  add    ebx,0x10
  mov    eax,0x80ff0000            ; EAX (packed_color) = translucent red
  lea    ecx,[esp+0x8]             ; ECX (rect) = &rect
  mov    WORD PTR [esp+0xe],si
  mov    WORD PTR [esp+0xc],bx
  call   0x449780                  ; interface_draw_cursor_fallback_fill
  pop    esi
  pop    ebx
  add    esp,0x8
  ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
