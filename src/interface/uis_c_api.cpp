/**
 * C linkage shims for the interface UI functions: one extern "C" function per original symbol, forwarding to
 * the behaviour classes in namespace halo::ui.
 */

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "cutscene.h"
#include <wchar.h>
#include <stdio.h>
#include "rasterizer.h"
#include <stdlib.h>
#include "saved_games.h"

#include "halo/interface/uis_screens.hpp"
#include "halo/interface/uis_game_setup.hpp"
#include "halo/interface/uis_profiles.hpp"
#include "halo/interface/uis_draw.hpp"
#include "halo/interface/uis_carousels.hpp"
#include "halo/interface/uis_controls_menu.hpp"
#include "halo/interface/uis_event_handlers.hpp"
#include "halo/interface/uis_game_data_inputs.hpp"
#include "halo/interface/uis_lists.hpp"
#include "halo/interface/uis_network_menu.hpp"
#include "halo/interface/uis_strings.hpp"
#include "halo/interface/uis_tab_groups.hpp"
#include "halo/interface/uis_widgets.hpp"

extern "C" void ui_audio_options_apply_volume_sliders(widget_instance *widget)
{
    halo::ui::UiScreens::audio_options_apply_volume_sliders(widget);
}

extern "C" void ui_chat_window_reset_position(void)
{
    halo::ui::UiScreens::chat_window_reset_position();
}

extern "C" uint32_t ui_check_for_pause_game(void)
{
    return halo::ui::UiScreens::check_for_pause_game();
}

extern "C" void ui_cursor_update(void)
{
    halo::ui::UiScreens::cursor_update();
}

extern "C" void ui_error_modal_update(void)
{
    halo::ui::UiScreens::error_modal_update();
}

extern "C" ColorRGB * ui_get_saved_color(ColorRGB *out)
{
    return halo::ui::UiScreens::get_saved_color(out);
}

extern "C" ColorARGB * ui_get_saved_pulse_color(ColorARGB *out)
{
    return halo::ui::UiScreens::get_saved_pulse_color(out);
}

extern "C" void ui_handler_4a68f0(uint8_t *widget)
{
    halo::ui::UiScreens::handler_4a68f0(widget);
}

extern "C" uint32_t ui_build_level_select_list(widget_instance *widget, void *param_2, void *param_3)
{
    return halo::ui::UiGameSetup::build_level_select_list(widget, param_2, param_3);
}

extern "C" void ui_build_level_select_list_coop(widget_instance *widget, void *param_2, void *param_3)
{
    halo::ui::UiGameSetup::build_level_select_list_coop(widget, param_2, param_3);
}

extern "C" void ui_game_variant_flag_list_widget_build(widget_instance *widget)
{
    halo::ui::UiGameSetup::game_variant_flag_list_widget_build(widget);
}

extern "C" void ui_game_variant_list_widget_build(widget_instance *widget)
{
    halo::ui::UiGameSetup::game_variant_list_widget_build(widget);
}

extern "C" uint8_t ui_level_select_confirm_choice(widget_instance *widget)
{
    return halo::ui::UiGameSetup::level_select_confirm_choice(widget);
}

extern "C" uint8_t ui_map_select_confirm_choice(widget_instance *widget)
{
    return halo::ui::UiGameSetup::map_select_confirm_choice(widget);
}

extern "C" uint32_t ui_restart_saved_game(void)
{
    return halo::ui::UiGameSetup::restart_saved_game();
}

extern "C" uint32_t ui_start_campaign_from_level_one(void *widget, int16_t *event)
{
    return halo::ui::UiGameSetup::start_campaign_from_level_one(widget, event);
}

extern "C" uint8_t ui_variant_name_is_available(const uint16_t *name)
{
    return halo::ui::UiGameSetup::variant_name_is_available(name);
}

extern "C" uint32_t ui_build_profile_list(widget_instance *widget)
{
    return halo::ui::UiProfiles::build_profile_list(widget);
}

extern "C" uint32_t ui_free_profile_list(widget_instance *widget)
{
    return halo::ui::UiProfiles::free_profile_list(widget);
}

extern "C" uint32_t ui_new_profile_name_entry_commit(void)
{
    return halo::ui::UiProfiles::new_profile_name_entry_commit();
}

extern "C" uint8_t ui_new_profile_name_entry_open(void *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiProfiles::new_profile_name_entry_open(widget, event, out_handled);
}

extern "C" void ui_profile_carousel_fetch_name(widget_instance *widget)
{
    halo::ui::UiProfiles::profile_carousel_fetch_name(widget);
}

extern "C" void ui_profile_carousel_fetch_sensitivity(widget_instance *widget)
{
    halo::ui::UiProfiles::profile_carousel_fetch_sensitivity(widget);
}

extern "C" void ui_profile_carousel_slot_cache_populate(int32_t count, const int32_t *candidate_ids)
{
    halo::ui::UiProfiles::profile_carousel_slot_cache_populate(count, candidate_ids);
}

extern "C" void ui_profile_details_list_widget_build(widget_instance *widget)
{
    halo::ui::UiProfiles::profile_details_list_widget_build(widget);
}

extern "C" uint8_t ui_profile_list_apply_selection(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiProfiles::profile_list_apply_selection(widget, event, out_handled);
}

extern "C" uint32_t ui_profile_list_apply_selection_for_player(widget_instance *widget, int16_t *context)
{
    return halo::ui::UiProfiles::profile_list_apply_selection_for_player(widget, context);
}

extern "C" uint8_t ui_profile_require_existing(void *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiProfiles::profile_require_existing(widget, event, out_handled);
}

extern "C" uint8_t ui_profile_select_or_create(void *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiProfiles::profile_select_or_create(widget, event, out_handled);
}

extern "C" void ui_button_prompt_draw_icon(HUDGlobalsButtonIcon *icon)
{
    halo::ui::UiDraw::button_prompt_draw_icon(icon);
}

extern "C" int16_t ui_button_prompt_index_from_string(uint16_t *text)
{
    return halo::ui::UiDraw::button_prompt_index_from_string(text);
}

extern "C" void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect)
{
    halo::ui::UiDraw::draw_filled_rectangle(packed_color, rect);
}

extern "C" void ui_draw_rotated_screen_quad(int16_t *origin, int32_t source_record, float *corner_uvs,
                                  float scale, float rotation_radians, float alpha_fraction)
{
    halo::ui::UiDraw::draw_rotated_screen_quad(origin, source_record, corner_uvs, scale, rotation_radians, alpha_fraction);
}

extern "C" void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                          int16_t *clip_rect, uint32_t vertex_color)
{
    halo::ui::UiDraw::draw_screen_quad(source_rect, dest_rect, bitmap_data, clip_rect, vertex_color);
}

extern "C" void ui_draw_trouble_brewing_indicator(void)
{
    halo::ui::UiDraw::draw_trouble_brewing_indicator();
}

extern "C" uint8_t ui_string_has_button_prompt_token(uint16_t *text)
{
    return halo::ui::UiDraw::string_has_button_prompt_token(text);
}

extern "C" void ui_widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color, const uint16_t *text)
{
    halo::ui::UiDraw::widget_draw_formatted_prompt_string(bounds, use_text_color, text);
}

extern "C" void ui_widget_draw_prompt_span(const uint16_t *text, Rectangle2D *cursor, Rectangle2D *origin)
{
    halo::ui::UiDraw::widget_draw_prompt_span(text, cursor, origin);
}

extern "C" int32_t ui_carousel_slot_compare_valid_first(const int32_t *a, const int32_t *b)
{
    return halo::ui::UiCarousels::carousel_slot_compare_valid_first(a, b);
}

extern "C" void ui_level_carousel_refresh(widget_instance *widget)
{
    halo::ui::UiCarousels::level_carousel_refresh(widget);
}

extern "C" void ui_level_carousel_row_refresh(widget_instance *widget, int32_t level_index)
{
    halo::ui::UiCarousels::level_carousel_row_refresh(widget, level_index);
}

extern "C" void ui_map_list_carousel_refresh_window(widget_instance *widget)
{
    halo::ui::UiCarousels::map_list_carousel_refresh_window(widget);
}

extern "C" void ui_variant_carousel_slot_cache_populate(int32_t *candidate_ids, int32_t count)
{
    halo::ui::UiCarousels::variant_carousel_slot_cache_populate(candidate_ids, count);
}

extern "C" void ui_controls_4wide_selector_refresh(widget_instance *widget)
{
    halo::ui::UiControlsMenu::controls_4wide_selector_refresh(widget);
}

extern "C" uint32_t ui_controls_options_free_list(widget_instance *widget)
{
    return halo::ui::UiControlsMenu::controls_options_free_list(widget);
}

extern "C" uint32_t ui_controls_options_populate_from_profile(widget_instance *widget)
{
    return halo::ui::UiControlsMenu::controls_options_populate_from_profile(widget);
}

extern "C" uint8_t ui_controls_options_reload_profile(void)
{
    return halo::ui::UiControlsMenu::controls_options_reload_profile();
}

extern "C" void ui_controls_populate_bind_rows(widget_instance *widget, uint32_t packed)
{
    halo::ui::UiControlsMenu::controls_populate_bind_rows(widget, packed);
}

extern "C" void ui_controls_populate_input_row(widget_instance *widget, const uint8_t *profile_record)
{
    halo::ui::UiControlsMenu::controls_populate_input_row(widget, profile_record);
}

extern "C" void ui_controls_populate_sensitivity_row(widget_instance *widget, const uint8_t *profile_record)
{
    halo::ui::UiControlsMenu::controls_populate_sensitivity_row(widget, profile_record);
}

extern "C" uint32_t ui_controls_sensitivity_row_refresh(widget_instance *widget, const uint8_t *profile_record)
{
    return halo::ui::UiControlsMenu::controls_sensitivity_row_refresh(widget, profile_record);
}

extern "C" uint8_t ui_event_49cdd0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49cdd0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49cfa0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49cfa0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d0d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d0d0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d100(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d100(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d120(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d120(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d140(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d140(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d160(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d160(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d1a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d1a0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d1b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d1b0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d440(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d440(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d450(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d450(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d480(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d480(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d520(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d520(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d540(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d540(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d5b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d5b0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d5d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d5d0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d5f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d5f0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d7a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d7a0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49d8b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49d8b0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49dab0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49dab0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49dbc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49dbc0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49dca0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49dca0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49e170(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49e170(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49e210(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49e210(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49e220(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49e220(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49e2c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49e2c0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49e300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49e300(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49e5d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49e5d0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49e7e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49e7e0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49ea50(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49ea50(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49edc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49edc0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49f030(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49f030(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49f300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49f300(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49f470(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49f470(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49f560(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49f560(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49f610(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49f610(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49f680(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49f680(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49f8f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49f8f0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49fad0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49fad0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_49fd30(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_49fd30(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a02a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a02a0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0590(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0590(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0700(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0700(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a07e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a07e0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0860(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0860(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0a80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0a80(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0ae0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0ae0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0bc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0bc0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0c00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0c00(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0c60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0c60(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0d60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0d60(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0e90(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0e90(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a0fb0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a0fb0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a10f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a10f0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1180(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1180(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a11e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a11e0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1280(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1280(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a12c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a12c0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a12f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a12f0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1310(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1310(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1480(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1480(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1570(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1570(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a15e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a15e0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1650(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1650(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a16a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a16a0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a16c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a16c0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a16d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a16d0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a16e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a16e0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1700(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1700(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1740(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1740(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1790(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1790(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1900(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1900(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1b00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1b00(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1b60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1b60(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1bf0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1bf0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1c80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1c80(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1ca0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1ca0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1cd0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1cd0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1d00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1d00(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1d30(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1d30(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1d60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1d60(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1d90(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1d90(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a1dc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a1dc0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a2190(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a2190(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a21c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a21c0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a2490(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a2490(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a24c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a24c0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a2950(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a2950(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a2a00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a2a00(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a2c50(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a2c50(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a2c80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a2c80(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a2f10(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a2f10(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3000(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3000(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3050(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3050(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3150(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3150(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a33a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a33a0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3510(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3510(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3540(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3540(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3790(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3790(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3870(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3870(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a39c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a39c0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a39e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a39e0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3a70(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3a70(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a3d40(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a3d40(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a4110(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a4110(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a4190(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a4190(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a41a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a41a0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a4270(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a4270(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a44f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a44f0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a4570(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a4570(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a4580(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a4580(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a45d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a45d0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a45f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a45f0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a47b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a47b0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a4870(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a4870(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4a4af0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4a4af0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4b4980(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4b4980(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4b4af0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4b4af0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4b4c40(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4b4c40(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4b52f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4b52f0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4b5350(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4b5350(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4b54a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4b54a0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4b54c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4b54c0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4bb290(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4bb290(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4bb300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4bb300(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4bb360(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4bb360(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4bb7e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4bb7e0(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4bb970(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4bb970(widget, event, out_handled);
}

extern "C" uint8_t ui_event_4bba80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiEventHandlers::event_4bba80(widget, event, out_handled);
}

extern "C" void ui_game_data_input_4a3b70(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a3b70(widget);
}

extern "C" void ui_game_data_input_4a4c70(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a4c70(widget);
}

extern "C" void ui_game_data_input_4a5740(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a5740(widget);
}

extern "C" void ui_game_data_input_4a6880(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6880(widget);
}

extern "C" void ui_game_data_input_4a6a60(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6a60(widget);
}

extern "C" void ui_game_data_input_4a6ab0(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6ab0(widget);
}

extern "C" void ui_game_data_input_4a6b70(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6b70(widget);
}

extern "C" void ui_game_data_input_4a6d50(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6d50(widget);
}

extern "C" void ui_game_data_input_4a6e50(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6e50(widget);
}

extern "C" void ui_game_data_input_4a6e90(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6e90(widget);
}

extern "C" void ui_game_data_input_4a6f00(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6f00(widget);
}

extern "C" void ui_game_data_input_4a6fa0(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a6fa0(widget);
}

extern "C" void ui_game_data_input_4a7180(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7180(widget);
}

extern "C" void ui_game_data_input_4a7210(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7210(widget);
}

extern "C" void ui_game_data_input_4a7280(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7280(widget);
}

extern "C" void ui_game_data_input_4a7300(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7300(widget);
}

extern "C" void ui_game_data_input_4a7340(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7340(widget);
}

extern "C" void ui_game_data_input_4a7350(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7350(widget);
}

extern "C" void ui_game_data_input_4a73d0(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a73d0(widget);
}

extern "C" void ui_game_data_input_4a7660(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7660(widget);
}

extern "C" void ui_game_data_input_4a7880(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4a7880(widget);
}

extern "C" void ui_game_data_input_4b5ce0(widget_instance *widget)
{
    halo::ui::UiGameDataInputs::input_4b5ce0(widget);
}

extern "C" void ui_list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob,
                        uint32_t data_size, uint8_t is_default)
{
    halo::ui::UiLists::list_add_entry(group_index, name, id, data_blob, data_size, is_default);
}

extern "C" uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items)
{
    return halo::ui::UiLists::list_default_item_format(item_buffer, item_index, list_items);
}

extern "C" int32_t ui_list_find_default(int32_t group_index)
{
    return halo::ui::UiLists::list_find_default(group_index);
}

extern "C" void ui_list_free_all(void)
{
    halo::ui::UiLists::list_free_all();
}

extern "C" void * ui_list_get_data(int32_t index)
{
    return halo::ui::UiLists::list_get_data(index);
}

extern "C" int32_t ui_list_get_id(int32_t index)
{
    return halo::ui::UiLists::list_get_id(index);
}

extern "C" uint8_t ui_list_item_format_name_and_cache_flag(uint16_t *out_name, int32_t item_index)
{
    return halo::ui::UiLists::list_item_format_name_and_cache_flag(out_name, item_index);
}

extern "C" int32_t ui_list_widget_compute_scroll_start(widget_instance *widget)
{
    return halo::ui::UiLists::list_widget_compute_scroll_start(widget);
}

extern "C" void ui_list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item)
{
    halo::ui::UiLists::list_widget_rebuild_rows(widget, format_item);
}

extern "C" void ui_selection_list_mirror_value_build(widget_instance *widget)
{
    halo::ui::UiLists::selection_list_mirror_value_build(widget);
}

extern "C" void ui_widget_list_item_activate(widget_instance *widget, UIWidgetDefinition *tag, int16_t *event,
                                   EventHandlerReference *handler, uint8_t *out_handled)
{
    halo::ui::UiLists::widget_list_item_activate(widget, tag, event, handler, out_handled);
}

extern "C" void ui_network_adapter_details_refresh(widget_instance *widget)
{
    halo::ui::UiNetworkMenu::network_adapter_details_refresh(widget);
}

extern "C" void ui_network_adapter_list_widget_build(widget_instance *widget)
{
    halo::ui::UiNetworkMenu::network_adapter_list_widget_build(widget);
}

extern "C" uint8_t ui_network_client_connect_and_save(void)
{
    return halo::ui::UiNetworkMenu::network_client_connect_and_save();
}

extern "C" uint8_t ui_network_game_options_populate(widget_instance *widget, const uint8_t *options_record)
{
    return halo::ui::UiNetworkMenu::network_game_options_populate(widget, options_record);
}

extern "C" void ui_network_game_options_refresh(widget_instance *widget, const uint8_t *options_record)
{
    halo::ui::UiNetworkMenu::network_game_options_refresh(widget, options_record);
}

extern "C" uint8_t ui_network_host_setup_defaults_init(widget_instance *widget)
{
    return halo::ui::UiNetworkMenu::network_host_setup_defaults_init(widget);
}

extern "C" void ui_network_host_setup_refresh(widget_instance *widget)
{
    halo::ui::UiNetworkMenu::network_host_setup_refresh(widget);
}

extern "C" void ui_network_name_fields_refresh(widget_instance *widget)
{
    halo::ui::UiNetworkMenu::network_name_fields_refresh(widget);
}

extern "C" uint32_t ui_network_name_fields_reset(void)
{
    return halo::ui::UiNetworkMenu::network_name_fields_reset();
}

extern "C" void ui_network_wait_timeout_check(void)
{
    halo::ui::UiNetworkMenu::network_wait_timeout_check();
}

extern "C" void ui_network_wait_timeout_start(void)
{
    halo::ui::UiNetworkMenu::network_wait_timeout_start();
}

extern "C" uint8_t ui_server_list_connect_selected(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::ui::UiNetworkMenu::server_list_connect_selected(widget, event, out_handled);
}

extern "C" uint8_t ui_server_type_option_selected(widget_instance *widget)
{
    return halo::ui::UiNetworkMenu::server_type_option_selected(widget);
}

extern "C" int32_t ui_real_to_int_truncate(float value)
{
    return halo::ui::UiStrings::real_to_int_truncate(value);
}

extern "C" void * ui_replace_empty(widget_instance *widget)
{
    return halo::ui::UiStrings::replace_empty(widget);
}

extern "C" void * ui_replace_player_number(widget_instance *widget)
{
    return halo::ui::UiStrings::replace_player_number(widget);
}

extern "C" void * ui_replace_product_id(widget_instance *widget)
{
    return halo::ui::UiStrings::replace_product_id(widget);
}

extern "C" void * ui_replace_version(widget_instance *widget)
{
    return halo::ui::UiStrings::replace_version(widget);
}

extern "C" const uint16_t * ui_search_replace_function_call(int16_t index, widget_instance *widget)
{
    return halo::ui::UiStrings::search_replace_function_call(index, widget);
}

extern "C" int32_t ui_string_replace_all(wchar_t *search, uint16_t *replacement, wchar_t **buffer)
{
    return halo::ui::UiStrings::string_replace_all(search, replacement, buffer);
}

extern "C" uint8_t ui_wide_string_has_non_whitespace(const uint16_t *text)
{
    return halo::ui::UiStrings::wide_string_has_non_whitespace(text);
}

extern "C" void ui_tab_group_sync_5wide(widget_instance *widget)
{
    halo::ui::UiTabGroups::tab_group_sync_5wide(widget);
}

extern "C" void ui_tab_group_sync_7wide(widget_instance *widget)
{
    halo::ui::UiTabGroups::tab_group_sync_7wide(widget);
}

extern "C" void ui_tab_group_sync_9wide(widget_instance *widget)
{
    halo::ui::UiTabGroups::tab_group_sync_9wide(widget);
}

extern "C" void ui_tab_group_sync_grouped(widget_instance *widget)
{
    halo::ui::UiTabGroups::tab_group_sync_grouped(widget);
}

extern "C" void ui_widget_sync_profile_status_flag(widget_instance *widget)
{
    halo::ui::UiWidgets::widget_sync_profile_status_flag(widget);
}

extern "C" void ui_widget_text_ensure_and_refresh(widget_instance *widget)
{
    halo::ui::UiWidgets::widget_text_ensure_and_refresh(widget);
}

extern "C" void ui_widget_text_from_hud_objective(widget_instance *widget)
{
    halo::ui::UiWidgets::widget_text_from_hud_objective(widget);
}
