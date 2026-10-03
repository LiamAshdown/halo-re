#include "halo/interface/ifr1_error_dialogs.hpp"

/**
 * C ABI entry point; forwards to halo::interface::ErrorDialogs::show.
 *
 * @address 0x498f20
 */
extern "C" void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error)
{
    halo::interface::ErrorDialogs::show(error_string_index, player_index, modal, is_error);
}
