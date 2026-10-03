/**
 * The UI selection lists (add, find, format, rebuild rows and scrolling).
 */

#include "crt.h"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "saved_games.h"
#include <string.h>

#include "halo/interface/uis_lists.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/flags.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/wide_text.hpp"

extern "C" {
extern uint8_t ui_list_has_default;
extern growable_array ui_lists[3];
extern int32_t ui_list_current;
extern map_list_entry *map_list;
extern int32_t map_list_count;
extern uint8_t ui_widget_opened;
extern heap *widget_memory_pool;
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles];
extern void *ui_event_function_table[0xbe];
extern widget_instance *ui_root_widget[1];
extern widget_history_node *ui_widget_history[3];
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection);
extern void widget_close(widget_instance *widget);
extern void widget_play_sound_effect(int16_t effect_id);
extern widget_instance *widget_find_by_tag_id(widget_instance *widget, datum_index tag_id);
extern void widget_instance_relink_focus(widget_instance *widget, widget_instance *child);
extern void widget_instance_close_and_restore_previous(widget_instance *widget);
extern widget_instance *widget_reopen_as_root_with_history(widget_instance *widget, datum_index open_tag);
}

namespace halo::ui {

/**
 * Appends one entry to a UI selection list group: a GlobalAlloc'd copy of name, the given id, and an optional
 * GlobalAlloc'd copy of a data blob. Setting is_default also raises the shared ui_list_has_default flag.
 *
 * Register convention: EAX -> group_index, CL -> is_default, stack -> name, id, data_blob, data_size
 *
 * @address 0x4a7ba0
 */
void UiLists::list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob,
                        uint32_t data_size, uint8_t is_default)
{
    int32_t index = halo::memory::growable_array_add_element(&ui_lists[group_index]);
    ui_list_item *entry;
    int32_t length;

    if (index == -1) {
        return;
    }

    length = wcslen((const wchar_t *)name);
    entry = (ui_list_item *)ui_lists[group_index].data + index;
    entry->data = 0;
    entry->name = (uint16_t *)(GlobalAlloc(0, length * 2 + 2));
    entry->id = id;
    entry->is_default = is_default;
    if (is_default != 0) {
        ui_list_has_default = 1;
    }
    wcslen((const wchar_t *)name);
    wcscpy((wchar_t *)entry->name, (const wchar_t *)name);

    if (data_blob != 0 && data_size != 0) {
        uint8_t *dst = (uint8_t *)GlobalAlloc(0, data_size);
        const uint8_t *src = (const uint8_t *)data_blob;
        uint32_t words = data_size >> 2;
        uint32_t bytes_left = data_size & 3;

        entry->data = dst;
        while (words != 0) {
            *(uint32_t *)dst = *(const uint32_t *)src;
            src = src + 4;
            dst = dst + 4;
            words = words - 1;
        }
        while (bytes_left != 0) {
            *dst = *src;
            src = src + 1;
            dst = dst + 1;
            bytes_left = bytes_left - 1;
        }
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_list_default_item_format.c.txt for the recovery notes.
 *
 * @address 0x4a8310
 */
uint8_t UiLists::list_default_item_format(void *item_buffer, int32_t item_index, void *list_items)
{
    uint16_t *out = (uint16_t *)item_buffer;
    const uint16_t *name = halo::interface::wide(L"");

    if (item_index >= 0 && item_index < ui_lists[ui_list_current].count) {
        name = ((ui_list_item *)ui_lists[ui_list_current].data)[item_index].name;
    }
    wcsncpy((wchar_t *)out, (const wchar_t *)name, 0x3f);
    out[0x3f] = 0;
    return (uint8_t)(out[0] != 0);
}

/**
 * Scans one of the three UI selection lists for the first entry flagged is_default, returning its index, or -1
 * if ui_list_has_default is clear, the list is empty, or no entry is flagged.
 *
 * Register convention: group index -> ECX
 *
 * @address 0x4a7cb0
 */
int32_t UiLists::list_find_default(int32_t group_index)
{
    int32_t count;
    uint8_t *entry;
    int32_t index;

    if (!ui_list_has_default) {
        return -1;
    }
    count = ui_lists[group_index].count;
    if (count <= 0) {
        return -1;
    }
    entry = (uint8_t *)ui_lists[group_index].data + 0x0c;
    index = 0;
    while (*entry == 0) {
        index = index + 1;
        entry = entry + 0x10;
        if (count <= index) {
            return -1;
        }
    }
    return index;
}

/**
 * Frees every GlobalAlloc'd name and data blob across all three UI selection lists, then frees each list's own
 * backing storage and resets its element_size/count to -1.
 *
 * @address 0x4a7b20
 */
void UiLists::list_free_all(void)
{
    int32_t group;

    for (group = 0; group < 3; group = group + 1) {
        int32_t count = ui_lists[group].count;
        int32_t i;

        for (i = 0; i < count; i = i + 1) {
            ui_list_item *entry = (ui_list_item *)ui_lists[group].data + i;
            if (entry->name != 0) {
                GlobalFree(entry->name);
            }
            if (entry->data != 0) {
                GlobalFree(entry->data);
            }
        }

        ui_lists[group].element_size = -1;
        ui_lists[group].count = -1;
        if (ui_lists[group].data != 0) {
            GlobalFree(ui_lists[group].data);
            ui_lists[group].data = 0;
        }
    }
}

/**
 * Returns the data pointer of the current UI selection list's entry at index, or NULL if index is out of range.
 *
 * Register convention: index -> EDX
 *
 * @address 0x4a7c50
 */
void * UiLists::list_get_data(int32_t index)
{
    if (index > -1 && index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + index;
        return entry->data;
    }
    return 0;
}

/**
 * Returns the id of the current UI selection list's entry at index, or -1 if index is out of range.
 *
 * Register convention: index -> EDX
 *
 * @address 0x4a7c80
 */
int32_t UiLists::list_get_id(int32_t index)
{
    if (index > -1 && index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + index;
        return entry->id;
    }
    return -1;
}

/**
 * Formats item_index's display name out of the current ui_lists group into a 64 wide-char buffer, and reports
 * map_list[item_index]'s cache_file_exists byte as the success/validity result (see the header note -- the two
 * lists are not obviously related).
 *
 * @address 0x4a83d0
 */
uint8_t UiLists::list_item_format_name_and_cache_flag(uint16_t *out_name, int32_t item_index)
{
    const uint16_t *source = halo::interface::wide(L"");
    uint8_t result;

    if (item_index > -1 && item_index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + item_index;
        source = entry->name;
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 0x3f);
    out_name[0x3f] = 0;

    result = 0;
    if (out_name[0] != 0 && item_index > -1 && item_index < map_list_count) {
        result = map_list[item_index].cache_file_exists;
    }
    return result;
}

/**
 * Recomputes the first visible row of a scrollable list widget so its window still covers the row named by the
 * widget's cached "selected index" (offset 0x3c), clamping to the widget's tag-defined row count and its own
 * item_count, and caches the result back at offset 0x3e.
 *
 * Register convention: widget -> EAX
 *
 * @address 0x4a7d00
 */
int32_t UiLists::list_widget_compute_scroll_start(widget_instance *widget)
{
    UIWidgetDefinition *tag_data;
    int32_t visible_rows;
    int16_t *scroll_start = (int16_t *)((uint8_t *)widget + 0x3e);
    int16_t *selected_index_field = &halo::interface::widget_list_committed(widget);
    int32_t scroll_start_value;
    int32_t selected_index;
    int32_t window_size;
    int32_t item_count;
    uint8_t needs_paging;

    tag_data = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
    visible_rows = (int32_t)tag_data->child_widgets.count;
    scroll_start_value = *scroll_start;

    if (widget->first_child != 0 && widget->first_child->first_child != 0 &&
        widget->first_child->first_child->widget_type == uiwidgettype_spinner_list) {
        visible_rows = visible_rows - 1;
    }

    item_count = (uint16_t)widget->item_count;
    needs_paging = (!halo::interface::has_bit(tag_data->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_single_preview_no_scroll) && (visible_rows - 1 < item_count)) ? 0 : 1;
    window_size = visible_rows - (needs_paging ? 1 : 3);
    if (item_count < window_size) {
        window_size = item_count;
    }

    selected_index = *selected_index_field;
    if (selected_index < scroll_start_value || window_size + scroll_start_value <= selected_index) {
        if (selected_index < 0) {
            *scroll_start = 0;
            return 0;
        }
        *scroll_start = (int16_t)((item_count - window_size <= selected_index) ?
                                       (item_count - window_size) : selected_index);
    } else if (scroll_start_value == -1) {
        *scroll_start = 0;
        return *scroll_start;
    }
    return *scroll_start;
}

/**
 * Item-format callback: fills a caller-owned 0x80 byte scratch buffer for the row at item_index out of the
 * caller-supplied list_items array (widget->list_items), returning nonzero on success. Rebuilds a scrollable
 * selection-list widget's visible child rows to reflect one of the three shared ui_lists groups (network
 * adapters, ports, etc.), first re-selecting which group is current if the widget's embedded group-picker
 * spinner_list says it changed, then walking the widget's row children, formatting each one through format_item
 * and toggling its label/value sub-widgets' visibility, scale and highlight color to match focus and scroll
 * position.
 *
 * Register convention: widget -> first parameter, format_item -> second parameter
 *
 * @address 0x4a7db0
 */
void UiLists::list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item)
{
    UIWidgetDefinition *tag_data = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
    int32_t visible_rows = (int32_t)tag_data->child_widgets.count;
    widget_instance *first_row = widget->first_child;
    int16_t *scroll_start_field = (int16_t *)((uint8_t *)widget + 0x3e);
    int16_t *selected_index_field = &halo::interface::widget_list_committed(widget);
    int32_t scroll_start = *scroll_start_field;
    uint8_t has_embedded_spinner;
    uint8_t focused_not_last;
    int32_t group_index = 0;
    uint8_t group_changed = 0;
    widget_instance *spinner = first_row;

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
                if (probe > 2) {
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
            int32_t default_entry;
forced_rebuild:
            default_entry = halo::interface::ui_list_find_default(group_index);
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
        int32_t item_count = (uint16_t)widget->item_count;
        int32_t window_size;
        uint8_t single_page;
        int32_t row_slot;
        int32_t item_index;
        widget_instance *row;

        if (has_embedded_spinner) {
            visible_rows = visible_rows - 1;
        }
        single_page = (halo::interface::has_bit(tag_data->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_single_preview_no_scroll)) || (visible_rows - 1 >= item_count);
        window_size = visible_rows - (single_page ? 1 : 3);
        if (item_count < window_size) {
            window_size = item_count;
        }
        if (scroll_start == -1) {
            scroll_start = halo::interface::ui_list_widget_compute_scroll_start(widget);
        }

        row = widget->first_child;
        row_slot = 0;
        item_index = scroll_start;
        for (; row != 0 && row->next_sibling != 0; row = row->next_sibling) {
            widget_instance *label;
            widget_instance *value;
            widget_instance *value2;
            widget_instance *value3;

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
                        goto tail;
                    }
                    if (!single_page) goto near_end_row;
                    goto render_row;
                } else {
                    if ((row_slot != 1) || single_page || !has_embedded_spinner) goto render_row;
                    goto near_end_row;
                }

render_row:
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
                            *(ColorARGB *)&((struct widget_instance *)label)->list_items = *halo::interface::ui_get_saved_pulse_color(&highlight);
                        } else {
                            *(uint32_t *)&((struct widget_instance *)label)->list_items = 0;
                        }
                    } else {
                        if (widget->focused_child == row) {
                            ColorARGB highlight;
                            widget->selection_index = (int16_t)item_index;
                            *(ColorARGB *)&((struct widget_instance *)label)->list_items = *halo::interface::ui_get_saved_pulse_color(&highlight);
                        } else {
                            *(uint32_t *)&((struct widget_instance *)label)->list_items = 0;
                        }
                        row->background_bitmap_frame = 0;
                    }
                    value->state = 0;

                    item_buffer = halo::memory::heap_reallocate(label->text, 0x80, widget_memory_pool);
                    label->text = item_buffer;
                    if (item_buffer == 0 || !format_item(item_buffer, item_index, widget->list_items)) {
                        row->scale = 0.333f ;
                        goto row_hidden;
                    }
                    if (focused_not_last || item_index == (int16_t)*selected_index_field) {
                        row->scale = 1.0f;
                        row->hidden = 0;
                    } else {
                        row->scale = 0.333f ;
                        row->hidden = 0;
                    }
                    goto tail;
                } else {
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
                        row->scale = 0.333f ;
                        if (widget->focused_child == row) {
                            widget->focused_child = row->previous_sibling;
                        }
                    }
                    goto tail;
                }

near_end_row:
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
                    row->scale = 0.333f ;
                    if (widget->focused_child == row) {
                        item_index = item_index - 1;
                        widget->focused_child = row->next_sibling;
                        goto tail_no_decrement;
                    }
                } else {
                    row->hidden = 0;
                    row->scale = 1.0f;
                }
                item_index = item_index - 1;
                goto tail_no_decrement;
            } else {
                row->state = 0;
row_hidden:
                row->hidden = 1;
            }
tail:
tail_no_decrement:
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

/**
 * Rebuilds this widget's list rows, then mirrors its cached "selected combo index" (offset 0x3c) into a widget
 * three levels under extended_description, keeping that paired display in sync with the selection.
 *
 * @address 0x4a6810
 */
void UiLists::selection_list_mirror_value_build(widget_instance *widget)
{
    saved_player_profile profile_record;
    int16_t selected_value;
    widget_instance *target;

    profile_record = profile_globals_block[0].profile;
    halo::interface::set_profile_name(widget->extended_description->first_child, profile_record.name);

    selected_value = halo::interface::widget_list_committed(widget);
    target = widget->extended_description->first_child->next_sibling->first_child;
    target->selection_index = selected_value;
    target->next_sibling->background_bitmap_frame = selected_value;

    halo::interface::ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)halo::interface::ui_list_default_item_format));
}

/**
 * Executes the set of actions encoded in a widget event/list-item definition (scenario script call, run_function
 * callback, close/open/replace/go-back widget actions, focus change, sound) when that handler is triggered,
 * honoring try_to_branch_on_failure for the tag's own conditional_widgets when a run_function callback fails.
 *
 * @address 0x49a430
 */
void UiLists::widget_list_item_activate(widget_instance *widget, UIWidgetDefinition *tag, int16_t *event,
                                   EventHandlerReference *handler, uint8_t *out_handled)
{
    uint8_t close_self_via_root = 0;
    uint8_t handled = 0;
    uint8_t ok = 1;
    uint8_t function_failed = 0;
    uint8_t close_current = 0;
    uint8_t close_all = 0;
    int32_t action_kind = 0;

    if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::run_scenario_script) && handler->script.string[0] != 0) {
        int16_t script_index = halo::hs::hs_script_find_by_name(handler->script.string);

        if (script_index != -1) {
            ScenarioScript *scripts = halo::interface::reflexive_elements<ScenarioScript>(halo::scenario::globals().scenario->scripts);

            halo::hs::hs_evaluate_expression(scripts[script_index].root_expression_index);
        }
    }

    if ((int8_t)handler->flags < 0 && handled == 0) {
        int16_t function_id = handler->function;

        if (function_id < 0 || function_id > 0xbd ||
            ((ui_event_function)ui_event_function_table[function_id])(widget, event, &handled) == 0) {
            function_failed = 1;
            goto after_run_function;
        }
    }
    {
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::give_focus_to_widget) && handled == 0) {
            if (halo::interface::tag_handle(handler->widget_tag.tag_id) == 0xffffffffu) {
                ok = 0;
            } else {
                widget_instance *root = widget;
                widget_instance *found;

                while (root->parent != (widget_instance *)0) {
                    root = root->parent;
                }
                found = halo::interface::widget_find_by_tag_id(root, halo::interface::tag_handle(handler->widget_tag.tag_id));
                if (found != (widget_instance *)0) {
                    halo::interface::widget_instance_relink_focus(root, found);
                }
                action_kind = 1;
            }
        }
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::reload_other_widget) && handled == 0 &&
            halo::interface::tag_handle(handler->widget_tag.tag_id) == 0xffffffffu) {
            ok = 0;
        }
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::close_current_widget) && handled == 0) {
            close_current = 1;
        }
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::close_other_widget) && handled == 0 &&
            halo::interface::tag_handle(handler->widget_tag.tag_id) != 0xffffffffu) {
            widget_instance *found = (widget_instance *)0;
            int32_t i;

            for (i = 0; i < 1; i++) {
                if (ui_root_widget[i] != (widget_instance *)0) {
                    found = halo::interface::widget_find_by_tag_id(ui_root_widget[i], halo::interface::tag_handle(handler->widget_tag.tag_id));
                    if (found != (widget_instance *)0) {
                        break;
                    }
                }
            }
            if (found == (widget_instance *)0) {
                ok = 0;
            } else if (found == widget) {
                close_self_via_root = 1;
            } else {
                halo::interface::widget_close(found);
            }
        }
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::close_all_widgets) && handled == 0) {
            close_all = 1;
        }
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::open_widget) && halo::interface::tag_handle(handler->widget_tag.tag_id) != 0xffffffffu) {
            if (halo::interface::widget_reopen_as_root_with_history(widget, halo::interface::tag_handle(handler->widget_tag.tag_id)) == 0) {
                ok = 0;
            } else {
                if (action_kind == 0) action_kind = 2;
                handled = 1;
            }
        }
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::replace_self_w_widget) && handled == 0 &&
            halo::interface::tag_handle(handler->widget_tag.tag_id) != 0xffffffffu) {
            widget_instance *replacement =
                halo::interface::chimera__load_ui_widget(nullptr, halo::interface::tag_handle(handler->widget_tag.tag_id),
                                         widget, widget->controller_index, (datum_index)-1,
                                         (datum_index)-1, -1);

            if (replacement == (widget_instance *)0) {
                ok = 0;
            } else {
                widget_instance *parent = widget->parent;
                widget_instance *next = widget->next_sibling;
                widget_instance *prev = widget->previous_sibling;
                int32_t i;

                if (replacement->previous_sibling != (widget_instance *)0) {
                    replacement->previous_sibling->next_sibling = (widget_instance *)0;
                }
                replacement->previous_sibling = (widget_instance *)0;
                replacement->parent = (widget_instance *)0;
                replacement->local_x = replacement->local_x + widget->local_x;
                replacement->local_y = replacement->local_y + widget->local_y;
                if (parent != (widget_instance *)0) {
                    replacement->parent = parent;
                    if (parent->first_child == widget) parent->first_child = replacement;
                    if (parent->focused_child == widget) parent->focused_child = replacement;
                }
                if (next != (widget_instance *)0) {
                    next->previous_sibling = replacement;
                }
                replacement->next_sibling = next;
                if (prev != (widget_instance *)0) {
                    prev->next_sibling = replacement;
                }
                replacement->previous_sibling = prev;
                for (i = 0; i < 1; i++) {
                    if (ui_root_widget[i] == replacement) {
                        ui_root_widget[i] = (widget_instance *)0;
                        break;
                    }
                }
                if (action_kind == 0) action_kind = 2;
                widget->previous_sibling = (widget_instance *)0;
                widget->next_sibling = (widget_instance *)0;
                widget->parent = (widget_instance *)0;
                close_self_via_root = 1;
            }
        }
        if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::go_back_to_previous_widget)) {
            halo::interface::widget_instance_close_and_restore_previous(widget);
            if (action_kind == 0) action_kind = 3;
            handled = 1;
        }
        if (halo::interface::tag_handle(handler->sound_effect.tag_id) != 0xffffffffu) {
            float position[3] = {0.0f, 1.0f, 1.0f};

            halo::sound::sound_play_new(halo::interface::tag_handle(handler->sound_effect.tag_id), (sound_location *)position, -1, 0, 0, 0, 0);
        }
        if (close_all == 0) {
            widget_instance *target = widget;

            if (close_current != 0) {
                while (target->parent != (widget_instance *)0) {
                    target = target->parent;
                }
                halo::interface::widget_close(target);
                handled = 1;
                goto after_close;
            }
            if (close_self_via_root != 0) {
                halo::interface::widget_close(target);
                handled = 1;
                goto after_close;
            }
        } else {
            if (ui_root_widget[0] != (widget_instance *)0) {
                halo::interface::widget_close(ui_root_widget[0]);
            }
            {
                widget_history_node *node = ui_widget_history[0];

                while (node != (widget_history_node *)0) {
                    heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
                    uint32_t size = block->size;
                    int32_t slot = block->slot;

                    ui_widget_history[0] = node->next;
                    if (block->previous != (heap_block *)0) block->previous->next = block->next;
                    if (block->next != (heap_block *)0) block->next->previous = block->previous;
                    if (block == widget_memory_pool->first_block) widget_memory_pool->first_block = block->next;
                    if (block == widget_memory_pool->last_block) widget_memory_pool->last_block = block->previous;
                    widget_memory_pool->blocks[slot] = (heap_block *)0;
                    widget_memory_pool->next_free_slot =
                        (widget_memory_pool->first_block != (heap_block *)0) ? slot : 0;
                    widget_memory_pool->bytes_allocated -= (int32_t)(size & halo::interface::k_pool_block_size_mask);
                    widget_memory_pool->allocation_count -= 1;
                    node = ui_widget_history[0];
                }
            }
            handled = 1;
        }
        if (ok != 0) {
            goto after_run_function;
        }
    }

after_close:
after_run_function:
    if (halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::try_to_branch_on_failure) && tag->conditional_widgets.count > 0) {
        uint8_t *entries = (uint8_t *)tag->conditional_widgets.pointer;
        int32_t i;

        for (i = 0; i < tag->conditional_widgets.count; i++) {
            ConditionalWidgetReference *entry = (ConditionalWidgetReference *)(entries + i * 0x50);

            if (function_failed == 1 && halo::interface::has_bit(entry->flags, halo::tags::conditional_widget_reference_tag_flag::load_if_event_handler_function_fails) && handled == 0) {
                datum_index open_tag = halo::interface::tag_handle(entry->widget_tag.tag_id);

                if (open_tag != (datum_index)-1 && halo::interface::widget_reopen_as_root_with_history(widget, open_tag) != 0) {
                    handled = 1;
                }
            }
        }
    }

    halo::interface::widget_play_sound_effect((int16_t)action_kind);
    *out_handled = handled;
}

}
