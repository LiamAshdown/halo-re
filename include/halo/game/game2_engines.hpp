#pragma once

#include "halo/game/game2_types.hpp"

namespace halo::game {

/**
 * Callback slots of a built-in multiplayer game engine, mirroring the slots of its game_engine_definition row.
 * Slots an engine does not implement fall back to the inert default here, matching the no-op or NULL slot of
 * the original table.
 */
class GameEngineSlots {
public:
    virtual uint8_t initialize_for_new_game() { return {}; }
    virtual void player_new_life(datum_index) {}
    virtual void player_changed_object(datum_index) {}
    virtual void reset_round() {}
    virtual void update(datum_index) {}
    virtual void unknown_48() {}
    virtual int32_t get_score(datum_index, int32_t) { return {}; }
    virtual int32_t get_team_score(int32_t) { return {}; }
    virtual wchar_t * build_player_text(datum_index, wchar_t *) { return {}; }
    virtual wchar_t * build_score_header_text(wchar_t *) { return {}; }
    virtual wchar_t * build_team_score_text(int32_t, wchar_t *) { return {}; }
    virtual void player_killed(datum_index, datum_index, datum_index, uint8_t) {}
    virtual uint8_t build_message_text(datum_index, int32_t, datum_index, wchar_t *, uint32_t) { return {}; }
    virtual uint8_t allow_grenade_counts(datum_index) { return {}; }
    virtual uint8_t waypoint_filter(datum_index, int32_t) { return {}; }
    virtual uint8_t unknown_84(int32_t) { return {}; }
    virtual uint8_t time_scale_override(uint32_t, int32_t) { return {}; }
    virtual uint32_t is_winner(datum_index) { return {}; }
    virtual void profiles_updated(int32_t, int32_t) {}
    virtual void profile_post_update(void **) {}
    virtual void reset_objects() {}
    virtual uint8_t query_player_score(int32_t, int32_t, void *) { return {}; }
    virtual uint8_t query_team_score(int32_t, int32_t, void *) { return {}; }
};

/**
 * The slayer game engine: the callbacks its game_engine_definition row exposes, as virtual slots.
 */
class SlayerEngine final : public GameEngineSlots {
public:
    constexpr SlayerEngine() = default;
    uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count) override;
    wchar_t * build_player_text(datum_index player, wchar_t *buffer) override;
    wchar_t * build_team_score_text(int32_t team, wchar_t *buffer) override;
    int32_t get_score(datum_index player, int32_t team_mode) override;
    int32_t get_team_score(int32_t team) override;
    uint8_t initialize_for_new_game(void) override;
    void player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide) override;
    void player_new_life(datum_index player_index) override;
    void player_round_reset(datum_index player_index);
    void profile_post_update(void **context) override;
    void profiles_updated(int32_t mode, int32_t machine_index) override;
    uint8_t query_player_score(int32_t key, int32_t index, void *buffer) override;
    uint8_t query_team_score(int32_t key, int32_t team, void *buffer) override;
    void reset_objects(void) override;
    void reset_round(void) override;
    uint8_t unknown_84(int32_t kind) override;
    void update(datum_index player_index) override;

private:
    static const uint16_t * game_text(int16_t index);
    static ::player * player_if_valid(datum_index handle);
    static void add_score(datum_index player_index, int32_t delta);
    static void skip_unchanged_message(message_delta_decode_state *state);
    static uint8_t read_changed(void **context, void *changed_base, void *destination);
};

/**
 * The oddball game engine: the callbacks its game_engine_definition row exposes, as virtual slots.
 */
class OddballEngine final : public GameEngineSlots {
public:
    constexpr OddballEngine() = default;
    wchar_t * build_team_score_text(int32_t team, wchar_t *buffer) override;
    int32_t get_score(datum_index player, int32_t team_mode) override;
    int32_t get_team_score(int32_t team) override;
    uint8_t initialize_for_new_game(void) override;
    void player_killed(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide) override;
    void player_new_life(datum_index player_index) override;
    void player_round_reset(datum_index player_index);
    void profile_post_update(void **context) override;
    uint8_t query_player_score(int32_t key, int32_t index, void *buffer) override;
    uint8_t query_team_score(int32_t key, int32_t team, void *buffer) override;
    void reset_objects(void) override;
    uint8_t time_scale_override(uint32_t player, int32_t value) override;
    void unknown_48(void) override;
    uint8_t unknown_84(int32_t kind) override;

private:
    static uint8_t oddball_is_carrier(datum_index player_index);
    static uint8_t oddball_any_ball_free(void);
    static void skip_unchanged_message(message_delta_decode_state *state);
    static uint8_t read_changed(void **context, void *changed_base, void *destination);
};

/**
 * The race game engine: the callbacks its game_engine_definition row exposes, as virtual slots.
 */
class RaceEngine final : public GameEngineSlots {
public:
    constexpr RaceEngine() = default;
    uint8_t allow_grenade_counts(datum_index player_index) override;
    uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count) override;
    wchar_t * build_player_text(datum_index player, wchar_t *buffer) override;
    wchar_t * build_score_header_text(wchar_t *buffer) override;
    wchar_t * build_team_score_text(int32_t team, wchar_t *buffer) override;
    int32_t get_score(datum_index player, int32_t team_mode) override;
    int32_t get_team_score(int32_t team) override;
    uint32_t is_winner(datum_index player) override;
    void player_changed_object(datum_index player_index) override;
    void player_new_life(datum_index player) override;
    void player_round_reset(datum_index player_index, uint8_t team_flag);
    void profile_post_update(void **context) override;
    uint8_t query_player_score(int32_t key, int32_t index, void *buffer) override;
    uint8_t query_team_score(int32_t key, int32_t team, void *buffer) override;
    void unknown_48(void) override;
    void update(datum_index player_index) override;
    uint8_t waypoint_filter(datum_index player, int32_t team) override;

private:
    static datum_index race_pick_vehicle_tag(int32_t index);
    static void race_spawn_next_vehicle(datum_index player_index);
    static const uint16_t * game_text(int16_t index);
    static const uint16_t * place_text(datum_index recipient);
    static uint16_t * multiplayer_text(int16_t index);
    static void skip_unchanged_message(message_delta_decode_state *state);
    static uint8_t read_changed(void **context, void *changed_base, void *destination);
};

inline constinit SlayerEngine slayer_engine;
inline constinit OddballEngine oddball_engine;
inline constinit RaceEngine race_engine;

/**
 * Returns the engine behaviour object for a game_engine_index value, or null for engines that are not
 * converted (ctf, king, the stub).
 */
const GameEngineSlots *engine_slots_for(int32_t engine_index);

}
