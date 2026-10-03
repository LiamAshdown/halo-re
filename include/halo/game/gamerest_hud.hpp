#pragma once

#include <stdint.h>
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "networking.h"

namespace halo::game {

/**
 * Teammate nameplate selection, fading and drawing.
 */
class HudNameplates {
public:
    HudNameplates() = delete;

    static void draw_teammate_nameplate(datum_index player_handle);
    static void draw_teammate_nameplate_text(wchar_t *text, int32_t value);
    static datum_index find_nearby_teammate_for_nameplate(datum_index player_handle);
    static uint8_t nameplate_candidate_filter(uint32_t object_index, void *context);
    static void update_teammate_nameplate_fade();
};

/**
 * HUD text drawing helpers.
 */
class HudText {
public:
    HudText() = delete;

    static void scoreboard_row_text(int16_t row, wchar_t *text, int16_t column);
    static int32_t world_relative_text(hud_world_text_params *params, int16_t row, wchar_t *text, uint8_t highlighted);
};

/**
 * Scoreboard ordering and player selection.
 */
class Scoreboard {
public:
    Scoreboard() = delete;

    static int32_t select_players_to_display(int32_t mode, int32_t max_count, scoreboard_entry *out);
    static int32_t compare(const scoreboard_entry *a, const scoreboard_entry *b);
    static uint32_t compare_by_unknown_04(const scoreboard_entry *a, const scoreboard_entry *b);
};

/**
 * Script-registered custom waypoints.
 */
class CustomWaypoints {
public:
    CustomWaypoints() = delete;

    static void get_position(real_point3d *out, int16_t slot);
    static uint8_t matches_filter(int32_t candidate, player *reference_player, int32_t slot_index);
    static void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name, float height_offset, datum_index player_filter, int16_t team_filter);
};

/**
 * Chimera extension hooks.
 */
class ChimeraHooks {
public:
    ChimeraHooks() = delete;

    static void kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type, datum_index subject, char broadcast);
};

/**
 * Generates the multiplayer game-variant description text.
 */
class VariantDescription {
public:
    VariantDescription() = delete;

    static void generate(char *variant_name, ticker_text_buffer *ticker, int32_t fraglimit, wchar_t *game_flags_wide, wchar_t *player_flags_wide);
};

}  // namespace halo::game
