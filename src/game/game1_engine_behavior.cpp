/**
 * Strategy objects and registry of the game engines (ctf, king, oddball): each forwards to the static engine class
 * that holds the original function bodies.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

#include "halo/game/game1_ctf.hpp"
#include "halo/game/game1_king.hpp"
#include "halo/game/game1_oddball.hpp"
#include "halo/game/game1_engine_behavior.hpp"
#include "halo/game/api.hpp"

namespace halo::game::engine1 {

namespace {

/**
 * Text and scoring strategy of the capture-the-flag engine.
 */
class CtfBehavior final : public EngineText, public EngineScoring {
public:
    uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text,
                               uint32_t count) const override
    {
        return Ctf::build_message_text(recipient, message_type, subject, text, count);
    }
    wchar_t *build_player_text(datum_index player, wchar_t *buffer) const override
    {
        return Ctf::build_player_text(player, buffer);
    }
    wchar_t *build_score_header_text(wchar_t *buffer) const override
    {
        return Ctf::build_score_header_text(buffer);
    }
    int32_t get_score(datum_index player, int32_t team_mode) const override
    {
        return Ctf::get_score(player, team_mode);
    }
    int32_t get_team_score(int32_t team) const override
    {
        return Ctf::get_team_score(team);
    }
    wchar_t *build_team_score_text(int32_t team, wchar_t *buffer) const override
    {
        return Ctf::build_team_score_text(team, buffer);
    }
    uint8_t query_player_score(int32_t key, int32_t index, void *buffer) const override
    {
        return Ctf::query_player_score(key, index, buffer);
    }
    uint8_t query_team_score(int32_t key, int32_t team, void *buffer) const override
    {
        return Ctf::query_team_score(key, team, buffer);
    }
};

/**
 * Text and scoring strategy of the king-of-the-hill engine.
 */
class KingBehavior final : public EngineText, public EngineScoring {
public:
    uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text,
                               uint32_t count) const override
    {
        return King::build_message_text(recipient, message_type, subject, text, count);
    }
    wchar_t *build_player_text(datum_index player, wchar_t *buffer) const override
    {
        return King::build_player_text(player, buffer);
    }
    wchar_t *build_score_header_text(wchar_t *buffer) const override
    {
        return King::build_score_header_text(buffer);
    }
    int32_t get_score(datum_index player, int32_t team_mode) const override
    {
        return King::get_score(player, team_mode);
    }
    int32_t get_team_score(int32_t team) const override
    {
        return King::get_team_score(team);
    }
    wchar_t *build_team_score_text(int32_t team, wchar_t *buffer) const override
    {
        return King::build_team_score_text(team, buffer);
    }
    uint8_t query_player_score(int32_t key, int32_t index, void *buffer) const override
    {
        return King::query_player_score(key, index, buffer);
    }
    uint8_t query_team_score(int32_t key, int32_t team, void *buffer) const override
    {
        return King::query_team_score(key, team, buffer);
    }
};

/**
 * Text strategy of the oddball engine.
 */
class OddballBehavior final : public EngineText {
public:
    uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text,
                               uint32_t count) const override
    {
        return Oddball::build_message_text(recipient, message_type, subject, text, count);
    }
    wchar_t *build_player_text(datum_index player, wchar_t *buffer) const override
    {
        return Oddball::build_player_text(player, buffer);
    }
    wchar_t *build_score_header_text(wchar_t *buffer) const override
    {
        return Oddball::build_score_header_text(buffer);
    }
};

const CtfBehavior ctf_behavior;
const KingBehavior king_behavior;
const OddballBehavior oddball_behavior;

}

/**
 * Returns the text strategy registered for the engine, or null.
 */
const EngineText *engine_text(EngineId engine)
{
    switch (engine) {
    case EngineId::ctf:
        return &ctf_behavior;
    case EngineId::oddball:
        return &oddball_behavior;
    case EngineId::king:
        return &king_behavior;
    default:
        return nullptr;
    }
}

/**
 * Returns the scoring strategy registered for the engine, or null.
 */
const EngineScoring *engine_scoring(EngineId engine)
{
    switch (engine) {
    case EngineId::ctf:
        return &ctf_behavior;
    case EngineId::king:
        return &king_behavior;
    default:
        return nullptr;
    }
}

}
