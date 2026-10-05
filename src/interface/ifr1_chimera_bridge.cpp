#include "halo/interface/ifr1_chimera_bridge.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
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
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/text/text.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/flags.hpp"
#include "halo/interface/wide_text.hpp"

static auto &join_ui_state = halo::link::ref<progress_screen_state>(halo::networking::vars().join_ui_state);
static auto &interface_loading_screen_address_b = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_b);
static auto &interface_loading_screen_address_a = halo::link::ref<uint32_t>(halo::main::vars().interface_loading_screen_address_a);
static auto &interface_loading_screen_request_id = halo::link::ref<datum_index>(halo::networking::vars().interface_loading_screen_request_id);
static auto &interface_loading_screen_progress = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_progress);
static auto &progress_screen_text = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_text);
static auto &progress_screen_subtext = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_subtext);
static auto &chimera_loading_screen_cleanup_gate = halo::link::ref<uint8_t>(halo::game::vars().chimera_loading_screen_cleanup_gate);
static auto &chat_state_00719a7a = halo::link::ref<uint8_t>(halo::ui::vars().chat_state_00719a7a);
static auto &chat_state_00719a9a = halo::link::ref<uint8_t>(halo::ui::vars().chat_state_00719a9a);
static auto &chat_state_00719a79 = halo::link::ref<uint8_t>(halo::ui::vars().chat_state_00719a79);
static auto &split_screen_quit_prompt_string = halo::link::ref<uint16_t>(halo::ui::vars().split_screen_quit_prompt_string);
static auto &split_screen_quit_prompt_armed = halo::link::ref<uint8_t>(halo::ui::vars().split_screen_quit_prompt_armed);
static auto &main_menu_reload_pending = halo::link::ref<uint8_t>(halo::ui::vars().main_menu_reload_pending);
static auto &ui_input_batch_mode = halo::link::ref<uint8_t>(halo::ui::vars().ui_input_batch_mode);
static auto &loading_thread_result = halo::link::ref<uint8_t>(halo::ui::vars().loading_thread_result);
static auto &loading_thread = halo::link::ref<loading_thread_record *>(halo::ui::vars().loading_thread);
static auto &cached_saved_game_something = halo::link::ref<datum_index>(halo::ui::vars().cached_saved_game_something);
static auto &ui_cursor_bitmap = halo::link::ref<datum_index>(halo::ui::vars().ui_cursor_bitmap);
static auto &ui_widget_opened = halo::link::ref<uint8_t>(halo::ui::vars().ui_widget_opened);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &ui_root_widget = halo::link::ref<widget_instance *[1]>(halo::ui::vars().ui_root_widget);
static auto &ui_widget_history = halo::link::ref<widget_history_node *[3]>(halo::ui::vars().ui_widget_history);

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
    BitmapData *bitmap_data;
    uint32_t packed_color;
    ColorARGB text_color;
    Rectangle2D bounds;
    uint16_t text_buffer[halo::interface::k_long_text_chars];

    if (join_ui_state == 0) {
        return;
    }
    alpha = 1.0f;
    if (interface_loading_screen_address_b == -1) {
        interface_loading_screen_address_b = (int32_t)halo::cseries::time_query_performance_counter_ms();
    }

    if (interface_loading_screen_address_a != halo::k_dword_none) {
        uint32_t now = halo::cseries::time_query_performance_counter_ms();
        if (now >= interface_loading_screen_address_a) {
            interface_loading_screen_address_a = halo::k_dword_none;
            interface_loading_screen_address_b = -1;
            interface_loading_screen_request_id = k_datum_index_none;
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
            split_screen_quit_prompt_string = halo::k_word_none;
            chat_state_00719a7a = 0;
            chat_state_00719a9a = 0;
            chat_state_00719a79 = 0;
            halo::networking::globals().join_error_reason = 0;
            split_screen_quit_prompt_armed = 1;
            return;
        case 4:
            if (interface_loading_screen_request_id != k_datum_index_none) {
                NNCancel(interface_loading_screen_request_id);
                interface_loading_screen_request_id = k_datum_index_none;
                split_screen_quit_prompt_string = halo::k_word_none;
                halo::networking::globals().join_error_reason = 0;
                split_screen_quit_prompt_armed = 1;
            }
            break;
        default:
            break;
        }
    }

    font = halo::interface::lookup_tag(halo::fourcc('f', 'o', 'n', 't'), halo::tag_paths::large_ui_font);
    background = halo::interface::lookup_tag(halo::fourcc('b', 'i', 't', 'm'), halo::tag_paths::shell_background_bitmap);
    strings = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::loading_strings);
    if (font == k_datum_index_none || background == k_datum_index_none || strings == k_datum_index_none) {
        return;
    }

    bitmap_data = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(background, 0, 0);
    packed_color = halo::interface::color_argb_scale_alpha(halo::k_dword_none, alpha);
    text_color.alpha = alpha;
    text_color.red = 1.0f;
    text_color.green = 1.0f;
    text_color.blue = 1.0f;
    bounds.top = 0;
    bounds.left = 0;
    bounds.bottom = halo::interface::k_base_screen_height;
    bounds.right = halo::interface::k_base_screen_width;
    if (bitmap_data != 0) {
        halo::interface::ui_draw_screen_quad(&bounds, &bounds, bitmap_data, nullptr,
                            packed_color);
    }
    halo::text::text_context::set_render_context(font, &text_color, -1, 2, 0);

    bounds.left = 0;
    bounds.top = halo::interface::k_base_screen_height - 70;
    bounds.right = halo::interface::k_base_screen_width;
    bounds.bottom = halo::interface::k_base_screen_height - 50;
    switch (join_ui_state) {
    case 2:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 1));
        halo::interface::draw_text16(0, &bounds, text_buffer);
        break;
    case 3:
    case 5:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 2), progress_screen_text);
        halo::interface::draw_text16(0, &bounds, text_buffer);
        break;
    case 4:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 0), progress_screen_text);
        halo::interface::draw_text16(0, &bounds, text_buffer);
        break;
    case 6:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 3), progress_screen_text,
                              interface_loading_screen_progress);
        halo::interface::draw_text16(0, &bounds, text_buffer);
        break;
    case 7:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 4));
        halo::interface::draw_text16(0, &bounds, text_buffer);
        break;
    case 8:
        halo::text::string_format_wide_va(text_buffer,
                              halo::text::text_string_list_get_string(strings, (halo::networking::globals().game_mode == 2) ? 6 : 5),
                              progress_screen_subtext);
        halo::interface::draw_text16(0, &bounds, text_buffer);
        break;
    case 9:
        halo::text::string_format_wide_va(text_buffer, halo::text::text_string_list_get_string(strings, 8), progress_screen_subtext);
        halo::interface::draw_text16(0, &bounds, text_buffer);
        break;
    default:
        break;
    }

    bounds.top = halo::interface::k_base_screen_height - 50;
    bounds.bottom = halo::interface::k_base_screen_height - 30;
    switch (join_ui_state) {
    case 2: case 3: case 4: case 5: case 6: case 7: case 9:
        halo::interface::draw_text16(0, &bounds, halo::text::text_string_list_get_string(strings, 7));
    case 8:
        bounds.top = halo::interface::k_base_screen_height - 20;
        bounds.bottom = halo::interface::k_base_screen_height;
        halo::interface::draw_text16(0, &bounds, halo::text::text_string_list_get_string(strings, 9));
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
        if (halo::shell::globals().command_line != nullptr) {
            _stricmp(halo::shell::globals().command_line, "xdemo");
        }
        ui_input_batch_mode = 1;
        loading_thread_result = 0;
        loading_thread = (loading_thread_record *)0;
        halo::interface::player_profile_check_storage_and_defaults();
        ui_input_batch_mode = 0;
        halo::input::InputSystem::time_base_resync();
    }
    halo::input::UiEvents::queue_sample_time_update();
    halo::interface::widget_close_all();
    halo::interface::chimera__load_ui_widget(halo::tag_paths::main_menu_widget, k_datum_index_none, (widget_instance *)0, halo::k_word_none,
                            k_datum_index_none, k_datum_index_none, -1);
    if (halo::networking::globals().join_error_code != -1) {
        halo::interface::display_error(halo::networking::globals().join_error_code, -1, 1, 0);
        halo::networking::globals().join_error_code = -1;
    }
    if (halo::main::globals().menu_music_pending == 0) {
        halo::interface::main_menu_play_title_music();
    }
    cached_saved_game_something = k_datum_index_none;
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
widget_instance * ChimeraBridge::load_ui_widget(const char *tag_path, datum_index tag_index, widget_instance *parent, uint16_t controller_index, datum_index history_definition, datum_index history_list_definition, int16_t history_selection)
{
    widget_instance *widget = (widget_instance *)0;
    int16_t slot = (controller_index == halo::k_word_none) ? 0 : (int16_t)controller_index;
    UIWidgetDefinition *tag;

    ui_cursor_bitmap = halo::interface::lookup_tag(halo::fourcc('b', 'i', 't', 'm'), halo::tag_paths::shell_cursor_bitmap);
    ui_widget_opened = 1;

    if (tag_index == k_datum_index_none) {
        tag_index = halo::interface::lookup_tag(halo::fourcc('D', 'e', 'L', 'a'), tag_path);
        if (tag_index == k_datum_index_none) {
            return (widget_instance *)0;
        }
    }
    tag = halo::interface::tag_data<UIWidgetDefinition>(tag_index);
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

        if (history_definition != k_datum_index_none) {
            UIWidgetDefinition *history_tag_data = halo::interface::tag_data<UIWidgetDefinition>(history_definition);

            if (!halo::interface::has_bit(history_tag_data->flags, halo::interface::widget_flag::don_t_push_history)) {
                widget_history_node history_template;

                history_template.definition = history_definition;
                history_template.list_definition = history_list_definition;
                history_template.selection = history_selection;
                history_template.controller_index = previous_controller;
                halo::interface::list_node_prepend(&history_template, &ui_widget_history[slot]);
            }
        }
    }

    if (controller_index == halo::k_word_none) {
        switch (*(int16_t *)&((struct UIWidgetDefinition *)tag)->controller_index) {
        case 0: controller_index = 0; break;
        case 1: controller_index = 1; break;
        case 2: controller_index = 2; break;
        case 3: controller_index = 3; break;
        case 4: controller_index = halo::k_word_none; break;
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
        datum_index sound_tag = halo::interface::lookup_tag(halo::fourcc('l', 's', 'n', 'd'), "sound\\music\\title1\\title1");
        if (sound_tag != k_datum_index_none) {
            halo::sound::sound_looping_stop(sound_tag);
        }
        halo::main::globals().menu_music_pending = 0;
    }
    halo::sound::sound_stop_all();
    if (finalize_render_frame != 0) {
        halo::rasterizer::rasterizer_end_frame();
    }
    if (finalize_render_frame != 0) {
        halo::rasterizer::rasterizer_reset_device_if_needed();
    }
    if (halo::main::globals().menu_music_pending == 0) {
        halo::interface::main_menu_play_title_music();
    }
}

}
