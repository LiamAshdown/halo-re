#pragma once

#include <cstdint>

namespace halo {

/** Number of bits in one word of an engine bit array. */
inline constexpr uint32_t k_bit_array_word_bits = 32;

/** Mask that extracts the bit position inside a word of a bit array. */
inline constexpr uint32_t k_bit_array_bit_mask = k_bit_array_word_bits - 1;

/** Shift that converts a bit index into a word index. */
inline constexpr uint32_t k_bit_array_word_shift = 5;

/** Index of the word that holds `bit` in a 32-bit-word bit array. */
constexpr int32_t bit_array_word(int32_t bit) noexcept { return bit >> k_bit_array_word_shift; }

/** Mask of `bit` inside its word. */
constexpr uint32_t bit_array_mask(int32_t bit) noexcept { return 1u << (static_cast<uint32_t>(bit) & k_bit_array_bit_mask); }

/** Number of 32-bit words needed for `bit_count` bits. */
constexpr int32_t bit_array_word_count(int32_t bit_count) noexcept {
    return (bit_count + static_cast<int32_t>(k_bit_array_bit_mask)) >> k_bit_array_word_shift;
}

}  // namespace halo
