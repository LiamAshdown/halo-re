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

}  // namespace halo::tag_paths
