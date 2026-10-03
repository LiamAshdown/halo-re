#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Capture-the-flag game engine: flag objects, scoring, round resets and score text. Stateless service class:
 * every function is a static member and the state it acts on lives in the engine globals.
 */
class Ctf {
public:
    static void assign_flag_ids(void);
    static void broadcast_state(void *request_fields, int32_t machine_index);
    static uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count);
    static wchar_t *build_player_text(datum_index player, wchar_t *buffer);
    static wchar_t *build_score_header_text(wchar_t *buffer);
    static wchar_t *build_team_score_text(int32_t team, wchar_t *buffer);
    static datum_index create_flag_object(real_point3d *position, uint16_t name_index);
    static int32_t get_score(datum_index player, int32_t team_mode);
    static int32_t get_team_score(int32_t team);
    static int32_t initialize_flags(void);
    static uint8_t initialize_for_new_game(void);
    static uint8_t is_flag_eligible_for_capture(uint32_t team, int32_t flag_id);
    static void notify_both_teams(int32_t team);
    static void notify_flag_carried_throttled(int32_t target_player);
    static void object_expired(datum_index object_index);
    static void on_flag_captured(uint32_t flag_index);
    static int32_t pick_random_flag(int32_t exclude_flag_index);
    static void player_drop_flag(uint32_t player_index, datum_index flag_object_index);
    static uint8_t player_flag_tick(uint32_t flag_handle, uint32_t player_index);
    static void player_round_reset(datum_index player_index);
    static void player_touch_flag(uint32_t player_index, int32_t team);
    static uint8_t point_within_team_flag_radius(float radius, int32_t team, real_point3d *point);
    static void profile_post_update(void **context);
    static void profiles_updated(int32_t mode, int32_t machine_index);
    static uint8_t query_player_score(int32_t key, int32_t index, void *buffer);
    static uint8_t query_team_score(int32_t key, int32_t team, void *buffer);
    static void reset_objects(void);
    static void reset_round(void);
    static void reset_team_return_credit(uint32_t object_index);
    static void respawn_team_flag(int32_t team, real_point3d *forwarded_position, uint16_t forwarded_name_index);
    static void return_all_flags(void);
    static void score_flag(uint32_t team, int32_t scenario_flag_index);
    static uint8_t unit_is_flag_holder(player *p);
    static uint8_t unit_weapon_must_be_readied(datum_index unit_handle);
    static void unknown_48(void);
    static uint8_t unknown_60(datum_index unit_index, datum_index item_index);
    static float unknown_70(datum_index player_index, real_point3d *position);
    static uint8_t unknown_84(int32_t kind);
    static void update(datum_index player_index);

private:
    static const uint16_t *game_text(int16_t index);
    static const uint16_t *place_text(datum_index recipient);
    static uint16_t *multiplayer_text(int16_t index);
    static float distance_squared(const real_point3d *a, const real_point3d *b);
    static void skip_unchanged_message(message_delta_decode_state *state);
    static uint8_t read_changed(void **context, void *changed_base, void *destination);
};

}
