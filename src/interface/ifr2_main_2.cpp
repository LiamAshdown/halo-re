#include "win32.h"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/interface/ifr2_main.hpp"
#include "halo/interface/engine_state.hpp"
#include "saved_games.h"
#include "input.h"

extern "C" input_event_queue input_event_queue_active;
#include "crt.h"
#include <string.h>
#include <ctype.h>
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern uint8_t ui_force_quit;
extern uint8_t ui_split_screen;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t split_screen_quit_prompt_armed;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern int32_t ui_time_milliseconds;
extern loading_thread_record *loading_thread;
extern int16_t loading_thread_result;
extern uint8_t ui_input_batch_mode;
extern uint8_t virtual_keyboard;
extern widget_instance *ui_root_widget[1];
extern widget_history_node *ui_widget_history[3];
extern ui_pending_error ui_pending_error_alternate;
extern uint8_t ui_cursor_changed;
extern uint8_t ui_widget_opened;
extern int32_t ui_cursor_x;
extern int32_t ui_cursor_y;
extern uint8_t controls_input_capture_flags;
extern void display_error(int16_t error_string_index, int32_t unknown, uint8_t modal, uint8_t is_error);
extern uint8_t ui_check_for_pause_game(void);
extern void virtual_keyboard_process_input(void);
extern void widget_instance_handle_input_event(widget_instance *widget, UIWidgetDefinition *tag, uint8_t *event_scratch, uint8_t *out_handled);
extern void list_node_pop(widget_history_node *out, widget_history_node **head);
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index, widget_instance *parent, uint16_t controller_index, datum_index history_definition, datum_index history_list_definition, int16_t history_selection);
extern void widget_instance_select_list_index(widget_instance *widget, datum_index list_definition , int32_t selection );
extern widget_instance *widget_instance_find_at_point(widget_instance *root, int32_t x, int32_t y, int32_t initial_hint);
extern uint8_t widget_instance_verify_stack_chain(widget_instance *node);
extern void widget_play_sound_effect(int16_t effect_id);
extern void widget_list_scroll_window(int32_t out[3], widget_instance *widget);
extern int32_t widget_get_sibling_index(widget_instance *widget);
extern map_list_entry *map_list;
extern int32_t map_list_count;
extern int32_t map_list_capacity;
extern uint8_t product_id_read;
extern uint32_t cached_product_id;
}

#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002)
#define KEY_QUERY_VALUE 0x0001
#define KEY_WOW64_32KEY 0x0200
#define KEY_READ_32 0x00020019

namespace halo::interface {

/**
 * Either force-quits the process immediately (unloading the keystone/DRM library first) when ui_force_quit is
 * set, or arms a "are you sure you want to quit" confirmation prompt: the split-screen shaped one when
 * ui_split_screen is set, otherwise the single-player error-dialog shaped one (only if neither is already
 * armed).
 *
 * @address 0x499170
 */
void InterfaceMain::handle_quit_request()
{
    if (ui_force_quit != 0) {
        halo::shell::keystone_library_unload();
        ExitProcess(0xffffec7a);
    }
    if (ui_split_screen == 0) {
        if (halo::networking::globals().join_error_code == -1) {
            halo::networking::globals().join_error_code = 0x23;
        }
        split_screen_quit_prompt_string = halo::k_word_none;
        halo::networking::globals().join_error_reason = 0;
        split_screen_quit_prompt_armed = 1;
        ui_force_quit = 0;
        return;
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x23;
        quit_confirm_error_unknown_ae = 0;
        quit_confirm_error_modal = 0;
        quit_confirm_error_is_error = 0;
    }
    ui_force_quit = 0;
}

/**
 * Main per-frame update for the interface/menu system: refreshes ui_time_milliseconds from the performance
 * counter; if a background loading thread is still tracked, polls it and, once it has exited, tears it down
 * and surfaces its queued error (if any) -- either way skipping the rest of this tick's widget-input handling
 * for that case;
 *
 * @address 0x497e80
 */
void InterfaceMain::tick()
{
    large_integer counter;
    widget_instance *root;
    uint8_t handled = 0;

    uint8_t event_scratch[16] = {0};

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    ui_time_milliseconds = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    if (loading_thread != (loading_thread_record *)0) {
        uint32_t exit_code;
        int32_t got_exit_code = GetExitCodeThread(loading_thread->handle, (LPDWORD)&exit_code);

        root = ui_root_widget[0];
        if (got_exit_code != 0 && exit_code != 0x103  ) {
            CloseHandle(loading_thread->handle);
            loading_thread->handle = (void *)0;
            loading_thread->unknown_04 = 0;
            loading_thread = (loading_thread_record *)0;
            ui_input_batch_mode = 0;
            if (loading_thread_result == 1) {
                halo::interface::display_error(0x21, -1, 1, 0);
                root = ui_root_widget[0];
            } else if (loading_thread_result == 2) {
                halo::interface::display_error(0x22, -1, 1, 0);
                root = ui_root_widget[0];
            }
        }
        goto after_widget_pass;
    }

    if (virtual_keyboard == 0) {
        if (ui_pending_error_alternate.error_string_index == -1) {
            if (quit_confirm_error_string_index == -1) {
                uint8_t is_paused = halo::interface::ui_check_for_pause_game();
                widget_instance *widget = ui_root_widget[0];

                root = widget;
                if (widget != (widget_instance *)0) {
                    UIWidgetDefinition *tag = (UIWidgetDefinition *)halo::cache::globals().tag_instances[widget->definition & halo::k_slot_mask].data;
                    int32_t scratch_i;
                    uint8_t looped = 0;

                    for (scratch_i = 0; scratch_i < 16; scratch_i++) event_scratch[scratch_i] = 0;
                    root = widget;

                    if (ui_input_batch_mode == 0) {
                        uint8_t got_event = halo::input::input_queue_pop_event((ui_input_event *)event_scratch, widget->controller_index);

                        if (got_event != 0) {
                            looped = 1;
                            do {
                                if ((is_paused == 0 &&
                                     (halo::interface::widget_instance_handle_input_event(widget, tag, (int16_t *)event_scratch, &handled),
                                      root = ui_root_widget[0], handled == 1)) ||
                                    widget != root) {
                                    break;
                                }
                                got_event = halo::input::input_queue_pop_event((ui_input_event *)event_scratch, widget->controller_index);
                            } while (got_event != 0);
                        }
                    }
                    if (looped == 0 && is_paused == 0) {
                        *(uint16_t *)(event_scratch + 2) = widget->controller_index;
                        halo::interface::widget_instance_handle_input_event(widget, tag, (int16_t *)event_scratch, &handled);
                        root = ui_root_widget[0];
                    }
                    handled = 1;
                    if (root == (widget_instance *)0 && ui_widget_history[0] != (widget_history_node *)0) {
                        widget_history_node popped;

                        halo::interface::list_node_pop(&popped, &ui_widget_history[0]);
                        root = ui_root_widget[0];
                        if (popped.definition != (datum_index)-1) {
                            widget_instance *reopened = halo::interface::chimera__load_ui_widget(
                                (char *)0, popped.definition, (widget_instance *)0,
                                (uint16_t)popped.controller_index, (datum_index)-1, (datum_index)-1, -1);

                            root = ui_root_widget[0];
                            if (reopened != (widget_instance *)0) {

                                halo::interface::widget_instance_select_list_index(reopened, popped.list_definition,
                                                                   popped.selection);
                                root = ui_root_widget[0];
                            }
                        }
                    }
                }
                if (handled != 0) {
                    goto shared_tail;
                }
            } else {

                int16_t error_string_index = quit_confirm_error_string_index;

                if (ui_split_screen != 0 || halo::networking::network_game_is_active() != 0 ||
                    halo::game::globals().game_time->game_time > 0x1d) {
                    halo::interface::display_error(error_string_index, (int32_t)(uint16_t)quit_confirm_error_unknown_ae,
                                  quit_confirm_error_modal, quit_confirm_error_is_error);
                    quit_confirm_error_string_index = -1;
                    root = ui_root_widget[0];
                }
            }
        } else {
            halo::interface::display_error(ui_pending_error_alternate.error_string_index, -1, 1, 0);
            ui_pending_error_alternate.error_string_index = -1;
            root = ui_root_widget[0];
        }
    } else {
        halo::interface::virtual_keyboard_process_input();
        root = ui_root_widget[0];
        goto shared_tail;
    }
    goto after_widget_pass;

shared_tail:
    {
        memset(input_event_queue_active.events, 0, sizeof(input_event_queue_active.events));
        if (ui_cursor_changed != 0 || ui_widget_opened != 0) {
            ui_widget_opened = 0;
            if (root == (widget_instance *)0) {
                goto after_widget_pass;
            }
            {

                widget_instance *hit = halo::interface::widget_instance_find_at_point(
                    root, ui_cursor_x, ui_cursor_y, *(int32_t *)&((struct widget_instance *)root)->local_x);

                root = ui_root_widget[0];
                if (hit != (widget_instance *)0 && hit->parent != (widget_instance *)0 &&
                    halo::interface::widget_instance_verify_stack_chain(hit) == 0) {
                    widget_instance *parent = hit->parent;
                    UIWidgetDefinition *parent_tag =
                        (UIWidgetDefinition *)halo::cache::globals().tag_instances[parent->definition & halo::k_slot_mask].data;

                    if (parent->focused_child != hit) {
                        halo::interface::widget_play_sound_effect(1);
                    }
                    if (parent->widget_type == 2) {
                        if (parent_tag->child_widgets.count > 1) {
                            halo::interface::widget_list_scroll_window((int32_t *)event_scratch, parent);
                            {

                                int32_t sibling = halo::interface::widget_get_sibling_index(hit);
                                int32_t idx = (sibling < 0) ? 0 : ((sibling < 4) ? sibling : 3);

                                parent->selection_index = *(int16_t *)(event_scratch + idx * 4);
                            }
                        }
                        {
                            widget_instance *cursor = hit;

                            while (cursor->parent != (widget_instance *)0) {
                                cursor->parent->focused_child = cursor;
                                cursor = cursor->parent;
                            }
                        }
                    } else if (parent->widget_type == 3) {
                        parent->selection_index = (int16_t)halo::interface::widget_get_sibling_index(hit);
                        parent->focused_child = hit;
                        {
                            widget_instance *cursor = parent;

                            while (cursor->parent != (widget_instance *)0) {
                                widget_instance *up = cursor->parent;

                                up->focused_child = cursor;
                                if (up->widget_type == 3) {
                                    up->selection_index = (int16_t)halo::interface::widget_get_sibling_index(cursor);
                                }
                                cursor = up;
                            }
                        }
                    } else {
                        widget_instance *cursor = hit;

                        while (cursor->parent != (widget_instance *)0) {
                            cursor->parent->focused_child = cursor;
                            cursor = cursor->parent;
                        }
                    }
                }
            }
        }
    }

after_widget_pass:
    if (root != (widget_instance *)0) {
        if ((controls_input_capture_flags & 2) != 0) {
            return;
        }
        controls_input_capture_flags = controls_input_capture_flags | 2;
        return;
    }
    if ((controls_input_capture_flags & 2) != 0) {
        controls_input_capture_flags = controls_input_capture_flags & 0xfd;
    }
}

/**
 * blam-cc: EAX -> path, stack -> map_id Appends one entry to the growable map_list: grows the array by 0x13
 * entries first if it is full, copies `path` into a fresh GlobalAlloc buffer, strips a trailing ".map"
 * extension if present, lowercases the result, and records whether the corresponding cache file (looked up by
 * the filename after the last backslash) exists.
 *
 * @address 0x4950c0
 */
void MapList::add_entry(char *path, int32_t map_id)
{
    map_list_entry *entry;
    uint32_t path_length;
    char *extension;
    char *filename;
    char *cursor;
    cache_file_header header;

    if (map_list_capacity <= map_list_count) {
        map_list_capacity = map_list_capacity + 0x13;
        if (map_list == (map_list_entry *)0) {
            map_list = (map_list_entry *)GlobalAlloc(0, map_list_capacity * sizeof(map_list_entry));
        } else if (map_list_capacity * sizeof(map_list_entry) == 0) {
            GlobalFree(map_list);
            map_list = (map_list_entry *)0;
        } else {
            map_list = (map_list_entry *)GlobalReAlloc(
                map_list, map_list_capacity * sizeof(map_list_entry), 2);
        }
    }

    entry = &map_list[map_list_count];
    entry->path = (char *)0;
    entry->map_id = map_id;
    entry->cache_file_exists = 0;

    path_length = strlen(path);
    entry->path = (char *)GlobalAlloc(0, path_length + 1);
    strcpy(entry->path, path);

    extension = strstr(entry->path, ".map");
    if (extension != (char *)0) {
        *extension = '\0';
    }

    for (cursor = entry->path; *cursor != '\0'; cursor++) {
        *cursor = (char)tolower((uint8_t)*cursor);
    }

    filename = strrchr(entry->path, '\\');
    filename = (filename != (char *)0) ? filename + 1 : entry->path;
    entry->cache_file_exists = halo::cache::cache_file_exists(filename, &header);
    map_list_count = map_list_count + 1;
}

/**
 * Frees every entry's path buffer, then the map_list array itself, and resets the list to empty.
 *
 * @address 0x495260
 */
void MapList::free_all()
{
    int32_t i;

    for (i = 0; i < map_list_count; i++) {
        GlobalFree(map_list[i].path);
    }
    GlobalFree(map_list);
    map_list = (map_list_entry *)0;
    map_list_count = 0;
    map_list_capacity = 0;
}

/**
 * Reads (once, cached thereafter) the game's "PID" registry value under HKLM\Software\Microsoft\Microsoft
 * Games\Halo and returns a pointer to the cached 4 byte value.
 *
 * @address 0x4a8790
 */
void * InterfaceMain::registry_get_product_id()
{
    LSTATUS status;
    HKEY key;
    DWORD size;

    if (product_id_read == 0) {
        size = 0x20;
        product_id_read = 1;
        status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                                KEY_READ_32, (PHKEY)&key);
        if (status == 0) {
            status = RegQueryValueExA(key, "PID", 0, 0, (uint8_t *)&cached_product_id, &size);
            if (status != 0) {
                cached_product_id = 0;
            }
            RegCloseKey(key);
        }
    }
    return &cached_product_id;
}

} // namespace halo::interface

namespace halo::interface {

void interface_handle_quit_request(void)
{
    halo::interface::InterfaceMain::handle_quit_request();
}

void interface_tick(void)
{
    halo::interface::InterfaceMain::tick();
}

void map_list_add_entry(char *path, int32_t map_id)
{
    halo::interface::MapList::add_entry(path, map_id);
}

void map_list_free_all(void)
{
    halo::interface::MapList::free_all();
}

void * registry_get_product_id(void)
{
    return halo::interface::InterfaceMain::registry_get_product_id();
}

}
