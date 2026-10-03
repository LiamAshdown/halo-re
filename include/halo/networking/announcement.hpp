#pragma once

#include <cstdint>

#include "halo/core/flags.hpp"

namespace halo::networking {

/** Bits of `network_game_announcement::flags`. */
enum class announcement_flags : uint8_t {
    none = 0,
    joinable = 1u << 1,
    stats_logging = 1u << 2,
    oddball_marker = 1u << 3,
};

}  // namespace halo::networking

namespace halo {
template <> struct enable_bit_flags<networking::announcement_flags> : std::true_type {};
}  // namespace halo
