// widget_instance_render  (Ghidra: widget_instance_render, already named)
// address 0x49a8c0, size 879 bytes (0x49a8c0..0x49ac2e, single `ret`; Ghidra's metadata said 320. The
//   range 0x49aa00.. that was catalogued as the bogus function "ui_widget_load_by_name_or_tag" is
//   the middle of this body -- the background quad, type dispatch, child recursion and
//   post-render events below -- re-checked against objdump by orphan pass 4)
// name confidence: 0.6   rewrite confidence: 0.45
// evidence: matches the given name; called from widget_draw_split_screen_region and
// widget_draw_fullscreen_region with (widget, dest_rect, clip_rect_or_NULL, flag1, flag2), and
// recurses into itself for every child, updating flag1 (a byte-packed "is the focused chain"
// history) and flag2 (a "force override color" byte) along the way. Runs the tag's game_data_
// inputs (offset 0x48, stride 0x24) each frame, draws the background bitmap when present (with
// an "always_use_nifty_render_fx" pulsing-alpha special case when the tag flags bit 0x4 is set),
// dispatches to the type-specific renderer, recurses into children, then runs any "post_render"
// (event_type 0x21) event handlers.
// register convention: cdecl, all five recognized stack parameters.
// Background quad fixed from objdump 0x49a976..0x49aaac in the phase-4 review: the bitmap is
// looked up with the tag background bitmap (tag+0x44) in EAX and widget->background_bitmap_frame
// in DI; the quad gets the tag bounds offset by offset_xy as both source and dest (EAX = ECX),
// the offset *dest as clip (NULL without dest), and alpha * 255 rounded into the top byte of a
// packed white vertex color. The first rewrite passed the offset dest as the draw rect.
// UNSURE: widget_instance_render_column_list_items (0x49bac0, column_list renderer, just outside this module's range) and the
// per-game-data-input function table at 0x00692b18 are declared with best-guess signatures only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double cos(double x);
extern double sin(double x);
extern tag_instance *tag_instances; // 0x0087bc14
extern void *game_data_input_function_table[0x3b]; // 0x00692b18, UNSURE: indexed by GameDataInputReference::function
extern float override_color_00879f40; // 0x00879f40, UNSURE name
extern float override_color_00879f44; // 0x00879f44
extern float override_color_00879f48; // 0x00879f48
extern float override_color_00879f4c; // 0x00879f4c
extern int32_t ui_time_milliseconds; // 0x00718f9c

extern int32_t bitmap_group_sequence_get_bitmap_data(datum_index bitmap, int16_t sequence,
                                                     int16_t frame); // 0x43f290; blam-cc: EAX -> bitmap, EDI -> frame, stack -> sequence
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                                 int16_t *clip_rect, uint32_t vertex_color); // 0x498b20, UNSURE call mapping, see file header
extern uint8_t widget_instance_is_top_of_stack(widget_instance *widget); // 0x499cb0
extern void widget_instance_render_text_box(widget_instance *widget, UIWidgetDefinition *tag,
                                             Rectangle2D *dest, int32_t offset_xy,
                                             uint32_t flags); // 0x49b1d0
extern void widget_instance_render_list_head(widget_instance *widget, UIWidgetDefinition *tag,
                                              Rectangle2D *dest, int32_t offset_xy,
                                              uint32_t flags); // 0x49b560
extern void widget_instance_render_column_list_items(widget_instance *widget, UIWidgetDefinition *tag, Rectangle2D *dest,
                         int32_t offset_xy, uint32_t flags); // 0x49bac0, blam-cc: EDI widget
extern void ui_widget_list_item_activate(widget_instance *widget, UIWidgetDefinition *tag,
                                          int16_t *event, void *handler, uint8_t *out_handled); // 0x49a430

// Recursively renders a widget instance and all of its children: applies inherited scale/fade,
// runs the tag's per-frame game-data-input bindings, draws its background bitmap (if any) and
// per-type content, recurses into every child (tracking whether each is the focused one), then
// fires any bound "post_render" event handler.
void widget_instance_render(widget_instance *widget, Rectangle2D *dest, int32_t offset_xy,
                             uint32_t flag1, int32_t flag2)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    float scale = widget->scale;
    widget_instance *ancestor;
    int32_t i;

    for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        scale = scale * ancestor->scale;
    }

    if ((int8_t)flag2 == 0 && (tag->flags & 0x2000) != 0) { // always_use_nifty_render_fx
        flag2 = (flag2 & ~0xff) | 1;
    }

    offset_xy = (int32_t)(((int16_t)(offset_xy >> 16) + widget->local_y) << 16) |
                (uint16_t)((int16_t)offset_xy + widget->local_x);

    if (tag->game_data_inputs.count > 0) {
        uint8_t *entry = (uint8_t *)tag->game_data_inputs.pointer;

        for (i = 0; i < tag->game_data_inputs.count; i++, entry += 0x24) {
            int16_t function_id = *(int16_t *)entry;

            if (function_id >= 0 && function_id < 0x3b) {
                ((ui_game_data_input_function)game_data_input_function_table[function_id])(widget);
            }
        }
    }

    if (widget->state != 0) {
        int32_t bitmap_data = bitmap_group_sequence_get_bitmap_data(
            *(datum_index *)&((struct UIWidgetDefinition *)tag)->background_bitmap.tag_id, 0, widget->background_bitmap_frame);

        if (bitmap_data != 0) {
            float alpha = scale;
            int16_t x = (int16_t)offset_xy;
            int16_t y = (int16_t)(offset_xy >> 16);
            Rectangle2D bounds = tag->bounds;
            Rectangle2D clip;
            Rectangle2D *clip_arg = (Rectangle2D *)0;

            if ((int8_t)flag2 != 0) {
                override_color_00879f40 = 0.0f;
                override_color_00879f44 = 0.05f;
                override_color_00879f48 = 0.05f;
                override_color_00879f4c = 0.05f;
            }
            bounds.top = (int16_t)(bounds.top + y);
            bounds.left = (int16_t)(bounds.left + x);
            bounds.bottom = (int16_t)(bounds.bottom + y);
            bounds.right = (int16_t)(bounds.right + x);
            if (dest != (Rectangle2D *)0) {
                clip.top = (int16_t)(dest->top + y);
                clip.left = (int16_t)(dest->left + x);
                clip.bottom = (int16_t)(dest->bottom + y);
                clip.right = (int16_t)(dest->right + x);
                clip_arg = &clip;
            }
            if ((tag->flags & 4) != 0) { // flash_background_bitmap
                double t = (double)ui_time_milliseconds;

                if (ui_time_milliseconds < 0) t += 4294967296.0;
                alpha = (float)((cos(t * 0.003) + 1.0) * 0.5 * (double)alpha);
            }
            // fistp rounds to nearest; +0.5 and truncation differ only on exact .5 ties
            ui_draw_screen_quad((int16_t *)&bounds, (int16_t *)&bounds, bitmap_data, (int16_t *)clip_arg,
                                 (uint32_t)((int32_t)(alpha * 255.0f + 0.5f) << 24) | 0xffffffu);
            if ((int8_t)flag2 != 0) {
                override_color_00879f40 = 0.0f;
                override_color_00879f44 = 0.0f;
                override_color_00879f48 = 0.0f;
                override_color_00879f4c = 0.0f;
            }
        }
    }

    if (widget->widget_type == 1) { // text_box
        uint32_t use_flag1 = flag1;

        if ((tag->flags_1 & 8) == 0) { // UNSURE: bit 3 of flags_1, per this call's own gate
            use_flag1 = widget_instance_is_top_of_stack(widget);
        }
        widget_instance_render_text_box(widget, tag, dest, offset_xy, use_flag1 & 0xff);
    } else if (widget->widget_type == 2) { // spinner_list
        widget_instance_render_list_head(widget, tag, dest, offset_xy, flag1);
        if ((tag->flags_2 & 2) != 0 && tag->child_widgets.count == 0) {
            goto post_render;
        }
    } else if (widget->widget_type == 3) { // column_list
        widget_instance_render_column_list_items(widget, tag, dest, offset_xy, flag1);
        if ((tag->flags_2 & 1) != 0) {
            goto post_render;
        }
    }

    {
        widget_instance *child;

        for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
            uint32_t child_flag1 = (flag1 & 0xffffff00) | (child == widget->focused_child);
            int32_t child_flag2;

            if (child == widget->focused_child && (widget->widget_type == 2 || widget->widget_type == 3)) {
                child_flag2 = (flag2 & ~0xff) | 1;
            } else {
                child_flag2 = (flag2 >> 8) << 8;
            }
            widget_instance_render(child, dest, offset_xy, child_flag1, child_flag2);
        }
    }

post_render:
    if (tag->event_handlers.count > 0) {
        uint8_t *entry = (uint8_t *)tag->event_handlers.pointer;

        for (i = 0; i < tag->event_handlers.count; i++, entry += 0x48) {
            if (*(int16_t *)(entry + 4) == 0x21) { // uieventtype_post_render
                int16_t event[4] = {0, 0, 0, 0};
                uint8_t handled;

                event[1] = widget->controller_index; // matches Ghidra's sStack_6 = (short)param_1[2]
                ui_widget_list_item_activate(widget, tag, event, entry, &handled);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x49a8c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void widget_instance_render
               (uint *param_1,undefined4 *param_2,undefined4 param_3,uint param_4,int param_5)

{
  ushort uVar1;
  short sVar2;
  float fVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  short *psVar10;
  float10 fVar11;
  short sStack_8;
  short sStack_6;
  short sStack_4;
  short sStack_2;

  fVar3 = (float)param_1[9];
  iVar4 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  for (uVar6 = param_1[0xc]; uVar6 != 0; uVar6 = *(uint *)(uVar6 + 0x30)) {
    fVar3 = fVar3 * *(float *)(uVar6 + 0x24);
  }
  if (((char)param_5 == '\0') && ((*(uint *)(iVar4 + 0x2c) & 0x2000) != 0)) {
    param_5 = CONCAT31(param_5._1_3_,1);
  }
  iVar8 = 0;
  param_3 = CONCAT22(param_3._2_2_ + (short)param_1[3],
                     (short)param_3 + *(short *)((int)param_1 + 10));
  if (0 < *(int *)(iVar4 + 0x48)) {
    iVar9 = 0;
    do {
      uVar1 = *(ushort *)(iVar9 + *(int *)(iVar4 + 0x4c));
      if ((-1 < (short)uVar1) && (uVar1 < 0x3b)) {
        (*(code *)(&PTR_FUN_00692b18)[(short)uVar1])(param_1);
      }
      iVar8 = iVar8 + 1;
      iVar9 = iVar9 + 0x24;
    } while (iVar8 < *(int *)(iVar4 + 0x48));
  }
  if ((char)param_1[4] == '\0') {
    return;
  }
  iVar8 = bitmap_group_sequence_get_bitmap_data(0);
  if (iVar8 != 0) {
    fVar11 = (float10)fVar3;
    if ((char)param_5 != '\0') {
      _DAT_00879f40 = 0;
      _DAT_00879f44 = 0x3d4ccccd;
      _DAT_00879f48 = 0x3d4ccccd;
      _DAT_00879f4c = 0x3d4ccccd;
    }
    psVar10 = (short *)0x0;
    if (param_2 != (undefined4 *)0x0) {
      sStack_8 = (short)*param_2;
      sStack_4 = (short)param_2[1];
      sStack_8 = sStack_8 + param_3._2_2_;
      sStack_6 = (short)((uint)*param_2 >> 0x10) + (short)param_3;
      sStack_2 = (short)((uint)param_2[1] >> 0x10) + (short)param_3;
      sStack_4 = sStack_4 + param_3._2_2_;
      psVar10 = &sStack_8;
    }
    if ((*(byte *)(iVar4 + 0x2c) & 4) != 0) {
      fVar11 = (float10)DAT_00718f9c;
      if (DAT_00718f9c < 0) {
        fVar11 = fVar11 + (float10)4.2949673e+09;
      }
      fVar11 = (float10)fcos(fVar11 * (float10)0.003);
      fVar11 = (fVar11 + (float10)1.0) * (float10)0.5 * (float10)fVar3;
    }
    FUN_00498b20(iVar8,psVar10,(int)ROUND((float)(fVar11 * (float10)255.0)) << 0x18 | 0xffffff);
    if ((char)param_5 != '\0') {
      _DAT_00879f40 = 0;
      _DAT_00879f44 = 0;
      _DAT_00879f48 = 0;
      _DAT_00879f4c = 0;
    }
  }
  sVar2 = *(short *)((int)param_1 + 0xe);
  if (sVar2 == 1) {
    uVar6 = param_4;
    if ((*(byte *)(iVar4 + 0x11e) & 8) == 0) {
      uVar6 = FUN_00499cb0();
    }
    widget_instance_render_text_box(param_1,iVar4,param_2,param_3,uVar6 & 0xff);
  }
  else if (sVar2 == 2) {
    widget_instance_render_list_head(param_1,iVar4,param_2,param_3,param_4);
    if (((*(byte *)(iVar4 + 0x150) & 2) != 0) && (*(int *)(iVar4 + 0x3e0) == 0)) goto LAB_0049abd1;
  }
  else if ((sVar2 == 3) &&
          (FUN_0049bac0(iVar4,param_2,param_3,param_4), (~*(byte *)(iVar4 + 0x150) & 1) == 0))
  goto LAB_0049abd1;
  for (uVar6 = param_1[0xd]; uVar6 != 0; uVar6 = *(uint *)(uVar6 + 0x2c)) {
    uVar5 = param_4 >> 8;
    param_4 = CONCAT31((int3)uVar5,uVar6 == param_1[0xe]);
    param_5._1_3_ = (uint3)((uint)param_5 >> 8);
    if ((uVar6 == param_1[0xe]) &&
       ((*(short *)((int)param_1 + 0xe) == 2 || (*(short *)((int)param_1 + 0xe) == 3)))) {
      param_5 = CONCAT31(param_5._1_3_,1);
    }
    else {
      param_5 = (uint)param_5._1_3_ << 8;
    }
    widget_instance_render(uVar6,param_2,param_3,param_4,param_5);
  }
LAB_0049abd1:
  iVar8 = 0;
  if (0 < *(int *)(iVar4 + 0x54)) {
    iVar9 = 0;
    do {
      iVar7 = *(int *)(iVar4 + 0x58) + iVar9;
      if (*(short *)(iVar7 + 4) == 0x21) {
        sStack_6 = (short)param_1[2];
        sStack_4 = 0;
        sStack_2 = 0;
        sStack_8 = 0;
        ui_widget_list_item_activate(param_1,iVar4,&sStack_8,iVar7,&param_3);
      }
      iVar8 = iVar8 + 1;
      iVar9 = iVar9 + 0x48;
    } while (iVar8 < *(int *)(iVar4 + 0x54));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
