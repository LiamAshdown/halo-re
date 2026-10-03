#pragma once

#include <cstdint>
#include "halo/core/bit_cast.hpp"

namespace halo {

/** Mask that extracts the array slot from a datum handle (the identifier lives in the high half). */
inline constexpr uint32_t k_datum_slot_mask = 0xffff;

/** The 16-bit "none" sentinel used by index/tag fields of records. */
inline constexpr uint16_t k_word_none = 0xffff;

/** The 32-bit "none" sentinel used by datum handles. */
inline constexpr uint32_t k_dword_none = 0xffffffffu;

/** Mask that strips the "no leaf" sign bit from a BSP leaf index. */
inline constexpr uint32_t k_leaf_index_mask = 0x7fffffff;

/** Array slot of a datum handle. */
constexpr uint32_t datum_slot(uint32_t handle) noexcept { return handle & k_datum_slot_mask; }

/** The 32-bit handle packed in a tag id (index in the low half, identifier in the high half) as an integer. */
template <typename R = uint32_t, typename T>
constexpr R tag_id_bits(const T &tag_id) noexcept
{
    static_assert(sizeof(T) == sizeof(uint32_t), "a tag id is one dword");
    return static_cast<R>(halo::bit_cast<uint32_t>(tag_id));
}

}  // namespace halo
