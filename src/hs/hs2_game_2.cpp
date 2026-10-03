#include "halo/hs/hs2_commands.hpp"
#include "halo/hs/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"


static auto &game_time = halo::link::ref<uint8_t *>(halo::ai::vars().game_time);

namespace halo::hs {

/**
 * Evaluate handler of hs function "game_time"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f080
 */
void GameCommands::evaluate_game_time(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    
    halo::hs::hs_thread_return(*(int32_t *)(game_time + 0xc), thread_index);
}

}
