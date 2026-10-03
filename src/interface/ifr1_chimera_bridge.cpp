#include "halo/interface/ifr1_chimera_bridge.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern progress_screen_state join_ui_state;
extern int32_t interface_loading_screen_address_b;
extern uint32_t interface_loading_screen_address_a;
extern datum_index interface_loading_screen_request_id;
extern int32_t interface_loading_screen_progress;
extern uint16_t progress_screen_text[0x20];
extern uint16_t progress_screen_subtext[0x20];
extern uint8_t chimera_loading_screen_cleanup_gate;
extern uint8_t chat_state_00719a7a;
extern uint8_t chat_state_00719a9a;
extern uint8_t chat_state_00719a79;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t split_screen_quit_prompt_armed;
extern int32_t bitmap_group_sequence_get_bitmap_data(datum_index bitmap, int16_t sequence,
                                                     int16_t frame);
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text);
extern void NNCancel(datum_index tag);
extern uint8_t main_menu_reload_pending;
extern uint8_t ui_input_batch_mode;
extern uint8_t loading_thread_result;
extern loading_thread_record *loading_thread;
extern datum_index cached_saved_game_something;
extern datum_index ui_cursor_bitmap;
extern uint8_t ui_widget_opened;
extern heap *widget_memory_pool;
extern widget_instance *ui_root_widget[1];
extern widget_history_node *ui_widget_history[3];
extern void sound_looping_stop(datum_index sound_tag);
extern void sound_stop_all(void);
}

namespace halo::interface {

/**
 * Per-frame progress-screen driver.
 *
 * @address 0x497410
 */
void ChimeraBridge::do_show_loading_screen(void)
{
    float alpha;
    datum_index font, background, strings;
    int32_t bitmap_data;
    uint32_t packed_color;
    ColorARGB text_color;
    Rectangle2D bounds;
    uint16_t text_buffer[0x200];

    if (join_ui_state == 0) {
        return;
    }
    alpha = 1.0f;
    if (interface_loading_screen_address_b == -1) {
        interface_loading_screen_address_b = (int32_t)halo::cseries::time_query_performance_counter_ms();
    }

    if (interface_loading_screen_address_a != 0xffffffffu) {
        uint32_t now = halo::cseries::time_query_performance_counter_ms();
        if (now >= interface_loading_screen_address_a) {
            interface_loading_screen_address_a = 0xffffffffu;
            interface_loading_screen_address_b = -1;
            interface_loading_screen_request_id = (datum_index)-1;
            join_ui_state = (progress_screen_state)0;
            interface_loading_screen_progress = 0;
            progress_screen_text[0] = 0;
            progress_screen_subtext[0] = 0;
            return;
        }
        alpha = (float)(interface_loading_screen_address_a - now) * 0.0013333333f;
        if (alpha < 0.0f) {
            alpha = 0.0f;
        } else if (alpha > 1.0f) {
            alpha = 1.0f;
        }
    } else if (chimera_loading_screen_cleanup_gate != 0) {
        switch (join_ui_state) {
        case 2: case 5: case 6: case 7: case 9:
            chat_state_00719a7a = 0;
            chat_state_00719a9a = 0;
            chat_state_00719a79 = 0;
            halo::networking::globals().host_handoff_requested = 1;
            halo::interface::chat_close();
            return;
        case 3:
            split_screen_quit_prompt_string = 0xffff;
            chat_state_00719a7a = 0;
            chat_state_00719a9a = 0;
            chat_state_00719a79 = 0;
            halo::networking::globals().join_error_reason = 0;
            split_screen_quit_prompt_armed = 1;
            return;
        case 4:
            if (interface_loading_screen_request_id != (datum_index)-1) {
                NNCancel(interface_loading_screen_request_id);
                interface_loading_screen_request_id = (datum_index)-1;
                split_screen_quit_prompt_string = 0xffff;
                halo::networking::globals().join_error_reason = 0;
                split_screen_quit_prompt_armed = 1;
            }
            break;
        default:
            break;
        }
    }

    font = halo::cache::tag_lookup(0x666f6e74, (char *)"ui\\large_ui");
    background = halo::cache::tag_lookup(0x6269746d, (char *)"ui\\shell\\bitmaps\\background");
    strings = halo::cache::tag_lookup(0x75737472, (char *)"ui\\shell\\strings\\loading");
    if (font == (datum_index)-1 || background == (datum_index)-1 || strings == (datum_index)-1) {
        return;
    }

    bitmap_data = reinterpret_cast<int32_t>(halo::bitmaps::bitmap_group_sequence_get_bitmap_data(background, 0, 0));
    packed_color = halo::interface::color_argb_scale_alpha(0xffffffff, alpha);
    text_color.alpha = alpha;
    text_color.red = 1.0f;
    text_color.green = 1.0f;
    text_color.blue = 1.0f;
    bounds.top = 0;
    bounds.left = 0;
    bounds.bottom = 0x1e0;
    bounds.right = 0x280;
    if (bitmap_data != 0) {
        halo::interface::ui_draw_screen_quad((int16_t *)&bounds, (int16_t *)&bounds, bitmap_data, (int16_t *)0,
                            packed_color);
    }
    halo::text::text_set_render_context(font, &text_color, -1, 2, 0);

    bounds.left = 0;
    bounds.top = 0x19a;
    bounds.right = 0x280;
    bounds.bottom = 0x1ae;
    switch (join_ui_state) {
    case 2:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 1));
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)(&bounds), 0, 0, (const int16_t *)text_buffer);
        break;
    case 3:
    case 5:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 2), progress_screen_text);
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)(&bounds), 0, 0, (const int16_t *)text_buffer);
        break;
    case 4:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 0), progress_screen_text);
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)(&bounds), 0, 0, (const int16_t *)text_buffer);
        break;
    case 6:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 3), progress_screen_text,
                              interface_loading_screen_progress);
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)&bounds, 0, 0, (const int16_t *)text_buffer);
        break;
    case 7:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 4));
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)(&bounds), 0, 0, (const int16_t *)text_buffer);
        break;
    case 8:
        halo::text::string_format_wide_va(text_buffer,
                              halo::text::text_string_list_get_string(strings, (halo::networking::globals().game_mode == 2) ? 6 : 5),
                              progress_screen_subtext);
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)&bounds, 0, 0, (const int16_t *)text_buffer);
        break;
    case 9:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 8), progress_screen_subtext);
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)(&bounds), 0, 0, (const int16_t *)text_buffer);
        break;
    default:
        break;
    }

    bounds.top = 0x1ae;
    bounds.bottom = 0x1c2;
    switch (join_ui_state) {
    case 2: case 3: case 4: case 5: case 6: case 7: case 9:
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)(&bounds), 0, 0, (const int16_t *)(halo::text::text_string_list_get_string(strings, 7)));
    case 8:
        bounds.top = 0x1cc;
        bounds.bottom = 0x1e0;
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)(&bounds), 0, 0, (const int16_t *)(halo::text::text_string_list_get_string(strings, 9)));
        break;
    default:
        break;
    }
}

/**
 * Loads and opens the main menu UI widget: if a reload is pending, tears down any tracked loading-thread state
 * and resyncs input timing (checking, but not acting on, whether the command line names the demo build);
 * always resets the first-person weapon interface and closes every open widget first, then opens the main
 * menu, surfaces any pending generic UI error, starts the title music if it is not already pending, and
 * (re)initializes the virtual keyboard.
 *
 * @address 0x4989f0
 */
void ChimeraBridge::load_main_menu(void)
{
    ui_input_batch_mode = 0;
    if (main_menu_reload_pending == 1) {
        if (halo::shell::globals().command_line != (char *)0) {
            _stricmp(halo::shell::globals().command_line, "xdemo");
        }
        ui_input_batch_mode = 1;
        loading_thread_result = 0;
        loading_thread = (loading_thread_record *)0;
        halo::interface::player_profile_check_storage_and_defaults();
        ui_input_batch_mode = 0;
        halo::input::input_time_base_resync();
    }
    halo::input::input_queue_sample_time_update();
    halo::interface::widget_close_all();
    halo::interface::chimera__load_ui_widget((char *)"ui\\shell\\main_menu\\main_menu", (datum_index)-1, (widget_instance *)0, 0xffff,
                            (datum_index)-1, (datum_index)-1, -1);
    if (halo::networking::globals().join_error_code != -1) {
        halo::interface::display_error(halo::networking::globals().join_error_code, -1, 1, 0);
        halo::networking::globals().join_error_code = -1;
    }
    if (halo::main::globals().menu_music_pending == 0) {
        halo::interface::main_menu_play_title_music();
    }
    cached_saved_game_something = (datum_index)-1;
    halo::interface::virtual_keyboard_initialize();
    main_menu_reload_pending = 0;
}

/**
 * Opens a UI widget: resolves the widget tag by index or by path, allocates a widget_instance from the widget
 * heap and initializes it. With no parent the widget becomes the controller slot's root widget (closing
 * whatever was there first) and, when history_definition names a widget whose tag does not set bit 0x4000 of
 * definition+0x2c, a go-back record is pushed holding history_definition, history_list_definition,
 * history_selection and the replaced root's controller_index. With a parent the widget is created as that
 * parent's child and the root and history are left alone (widget_initialize_from_tag does the linking). Also
 * re-caches the UI cursor bitmap tag and marks the UI as having opened a widget.
 * blam-cc: cdecl, 7 stack arguments (every caller pushes 0x1c bytes); objdump 0x497a70..0x497bdc
 *
 * @address 0x497a70
 */
widget_instance * ChimeraBridge::load_ui_widget(char *tag_path, datum_index tag_index, widget_instance *parent, uint16_t controller_index, datum_index history_definition, datum_index history_list_definition, int16_t history_selection)
{
    widget_instance *widget = (widget_instance *)0;
    int16_t slot = (controller_index == 0xffff) ? 0 : (int16_t)controller_index;
    UIWidgetDefinition *tag;

    ui_cursor_bitmap = halo::cache::tag_lookup(0x6269746d, (char *)"ui\\shell\\bitmaps\\cursor");
    ui_widget_opened = 1;

    if (tag_index == (datum_index)-1) {
        tag_index = halo::cache::tag_lookup(0x44654c61, tag_path);
        if (tag_index == (datum_index)-1) {
            return (widget_instance *)0;
        }
    }
    tag = (UIWidgetDefinition *)halo::cache::globals().tag_instances[tag_index & 0xffff].data;
    widget = (widget_instance *)halo::memory::heap_allocate(sizeof(widget_instance), widget_memory_pool);
    if (widget == (widget_instance *)0) {
        return (widget_instance *)0;
    }

    if (parent == (widget_instance *)0) {
        widget_instance *previous_root = ui_root_widget[slot];
        int16_t previous_controller = -1;

        if (previous_root != (widget_instance *)0) {
            previous_controller = previous_root->controller_index;
            halo::interface::widget_close(previous_root);
        }
        ui_root_widget[slot] = widget;

        if (history_definition != (datum_index)-1) {
            uint8_t *history_tag_data = (uint8_t *)halo::cache::globals().tag_instances[history_definition & 0xffff].data;

            if ((*(uint32_t *)(history_tag_data + 0x2c) & 0x4000) == 0) {
                widget_history_node history_template;

                history_template.definition = history_definition;
                history_template.list_definition = history_list_definition;
                history_template.selection = history_selection;
                history_template.controller_index = previous_controller;
                halo::interface::list_node_prepend(&history_template, &ui_widget_history[slot]);
            }
        }
    }

    if (controller_index == 0xffff) {
        switch (*(int16_t *)&((struct UIWidgetDefinition *)tag)->controller_index) {
        case 0: controller_index = 0; break;
        case 1: controller_index = 1; break;
        case 2: controller_index = 2; break;
        case 3: controller_index = 3; break;
        case 4: controller_index = 0xffff; break;
        default: break;
        }
    }
    halo::interface::widget_initialize_from_tag(widget, tag_index, parent, controller_index, tag);
    return widget;
}

/**
 * Stops the main menu's looping title theme if it is still marked pending, stops every other sound, plays the
 * "ending.bik" movie (bracketing it with an end-of-frame/device-reset pair when finalize_render_frame is set),
 * and restarts the title music if it was not already stopped.
 *
 * @address 0x4921a0
 */
void ChimeraBridge::main_menu_music(uint8_t finalize_render_frame)
{
    if (halo::main::globals().menu_music_pending == 1) {
        datum_index sound_tag = halo::cache::tag_lookup(0x6c736e64, (char *)"sound\\music\\title1\\title1");
        if (sound_tag != (datum_index)-1) {
            halo::sound::sound_looping_stop(sound_tag);
        }
        halo::main::globals().menu_music_pending = 0;
    }
    halo::sound::sound_stop_all();
    if (finalize_render_frame != 0) {
        halo::rasterizer::rasterizer_end_frame();
    }
    halo::main::movie_play_bink("ending.bik");
    if (finalize_render_frame != 0) {
        halo::rasterizer::rasterizer_reset_device_if_needed();
    }
    if (halo::main::globals().menu_music_pending == 0) {
        halo::interface::main_menu_play_title_music();
    }
}

}
