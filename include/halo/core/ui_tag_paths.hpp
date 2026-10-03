#pragma once

namespace halo::tag_paths {

/**
 * Tag paths of the multiplayer text lists the game and network code looks up. They are char arrays because
 * tag_lookup takes a mutable C string.
 */
inline char multiplayer_game_text[] = "ui\\multiplayer_game_text";
inline char join_game_rules_strings[] = "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings";
inline char join_game_ticker_labels[] = "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_ticker_labels";
inline char var_vehicle_set[] = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicle_set";
inline char var_trait_with_ball[] = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_trait_with_ball";
inline char var_speed_with_ball[] = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_speed_with_ball";


/** Tag paths of the interface, game and network UI tags, grouped by lookup. */
inline constexpr const char *virtual_keyboard_english = "ui\\english";
inline constexpr const char *large_ui_font = "ui\\large_ui";
inline constexpr const char *random_player_names = "ui\\random_player_names";
inline constexpr const char *shell_background_bitmap = "ui\\shell\\bitmaps\\background";
inline constexpr const char *shell_cursor_bitmap = "ui\\shell\\bitmaps\\cursor";
inline constexpr const char *team_background = "ui\\shell\\bitmaps\\team_background";
inline constexpr const char *team_icon_ctf = "ui\\shell\\bitmaps\\team_icon_ctf";
inline constexpr const char *team_icon_king = "ui\\shell\\bitmaps\\team_icon_king";
inline constexpr const char *team_icon_oddball = "ui\\shell\\bitmaps\\team_icon_oddball";
inline constexpr const char *team_icon_race = "ui\\shell\\bitmaps\\team_icon_race";
inline constexpr const char *team_icon_slayer = "ui\\shell\\bitmaps\\team_icon_slayer";
inline constexpr const char *trouble_brewing = "ui\\shell\\bitmaps\\trouble_brewing";
inline constexpr const char *shell_white_bitmap = "ui\\shell\\bitmaps\\white";
inline constexpr const char *error_modal_fullscreen = "ui\\shell\\error\\error_modal_fullscreen";
inline constexpr const char *error_modal_halfscreen = "ui\\shell\\error\\error_modal_halfscreen";
inline constexpr const char *error_modal_qtrscreen = "ui\\shell\\error\\error_modal_qtrscreen";
inline constexpr const char *error_nonmodal_fullscreen = "ui\\shell\\error\\error_nonmodal_fullscreen";
inline constexpr const char *error_nonmodal_halfscreen = "ui\\shell\\error\\error_nonmodal_halfscreen";
inline constexpr const char *error_nonmodal_qtrscreen = "ui\\shell\\error\\error_nonmodal_qtrscreen";
inline constexpr const char *main_menu_widget = "ui\\shell\\main_menu\\main_menu";
inline constexpr const char *map_list_oneline = "ui\\shell\\main_menu\\map_list_oneline";
inline constexpr const char *map_list_short = "ui\\shell\\main_menu\\map_list_short";
inline constexpr const char *mp_map_list = "ui\\shell\\main_menu\\mp_map_list";
inline constexpr const char *connected_pregame_screen = "ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen";
inline constexpr const char *button_set_long_descriptions = "ui\\shell\\main_menu\\player_profiles_select\\button_set_long_descriptions";
inline constexpr const char *button_set_short_descriptions = "ui\\shell\\main_menu\\player_profiles_select\\button_set_short_descriptions";
inline constexpr const char *joystick_set_defaults_descriptions = "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_defaults_descriptions";
inline constexpr const char *joystick_set_short_descriptions = "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_short_descriptions";
inline constexpr const char *profile_description_labels = "ui\\shell\\main_menu\\player_profiles_select\\profile_description_labels";
inline constexpr const char *var_weapon_set = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\item_options_edit\\var_weapon_set";
inline constexpr const char *var_race_type = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\race_edit\\var_race_type";
inline constexpr const char *var_team_scoring = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\race_edit\\var_team_scoring";
inline constexpr const char *var_friendly_fire = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\teamplay_options_edit\\var_friendly_fire";
inline constexpr const char *var_friendly_fire_penalty = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\teamplay_options_edit\\var_friendly_fire_penalty";
inline constexpr const char *var_vehicles_respawn = "ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicles_respawn";
inline constexpr const char *profile_color_names = "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\color_edit\\colors_list";
inline constexpr const char *controls_device_labels = "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_device_labels";
inline constexpr const char *_1p_pause_game = "ui\\shell\\multiplayer_game\\pause_game\\1p_pause_game";
inline constexpr const char *_2p_pause_game = "ui\\shell\\multiplayer_game\\pause_game\\2p_pause_game";
inline constexpr const char *_4p_pause_game = "ui\\shell\\multiplayer_game\\pause_game\\4p_pause_game";
inline constexpr const char *solo_pause_game_widget = "ui\\shell\\solo_game\\pause_game\\pause_game";
inline constexpr const char *pause_game_split_screen = "ui\\shell\\solo_game\\pause_game\\pause_game_split_screen";
inline constexpr const char *player_help_screen_a10 = "ui\\shell\\solo_game\\player_help\\player_help_screen_a10";
inline constexpr const char *player_help_screen_a30 = "ui\\shell\\solo_game\\player_help\\player_help_screen_a30";
inline constexpr const char *player_help_screen_a50 = "ui\\shell\\solo_game\\player_help\\player_help_screen_a50";
inline constexpr const char *player_help_screen_b30 = "ui\\shell\\solo_game\\player_help\\player_help_screen_b30";
inline constexpr const char *player_help_screen_b40 = "ui\\shell\\solo_game\\player_help\\player_help_screen_b40";
inline constexpr const char *player_help_screen_c10 = "ui\\shell\\solo_game\\player_help\\player_help_screen_c10";
inline constexpr const char *player_help_screen_c20 = "ui\\shell\\solo_game\\player_help\\player_help_screen_c20";
inline constexpr const char *player_help_screen_c40 = "ui\\shell\\solo_game\\player_help\\player_help_screen_c40";
inline constexpr const char *player_help_screen_d20 = "ui\\shell\\solo_game\\player_help\\player_help_screen_d20";
inline constexpr const char *player_help_screen_d40 = "ui\\shell\\solo_game\\player_help\\player_help_screen_d40";
inline constexpr const char *common_button_captions = "ui\\shell\\strings\\common_button_captions";
inline constexpr const char *default_player_profile_names = "ui\\shell\\strings\\default_player_profile_names";
inline constexpr const char *game_variant_descriptions = "ui\\shell\\strings\\game_variant_descriptions";
inline constexpr const char *loading_strings = "ui\\shell\\strings\\loading";
inline constexpr const char *temp_string_list = "ui\\shell\\strings\\temp_strings";
inline constexpr const char *small_ui_font = "ui\\small_ui";

}  // namespace halo::tag_paths
