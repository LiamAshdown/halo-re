// ui_widget_draw_formatted_prompt_string  (Ghidra: ui_widget_draw_formatted_prompt_string,
// already named)
// address 0x49ade0, size 983 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: matches the given name; splits the source string on '%' and draws each plain-text
// span, then resolves what follows each '%' as a button-prompt token
// (ui_button_prompt_index_from_string) and draws either a literal percent sign, a quoted key
// name, or the HUDGlobalsButtonIcon of the token.
// register convention: cdecl, two stack parameters (bounds, use_text_color); the source text in
// EDX. Ghidra recognized only the first stack parameter; [ebp+0xc] is read at 0x49b0ae.
// blam-cc: stack -> bounds, use_text_color; EDX -> text
// Rewritten from objdump 0x49ade0..0x49b1b9 in the phase-4 review. The earlier rewrite modeled
// the state as a 4 byte {color, x} pair; it is the first half of a Rectangle2D. The routine keeps
// a local copy R of *bounds as the running layout cursor, and after every span writes R.top back
// into bounds->top. Other corrections: the measuring routine 0x5562d0 takes the origin rect in
// EBX, the cursor rect in ESI and an output rect in EDI; the constants at 0x00669cd4 (L"%"),
// 0x00669cd0 (a double quote) and 0x00669cc8 (L"???") are the strings themselves, not
// pointers; a failed key-name lookup draws L"???" and a successful one draws the name in quotes
// (the two branches were swapped); the icon override flag table is 0x006926f4, not 0x00692796;
// after an unmapped token the cursor only retreats by another 3.
// UNSURE: the icon color built at 0x49b06a..0x49b154 (the global text color when the icon has no
// override color or use_text_color is set, premultiplied by the text alpha and packed to
// ARGB8888) is stored in a local that nothing reads; ui_button_prompt_draw_icon takes only the icon in ESI.
// It is reproduced for fidelity.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint16_t formatted_prompt_scratch[0x100]; // 0x006b2fe8, copy of the source text split in place
extern int16_t ui_prompt_clip_y; // 0x006e4770, word store
extern int16_t ui_prompt_clip_x; // 0x006e476e, clamped to >= 0
extern uint16_t prompt_percent_text[];     // 0x00669cd4, L"%"
extern uint16_t prompt_quote_text[];       // 0x00669cd0, a one character double quote string
extern uint16_t prompt_unknown_key_text[]; // 0x00669cc8, L"???"
extern int8_t prompt_key_token_table[];    // 0x00692796, indexed by token 0x12..0x1f
extern uint8_t prompt_icon_override_table[0x12]; // 0x006926f4, 1 for tokens 6, 7, 0xe..0x11
extern float hud_text_draw_color_a;        // 0x006e4738
extern float hud_text_draw_color_r;        // 0x006e473c
extern float hud_text_draw_color_g;        // 0x006e4740
extern float hud_text_draw_color_b;        // 0x006e4744
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern uint16_t *ui_button_caption[0x28];  // 0x00692708

extern void _wcscpy(uint16_t *dest, const uint16_t *src); // 0x625bba
extern uint16_t *_wcschr(uint16_t *s, uint16_t c);        // 0x625b58
extern uint32_t wcslen(const uint16_t *s);          // 0x625b7a, wide strlen
extern int32_t __ftol(double x);                          // 0x6391b4
extern void text_measure_string_extents(Rectangle2D *origin, Rectangle2D *cursor, Rectangle2D *out_bounds,
                         const uint16_t *text); // 0x5562d0, measure a span
    // blam-cc: EBX -> origin, ESI -> cursor (read and rewritten), EDI -> out_bounds, stack -> text
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text); // 0x514ab0
    // blam-cc: EAX -> clip (NULL here), ECX -> bounds
extern int16_t ui_button_prompt_index_from_string(uint16_t *text); // 0x49ac30; blam-cc: EBX -> text
extern void ui_widget_draw_prompt_span(const uint16_t *text, Rectangle2D *cursor, Rectangle2D *origin); // 0x49ad30
    // blam-cc: EAX -> cursor, ECX -> origin
extern uint8_t input_get_last_used_binding(int16_t key, uint8_t *out_binding); // 0x48bde0; blam-cc: EAX -> key, stack -> 12 byte out
extern void input_get_binding_display_name(uint8_t *binding, uint16_t *out_name); // 0x48c7f0; blam-cc: EAX -> binding, ECX -> out_name
extern void color_argb_int_to_real(ColorARGB *out, uint32_t packed); // 0x43f5a0; blam-cc: EAX -> out, ECX -> packed
extern void ui_button_prompt_draw_icon(HUDGlobalsButtonIcon *icon); // 0x49ac80; blam-cc: ESI -> icon

// Draws one span at the running cursor: clip offset is the non-negative distance the cursor has
// moved right of the origin, the span is measured, the cursor retreats 3, and the span is drawn
// with its left edge pinned to the origin. Same sequence as ui_widget_draw_prompt_span, inlined
// by the compiler at 0x49ae3f and 0x49aeb6.
static void draw_span_inline(Rectangle2D *origin, Rectangle2D *cursor, const uint16_t *text)
{
    Rectangle2D out;
    int16_t delta = (int16_t)(cursor->left - origin->left);

    ui_prompt_clip_y = 0;
    ui_prompt_clip_x = (delta < 0) ? 0 : delta;
    text_measure_string_extents(origin, cursor, &out, text);
    cursor->left = (int16_t)(cursor->left - 3);
    out.left = origin->left;
    chimera__draw_16_bit_text((Rectangle2D *)0, &out, 0, 0, text);
    origin->top = cursor->top;
}

// blam-cc: stack -> bounds, use_text_color; EDX -> text
// Renders a caption string mixing plain text with %token placeholders, drawing each text span
// and substituting the matching controller-button icon or key name for every recognised token.
void ui_widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color, const uint16_t *text)
{
    Rectangle2D cursor_rect = *bounds;
    uint16_t *cursor = formatted_prompt_scratch;

    _wcscpy(formatted_prompt_scratch, text);
    for (;;) {
        uint16_t *percent = _wcschr(cursor, 0x25);
        uint16_t *next;
        int16_t token;

        if (percent == (uint16_t *)0) {
            if (cursor != (uint16_t *)0) {
                ui_widget_draw_prompt_span(cursor, &cursor_rect, bounds);
            }
            ui_prompt_clip_x = 0;
            ui_prompt_clip_y = 0;
            return;
        }
        *percent = 0;
        next = percent + 1;
        draw_span_inline(bounds, &cursor_rect, cursor);
        cursor = next;

        token = ui_button_prompt_index_from_string(next);
        if (token == -1) {
            draw_span_inline(bounds, &cursor_rect, prompt_percent_text);
        } else {
            cursor = next + wcslen(ui_button_caption[token]);
            if (token > 0x11) {
                if (token > 0x1f) {
                    goto next_span;
                }
                if (token <= 0x1c) {
                    uint8_t binding[12];
                    uint16_t key_name[0x40];

                    if (input_get_last_used_binding((int16_t)prompt_key_token_table[token], binding) != 0) {
                        input_get_binding_display_name(binding, key_name);
                        ui_widget_draw_prompt_span(prompt_quote_text, &cursor_rect, bounds);
                        ui_widget_draw_prompt_span(key_name, &cursor_rect, bounds);
                        ui_widget_draw_prompt_span(prompt_quote_text, &cursor_rect, bounds);
                    } else {
                        ui_widget_draw_prompt_span(prompt_unknown_key_text, &cursor_rect, bounds);
                    }
                    goto next_span;
                }
                // jump table at 0x49b1bc; token 0x1c cannot reach it
                switch (token) {
                case 0x1d: token = 0xd; break;
                case 0x1e: token = 0x10; break;
                case 0x1f: token = 0x11; break;
                }
            }
            {
                HUDGlobalsButtonIcon *icon =
                    (HUDGlobalsButtonIcon *)*(uint8_t **)((uint8_t *)hud_globals_tag_data + 0xc8) + token;
                HUDInterfaceMessagingFlags saved_flags = icon->flags;
                int16_t saved_width = icon->width_offset;
                ColorARGB icon_color;
                ColorARGB text_color;
                uint32_t packed_color;

                color_argb_int_to_real(*(uint32_t *)&icon->override_icon_color, &icon_color);
                icon->flags = (HUDInterfaceMessagingFlags)(saved_flags & 0xfd);
                if (prompt_icon_override_table[token] != 0) {
                    icon->flags = (HUDInterfaceMessagingFlags)(icon->flags & 0xfb);
                    icon->width_offset = -5;
                }
                text_color.alpha = hud_text_draw_color_a;
                text_color.red = hud_text_draw_color_r;
                text_color.green = hud_text_draw_color_g;
                text_color.blue = hud_text_draw_color_b;
                packed_color = (uint32_t)__ftol(text_color.alpha * 255.0f) << 24; // dead, see header
                if (*(uint32_t *)&icon->override_icon_color == 0 || use_text_color != 0) {
                    icon_color = text_color;
                }
                icon_color.red = icon_color.red * text_color.alpha;
                icon_color.green = icon_color.green * text_color.alpha;
                icon_color.blue = icon_color.blue * text_color.alpha;
                packed_color = (uint32_t)(int32_t)(icon_color.blue * 255.0f) |
                               ((uint32_t)(int32_t)(icon_color.green * 255.0f) << 8) |
                               ((uint32_t)(int32_t)(icon_color.red * 255.0f) << 16) |
                               ((uint32_t)(int32_t)(icon_color.alpha * 255.0f) << 24); // dead, see header
                (void)packed_color;
                ui_button_prompt_draw_icon(icon);
                bounds->left = (int16_t)(bounds->left + 1);
                icon->flags = saved_flags;
                icon->width_offset = saved_width;
            }
        }
    next_span:
        if (cursor == (uint16_t *)0) {
            ui_prompt_clip_x = 0;
            ui_prompt_clip_y = 0;
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x49ade0):

/* WARNING: Restarted to delay deadcode elimination for space: stack */

void ui_widget_draw_formatted_prompt_string(undefined4 *param_1)

{
  short sVar1;
  undefined1 uVar2;
  byte bVar3;
  undefined2 uVar4;
  short sVar5;
  char cVar6;
  ushort uVar7;
  short sVar8;
  wchar_t *pwVar9;
  int iVar10;
  int iVar11;
  wchar_t *in_EDX;
  undefined4 local_f4;
  wchar_t *local_e4;
  undefined1 local_94 [12];
  undefined1 local_88 [132];

  local_f4 = *param_1;
  local_e4 = (wchar_t *)&DAT_006b2fe8;
  _wcscpy((wchar_t *)&DAT_006b2fe8,in_EDX);
  do {
    pwVar9 = _wcschr(local_e4,L'%');
    if (pwVar9 == (wchar_t *)0x0) {
      if (local_e4 != (wchar_t *)0x0) {
        FUN_0049ad30(local_e4);
      }
      DAT_006e476e = 0;
      DAT_006e4770 = 0;
      return;
    }
    *pwVar9 = L'\0';
    sVar5 = local_f4._2_2_;
    uVar7 = local_f4._2_2_ - *(short *)((int)param_1 + 2);
    DAT_006e4770 = 0;
    DAT_006e476e = uVar7 & ((short)uVar7 < 0) - 1;
    FUN_005562d0(local_e4);
    sVar1 = local_f4._2_2_ + -3;
    local_f4 = CONCAT22(sVar1,(undefined2)local_f4);
    chimera__draw_16_bit_text(0,0,local_e4);
    *(undefined2 *)param_1 = (undefined2)local_f4;
    sVar8 = ui_button_prompt_index_from_string();
    if (sVar8 == -1) {
      uVar7 = sVar1 - *(short *)((int)param_1 + 2);
      DAT_006e4770 = 0;
      DAT_006e476e = uVar7 & ((short)uVar7 < 0) - 1;
      FUN_005562d0(&DAT_00669cd4);
      local_f4 = CONCAT22(sVar5 + -6,(undefined2)local_f4);
      chimera__draw_16_bit_text(0,0,&DAT_00669cd4);
      *(undefined2 *)param_1 = (undefined2)local_f4;
      local_e4 = pwVar9 + 1;
    }
    else {
      iVar10 = (int)sVar8;
      iVar11 = FUN_00625b7a((&PTR_u_a_button_00692708)[iVar10]);
      local_e4 = pwVar9 + 1 + iVar11;
      if (sVar8 < 0x12) {
switchD_0049afe9_default:
        if (sVar8 != -1) {
LAB_0049b016:
          iVar10 = sVar8 * 0x10;
          uVar2 = *(undefined1 *)(iVar10 + 0xd + *(int *)(DAT_0071941c + 200));
          iVar10 = iVar10 + *(int *)(DAT_0071941c + 200);
          uVar4 = *(undefined2 *)(iVar10 + 2);
          color_argb_int_to_real();
          bVar3 = *(byte *)(iVar10 + 0xd);
          *(byte *)(iVar10 + 0xd) = bVar3 & 0xfd;
          if (*(char *)(sVar8 + 0x6926f4) != '\0') {
            *(byte *)(iVar10 + 0xd) = bVar3 & 0xf9;
            *(undefined2 *)(iVar10 + 2) = 0xfffb;
          }
          __ftol();
          FUN_0049ac80();
          *(short *)((int)param_1 + 2) = *(short *)((int)param_1 + 2) + 1;
          *(undefined1 *)(iVar10 + 0xd) = uVar2;
          *(undefined2 *)(iVar10 + 2) = uVar4;
        }
      }
      else if (sVar8 < 0x20) {
        if (0x1c < sVar8) {
          sVar8 = (short)(char)(&DAT_00692796)[iVar10];
          switch(iVar10) {
          case 0x1c:
            sVar8 = 0xc;
            break;
          case 0x1d:
            sVar8 = 0xd;
            break;
          case 0x1e:
            sVar8 = 0x10;
            break;
          case 0x1f:
            sVar8 = 0x11;
            break;
          default:
            goto switchD_0049afe9_default;
          }
          goto LAB_0049b016;
        }
        cVar6 = FUN_0048bde0(local_94);
        if (cVar6 == '\0') {
          FUN_0049ad30(&DAT_00669cc8);
        }
        else {
          FUN_0048c7f0();
          FUN_0049ad30(&DAT_00669cd0);
          FUN_0049ad30(local_88);
          FUN_0049ad30(&DAT_00669cd0);
        }
      }
    }
    if (local_e4 == (wchar_t *)0x0) {
      DAT_006e476e = 0;
      DAT_006e4770 = 0;
      return;
    }
  } while( true );
}
#endif
