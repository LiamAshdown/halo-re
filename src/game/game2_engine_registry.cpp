#include "halo/game/game2_engines.hpp"

namespace halo::game {

constinit SlayerEngine slayer_engine;
constinit OddballEngine oddball_engine;
constinit RaceEngine race_engine;

/**
 * Looks up the engine behaviour for a game_engine_index; the table covers the engines converted so far.
 */
const GameEngineSlots *engine_slots_for(int32_t engine_index)
{
    switch (engine_index) {
    case _game_engine_slayer:
        return &slayer_engine;
    case _game_engine_oddball:
        return &oddball_engine;
    case _game_engine_race:
        return &race_engine;
    default:
        return nullptr;
    }
}

}
