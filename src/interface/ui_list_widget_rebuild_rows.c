// ui_list_widget_rebuild_rows  (Ghidra: FUN_004a7db0, renamed)
// address 0x4a7db0, size 1365 bytes
// name confidence: 0.35 (chosen; types/interface.h's ui_list_item note calls this "the
// list-widget builder at 0x4a7db0" without giving it a name of its own)   rewrite confidence: 0.7
// evidence: types/interface.h widget_instance for every offset below 0x60 (first_child,
// next_sibling, focused_child, widget_type, state, hidden, scale, selection_index, item_count,
// background_bitmap_frame all check out field for field); the "text_box reuses 0x44..0x53 as a
// ColorARGB text color override" note explains the four-dword copy into row->first_child+0x44
// and the "alpha 0 means use the tag color" fallback; types/tags.h UIWidgetDefinition::
// child_widgets (the TagReflexive at the very end of the tag, landing on +0x3e0) and flags_2
// bit 3 (list_single_preview_no_scroll, +0x150 & 8), both already used by
// ui_list_widget_compute_scroll_start.c and ui_list_find_default.c.
// Phase-4 s2 review against objdump 0x4a7db0..0x4a8307: control flow confirmed branch by branch;
// fixed the row scale constant (0x3eaa7efa is 0.333f, not 1/3) and the highlight color call
// (ui_get_saved_pulse_color fills a caller ColorARGB passed in EAX). widget_instance +0x3c/+0x3e hold the
// committed selection and the scroll start of a list widget (movsx word reads and writes here and
// in 0x4a7d00); types/interface.h documents the overlap with the text_box text pointer. UNSURE: the exact
// purpose of the "row has at least 2 more siblings, or a single-page list" branch versus the
// "last visible row" branch is inferred from the field writes (item_count-relative scale/hidden
// toggles on the row's descendants), not confirmed by a caller.
// register convention: widget in the recognized first parameter (param_1, a uint* in Ghidra),
//   item-format callback in the recognized second parameter (param_2, a code* in Ghidra).
//   Ghidra resolved both as ordinary parameters; no unresolved registers appear.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_memory.h"
#include "fn_interface.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t ui_list_current;     // 0x00692c04
extern growable_array ui_lists[3];  // 0x006b3830, element size 0x10 (ui_list_item)
extern uint8_t ui_widget_opened;    // 0x00718fc8


extern heap *widget_memory_pool; // 0x006926c4


// Item-format callback: fills a caller-owned 0x80 byte scratch buffer for the row at item_index
// out of the caller-supplied list_items array (widget->list_items), returning nonzero on success.

// blam-cc: widget -> first parameter, format_item -> second parameter
// Rebuilds a scrollable selection-list widget's visible child rows to reflect one of the three
// shared ui_lists groups (network adapters, ports, etc.), first re-selecting which group is
// current if the widget's embedded group-picker spinner_list says it changed, then walking the
// widget's row children, formatting each one through format_item and toggling its label/value
// sub-widgets' visibility, scale and highlight color to match focus and scroll position.
void ui_list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item)
{
    UIWidgetDefinition *tag_data = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    int32_t visible_rows = (int32_t)tag_data->child_widgets.count;
    widget_instance *first_row = widget->first_child;
    int16_t *scroll_start_field = (int16_t *)((uint8_t *)widget + 0x3e); // list use of the text slot, see types/interface.h
    int16_t *selected_index_field = (int16_t *)((uint8_t *)widget + 0x3c);
    int32_t scroll_start = *scroll_start_field; // local_30
    uint8_t has_embedded_spinner; // bVar4
    uint8_t focused_not_last;     // bVar3
    int32_t group_index = 0;      // iVar11
    uint8_t group_changed = 0;    // bVar2 (first use)
    widget_instance *spinner = first_row; // uVar10 keeps its earlier value when !has_embedded_spinner

    has_embedded_spinner = (first_row != 0 && first_row->first_child != 0 &&
                             first_row->first_child->widget_type == uiwidgettype_spinner_list);
    focused_not_last = (widget->focused_child != 0 && widget->focused_child->next_sibling != 0);

    if (has_embedded_spinner) {
        spinner = first_row->first_child;
        group_index = spinner->selection_index;
        group_changed = (spinner->scroll_blink > 0);
    }

    if (group_index != ui_list_current) {
        int32_t matched = 1;
        if (ui_list_current == -1) {
            int32_t probe = group_index;
            while (ui_lists[probe].count == 0) {
                probe = probe + 1;
                if (probe > 2) { // 0x6b3857 bounds the 3-entry stride table
                    group_index = 0;
                    matched = 0;
                    goto forced_rebuild;
                }
            }
            group_index = probe;
        } else if (group_changed) {
            while (ui_lists[group_index].count == 0) {
                if (group_index == ui_list_current) goto matched_current;
                group_index = group_index + 1;
                if (group_index > 2) group_index = 0;
            }
        } else {
            while (ui_lists[group_index].count == 0) {
                if (group_index == ui_list_current) goto matched_current;
                group_index = group_index - 1;
                if (group_index < 0) group_index = 2;
            }
        }
        if (group_index != ui_list_current) {
            matched = 0;
        }
        if (matched) {
matched_current:
            spinner->selection_index = (int16_t)group_index;
            widget->item_count = (uint16_t)ui_lists[group_index].count;
        } else {
            int32_t default_entry; // iVar6
forced_rebuild:
            default_entry = ui_list_find_default(group_index);
            spinner->selection_index = (int16_t)group_index;
            *scroll_start_field = 0;
            widget->selection_index = 0;
            *selected_index_field = 0;
            widget->item_count = (uint16_t)ui_lists[group_index].count;
            if (default_entry == -1) {
                scroll_start = 0;
            } else {
                widget->selection_index = (int16_t)default_entry;
                *selected_index_field = (int16_t)default_entry;
                scroll_start = -1;
            }
            ui_list_current = group_index;
            widget->focused_child = 0;
            ui_widget_opened = 1;
        }
    }

    {
        int32_t item_count = (uint16_t)widget->item_count; // uVar5, re-read after the block above
        int32_t window_size;   // uVar7 / local_24
        uint8_t single_page;   // bVar2 (second use)
        int32_t row_slot;      // local_28
        int32_t item_index;    // local_2c
        widget_instance *row;  // uVar10

        if (has_embedded_spinner) {
            visible_rows = visible_rows - 1;
        }
        single_page = ((tag_data->flags_2 & 8) != 0) || (visible_rows - 1 >= item_count);
        window_size = visible_rows - (single_page ? 1 : 3);
        if (item_count < window_size) {
            window_size = item_count;
        }
        if (scroll_start == -1) {
            scroll_start = ui_list_widget_compute_scroll_start(widget);
        }

        row = widget->first_child;
        row_slot = 0;
        item_index = scroll_start;
        for (; row != 0 && row->next_sibling != 0; row = row->next_sibling) {
            widget_instance *label;  // row->first_child: the row's label text_box
            widget_instance *value;  // label->next_sibling
            widget_instance *value2; // value->next_sibling
            widget_instance *value3; // value2->next_sibling

            row->state = 1;
            if (item_index < item_count) {
                if (row_slot == 0) {
                    if (has_embedded_spinner) {
                        if (widget->focused_child == row) {
                            item_index = item_index - 1;
                            row->background_bitmap_frame = 1;
                            row->focused_child = row->first_child;
                        } else {
                            item_index = item_index - 1;
                            row->background_bitmap_frame = 0;
                            row->focused_child = 0;
                        }
                        goto tail; // skips both LAB_004a8038 and LAB_004a80b4
                    }
                    if (!single_page) goto near_end_row; // LAB_004a80b4
                    goto render_row; // LAB_004a8038
                } else {
                    if ((row_slot != 1) || single_page || !has_embedded_spinner) goto render_row;
                    goto near_end_row; // LAB_004a80b4
                }

render_row: // LAB_004a8038
                label = row->first_child;
                label->state = 1;
                value = label->next_sibling;
                value2 = value->next_sibling;
                value3 = value2->next_sibling;
                value2->state = 0;
                value3->state = 0;
                if (row->next_sibling->next_sibling != 0 || single_page) {
                    void *item_buffer;

                    if (item_index == (int16_t)*selected_index_field) {
                        value->state = 1;
                        row->background_bitmap_frame = 1;
                        if (widget->focused_child == row) {
                            ColorARGB highlight;
                            *(ColorARGB *)&((struct widget_instance *)label)->list_items = *ui_get_saved_pulse_color(&highlight); // text_box color override
                        } else {
                            *(uint32_t *)&((struct widget_instance *)label)->list_items = 0;
                        }
                    } else {
                        if (widget->focused_child == row) {
                            ColorARGB highlight;
                            widget->selection_index = (int16_t)item_index;
                            *(ColorARGB *)&((struct widget_instance *)label)->list_items = *ui_get_saved_pulse_color(&highlight);
                        } else {
                            *(uint32_t *)&((struct widget_instance *)label)->list_items = 0;
                        }
                        row->background_bitmap_frame = 0;
                    }
                    value->state = 0; // reset unconditionally, as Ghidra shows (see header note)

                    item_buffer = heap_reallocate(label->text, 0x80, widget_memory_pool);
                    label->text = item_buffer;
                    if (item_buffer == 0 || !format_item(item_buffer, item_index, widget->list_items)) {
                        row->scale = 0.333f /* 0x3eaa7efa */;
                        goto row_hidden; // LAB_004a829b
                    }
                    if (focused_not_last || item_index == (int16_t)*selected_index_field) {
                        row->scale = 1.0f;
                        row->hidden = 0;
                    } else {
                        row->scale = 0.333f /* 0x3eaa7efa */;
                        row->hidden = 0;
                    }
                    goto tail;
                } else {
                    // near the end of the row list and it does not fit on a single page: leave a
                    // trailing row reserved (value3 takes the highlight instead of value)
                    label->state = 0;
                    row->background_bitmap_frame = 0;
                    value->state = 0;
                    value3->state = 1;
                    if (widget->focused_child == row) {
                        value3->background_bitmap_frame = 1;
                    } else {
                        value3->background_bitmap_frame = 0;
                    }
                    if (scroll_start < (int32_t)(item_count - window_size)) {
                        row->hidden = 0;
                        row->scale = 1.0f;
                    } else {
                        row->hidden = 1;
                        row->scale = 0.333f /* 0x3eaa7efa */;
                        if (widget->focused_child == row) {
                            widget->focused_child = row->previous_sibling;
                        }
                    }
                    goto tail;
                }

near_end_row: // LAB_004a80b4
                label = row->first_child;
                value = label->next_sibling;
                value2 = value->next_sibling;
                value3 = value2->next_sibling;
                label->state = 0;
                row->background_bitmap_frame = 0;
                value->state = 0;
                value3->state = 0;
                value2->state = 1;
                if (widget->focused_child == row) {
                    value2->background_bitmap_frame = 1;
                } else {
                    value2->background_bitmap_frame = 0;
                }
                if (scroll_start == 0) {
                    row->hidden = 1;
                    row->scale = 0.333f /* 0x3eaa7efa */;
                    if (widget->focused_child == row) {
                        item_index = item_index - 1;
                        widget->focused_child = row->next_sibling;
                        goto tail_no_decrement; // LAB_004a829f directly
                    }
                } else {
                    row->hidden = 0;
                    row->scale = 1.0f;
                }
                item_index = item_index - 1;
                goto tail_no_decrement;
            } else {
                row->state = 0;
row_hidden: // LAB_004a829b
                row->hidden = 1;
            }
tail: // fall through to the shared per-slot increment
tail_no_decrement: // LAB_004a829f
            row_slot = row_slot + 1;
            item_index = item_index + 1;
        }
    }

    if (widget->focused_child == 0 || widget->focused_child->hidden != 0) {
        widget_instance *row = widget->first_child;
        widget->focused_child = 0;
        for (; row != 0; row = row->next_sibling) {
            if (row->hidden == 0) {
                widget->focused_child = row;
                break;
            }
        }
        if (widget->focused_child == 0) {
            widget->parent->focused_child = widget->next_sibling;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a7db0):

void FUN_004a7db0(uint *param_1,code *param_2)

{
  int iVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  char cVar5;
  int iVar6;
  undefined4 *puVar7;
  int *piVar8;
  int iVar9;
  uint uVar10;
  int iVar11;
  int local_30;
  int local_2c;
  int local_28;
  uint local_24;

  iVar1 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar9 = *(int *)(iVar1 + 0x3e0);
  uVar10 = param_1[0xd];
  local_30 = (int)*(short *)((int)param_1 + 0x3e);
  if (((uVar10 == 0) || (*(int *)(uVar10 + 0x34) == 0)) ||
     (bVar4 = true, *(short *)(*(int *)(uVar10 + 0x34) + 0xe) != 2)) {
    bVar4 = false;
  }
  if ((param_1[0xe] == 0) || (bVar3 = true, *(int *)(param_1[0xe] + 0x2c) == 0)) {
    bVar3 = false;
  }
  iVar11 = 0;
  bVar2 = false;
  if (bVar4) {
    uVar10 = *(uint *)(uVar10 + 0x34);
    iVar11 = (int)*(short *)(uVar10 + 0x40);
    if (0 < *(short *)(uVar10 + 0x42)) {
      bVar2 = true;
    }
  }
  if (iVar11 != DAT_00692c04) {
    if (DAT_00692c04 == -1) {
      piVar8 = &DAT_006b3834 + iVar11 * 3;
      iVar6 = (&DAT_006b3834)[iVar11 * 3];
      while (iVar6 == 0) {
        piVar8 = piVar8 + 3;
        iVar11 = iVar11 + 1;
        if (0x6b3857 < (int)piVar8) {
          iVar11 = 0;
          goto LAB_004a7ede;
        }
        iVar6 = *piVar8;
      }
    }
    else if (bVar2) {
      iVar6 = (&DAT_006b3834)[iVar11 * 3];
      while (iVar6 == 0) {
        if (iVar11 == DAT_00692c04) goto LAB_004a7f33;
        iVar11 = iVar11 + 1;
        if (2 < iVar11) {
          iVar11 = 0;
        }
        iVar6 = (&DAT_006b3834)[iVar11 * 3];
      }
    }
    else {
      iVar6 = (&DAT_006b3834)[iVar11 * 3];
      while (iVar6 == 0) {
        if (iVar11 == DAT_00692c04) goto LAB_004a7f33;
        iVar11 = iVar11 + -1;
        if (iVar11 < 0) {
          iVar11 = 2;
        }
        iVar6 = (&DAT_006b3834)[iVar11 * 3];
      }
    }
    if (iVar11 == DAT_00692c04) {
LAB_004a7f33:
      *(short *)(uVar10 + 0x40) = (short)iVar11;
      *(undefined2 *)(param_1 + 0x12) = *(undefined2 *)(&DAT_006b3834 + iVar11 * 3);
    }
    else {
LAB_004a7ede:
      iVar6 = FUN_004a7cb0();
      *(short *)(uVar10 + 0x40) = (short)iVar11;
      *(undefined2 *)((int)param_1 + 0x3e) = 0;
      *(undefined2 *)(param_1 + 0x10) = 0;
      *(undefined2 *)(param_1 + 0xf) = 0;
      *(undefined2 *)(param_1 + 0x12) = *(undefined2 *)(&DAT_006b3834 + iVar11 * 3);
      if (iVar6 == -1) {
        local_30 = 0;
      }
      else {
        *(short *)(param_1 + 0x10) = (short)iVar6;
        *(short *)(param_1 + 0xf) = (short)iVar6;
        local_30 = -1;
      }
      DAT_00692c04 = iVar11;
      param_1[0xe] = 0;
      DAT_00718fc8 = 1;
    }
  }
  if (bVar4) {
    iVar9 = iVar9 + -1;
  }
  if (((*(byte *)(iVar1 + 0x150) & 8) != 0) ||
     (bVar2 = false, (int)(uint)(ushort)param_1[0x12] <= iVar9 + -1)) {
    bVar2 = true;
  }
  local_24 = iVar9 - ((uint)!bVar2 * 2 + 1);
  if ((int)(uint)(ushort)param_1[0x12] < (int)local_24) {
    local_24 = (uint)(ushort)param_1[0x12];
  }
  if (local_30 == -1) {
    local_30 = FUN_004a7d00();
  }
  uVar10 = param_1[0xd];
  local_28 = 0;
  local_2c = local_30;
  for (; (uVar10 != 0 && (*(int *)(uVar10 + 0x2c) != 0)); uVar10 = *(uint *)(uVar10 + 0x2c)) {
    *(undefined1 *)(uVar10 + 0x10) = 1;
    if (local_2c < (int)(uint)(ushort)param_1[0x12]) {
      if (local_28 == 0) {
        if (bVar4) {
          if (param_1[0xe] == uVar10) {
            local_2c = local_2c + -1;
            *(undefined2 *)(uVar10 + 0x58) = 1;
            *(undefined4 *)(uVar10 + 0x38) = *(undefined4 *)(uVar10 + 0x34);
          }
          else {
            local_2c = local_2c + -1;
            *(undefined2 *)(uVar10 + 0x58) = 0;
            *(undefined4 *)(uVar10 + 0x38) = 0;
          }
        }
        else {
          if (!bVar2) goto LAB_004a80b4;
LAB_004a8038:
          if ((*(int *)(*(int *)(uVar10 + 0x2c) + 0x2c) != 0) || (bVar2)) {
            *(undefined1 *)(*(int *)(uVar10 + 0x34) + 0x10) = 1;
            *(undefined1 *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x10) = 0;
            *(undefined1 *)
             (*(int *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x2c) + 0x10) = 0
            ;
            if (local_2c == (short)param_1[0xf]) {
              *(undefined1 *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x10) = 1;
              *(undefined2 *)(uVar10 + 0x58) = 1;
              if (param_1[0xe] == uVar10) {
                puVar7 = (undefined4 *)FUN_0049c620();
                iVar1 = *(int *)(uVar10 + 0x34);
                *(undefined4 *)(iVar1 + 0x44) = *puVar7;
                *(undefined4 *)(iVar1 + 0x48) = puVar7[1];
                *(undefined4 *)(iVar1 + 0x4c) = puVar7[2];
                *(undefined4 *)(iVar1 + 0x50) = puVar7[3];
              }
              else {
                *(undefined4 *)(*(int *)(uVar10 + 0x34) + 0x44) = 0;
              }
            }
            else {
              if (param_1[0xe] == uVar10) {
                *(short *)(param_1 + 0x10) = (short)local_2c;
                puVar7 = (undefined4 *)FUN_0049c620();
                iVar1 = *(int *)(uVar10 + 0x34);
                *(undefined4 *)(iVar1 + 0x44) = *puVar7;
                *(undefined4 *)(iVar1 + 0x48) = puVar7[1];
                *(undefined4 *)(iVar1 + 0x4c) = puVar7[2];
                *(undefined4 *)(iVar1 + 0x50) = puVar7[3];
              }
              else {
                *(undefined4 *)(*(int *)(uVar10 + 0x34) + 0x44) = 0;
              }
              *(undefined2 *)(uVar10 + 0x58) = 0;
            }
            *(undefined1 *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x10) = 0;
            iVar1 = *(int *)(uVar10 + 0x34);
            iVar9 = heap_reallocate(0x80);
            *(int *)(iVar1 + 0x3c) = iVar9;
            if ((iVar9 == 0) || (cVar5 = (*param_2)(iVar9,local_2c,param_1[0x11]), cVar5 == '\0')) {
              *(undefined4 *)(uVar10 + 0x24) = 0x3eaa7efa;
              goto LAB_004a829b;
            }
            if ((bVar3) || (local_2c == (short)param_1[0xf])) {
              *(undefined4 *)(uVar10 + 0x24) = 0x3f800000;
              *(undefined1 *)(uVar10 + 0x12) = 0;
            }
            else {
              *(undefined4 *)(uVar10 + 0x24) = 0x3eaa7efa;
              *(undefined1 *)(uVar10 + 0x12) = 0;
            }
          }
          else {
            *(undefined1 *)(*(int *)(uVar10 + 0x34) + 0x10) = 0;
            *(undefined2 *)(uVar10 + 0x58) = 0;
            *(undefined1 *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x10) = 0;
            *(undefined1 *)
             (*(int *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x2c) + 0x10) = 1
            ;
            iVar1 = *(int *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x2c);
            if (param_1[0xe] == uVar10) {
              *(undefined2 *)(iVar1 + 0x58) = 1;
            }
            else {
              *(undefined2 *)(iVar1 + 0x58) = 0;
            }
            if (local_30 < (int)((ushort)param_1[0x12] - local_24)) {
              *(undefined1 *)(uVar10 + 0x12) = 0;
              *(undefined4 *)(uVar10 + 0x24) = 0x3f800000;
            }
            else {
              *(undefined1 *)(uVar10 + 0x12) = 1;
              *(undefined4 *)(uVar10 + 0x24) = 0x3eaa7efa;
              if (param_1[0xe] == uVar10) {
                param_1[0xe] = *(uint *)(uVar10 + 0x28);
              }
            }
          }
        }
      }
      else {
        if (((local_28 != 1) || (bVar2)) || (!bVar4)) goto LAB_004a8038;
LAB_004a80b4:
        *(undefined1 *)(*(int *)(uVar10 + 0x34) + 0x10) = 0;
        *(undefined2 *)(uVar10 + 0x58) = 0;
        *(undefined1 *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x10) = 0;
        *(undefined1 *)
         (*(int *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x2c) + 0x10) = 0;
        *(undefined1 *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x10) = 1;
        if (param_1[0xe] == uVar10) {
          *(undefined2 *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x58) = 1;
        }
        else {
          *(undefined2 *)(*(int *)(*(int *)(*(int *)(uVar10 + 0x34) + 0x2c) + 0x2c) + 0x58) = 0;
        }
        if (local_30 == 0) {
          *(undefined1 *)(uVar10 + 0x12) = 1;
          *(undefined4 *)(uVar10 + 0x24) = 0x3eaa7efa;
          if (param_1[0xe] == uVar10) {
            local_2c = local_2c + -1;
            param_1[0xe] = *(uint *)(uVar10 + 0x2c);
            goto LAB_004a829f;
          }
        }
        else {
          *(undefined1 *)(uVar10 + 0x12) = 0;
          *(undefined4 *)(uVar10 + 0x24) = 0x3f800000;
        }
        local_2c = local_2c + -1;
      }
    }
    else {
      *(undefined1 *)(uVar10 + 0x10) = 0;
LAB_004a829b:
      *(undefined1 *)(uVar10 + 0x12) = 1;
    }
LAB_004a829f:
    local_28 = local_28 + 1;
    local_2c = local_2c + 1;
  }
  if ((param_1[0xe] == 0) || (*(char *)(param_1[0xe] + 0x12) != '\0')) {
    uVar10 = param_1[0xd];
    param_1[0xe] = 0;
    for (; uVar10 != 0; uVar10 = *(uint *)(uVar10 + 0x2c)) {
      if (*(char *)(uVar10 + 0x12) == '\0') {
        param_1[0xe] = uVar10;
        break;
      }
    }
    if (param_1[0xe] == 0) {
      *(uint *)(param_1[0xc] + 0x38) = param_1[0xb];
    }
  }
  return;
}
#endif
