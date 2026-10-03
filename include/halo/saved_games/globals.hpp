#pragma once

#include <stdint.h>

/**
 * Link names of the engine globals the saved games module keeps in anonymous data. The definitions live in the
 * standalone data layer under these original names; the module reaches them through the named references of
 * halo::saved_games::globals below.
 */
extern "C" {
extern uint8_t unknown_0072132a;
}

namespace halo::saved_games::fields {

/**
 * Set to 1 at the end of the saved game file initialization once the default profile files exist. Nothing in
 * the image reads it.
 *
 * @address 0x72132a
 */
inline uint8_t &saved_game_files_initialized = unknown_0072132a;

}
