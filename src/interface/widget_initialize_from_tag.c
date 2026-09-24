// widget_initialize_from_tag  (Ghidra: widget_initialize_from_tag, already named)
// address 0x499780, size 456 bytes
// name confidence: 0.6   rewrite confidence: 0.45
// evidence: matches the given name; types/interface.h's widget_instance note attributes nearly
// every field write in this function to this exact address, field by field (definition, name,
// controller_index, widget_type, render_always/pauses_game_time from tag flags bits 9/1,
// creation_time, milliseconds_to_auto_close/fade clamped to >=0, scale=1.0, parent,
// background_bitmap_frames from the background bitmap's own BitmapGroupSequence[0].bitmap_count,
// default focus search). Confirmed here that the destination widget itself is a THIRD hidden
// register argument (ECX, zeroed for 0x18 dwords first) alongside the tag index (EAX) and parent
// (EDX) Ghidra already flags as unresolved -- five real arguments in total, though
// chimera__load_ui_widget.c's own call site (already written, not amended here) only shows the
// two stack ones.
// register convention: new widget in ECX (in_ECX, the return-style output), tag index (datum_
// index) in EAX (in_EAX), parent widget in EDX (in_EDX), controller_index and tag_data as the two
// recognized stack parameters.
// blam-cc: ECX -> widget, EAX -> tag_index, EDX -> parent, stack -> controller_index/tag_data
// UNSURE: chimera__load_ui_widget.c's own call to this function only supplies the two stack
// parameters; the widget/tag_index/parent registers it must also set are not shown by that
// file's own (independently reviewed) decompile either. Extern arity/register disagreement, not
// resolved here.

// Phase-4 review: the default focus loop calls 0x49bba0 with this widget in EAX and each eligible
// child in ECX, for every eligible child (objdump 0x4998b7..0x499905); 0x00719720 is
// network_game_mode, compared as a word.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t ui_time_milliseconds; // 0x00718f9c
extern uint8_t widget_creating_children; // 0x00718fc3
extern int16_t network_game_mode; // 0x00719720, types/game.h: 0 local, 1 client, 2 host (word access)
extern uint8_t ui_split_screen; // 0x00718fc9
extern int16_t ui_pause_depth; // 0x00718fa6
extern game_time_globals *game_time; // 0x006f1d6c (types/game.h)

extern uint8_t widget_create_children_from_tag(widget_instance *widget, UIWidgetDefinition *tag); // 0x499540
extern void ui_widget_list_item_activate(widget_instance *widget, UIWidgetDefinition *tag,
                                          int16_t *event, void *handler, uint8_t *out_handled); // 0x49a430, UNSURE call shape here (Ghidra shows 0 args)
extern void widget_instance_relink_focus(widget_instance *widget, widget_instance *child); // 0x49bba0, focus change; blam-cc: EAX -> widget, ECX -> child (objdump 0x49bba0: walks EAX up +0x30, tests ECX+0x12)

// blam-cc: ECX -> widget, EAX -> tag_index, EDX -> parent
// Populates a freshly-allocated widget instance's fields from its tag definition, builds its
// static children (unless already inside a nested widget_create_children_from_tag call), fires
// any "created" (event code 0x18) event handlers for its own children's entries, picks a default
// focused_child among its own children when none was set, and, when this widget pauses game
// time, raises the shared ui_pause_depth counter and updates the console pause-request state.
void widget_initialize_from_tag(widget_instance *widget, datum_index tag_index, widget_instance *parent,
                                 uint16_t controller_index, UIWidgetDefinition *tag)
{
    int32_t i;

    {
        int32_t *zero = (int32_t *)widget;

        for (i = 0; i < 0x18; i++) {
            zero[i] = 0;
        }
    }

    if ((tag->flags_2 & 2) != 0 && parent != (widget_instance *)0 && tag_index == parent->definition) {
        widget->widget_type = 1; // UNSURE: matches Ghidra's raw `*(short*)(ecx+0xe)=1` before widget_type is otherwise set below
    }
    widget->controller_index = controller_index;
    widget->definition = tag_index;
    widget->name = (char *)tag + 4;
    widget->widget_type = tag->widget_type;
    widget->state = 1;
    widget->render_always = (uint8_t)(tag->flags >> 9) & 1;
    widget->pauses_game_time = (uint8_t)(tag->flags >> 1) & 1;
    widget->creation_time = ui_time_milliseconds;
    widget->milliseconds_to_auto_close =
        tag->milliseconds_to_auto_close & ((tag->milliseconds_to_auto_close < 1) ? 0 : -1);
    widget->milliseconds_auto_close_fade =
        tag->milliseconds_auto_close_fade_time & ((tag->milliseconds_auto_close_fade_time < 1) ? 0 : -1);
    widget->scale = 1.0f;
    widget->parent = parent;
    if (widget->widget_type == 1) { // text_box
        widget->selection_index = -1;
        widget->list_render_data = (void *)0;
    }
    if (*(uint32_t *)&tag->background_bitmap.tag_id != 0xffffffffu) {
        tag_instance *bg = &tag_instances[*(uint32_t *)&tag->background_bitmap.tag_id & 0xffff];
        Bitmap *bitmap = (Bitmap *)bg->data;
        BitmapGroupSequence *seq = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;

        widget->background_bitmap_frames = seq[0].bitmap_count;
    }
    if (widget_creating_children == 0) {
        widget_create_children_from_tag(widget, tag);
    }
    if (tag->child_widgets.count > 0) {
        uint8_t *entry = (uint8_t *)tag->child_widgets.pointer;

        for (i = 0; i < tag->child_widgets.count; i++, entry += 0x48) {
            if (*(int16_t *)(entry + 4) == 0x18) { // uieventtype_created
                uint8_t handled;
                int16_t event[4] = {0, 0, 0, 0};

                ui_widget_list_item_activate(widget, tag, event, entry, &handled);
            }
        }
    }
    if (widget->focused_child == (widget_instance *)0) {
        widget_instance *child;

        for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
            UIWidgetDefinition *child_tag = (UIWidgetDefinition *)tag_instances[child->definition & 0xffff].data;

            if (child->hidden == 0 &&
                (child_tag->event_handlers.count > 0 || child->widget_type == 2 || child->widget_type == 3)) {
                widget_instance_relink_focus(widget, child);
            }
        }
    }
    if (widget->pauses_game_time == 1 && network_game_mode != 2 && ui_split_screen == 0) {
        ui_pause_depth = ui_pause_depth + 1;
        if (game_time->paused == 0) {
            if (game_time->unknown_00 != 0) {
                game_time->active = 0;
            }
            game_time->paused = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x499780):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void widget_initialize_from_tag(undefined2 param_1,undefined2 *param_2)

{
  uint uVar1;
  uint *puVar2;
  char *pcVar3;
  int in_EAX;
  int *in_ECX;
  int iVar4;
  int *in_EDX;
  int iVar5;
  int *piVar6;

  piVar6 = in_ECX;
  for (iVar4 = 0x18; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar6 = 0;
    piVar6 = piVar6 + 1;
  }
  if ((((*(byte *)(param_2 + 0xa8) & 2) != 0) && (in_EDX != (int *)0x0)) && (in_EAX == *in_EDX)) {
    *(undefined2 *)((int)in_ECX + 0xe) = 1;
  }
  *(undefined2 *)(in_ECX + 2) = param_1;
  *in_ECX = in_EAX;
  in_ECX[1] = (int)(param_2 + 2);
  *(undefined2 *)((int)in_ECX + 0xe) = *param_2;
  *(undefined1 *)(in_ECX + 4) = 1;
  *(byte *)((int)in_ECX + 0x11) = (byte)(*(uint *)(param_2 + 0x16) >> 9) & 1;
  *(byte *)((int)in_ECX + 0x13) = (byte)(*(uint *)(param_2 + 0x16) >> 1) & 1;
  in_ECX[6] = DAT_00718f9c;
  iVar4 = 0;
  in_ECX[7] = *(uint *)(param_2 + 0x18) & ((int)*(uint *)(param_2 + 0x18) < 1) - 1;
  uVar1 = *(uint *)(param_2 + 0x1a);
  in_ECX[9] = 0x3f800000;
  in_ECX[0xc] = (int)in_EDX;
  in_ECX[8] = uVar1 & ((int)uVar1 < 1) - 1;
  if (*(short *)((int)in_ECX + 0xe) == 1) {
    *(undefined2 *)(in_ECX + 0x10) = 0xffff;
    in_ECX[0x11] = 0;
  }
  if (*(uint *)(param_2 + 0x22) != 0xffffffff) {
    *(undefined2 *)((int)in_ECX + 0x5e) =
         *(undefined2 *)
          (*(int *)(*(int *)((*(uint *)(param_2 + 0x22) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                   0x58) + 0x22);
  }
  if (DAT_00718fc3 == '\0') {
    widget_create_children_from_tag(param_2);
  }
  if (0 < *(int *)(param_2 + 0x2a)) {
    iVar5 = 0;
    do {
      if (*(short *)(*(int *)(param_2 + 0x2c) + iVar5 + 4) == 0x18) {
        ui_widget_list_item_activate();
      }
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 0x48;
    } while (iVar4 < *(int *)(param_2 + 0x2a));
  }
  if (in_ECX[0xe] == 0) {
    for (puVar2 = (uint *)in_ECX[0xd]; puVar2 != (uint *)0x0; puVar2 = (uint *)puVar2[0xb]) {
      if ((*(char *)((int)puVar2 + 0x12) == '\0') &&
         (((0 < *(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x54) ||
           (*(short *)((int)puVar2 + 0xe) == 2)) || (*(short *)((int)puVar2 + 0xe) == 3)))) {
        FUN_0049bba0();
      }
    }
  }
  pcVar3 = DAT_006f1d6c;
  if (((*(char *)((int)in_ECX + 0x13) == '\x01') && (DAT_00719720 != 2)) &&
     ((DAT_00718fc9 == '\0' && (_DAT_00718fa6 = _DAT_00718fa6 + 1, DAT_006f1d6c[2] == '\0')))) {
    if (*DAT_006f1d6c != '\0') {
      DAT_006f1d6c[1] = '\0';
    }
    pcVar3[2] = '\x01';
  }
  return;
}
#endif
