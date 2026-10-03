#pragma once

#include "halo/game/game2_types.hpp"

namespace halo::game {

/**
 * Multiplayer HUD output: score rasterizers, messages, hints, sound queues and waypoints.
 */
class EngineHud {
public:
    static void rasterize_in_game_score(datum_index subject_player, float opacity);
    static void rasterize_message(void);
    static void post_rasterize_post_game(void);
    static uint8_t pick_hud_hint(datum_index player_index, int32_t maximum_length, uint16_t *out_text);
    static void play_multiplayer_sound(int32_t sound_index, datum_index recipient_player, uint8_t broadcast);
    static void queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
    static void queue_status_sound_message(int32_t sound_index, datum_index recipient_player);
    static void update_custom_waypoint_navpoints(int16_t local_player_slot);

private:
    static wchar_t * multiplayer_game_text_string(int16_t index);
    static int32_t scoreboard_text_font(void);
    static void scoreboard_draw_white_line(Rectangle2D *rect, wchar_t *text, float opacity);
    static void post_game_set_text_color(const uint32_t *color);
    static void post_game_set_tab_stops(uint32_t stops_a, uint32_t stops_b, uint32_t stops_c);
};

}
