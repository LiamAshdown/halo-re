#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/** Index of a built-in game engine (the row of the engine definition table). */
enum class EngineId : int32_t {
    ctf = 1,
    slayer = 2,
    oddball = 3,
    king = 4,
    race = 5,
};

/**
 * Strategy for the text a game engine shows: event messages, the per-player score text and the scoreboard header.
 * One stateless implementation per engine lives in static storage; the C entry points of the engine definition rows
 * dispatch through it.
 */
class EngineText {
public:
    virtual uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text,
                                       uint32_t count) const = 0;
    virtual wchar_t *build_player_text(datum_index player, wchar_t *buffer) const = 0;
    virtual wchar_t *build_score_header_text(wchar_t *buffer) const = 0;

protected:
    ~EngineText() = default;
};

/**
 * Strategy for the scores a game engine reports: the player and team score values, the team score text and the
 * scoreboard queries keyed by player or team.
 */
class EngineScoring {
public:
    virtual int32_t get_score(datum_index player, int32_t team_mode) const = 0;
    virtual int32_t get_team_score(int32_t team) const = 0;
    virtual wchar_t *build_team_score_text(int32_t team, wchar_t *buffer) const = 0;
    virtual uint8_t query_player_score(int32_t key, int32_t index, void *buffer) const = 0;
    virtual uint8_t query_team_score(int32_t key, int32_t team, void *buffer) const = 0;

protected:
    ~EngineScoring() = default;
};

/**
 * Registry of the engine behaviours implemented in this module: returns the text strategy of an engine, or null when
 * the engine has none here.
 */
const EngineText *engine_text(EngineId engine);

/**
 * Registry lookup of the scoring strategy of an engine, or null when the engine has none here.
 */
const EngineScoring *engine_scoring(EngineId engine);

}
