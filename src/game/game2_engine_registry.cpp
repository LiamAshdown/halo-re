#include "halo/game/game2_engines.hpp"

namespace halo::game {


/**
 * Looks up the engine behaviour for a game_engine_index; the table covers the engines converted so far.
 */
const GameEngineSlots *engine_slots_for(int32_t engine_index)
{
    switch (engine_index) {
    case 2:
        return &slayer_engine;
    case 3:
        return &oddball_engine;
    case 5:
        return &race_engine;
    default:
        return nullptr;
    }
}

}
