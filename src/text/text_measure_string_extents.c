// text_measure_string_extents  (Ghidra: FUN_005562d0, renamed; the first rewrite called
//   it text_edit_measure_cursor_extents, phase 2 review queue text_edit_box_reset_cursor_font)
// address 0x5562d0, size 222 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// naming: it draws nothing and belongs to no edit box. Its six callers (0x49ad68,
//   0x49ae6b, 0x49aeeb, 0x4a93bc, 0x4ad90b, 0x4aec21: the UI prompt, HUD message and
//   virtual keyboard drawers) all call it to measure a UTF-16 span before drawing it,
//   using the caret rect as the next pen and the extents rect as the span box.
// evidence (first rewrite): out/phase4/text_functions.md's phase-2 summary: "Resolves the current edit
//   box font/override, invokes the styled word-wrap routine to (re)compute cursor
//   position, and stores the resulting extents into an on-screen cursor/selection
//   structure passed in via ESI/EDI." out/phase4/text_types_notes.md: "0x556260 (the
//   glyph callback that 0x5562d0 passes to 0x556780) is not a Ghidra function. It owns
//   the writes to 0x006e4714..0x006e471f", i.e. it maintains text_measure_bounds as it
//   is invoked per visible glyph, and this function reads that global back once the
//   wrap/measure pass finishes. Resolves the style-dependent font the same way
//   text_parse_state_initialize (0x556b00) does, caches it in text_measure_font (so the
//   callback -- and this function, afterwards -- can read Font.ascending/descending
//   height off it), then runs the wide word-wrap (text_wrap_and_draw_wide, 0x556780)
//   over the string purely to measure it (its callback does not draw). The wrap's
//   final pen position becomes the caret rectangle (1 pixel wide, ascent above the
//   pen to descent below it); the accumulated text_measure_bounds becomes the second,
//   overall extents rectangle.
// register convention (objdump -d -M intel over 0x5562d0..0x5563b0): EBX = origin
//   bounds Rectangle2D*, ESI = out cursor rect Rectangle2D* (top/bottom = pen y -+
//   ascent/descent, left = pen.x, right = pen.x + 1), EDI = out extents rect
//   Rectangle2D* (top = origin bounds->top, left/right = text_measure_bounds.left/
//   right, bottom = the cursor rect's own bottom), stack = UTF-16 string.
//   // blam-cc: EBX -> origin_bounds, ESI -> out_cursor_rect, EDI -> out_extents_rect, stack -> string
// text_measure_glyph_callback (0x556260) is not a Ghidra function; it is rewritten from
//   objdump in src/text/text_measure_glyph_callback.c. Note it rewrites
//   text_measure_font with the font of EVERY glyph it sees, so the caret ascent/descent
//   read below come from the last glyph measured (or the style font set here when the
//   string draws no glyph).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "text.h"

extern tag_instance *tag_instances;             // 0x0087bc14
extern datum_index hud_text_draw_font_tag_id;                    // 0x006e472c
extern int16_t hud_text_draw_color_or_flags;                 // 0x006e4734

extern Rectangle2D text_measure_bounds;            // 0x006e4714, reset here, accumulated by the callback
extern uint32_t text_measure_font;                  // 0x006e471c, Font* of the last glyph measured

// Not a Ghidra function (out/phase4/text_types_notes.md); owns the writes to
// text_measure_bounds / text_measure_font while text_wrap_and_draw_wide runs.
extern void text_measure_glyph_callback(text_parse_state *state, void *font, void *character,
    uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y,
    int16_t width, int16_t height); // 0x556260

extern void text_wrap_and_draw_wide(text_glyph_draw_proc callback, Rectangle2D *bounds,
    Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string); // 0x556780

// blam-cc: EBX -> origin_bounds, ESI -> out_cursor_rect, EDI -> out_extents_rect, stack -> string
void text_measure_string_extents(Rectangle2D *origin_bounds, Rectangle2D *out_cursor_rect,
    Rectangle2D *out_extents_rect, void *string)
{
    Font *font;
    datum_index resolved_font;
    Point2DInt pen;

    text_measure_bounds.top = 0x7fff;
    text_measure_bounds.left = 0x7fff;
    text_measure_bounds.bottom = (int16_t)0x8000;
    text_measure_bounds.right = (int16_t)0x8000;

    resolved_font = hud_text_draw_font_tag_id;
    if (hud_text_draw_color_or_flags != (int16_t)-1) {
        Font *base_font = (Font *)tag_instances[hud_text_draw_font_tag_id & 0xffff].data;
        // style indexes the four style dependencies (bold=0, italic=1, condense=2,
        // underline=3); see text_parse_state_initialize.c for the confirmed offsets.
        TagDependency *style_dependency = &base_font->bold + hud_text_draw_color_or_flags;
        resolved_font = *(datum_index *)&style_dependency->tag_id;
    }
    if (resolved_font == (datum_index)0xffffffff) {
        resolved_font = hud_text_draw_font_tag_id;
    }
    text_measure_font = (uint32_t)tag_instances[resolved_font & 0xffff].data;

    text_wrap_and_draw_wide(text_measure_glyph_callback, origin_bounds, &pen,
        (Rectangle2D *)0, 0, string);

    font = (Font *)text_measure_font;

    out_cursor_rect->left = pen.x;
    out_cursor_rect->right = (int16_t)(pen.x + 1);
    out_cursor_rect->top = (int16_t)(pen.y - font->ascending_height);
    out_cursor_rect->bottom = (int16_t)(pen.y + font->descending_height);

    out_extents_rect->top = origin_bounds->top;
    out_extents_rect->left = text_measure_bounds.left;
    out_extents_rect->right = text_measure_bounds.right;
    out_extents_rect->bottom = out_cursor_rect->bottom;
}

#if 0
Original Ghidra decompilation (0x5562d0):

void FUN_005562d0(void)

{
  undefined2 uVar1;
  undefined2 uVar2;
  int iVar3;
  uint uVar4;
  undefined4 in_ECX;
  undefined2 *unaff_EBX;
  short *unaff_ESI;
  undefined2 *unaff_EDI;
  short sStack_2;

  DAT_006e4714 = 0x7fff;
  DAT_006e4716 = 0x7fff;
  DAT_006e4718 = 0x8000;
  DAT_006e471a = 0x8000;
  uVar4 = DAT_006e472c;
  if ((short)DAT_006e4734 != -1) {
    uVar4 = *(uint *)(*(int *)((DAT_006e472c & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x48 +
                     (short)DAT_006e4734 * 0x10);
  }
  if (uVar4 == 0xffffffff) {
    uVar4 = DAT_006e472c;
  }
  DAT_006e471c = *(int *)((uVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  FUN_00556780(&LAB_00556260);
  iVar3 = DAT_006e471c;
  unaff_ESI[1] = (short)in_ECX;
  unaff_ESI[3] = (short)in_ECX + 1;
  sStack_2 = (short)((uint)in_ECX >> 0x10);
  *unaff_ESI = sStack_2 - *(short *)(iVar3 + 4);
  uVar1 = DAT_006e4716;
  unaff_ESI[2] = *(short *)(iVar3 + 6) + sStack_2;
  uVar2 = DAT_006e471a;
  unaff_EDI[1] = uVar1;
  *unaff_EDI = *unaff_EBX;
  unaff_EDI[3] = uVar2;
  unaff_EDI[2] = unaff_ESI[2];
  return;
}

--- objdump -d -M intel (0x5562d0..0x5563b0) ---

005562d0 <.text+0x1552d0>:
  5562d0: push ecx
  5562d1: mov cx,[0x6e4734]                    ; cx = text_style_state
  5562d8: cmp cx,0xffff
  5562dc: mov edx,[0x87bc14]                     ; edx = tag_instances
  5562e2: mov eax,0x7fff
  5562e7: mov [0x6e4714],ax                       ; text_measure_bounds.top = 0x7fff
  5562ed: mov [0x6e4716],ax                        ; text_measure_bounds.left = 0x7fff
  5562f3: mov eax,0xffff8000
  5562f8: push ebp
  5562f9: mov ebp,[0x6e472c]                        ; ebp = text_font
  5562ff: mov [0x6e4718],ax                          ; text_measure_bounds.bottom = 0x8000
  556305: mov [0x6e471a],ax                           ; text_measure_bounds.right = 0x8000
  55630b: mov eax,ebp
  55630d: je 0x556325                                  ; style == -1: skip dependency resolve
  55630f: and eax,0xffff
  556314: shl eax,0x5
  556317: mov eax,[eax+edx+0x14]                        ; eax = tag_instances[font_idx].data (Font*)
  55631b: movsx ecx,cx
  55631e: shl ecx,0x4
  556321: mov eax,[eax+ecx+0x48]                          ; eax = style dependency tag_id
  556325: cmp eax,0xffffffff
  556328: jne 0x55632c
  55632a: mov eax,ebp                                      ; fallback: eax = text_font
  55632c: and eax,0xffff
  556331: shl eax,0x5
  556334: mov ecx,[eax+edx+0x14]                            ; ecx = tag_instances[idx].data (Font*)
  556338: mov edx,[esp+0xc]                                  ; string (stack arg)
  55633c: push edx
  55633d: push 0x0                                            ; clip = NULL
  55633f: push 0x0                                             ; extra_line_spacing = 0
  556341: lea eax,[esp+0x10]                                    ; &pen (local Point2DInt)
  556345: push eax
  556346: push ebx                                               ; origin_bounds
  556347: push 0x556260                                           ; callback
  55634c: mov [0x6e471c],ecx                                       ; text_measure_font = Font*
  556352: call 0x556780                                              ; text_wrap_and_draw_wide
  556357: mov eax,[esp+0x1c]                                          ; eax = pen (x low16, y high16)
  55635b: mov ecx,[0x6e471c]                                           ; ecx = text_measure_font (post-call)
  556361: mov [esi+0x2],ax                                              ; cursor_rect.left = pen.x
  556365: add esp,0x18
  556368: inc eax
  556369: mov [esi+0x6],ax                                              ; cursor_rect.right = pen.x + 1
  55636d: mov ax,[esp+0x6]                                              ; ax = pen.y
  556372: mov dx,ax
  556375: sub dx,[ecx+0x4]                                              ; dx = pen.y - font->ascending_height
  556379: pop ebp
  55637a: mov [esi],dx                                                  ; cursor_rect.top = dx
  55637d: mov cx,[ecx+0x6]                                              ; cx = font->descending_height
  556381: mov dx,[0x6e4716]                                             ; dx = text_measure_bounds.left
  556388: add cx,ax                                                     ; cx = pen.y + descending_height
  55638b: mov [esi+0x4],cx                                              ; cursor_rect.bottom = cx
  55638f: mov cx,[0x6e471a]                                             ; cx = text_measure_bounds.right
  556396: mov [edi+0x2],dx                                              ; extents_rect.left = dx
  55639a: mov ax,[ebx]                                                  ; ax = origin_bounds->top
  55639d: mov [edi],ax                                                  ; extents_rect.top = ax
  5563a0: mov [edi+0x6],cx                                              ; extents_rect.right = cx
  5563a4: mov dx,[esi+0x4]                                              ; dx = cursor_rect.bottom
  5563a8: mov [edi+0x4],dx                                              ; extents_rect.bottom = dx
  5563ac: pop ecx
  5563ad: ret
#endif
