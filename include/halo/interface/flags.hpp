/**
 * @file include/halo/interface/flags.hpp
 * Bit tests of the flag words the interface module reads from tag definitions and runtime records, written against the named
 * flag enums of halo/tags/flags.hpp and halo/units/flags.hpp.
 */
#pragma once

#include <stdint.h>
#include <type_traits>

#include "halo/core/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"

namespace halo::interface {

/**
 * True when every bit of `bit` is set in `value`, a raw flag word (an integer or a C enum of the same width as E) stored in a
 * tag definition or runtime record.
 */
template <halo::bit_flags E, typename V>
constexpr bool has_bit(V value, E bit) noexcept {
    using U = std::underlying_type_t<E>;
    return (static_cast<U>(value) & halo::to_bits(bit)) == halo::to_bits(bit);
}

/** saved_player_profile::flags (and the same low bits of game_variant::variant_flags). */
enum class profile_flag : uint16_t {
    none = 0,
    builtin = 0x1,
    device_generated = 0x2,
    end_credits_reached = 0x4,
};

/** The flag word of a ui_widget_definition tag. */
using widget_flag = halo::tags::ui_widget_definition_tag_flag;

}  // namespace halo::interface

namespace halo {
template <> struct enable_bit_flags<interface::profile_flag> : std::true_type {};
}  // namespace halo
