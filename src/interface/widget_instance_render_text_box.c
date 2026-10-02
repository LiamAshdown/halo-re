// widget_instance_render_text_box  (Ghidra: widget_instance_render_text_box, already named)
// address 0x49b1d0, size 845 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: matches the given name (cea-pdb hint too, via the "<out of memory>" string).
// Refreshes the text_box's own text buffer from the tag's string-list reference when set,
// applies the tag's search-and-replace table (each entry's replace_function (0..3) dispatched
// through ui_replace_function_table at 0x692c08), then draws the text (formatted, if it contains
// a button-prompt token) once its font/justification/state checks pass.
// register convention: cdecl, all five recognized stack parameters.
// Rewritten from objdump 0x49b1d0..0x49b552 in the phase-4 review. Corrections over the first
// rewrite: the string index is widget->selection_index, or tag+0x12e when that is -1 (not 0);
// the buffer is heap_reallocate(old text, size, widget_memory_pool); each search string is an
// 8-bit string at the entry start, widened by string_convert_ascii_to_unicode into a 0x40 byte local and handed
// with the replacement and &widget->text to ui_string_replace_all @0x49be10 (three arguments);
// 0x0066a8a0 is the L"<invalid>" string itself; the draw rect is {top + y + tag+0x132,
// left + x + tag+0x130, bottom + y, right + x} from the tag bounds and offset_xy, followed by a
// second rect that is *dest (or the raw tag bounds) and serves as the clip of the plain draw;
// text_set_render_context gets the font in ECX and the color in EAX; the formatted draw passes
// the text in EDX. The pulse test reads a byte at widget+0x54.
// Text box widgets reuse widget_instance 0x44..0x53 as a ColorARGB text color override (alpha
// 0 means none); see the widget_instance note in types/interface.h.
// UNSURE: tag offsets 0x108 (text font), 0x10c (text color, ColorARGB), 0x11c (justification),
// 0x11e (flags byte, bit 2 pulses), 0x12e (string list index), 0x130/0x132 (text inset) are read
// through UIWidgetDefinition fields where tags.h names them and as raw offsets otherwise.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t ui_time_milliseconds; // 0x00718f9c
extern heap *widget_memory_pool;     // 0x006926c4
extern double cos(double x);
extern void *ui_replace_function_table[4]; // 0x00692c08, ui_search_replace_function
extern uint16_t ui_invalid_replacement_text[]; // 0x0066a8a0, L"<invalid>"
extern uint16_t ui_out_of_memory_text[];       // 0x00669ca8, L"<out of memory>"

extern uint16_t *text_string_list_get_string(datum_index string_list_tag, int16_t index); // 0x5578c0; blam-cc: ECX -> tag, DX -> index
extern uint32_t wcslen(const uint16_t *s); // 0x625b7a, wide strlen
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, src/memory; blam-cc: EAX -> old, ESI -> self
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dest, int32_t dest_bytes, const char *source); // 0x557990, 8-bit to wide copy
    // blam-cc: EAX -> dest, EDI -> dest_bytes, EBX -> source; returns dest
extern void ui_string_replace_all(const uint16_t *search, const uint16_t *replacement, uint16_t **text); // 0x49be10
extern float widget_instance_get_cumulative_scale(widget_instance *widget); // 0x499c20; blam-cc: EAX -> widget
extern ColorARGB *ui_get_saved_pulse_color(ColorARGB *out); // 0x49c620, highlight color; blam-cc: EAX -> out, returns a pointer to the color
extern void text_set_render_context(datum_index font, ColorARGB *color, int32_t unknown_0,
                                    int32_t justification, int32_t unknown_1); // 0x5563b0
    // blam-cc: ECX -> font, EAX -> color, stack -> -1, justification, 0
extern uint8_t ui_string_has_button_prompt_token(uint16_t *text); // 0x49ada0; blam-cc: EAX -> text
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text); // 0x514ab0
    // blam-cc: EAX -> clip, ECX -> bounds
extern void ui_widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color,
                                                   const uint16_t *text); // 0x49ade0; blam-cc: EDX -> text

// Prepares and draws a text-box widget's caption: refreshes it from the tag's string list (if
// set), applies search-and-replace substitutions, then draws it plain or through the
// button-prompt formatter, colored/scaled/pulsed per the tag and widget state.
void widget_instance_render_text_box(widget_instance *widget, UIWidgetDefinition *tag,
                                      Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack)
{
    uint8_t *w = (uint8_t *)widget;
    uint8_t *t = (uint8_t *)tag;
    int32_t i;

    if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id != 0xffffffffu) {
        int16_t index = widget->selection_index;
        uint16_t *src;
        uint32_t byte_len;
        uint16_t *buf;

        if (index == -1) {
            index = *(int16_t *)&((struct UIWidgetDefinition *)t)->string_list_index;
        }
        src = text_string_list_get_string(*(datum_index *)&tag->text_label_unicode_strings_list.tag_id, index);
        byte_len = wcslen(src) * 2;
        buf = (uint16_t *)heap_reallocate(widget->text, byte_len + 2, widget_memory_pool);
        widget->text = buf;
        if (buf == (uint16_t *)0) {
            widget->text = ui_out_of_memory_text;
        } else {
            uint8_t *dst8 = (uint8_t *)buf;
            uint8_t *src8 = (uint8_t *)src;

            for (i = 0; i < (int32_t)byte_len; i++) {
                dst8[i] = src8[i];
            }
            *(uint16_t *)((uint8_t *)widget->text + byte_len) = 0;
        }
    }

    if (widget->text == (void *)0 || *(uint16_t *)widget->text == 0) {
        return;
    }

    for (i = 0; i < tag->search_and_replace_functions.count; i++) {
        uint8_t *entry = (uint8_t *)tag->search_and_replace_functions.pointer + i * 0x22;

        if (entry != (uint8_t *)0 && *entry != 0) {
            int16_t fn = *(int16_t *)(entry + 0x20);
            const uint16_t *replacement;
            uint16_t search[0x20];

            if (fn < 0 || fn >= 4) {
                replacement = ui_invalid_replacement_text;
            } else {
                replacement = (const uint16_t *)((ui_search_replace_function)ui_replace_function_table[fn])(widget);
            }
            ui_string_replace_all(string_convert_ascii_to_unicode(search, 0x40, (const char *)entry), replacement,
                                  (uint16_t **)&widget->text);
        }
    }

    if (*(uint32_t *)&tag->text_font.tag_id == 0xffffffffu) {
        return;
    }
    if (tag->justification < 0 || tag->justification >= 3) {
        return;
    }
    if (widget->state == 0) {
        return;
    }

    {
        float scale = widget_instance_get_cumulative_scale(widget);
        int16_t x = (int16_t)offset_xy;
        int16_t y = (int16_t)(offset_xy >> 16);
        Rectangle2D rects[2]; // esp+0x1c: [0] the draw rect, [1] the clip rect
        ColorARGB color;      // esp+0x2c
        ColorARGB highlight;  // esp+0x3c, filled by ui_get_saved_pulse_color

        rects[1] = (dest != (Rectangle2D *)0) ? *dest : tag->bounds;
        rects[0].top = (int16_t)(tag->bounds.top + y + ((struct UIWidgetDefinition *)t)->vert_offset);
        rects[0].left = (int16_t)(tag->bounds.left + x + ((struct UIWidgetDefinition *)t)->horiz_offset);
        rects[0].bottom = (int16_t)(tag->bounds.bottom + y);
        rects[0].right = (int16_t)(tag->bounds.right + x);

        if (*(float *)&((struct widget_instance *)w)->list_items != 0.0f) {
            color = *(ColorARGB *)&((struct widget_instance *)w)->list_items; // text_box color override
            color.alpha = color.alpha * scale;
        } else if (is_top_of_stack != 0) {
            color = *ui_get_saved_pulse_color(&highlight);
            color.alpha = *(float *)&((struct UIWidgetDefinition *)t)->text_color * scale;
        } else {
            color = ((struct UIWidgetDefinition *)t)->text_color;
            if (color.red == 1.0f && color.green == 1.0f && color.blue == 1.0f) {
                color = *ui_get_saved_pulse_color(&highlight);
                color.alpha = *(float *)&((struct UIWidgetDefinition *)t)->text_color;
            }
            color.alpha = color.alpha * scale;
        }
        if (w[0x54] != 0 || (t[0x11e] & 4) != 0) {
            double time = (double)ui_time_milliseconds;

            if (ui_time_milliseconds < 0) {
                time += 4294967296.0;
            }
            color.alpha = (float)((cos(time * 0.003) + 1.5) * 0.4 * (double)color.alpha);
        }

        text_set_render_context(*(datum_index *)&tag->text_font.tag_id, &color, -1, tag->justification, 0);
        if (ui_string_has_button_prompt_token((uint16_t *)widget->text) == 0) {
            chimera__draw_16_bit_text(&rects[1], &rects[0], 0, 0, (uint16_t *)widget->text);
            return;
        }
        ui_widget_draw_formatted_prompt_string(&rects[0], 0, (uint16_t *)widget->text);
    }
}

#if 0
Original Ghidra decompilation (0x49b1d0):

void widget_instance_render_text_box
               (int param_1,int param_2,undefined4 *param_3,undefined4 param_4,char param_5)

{
  ushort uVar1;
  short sVar2;
  uint uVar3;
  char cVar4;
  undefined4 *puVar5;
  uint uVar6;
  undefined4 *puVar7;
  undefined **ppuVar8;
  undefined4 uVar9;
  int iVar10;
  char *pcVar11;
  int iVar12;
  float10 fVar13;
  float10 fVar14;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  int local_7c;
  short sStack_70;
  short sStack_6e;
  short sStack_6c;
  short sStack_6a;
  undefined4 uStack_68;
  undefined4 uStack_64;
  float fStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_54;

  if (*(int *)(param_2 + 0xf8) != -1) {
    puVar5 = (undefined4 *)text_string_list_get_string();
    uVar6 = FUN_00625b7a(puVar5);
    uVar3 = uVar6 * 2;
    puVar7 = (undefined4 *)heap_reallocate(uVar3 + 2);
    *(undefined4 **)(param_1 + 0x3c) = puVar7;
    if (puVar7 == (undefined4 *)0x0) {
      *(uint16_t **)(param_1 + 0x3c) = (uint16_t *)L"<out of memory>";
    }
    else {
      for (uVar6 = (uVar6 & 0x7fffffff) >> 1; uVar6 != 0; uVar6 = uVar6 - 1) {
        *puVar7 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar7 = puVar7 + 1;
      }
      for (uVar6 = uVar3 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined1 *)puVar7 = *(undefined1 *)puVar5;
        puVar5 = (undefined4 *)((int)puVar5 + 1);
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      *(undefined2 *)(uVar3 + *(int *)(param_1 + 0x3c)) = 0;
    }
  }
  iVar12 = 0;
  if (*(short **)(param_1 + 0x3c) == (short *)0x0) {
    return;
  }
  if (**(short **)(param_1 + 0x3c) == 0) {
    return;
  }
  if (0 < *(int *)(param_2 + 0x60)) {
    local_7c = 0;
    do {
      pcVar11 = (char *)(*(int *)(param_2 + 100) + local_7c);
      if ((pcVar11 != (char *)0x0) && (*pcVar11 != '\0')) {
        uVar1 = *(ushort *)(pcVar11 + 0x20);
        if (((short)uVar1 < 0) || (3 < uVar1)) {
          ppuVar8 = &PTR_DAT_0066a8a0;
        }
        else {
          ppuVar8 = (undefined **)(*(code *)(&PTR_LAB_00692c08)[(short)uVar1])(param_1);
        }
        uVar9 = FUN_00557990(ppuVar8,(undefined4 *)(param_1 + 0x3c));
        ui_string_replace_all(uVar9);
      }
      iVar12 = iVar12 + 1;
      local_7c = local_7c + 0x22;
    } while (iVar12 < *(int *)(param_2 + 0x60));
  }
  if (*(int *)(param_2 + 0x108) == -1) {
    return;
  }
  sVar2 = *(short *)(param_2 + 0x11c);
  if (sVar2 < 0) {
    return;
  }
  if (2 < sVar2) {
    return;
  }
  if (*(char *)(param_1 + 0x10) == '\0') {
    return;
  }
  fVar13 = (float10)widget_instance_get_cumulative_scale();
  if (param_3 == (undefined4 *)0x0) {
    uStack_68 = *(undefined4 *)(param_2 + 0x24);
    uStack_64 = *(undefined4 *)(param_2 + 0x28);
  }
  else {
    uStack_68 = *param_3;
    uStack_64 = param_3[1];
  }
  sStack_70 = (short)*(undefined4 *)(param_2 + 0x24);
  sStack_6c = (short)*(undefined4 *)(param_2 + 0x28);
  _sStack_6c = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x28) >> 0x10) + (short)param_4,
                        sStack_6c + param_4._2_2_);
  _sStack_70 = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x24) >> 0x10) + (short)param_4 +
                        *(short *)(param_2 + 0x130),
                        sStack_70 + param_4._2_2_ + *(short *)(param_2 + 0x132));
  if (*(float *)(param_1 + 0x44) == 0.0) {
    if (param_5 != '\0') {
      iVar10 = FUN_0049c620();
      fVar14 = (float10)*(float *)(param_2 + 0x10c);
      fStack_5c = *(float *)(iVar10 + 4);
      fStack_58 = *(float *)(iVar10 + 8);
      fStack_54 = *(float *)(iVar10 + 0xc);
      fVar13 = extraout_ST0;
      goto LAB_0049b3e3;
    }
    fStack_60 = *(float *)(param_2 + 0x10c);
    fStack_5c = *(float *)(param_2 + 0x110);
    fStack_58 = *(float *)(param_2 + 0x114);
    fStack_54 = *(float *)(param_2 + 0x118);
    if (((fStack_5c == 1.0) && (fStack_58 == 1.0)) && (fStack_54 == 1.0)) {
      iVar10 = FUN_0049c620();
      fVar14 = (float10)*(float *)(param_2 + 0x10c);
      fStack_5c = *(float *)(iVar10 + 4);
      fStack_58 = *(float *)(iVar10 + 8);
      fStack_54 = *(float *)(iVar10 + 0xc);
      fVar13 = extraout_ST0_00;
      goto LAB_0049b3e3;
    }
  }
  else {
    fStack_60 = *(float *)(param_1 + 0x44);
    fStack_5c = *(float *)(param_1 + 0x48);
    fStack_58 = *(float *)(param_1 + 0x4c);
    fStack_54 = *(float *)(param_1 + 0x50);
  }
  fVar14 = (float10)fStack_60;
LAB_0049b3e3:
  fStack_60 = (float)(fVar14 * fVar13);
  if ((*(char *)(param_1 + 0x54) != '\0') || ((*(byte *)(param_2 + 0x11e) & 4) != 0)) {
    fVar13 = (float10)DAT_00718f9c;
    if (DAT_00718f9c < 0) {
      fVar13 = fVar13 + (float10)4.2949673e+09;
    }
    fVar13 = (float10)fcos(fVar13 * (float10)0.003);
    fStack_60 = (float)((fVar13 + (float10)1.5) * (float10)0.4 * (float10)fStack_60);
  }
  text_set_render_context(0xffffffff,CONCAT22((short)((uint)iVar12 >> 0x10),sVar2),0);
  cVar4 = ui_string_has_button_prompt_token();
  if (cVar4 == '\0') {
    chimera__draw_16_bit_text(0,0,*(undefined4 *)(param_1 + 0x3c));
    return;
  }
  ui_widget_draw_formatted_prompt_string(&sStack_70,0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
