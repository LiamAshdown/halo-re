#include "halo/interface/ifr1_checkpoint_list_rows.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::CheckpointListRows::add_row.
 *
 * @address 0x4a4280
 */
uint8_t checkpoint_list_add_row(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time, const void *time, void *user_data)
{
    return halo::interface::CheckpointListRows::add_row(index, name, level_index, difficulty, game_time, time, user_data);
}

}
