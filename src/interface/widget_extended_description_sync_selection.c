// widget_extended_description_sync_selection  (Ghidra: FUN_004a66b0, renamed)
// address 0x4a66b0, size 182 bytes
// name confidence: 0.3 (chosen; not attested by any string or caller name)   rewrite confidence: 0.55
// evidence: types/interface.h widget_instance (definition, first_child, next_sibling,
// focused_child, widget_type, selection_index, item_count, extended_description, state all
// match field for field); types/tags.h UIWidgetDefinition::flags_2 sits at byte offset 0x150
// once TagString/Rectangle2D/TagDependency/TagReflexive/ColorARGB are sized out, which lines
// up exactly with the `+0x150` tag-data read here, so the tested bit is
// list_items_only_one_tooltip (bit 2 of UIWidgetDefinitionFlags2); types/cache.h tag_instance
// (0x0087bc14, data at +0x14, stride 0x20) for the tag lookup idiom, already established in
// src/ai/actor_apply_unit_definition_properties.c.
// This widget walks its own children looking, in each child's own children, for a
// spinner_list (widget_type 2); the widget with widget_type 3 (column_list) as the LAST
// child, with no spinner_list beneath a non-focused sibling, bails out and disables the
// extended_description widget outright. Otherwise it accumulates a flattened index: the
// item_count of every spinner_list before the focused sibling, plus that sibling's own
// selection_index once it is reached (or +1 per sibling instead, for a spinner_list flagged
// list_items_only_one_tooltip), and pushes the result into extended_description->selection_index,
// enabling it. UNSURE: the exact intent (this reads as a "flatten several sub-lists into one
// combo selection index for a shared description widget" helper) is inferred from the field
// accesses, not from a caller or a string.
// register convention: widget as the recognized parameter (param_1).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14

// Enables (and reindexes) or disables a widget's extended_description widget by scanning the
// widget's own children for a nested spinner_list, flattening the count/selection of every
// sub-list into a single index for the shared description widget.
void widget_extended_description_sync_selection(widget_instance *widget)
{
    widget_instance *description; // iVar1
    widget_instance *focused;     // iVar2
    widget_instance *sibling;     // iVar5
    widget_instance *list_child;  // puVar3
    int32_t index;                // iVar6
    UIWidgetDefinition *list_tag;

    description = widget->extended_description;
    focused = widget->focused_child;
    if (focused == 0) {
        goto disable;
    }

    sibling = widget->first_child;
    index = 0;
    if (sibling == 0) {
        goto enable;
    }

    do {
        if (sibling->widget_type == uiwidgettype_column_list && sibling->next_sibling == 0) {
            goto disable;
        }

        list_child = sibling->first_child;
        for (; list_child != 0; list_child = list_child->next_sibling) {
            if (list_child->widget_type == uiwidgettype_spinner_list) {
                list_tag = (UIWidgetDefinition *)tag_instances[list_child->definition & 0xffff].data;
                if (sibling == focused) {
                    if ((list_tag->flags_2 & 4) == 0) {
                        index = index + list_child->selection_index;
                    }
                    goto enable;
                }
                if ((list_tag->flags_2 & 4) != 0) {
                    index = index + 1;
                } else {
                    index = index + (uint16_t)list_child->item_count;
                }
                goto next_sibling;
            }
        }
        if (sibling == focused) {
            break;
        }
next_sibling:
        sibling = sibling->next_sibling;
    } while (sibling != 0);

    if (index == -1) {
        goto disable;
    }

enable:
    description->selection_index = (int16_t)index;
    description->state = 1;
    return;

disable:
    description->state = 0;
    return;
}

#if 0
Original Ghidra decompilation (0x4a66b0):

void FUN_004a66b0(int param_1)

{
  int iVar1;
  int iVar2;
  uint *puVar3;
  int iVar4;
  int iVar5;
  int iVar6;

  iVar1 = *(int *)(param_1 + 0x4c);
  iVar2 = *(int *)(param_1 + 0x38);
  if (iVar2 != 0) {
    iVar5 = *(int *)(param_1 + 0x34);
    iVar6 = 0;
    if (iVar5 != 0) {
      do {
        if ((*(short *)(iVar5 + 0xe) == 3) && (*(int *)(iVar5 + 0x2c) == 0)) goto LAB_004a6759;
        for (puVar3 = *(uint **)(iVar5 + 0x34); puVar3 != (uint *)0x0; puVar3 = (uint *)puVar3[0xb])
        {
          if (*(short *)((int)puVar3 + 0xe) == 2) {
            iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (iVar5 == iVar2) {
              if ((*(byte *)(iVar4 + 0x150) & 4) == 0) {
                iVar6 = iVar6 + (short)puVar3[0x10];
              }
              goto LAB_004a6743;
            }
            if ((*(byte *)(iVar4 + 0x150) & 4) != 0) goto LAB_004a6706;
            iVar6 = iVar6 + (uint)(ushort)puVar3[0x12];
            goto LAB_004a6707;
          }
        }
        if (iVar5 == iVar2) break;
LAB_004a6706:
        iVar6 = iVar6 + 1;
LAB_004a6707:
        iVar5 = *(int *)(iVar5 + 0x2c);
      } while (iVar5 != 0);
LAB_004a6743:
      if (iVar6 == -1) goto LAB_004a6759;
    }
    *(short *)(iVar1 + 0x40) = (short)iVar6;
    *(undefined1 *)(iVar1 + 0x10) = 1;
    return;
  }
LAB_004a6759:
  *(undefined1 *)(iVar1 + 0x10) = 0;
  return;
}
#endif
