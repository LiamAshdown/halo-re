/**
 * @file include/halo/ai/flags.hpp
 * Bit-flag enums for the flag words of the ai records (types/ai.h) and the helper that tests a tag flag word against one of the tag flag enums.
 */
#pragma once

#include <stdint.h>
#include "halo/core/flags.hpp"

namespace halo::ai {

using halo::operator|;
using halo::operator&;
using halo::operator^;
using halo::operator~;
using halo::operator|=;
using halo::operator&=;
using halo::operator^=;
using halo::has;
using halo::any;

/** swarm_component.flags. */
enum class swarm_component_flag : uint16_t {
    none = 0,
    attacking = 0x0001,   // the form attacks: unit melee state 3 while free, 4 while it rides a living parent
    attached = 0x0002,    // the form rides its parent unit
    initialized = 0x0004, // the wander state was seeded
    active = 0x0008,      // the swarm runs a command list and this component takes part in it
    leaping = 0x0010,     // the form leaps now
};

/** True when any bit of `bit` is set in the tag flag word `value` (the engine tests tag bitfields as plain masks). */
template <typename E>
constexpr bool flag_set(uint32_t value, E bit) noexcept
{
    return (value & static_cast<uint32_t>(bit)) != 0;
}

}  // namespace halo::ai

namespace halo {
template <> struct enable_bit_flags<ai::swarm_component_flag> : std::true_type {};
}  // namespace halo
