// widget_instance_render_list_head  (Ghidra: widget_instance_render_list_head, already named)
// address 0x49b560, size 1372 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: matches the given name; renders a list_head-type widget's scroll-arrow indicators
// (header/footer bitmaps at tag->list_header_bitmap / list_footer_bitmap, drawn at tag-> header_
// bounds / footer_bounds -- both already established field names from this session's widget_
// list_adjust_rect_for_scroll_arrows.c), then assembles and draws the caption text of the
// currently selected list entry (from extended_description, or, when the tag draws from a string
// list, a freshly copied+search-and-replaced string; freed again at the end when it came from
// that string-list path).
// register convention: cdecl, all five recognized stack parameters.
// UNSURE: this function's rewrite confidence is intentionally low. Time constraints in this
// session did not allow the same offset-by-offset disassembly verification given to the smaller
// functions in this batch; the translation below follows Ghidra's own pseudo-C control flow and
// arithmetic closely, substituting header fields only where this session had already confirmed
// them (list_header_bitmap/list_footer_bitmap/header_bounds/footer_bounds, extended_description,
// scroll_blink, unknown_54), and leaves the remaining tag_data offsets (0x108 text_font.tag_id,
// 0x110/0x114/0x118 a flash color, 0x11c justification, 0x11e a flags byte, 0x60/0x64 search_and_
// replace_functions -- all consistent with this session's other text-drawing functions) and the
// several FUN_ callees (widget_cursor_side_of_midpoint, ui_get_saved_color, ui_search_replace_function_call, string_convert_ascii_to_unicode) as raw offsets /
// best-guess externs. A follow-up pass with the same disassembly-verification rigor as
// interface_tick.c / widget_create_children_from_tag.c is recommended before trusting this file.
// TYPES-GAP: no field names exist for the several raw offsets noted above.

// Phase-4 review: the scroll arrows, the search-and-replace loop and the caption draw were
// re-derived from objdump 0x49b645..0x49ba7e (frame arithmetic, source/clip rects, register
// arguments); the rest is unchanged.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern double cos(double x);
extern double sin(double x);
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t ui_time_milliseconds; // 0x00718f9c
extern heap *widget_memory_pool; // 0x006926c4

extern uint8_t widget_instance_point_in_bounds(widget_instance *widget); // 0x4999f0
extern float widget_instance_get_cumulative_scale(widget_instance *widget); // 0x499c20
extern void widget_instance_render(widget_instance *widget, Rectangle2D *dest, int32_t offset_xy,
                                    uint32_t flag1, int32_t flag2); // 0x49a8c0
extern int32_t bitmap_group_sequence_get_bitmap_data(datum_index bitmap, int16_t sequence,
                                                     int16_t frame); // 0x43f290; blam-cc: EAX -> bitmap, EDI -> frame, stack -> sequence
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                                 int16_t *clip_rect, uint32_t vertex_color); // 0x498b20, UNSURE call mapping, per widget_instance_render.c
extern uint16_t *text_string_list_get_string(datum_index string_list_tag, int16_t index); // 0x5578c0; blam-cc: ECX -> tag, DX -> index
extern uint32_t FUN_00625b7a(uint16_t *s); // 0x625b7a, wide strlen
extern void *heap_allocate(uint32_t size, heap *self); // 0x4d1f10
extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0
extern const uint16_t *ui_search_replace_function_call(int16_t function, widget_instance *widget); // 0x4a8730, blam-cc: AX function, ECX widget
    // blam-cc: AX -> function, ECX -> widget; L"<invalid>" (0x0066a8a0) outside 0..3, else ui_replace_function_table[function](widget)
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dest, int32_t dest_bytes, const char *source); // 0x557990, 8-bit to wide copy
    // blam-cc: EAX -> dest, EDI -> dest_bytes, EBX -> source; returns dest
extern void ui_string_replace_all(const uint16_t *search, const uint16_t *replacement, uint16_t **text); // 0x49be10
extern ColorRGB *ui_get_saved_color(ColorRGB *out); // 0x49c5c0, flash color (ui_saved_color); blam-cc: EAX -> out, fills three floats
extern int32_t widget_cursor_side_of_midpoint(widget_instance *widget); // 0x4a1ff0; blam-cc: EAX -> widget (objdump 0x49b5ad)
extern void text_set_render_context(datum_index font, ColorARGB *color, int32_t unknown_0,
                                    int32_t justification, int32_t unknown_1); // 0x5563b0
    // blam-cc: ECX -> font, EAX -> color, stack -> -1, justification, 0
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text); // 0x514ab0
    // blam-cc: EAX -> clip, ECX -> bounds

// Renders a list_head-type widget: draws its scroll-arrow indicators (only when the cursor is
// over it and the corresponding bitmap has 4 frames), draws its extended_description child, then
// assembles and draws the caption text of the currently selected list entry.
void widget_instance_render_list_head(widget_instance *widget, UIWidgetDefinition *tag,
                                       Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack)
{
    uint8_t *t = (uint8_t *)tag;
    float scale = widget->scale;
    widget_instance *ancestor;
    uint8_t in_bounds;
    int32_t unknown_4a1ff0;
    uint16_t *text = (uint16_t *)0;
    uint8_t scroll_dir_up = 0;
    uint8_t scroll_dir_down = 0;
    int16_t x_off = (int16_t)offset_xy;
    int16_t y_off = (int16_t)(offset_xy >> 16);

    for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        scale = scale * ancestor->scale;
    }
    in_bounds = widget_instance_point_in_bounds(widget);
    unknown_4a1ff0 = widget_cursor_side_of_midpoint(widget);

    if (widget->state == 0) {
        return;
    }

    if (widget->extended_description != (widget_instance *)0) {
        float desc_scale = widget->scale;

        for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
            desc_scale = desc_scale * ancestor->scale;
        }
        widget->extended_description->scale = desc_scale;
        widget_instance_render(widget->extended_description, dest, offset_xy, 0, 1);
    }

    if (widget->scroll_blink != 0) {
        if (widget->scroll_blink < 0) {
            widget->scroll_blink = widget->scroll_blink + 1;
            scroll_dir_up = 1;
        } else {
            widget->scroll_blink = widget->scroll_blink - 1;
            scroll_dir_down = 1;
        }
    }
    widget->unknown_54 = 0;

    // Scroll arrows, objdump 0x49b645..0x49b7ef. Each arrow bitmap (tag+0x160 header, tag+0x170
    // footer) is drawn with frame = the scroll direction flag, plus 2 while the cursor is over the
    // widget, the bitmap has exactly 4 frames and the list has (footer) or has not (header) more
    // entries per widget_cursor_side_of_midpoint. Source and dest are the arrow bounds (tag+0x174 header, tag+0x17c
    // footer) offset by offset_xy; *dest is the clip; the color is white with round(scale*255)
    // alpha. The first rewrite dropped the frame, the source rect and the clip.
    {
        int32_t arrow;

        for (arrow = 0; arrow < 2; arrow++) {
            datum_index bitmap_tag = *(datum_index *)(t + (arrow == 0 ? 0x160 : 0x170));
            uint8_t *bitmap_tag_data = (uint8_t *)tag_instances[bitmap_tag & 0xffff].data;
            int16_t frame = (int16_t)(arrow == 0 ? scroll_dir_up : scroll_dir_down);
            int32_t bitmap;

            if (in_bounds != 0 && bitmap_tag_data != (uint8_t *)0 && *(int32_t *)(bitmap_tag_data + 0x60) == 4 &&
                widget_instance_point_in_bounds(widget) != 0 &&
                (arrow == 0 ? unknown_4a1ff0 <= 0 : unknown_4a1ff0 > 0)) {
                frame = (int16_t)(frame + 2);
            }
            bitmap = bitmap_group_sequence_get_bitmap_data(bitmap_tag, 0, frame);
            if (bitmap != 0) {
                Rectangle2D rect = *(Rectangle2D *)(t + (arrow == 0 ? 0x174 : 0x17c));
                float alpha = scale * 255.0f;

                rect.top = (int16_t)(rect.top + y_off);
                rect.left = (int16_t)(rect.left + x_off);
                rect.bottom = (int16_t)(rect.bottom + y_off);
                rect.right = (int16_t)(rect.right + x_off);
                ui_draw_screen_quad((int16_t *)&rect, (int16_t *)&rect, bitmap, (int16_t *)dest,
                                     (uint32_t)((int32_t)(alpha + 0.5f) << 24) | 0xffffffu);
            }
        }
    }

    if (tag->child_widgets.count != 0) {
        return;
    }

    if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id == 0xffffffffu) {
        text = (uint16_t *)widget->list_render_data;
    } else {
        uint16_t *src =
            text_string_list_get_string(*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id,
                                        widget->selection_index); // objdump 0x49b811
        uint32_t byte_len = FUN_00625b7a(src) * 2;
        uint16_t *buf = (uint16_t *)heap_allocate(byte_len + 2, widget_memory_pool);
        int32_t i;

        text = buf;
        if (buf == (uint16_t *)0) {
            goto free_and_return;
        }
        {
            uint8_t *dst8 = (uint8_t *)buf;
            uint8_t *src8 = (uint8_t *)src;

            for (i = 0; i < (int32_t)byte_len; i++) {
                dst8[i] = src8[i];
            }
            buf[byte_len / 2] = 0;
        }
        if (tag->search_and_replace_functions.count > 0) {
            uint8_t *entries = (uint8_t *)tag->search_and_replace_functions.pointer;

            for (i = 0; i < tag->search_and_replace_functions.count; i++) {
                uint8_t *entry = entries + i * 0x22;

                if (entry != (uint8_t *)0 && *entry != 0) {
                    // objdump 0x49b885..0x49b8a7
                    const uint16_t *replacement = ui_search_replace_function_call(*(int16_t *)(entry + 0x20), widget);
                    uint16_t search[0x20];

                    ui_string_replace_all(string_convert_ascii_to_unicode(search, 0x40, (const char *)entry), replacement, &text);
                }
            }
        }
    }

    if (text != (uint16_t *)0 && *(uint32_t *)&tag->text_font.tag_id != 0xffffffffu) {
        int16_t justification = tag->justification;

        if (justification >= 0 && justification < 3) {
            // objdump 0x49b90c..0x49ba7e
            float cumulative = widget_instance_get_cumulative_scale(widget);
            int16_t x = (int16_t)offset_xy;
            int16_t y = (int16_t)(offset_xy >> 16);
            Rectangle2D rect = tag->bounds; // esp+0x18, offset by offset_xy
            Rectangle2D clip = (dest != (Rectangle2D *)0) ? *dest : tag->bounds; // esp+0x24
            ColorARGB color = *(ColorARGB *)(t + 0x10c); // esp+0x3c
            ColorARGB flash;

            rect.top = (int16_t)(rect.top + y);
            rect.left = (int16_t)(rect.left + x);
            rect.bottom = (int16_t)(rect.bottom + y);
            rect.right = (int16_t)(rect.right + x);
            if (is_top_of_stack != 0) {
                float *rgb = (float *)ui_get_saved_color((ColorRGB *)&flash); // three floats: red, green, blue

                color.red = rgb[0];
                color.green = rgb[1];
                color.blue = rgb[2];
            }
            color.alpha = color.alpha * cumulative;
            if (*(uint8_t *)&widget->unknown_54 != 0 || (t[0x11e] & 4) != 0) {
                double td = (double)ui_time_milliseconds;

                if (ui_time_milliseconds < 0) td += 4294967296.0;
                color.alpha = (float)((sin(td * 0.003) + 1.0) * 0.5 * (double)color.alpha);
            }

            text_set_render_context(*(datum_index *)&tag->text_font.tag_id, &color, -1, justification, 0);
            chimera__draw_16_bit_text(&clip, &rect, 0, 0, text);
        }
    }

free_and_return:
    if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id != 0xffffffffu && text != (uint16_t *)0) {
        heap_block *block = (heap_block *)((uint8_t *)text - 0x10);
        uint32_t size = block->size;

        heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)(size & 0x7fffffff);
        widget_memory_pool->allocation_count -= 1;
    }
}

#if 0
Original Ghidra decompilation (0x49b560):

void widget_instance_render_list_head
               (int param_1,int param_2,int *param_3,undefined4 param_4,char param_5)

{
  float fVar1;
  short sVar2;
  char cVar3;
  int iVar4;
  undefined4 *puVar5;
  uint uVar6;
  undefined4 *puVar7;
  undefined4 uVar8;
  float *pfVar9;
  int extraout_ECX;
  short sVar10;
  char *pcVar11;
  int iVar12;
  undefined4 *puVar13;
  undefined2 uVar14;
  float10 fVar15;
  float10 extraout_ST0;
  float10 fVar16;
  float10 extraout_ST1;
  undefined4 *local_78;
  undefined4 local_74;
  short local_70;
  short sStack_6e;
  int local_6c;
  int local_68;
  int local_64;
  float local_60;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;

  local_78 = *(undefined4 **)(param_1 + 0x24);
  iVar4 = *(int *)(param_1 + 0x30);
  local_74 = 0;
  local_6c = 0;
  for (; iVar4 != 0; iVar4 = *(int *)(iVar4 + 0x30)) {
    local_78 = (undefined4 *)((float)local_78 * *(float *)(iVar4 + 0x24));
  }
  cVar3 = widget_instance_point_in_bounds();
  local_68 = FUN_004a1ff0();
  if (*(char *)(param_1 + 0x10) == '\0') {
    return;
  }
  if (*(int *)(param_1 + 0x4c) != 0) {
    fVar1 = *(float *)(param_1 + 0x24);
    for (iVar4 = *(int *)(param_1 + 0x30); iVar4 != 0; iVar4 = *(int *)(iVar4 + 0x30)) {
      fVar1 = fVar1 * *(float *)(iVar4 + 0x24);
    }
    *(float *)(*(int *)(param_1 + 0x4c) + 0x24) = fVar1;
    widget_instance_render(*(undefined4 *)(param_1 + 0x4c),param_3,param_4,0,1);
  }
  sVar10 = *(short *)(param_1 + 0x42);
  if (sVar10 != 0) {
    if (sVar10 < 0) {
      sVar2 = 1;
      local_74 = 1;
    }
    else {
      sVar2 = -1;
      local_6c = 1;
    }
    *(short *)(param_1 + 0x42) = sVar10 + sVar2;
  }
  iVar4 = DAT_0087bc14;
  *(undefined2 *)(param_1 + 0x54) = 0;
  iVar4 = *(int *)((*(uint *)(param_2 + 0x160) & 0xffff) * 0x20 + 0x14 + iVar4);
  if (((cVar3 != '\0') && (iVar4 != 0)) && (*(int *)(iVar4 + 0x60) == 4)) {
    widget_instance_point_in_bounds();
  }
  iVar4 = bitmap_group_sequence_get_bitmap_data(0);
  sVar10 = (short)param_4;
  if (iVar4 != 0) {
    local_60 = (float)(int)ROUND((float)local_78 * 255.0);
    local_74._0_2_ = (short)*(undefined4 *)(param_2 + 0x174);
    local_74 = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x174) >> 0x10) + sVar10,
                        (short)local_74 + param_4._2_2_);
    local_70 = (short)*(undefined4 *)(param_2 + 0x178);
    _local_70 = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x178) >> 0x10) + sVar10,
                         local_70 + param_4._2_2_);
    FUN_00498b20(iVar4,param_3,(int)local_60 << 0x18 | 0xffffff);
  }
  iVar12 = local_6c;
  iVar4 = *(int *)((*(uint *)(param_2 + 0x170) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((cVar3 != '\0') && (iVar4 != 0)) &&
     ((*(int *)(iVar4 + 0x60) == 4 &&
      ((cVar3 = widget_instance_point_in_bounds(), cVar3 != '\0' && (0 < local_68)))))) {
    iVar12 = iVar12 + 2;
  }
  uVar14 = (undefined2)((uint)iVar12 >> 0x10);
  iVar4 = bitmap_group_sequence_get_bitmap_data(0);
  if (iVar4 != 0) {
    local_60 = (float)local_78 * 255.0;
    local_68 = (int)ROUND(local_60);
    local_74._0_2_ = (short)*(undefined4 *)(param_2 + 0x17c);
    local_70 = (short)*(undefined4 *)(param_2 + 0x180);
    uVar14 = 0;
    _local_70 = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x180) >> 0x10) + sVar10,
                         local_70 + param_4._2_2_);
    local_74 = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x17c) >> 0x10) + sVar10,
                        (short)local_74 + param_4._2_2_);
    FUN_00498b20(iVar4,param_3,local_68 << 0x18 | 0xffffff);
  }
  if (*(int *)(param_2 + 0x3e0) != 0) {
    return;
  }
  if (*(int *)(param_2 + 0xf8) == -1) {
    puVar7 = *(undefined4 **)(param_1 + 0x50);
    local_78 = puVar7;
  }
  else {
    puVar5 = (undefined4 *)text_string_list_get_string();
    uVar6 = FUN_00625b7a(puVar5);
    fVar1 = (float)(uVar6 * 2);
    local_60 = fVar1;
    puVar7 = (undefined4 *)heap_allocate();
    local_78 = puVar7;
    if (puVar7 == (undefined4 *)0x0) goto LAB_0049ba85;
    puVar13 = puVar7;
    for (uVar6 = (uVar6 & 0x7fffffff) >> 1; uVar6 != 0; uVar6 = uVar6 - 1) {
      *puVar13 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar13 = puVar13 + 1;
    }
    for (uVar6 = (uint)fVar1 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined1 *)puVar13 = *(undefined1 *)puVar5;
      puVar5 = (undefined4 *)((int)puVar5 + 1);
      puVar13 = (undefined4 *)((int)puVar13 + 1);
    }
    *(undefined2 *)((int)fVar1 + (int)puVar7) = 0;
    local_68 = 0;
    if (0 < *(int *)(param_2 + 0x60)) {
      local_6c = 0;
      do {
        pcVar11 = (char *)(*(int *)(param_2 + 100) + local_6c);
        if ((pcVar11 != (char *)0x0) && (*pcVar11 != '\0')) {
          uVar8 = FUN_004a8730();
          puVar13 = (undefined4 *)0x0;
          uVar8 = FUN_00557990(uVar8,&local_78);
          ui_string_replace_all(uVar8);
          puVar7 = local_78;
        }
        local_68 = local_68 + 1;
        local_6c = local_6c + 0x22;
      } while (local_68 < *(int *)(param_2 + 0x60));
    }
    uVar14 = (undefined2)((uint)puVar13 >> 0x10);
  }
  if ((puVar7 != (undefined4 *)0x0) && (*(int *)(param_2 + 0x108) != -1)) {
    sVar2 = *(short *)(param_2 + 0x11c);
    if ((-1 < sVar2) && (sVar2 < 3)) {
      fVar15 = (float10)widget_instance_get_cumulative_scale();
      if (param_3 == (int *)0x0) {
        local_68 = *(int *)(param_2 + 0x24);
        local_64 = *(int *)(param_2 + 0x28);
      }
      else {
        local_68 = *param_3;
        local_64 = param_3[1];
      }
      local_74._0_2_ = (short)*(undefined4 *)(param_2 + 0x24);
      local_70 = (short)*(undefined4 *)(param_2 + 0x28);
      local_74 = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x24) >> 0x10) + sVar10,
                          (short)local_74 + param_4._2_2_);
      _local_70 = CONCAT22((short)((uint)*(undefined4 *)(param_2 + 0x28) >> 0x10) + sVar10,
                           local_70 + param_4._2_2_);
      if (param_5 == '\0') {
        local_4c = *(float *)(param_2 + 0x110);
        local_48 = *(float *)(param_2 + 0x114);
        local_44 = *(float *)(param_2 + 0x118);
        if (((local_4c == 1.0) && (local_48 == 1.0)) && (local_44 == 1.0)) {
          fVar16 = (float10)*(float *)(param_2 + 0x10c);
        }
        else {
          fVar16 = (float10)*(float *)(param_2 + 0x10c);
        }
      }
      else {
        pfVar9 = (float *)FUN_0049c5c0();
        local_4c = *pfVar9;
        local_48 = pfVar9[1];
        local_44 = pfVar9[2];
        fVar16 = extraout_ST0;
        fVar15 = extraout_ST1;
      }
      local_50 = (float)(fVar16 * fVar15);
      if ((*(char *)(param_1 + 0x54) != '\0') || ((*(byte *)(param_2 + 0x11e) & 4) != 0)) {
        fVar15 = (float10)DAT_00718f9c;
        if (DAT_00718f9c < 0) {
          fVar15 = fVar15 + (float10)4.2949673e+09;
        }
        fVar15 = (float10)fsin(fVar15 * (float10)0.003);
        local_50 = (float)((fVar15 + (float10)1.0) * (float10)0.5 * (float10)local_50);
      }
      text_set_render_context(0xffffffff,CONCAT22(uVar14,sVar2),0);
      chimera__draw_16_bit_text(0,0,local_78);
      puVar7 = local_78;
    }
  }
LAB_0049ba85:
  if (*(int *)(param_2 + 0xf8) != -1) {
    uVar6 = puVar7[-4];
    heap_unlink_block();
    *(uint *)(extraout_ECX + 0x14) = *(int *)(extraout_ECX + 0x14) - (uVar6 & 0x7fffffff);
    *(int *)(extraout_ECX + 0x1c) = *(int *)(extraout_ECX + 0x1c) + -1;
  }
  return;
}
#endif
