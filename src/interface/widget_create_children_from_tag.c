// widget_create_children_from_tag  (Ghidra: widget_create_children_from_tag, already named)
// address 0x499540, size 569 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: matches the given name; types/interface.h's widget_instance note attributes the
// local_x/local_y-from-ChildWidgetReference+0x38/+0x36 write to this exact address. Ghidra's own
// decompile of the child_widgets loop is heavily incomplete (it shows a bare 2-argument
// chimera__load_ui_widget call with no controller-override or position logic at all); rewritten
// from disassembly (0x4995dc..0x49964e) instead, which shows the real ChildWidgetReference-driven
// logic: a per-child custom controller index (flags bit 0, custom_controller_index, both fields
// of tags.h's ChildWidgetReference) and the horizontal_offset/vertical_offset fields added onto
// the parent's own local_x/local_y. The string-list loop (list_items_from_string_list_tag) was
// confirmed by the same method to call chimera__load_ui_widget with this WIDGET's own definition
// as the tag index, repeated once per string -- i.e. every generated list item is another
// instance of this same widget's own tag; unexpected, but that is what 0x499581..0x499594 does.
// Both loops and the extended description use chimera__load_ui_widget's 7-argument form with
// this widget as the parent (argument 3 is a parent pointer, which keeps the child out of the
// root slot and the history), its controller_index, and -1 for the three history arguments.
// Phase-4 review: the extended description is also created with this widget as parent and its
// controller (objdump 0x4996a9); the first rewrite passed 0 and 0.
// register convention: tag data in the recognized stack parameter (Ghidra's own param_1);
// widget instance in ESI (unaff_ESI, confirmed by widget_initialize_from_tag's own `mov esi,ecx`
// at its own entry, which never changes ESI before calling this function).
// blam-cc: stack -> tag, ESI -> widget
// UNSURE: the final "pick a default focused_child" search's flags test
// (`-1 < (char)flags`, bit 7 = dont_focus_a_specific_child_widget clear) and bit-0
// (pass_unhandled_events_to_focused_child) gate are translated from Ghidra's pseudo-C directly,
// not independently re-verified from disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t widget_creating_children; // 0x00718fc3

extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args (objdump)

// blam-cc: stack -> tag, ESI -> widget
// Instantiates and links in all of a widget's static child widgets: once per string when the tag
// draws its list items from a string list (each such "child" reuses this widget's own tag), then
// once per explicit ChildWidgetReference entry (honoring a per-child custom controller index and
// its horizontal/vertical offset), then the extended_description child if this is a list type,
// finally picking a default focused_child among the real children when the tag allows it.
uint8_t widget_create_children_from_tag(widget_instance *widget, UIWidgetDefinition *tag)
{
    uint8_t ok = 1;
    int32_t i;

    if ((tag->flags_2 & 2) != 0) { // list_items_from_string_list_tag
        UnicodeStringList *list =
            (UnicodeStringList *)tag_instances[*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id & 0xffff].data;

        widget_creating_children = 1;
        for (i = 0; i < list->strings.count; i++) {
            widget_instance *child = chimera__load_ui_widget((char *)0, widget->definition, widget,
                                                               widget->controller_index, (datum_index)-1,
                                                               (datum_index)-1, -1);

            if (child == (widget_instance *)0) {
                ok = 0;
                break;
            }
            if (widget->first_child == (widget_instance *)0) {
                widget->first_child = child;
            } else {
                widget_instance *tail = widget->first_child;

                while (tail->next_sibling != (widget_instance *)0) {
                    tail = tail->next_sibling;
                }
                tail->next_sibling = child;
                child->previous_sibling = tail;
            }
            widget->item_count = widget->item_count + 1;
        }
        widget_creating_children = 0;
    }

    if (tag->child_widgets.count > 0) {
        uint8_t *entries = (uint8_t *)tag->child_widgets.pointer;

        for (i = 0; i < tag->child_widgets.count; i++) {
            ChildWidgetReference *entry = (ChildWidgetReference *)(entries + i * 0x50);
            datum_index child_tag_index = *(uint32_t *)&entry->widget_tag.tag_id;

            if (child_tag_index != (datum_index)-1) {
                uint16_t controller = widget->controller_index;
                widget_instance *child;

                if ((entry->flags & 1) != 0 && entry->custom_controller_index < 4) {
                    controller = entry->custom_controller_index;
                }
                child = chimera__load_ui_widget((char *)0, child_tag_index, widget, controller,
                                                 (datum_index)-1, (datum_index)-1, -1);
                if (child == (widget_instance *)0) {
                    ok = 0;
                    break;
                }
                child->local_x = entry->horizontal_offset + widget->local_x;
                child->local_y = entry->vertical_offset + widget->local_y;
                if (widget->first_child == (widget_instance *)0) {
                    widget->first_child = child;
                } else {
                    widget_instance *tail = widget->first_child;

                    while (tail->next_sibling != (widget_instance *)0) {
                        tail = tail->next_sibling;
                    }
                    tail->next_sibling = child;
                    child->previous_sibling = tail;
                }
            }
        }
    }

    if ((widget->widget_type == 2 || widget->widget_type == 3) &&
        *(uint32_t *)&tag->extended_description_widget.tag_id != 0xffffffffu) {
        widget_instance *desc = chimera__load_ui_widget(
            (char *)0, *(uint32_t *)&tag->extended_description_widget.tag_id, widget,
            widget->controller_index, (datum_index)-1, (datum_index)-1, -1); // objdump 0x4996a9: parent and controller are this widget's

        widget->extended_description = desc;
        if (desc != (widget_instance *)0) {
            if (desc->previous_sibling != (widget_instance *)0) {
                desc->previous_sibling->next_sibling = (widget_instance *)0;
            }
            desc->previous_sibling = (widget_instance *)0;
            desc->parent = (widget_instance *)0;
        }
    }

    if ((int8_t)tag->flags >= 0) { // bit 7 (dont_focus_a_specific_child_widget) clear
        if (widget->widget_type == 2 || widget->widget_type == 3) {
            widget->selection_index = 0;
            widget->scroll_blink = 0;
            widget->selection_direction = 0;
        } else if ((tag->flags & 1) == 0) { // pass_unhandled_events_to_focused_child clear
            return ok;
        }
        {
            widget_instance *child = widget->first_child;

            while (child != (widget_instance *)0) {
                if (widget->widget_type == 2 || widget->widget_type == 3) {
                    break;
                }
                {
                    UIWidgetDefinition *child_tag =
                        (UIWidgetDefinition *)tag_instances[child->definition & 0xffff].data;

                    if (child->hidden == 0 &&
                        (child_tag->event_handlers.count > 0 || child->widget_type == 2 ||
                         child->widget_type == 3)) {
                        break;
                    }
                }
                child = child->next_sibling;
            }
            if (child != (widget_instance *)0) {
                widget->focused_child = child;
            }
        }
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x499540):

uint widget_create_children_from_tag(int param_1)

{
  short sVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  uint *puVar5;
  int iVar6;
  int iVar7;
  undefined4 *unaff_ESI;
  int iVar8;
  byte local_1;

  local_1 = 1;
  if ((*(byte *)(param_1 + 0x150) & 2) != 0) {
    piVar2 = *(int **)((*(uint *)(param_1 + 0xf8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    DAT_00718fc3 = 1;
    iVar7 = 0;
    if (0 < *piVar2) {
      do {
        iVar3 = chimera__load_ui_widget(0,*unaff_ESI);
        if (iVar3 == 0) {
          local_1 = 0;
          break;
        }
        iVar8 = unaff_ESI[0xd];
        if (unaff_ESI[0xd] == 0) {
LAB_004995bc:
          unaff_ESI[0xd] = iVar3;
        }
        else {
          do {
            iVar4 = iVar8;
            iVar8 = *(int *)(iVar4 + 0x2c);
          } while (*(int *)(iVar4 + 0x2c) != 0);
          if (iVar4 == 0) goto LAB_004995bc;
          *(int *)(iVar4 + 0x2c) = iVar3;
          *(int *)(iVar3 + 0x28) = iVar4;
        }
        *(short *)(unaff_ESI + 0x12) = *(short *)(unaff_ESI + 0x12) + 1;
        iVar7 = iVar7 + 1;
      } while (iVar7 < *piVar2);
    }
    DAT_00718fc3 = 0;
  }
  iVar7 = 0;
  if (0 < *(int *)(param_1 + 0x3e0)) {
    iVar3 = 0;
    do {
      iVar8 = *(int *)(param_1 + 0x3e4) + iVar3;
      if (*(int *)(iVar8 + 0xc) != -1) {
        iVar4 = chimera__load_ui_widget(0,*(int *)(iVar8 + 0xc));
        if (iVar4 == 0) {
          local_1 = 0;
          break;
        }
        *(short *)(iVar4 + 10) = *(short *)(iVar8 + 0x38) + *(short *)((int)unaff_ESI + 10);
        *(short *)(iVar4 + 0xc) = *(short *)(iVar8 + 0x36) + *(short *)(unaff_ESI + 3);
        iVar8 = unaff_ESI[0xd];
        if (unaff_ESI[0xd] != 0) {
          do {
            iVar6 = iVar8;
            iVar8 = *(int *)(iVar6 + 0x2c);
          } while (*(int *)(iVar6 + 0x2c) != 0);
          if (iVar6 != 0) {
            *(int *)(iVar6 + 0x2c) = iVar4;
            *(int *)(iVar4 + 0x28) = iVar6;
            goto LAB_0049966d;
          }
        }
        unaff_ESI[0xd] = iVar4;
      }
LAB_0049966d:
      iVar7 = iVar7 + 1;
      iVar3 = iVar3 + 0x50;
    } while (iVar7 < *(int *)(param_1 + 0x3e0));
  }
  if (((*(short *)((int)unaff_ESI + 0xe) == 3) || (*(short *)((int)unaff_ESI + 0xe) == 2)) &&
     (*(int *)(param_1 + 0x1b0) != -1)) {
    iVar7 = chimera__load_ui_widget(0,*(int *)(param_1 + 0x1b0));
    unaff_ESI[0x13] = iVar7;
    if (iVar7 != 0) {
      if (*(int *)(iVar7 + 0x28) != 0) {
        *(undefined4 *)(*(int *)(iVar7 + 0x28) + 0x2c) = 0;
      }
      *(undefined4 *)(unaff_ESI[0x13] + 0x28) = 0;
      *(undefined4 *)(unaff_ESI[0x13] + 0x30) = 0;
    }
  }
  puVar5 = *(uint **)(param_1 + 0x2c);
  if (-1 < (char)puVar5) {
    sVar1 = *(short *)((int)unaff_ESI + 0xe);
    if ((sVar1 == 2) || (sVar1 == 3)) {
      *(undefined2 *)(unaff_ESI + 0x10) = 0;
      *(undefined2 *)((int)unaff_ESI + 0x42) = 0;
      *(undefined2 *)(unaff_ESI + 0x15) = 0;
    }
    else if (((uint)puVar5 & 1) == 0) goto LAB_00499770;
    puVar5 = (uint *)unaff_ESI[0xd];
    if (puVar5 != (uint *)0x0) {
      while (((sVar1 != 2 && (sVar1 != 3)) &&
             ((*(char *)((int)puVar5 + 0x12) != '\0' ||
              (((*(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x54) < 1 &&
                (*(short *)((int)puVar5 + 0xe) != 2)) && (*(short *)((int)puVar5 + 0xe) != 3)))))))
      {
        puVar5 = (uint *)puVar5[0xb];
        if (puVar5 == (uint *)0x0) {
          return (uint)local_1;
        }
      }
      unaff_ESI[0xe] = puVar5;
    }
  }
LAB_00499770:
  return CONCAT31((int3)((uint)puVar5 >> 8),local_1);
}
#endif
