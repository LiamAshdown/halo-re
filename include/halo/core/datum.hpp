#pragma once

#include <cstdint>

namespace halo {

/** Mask that extracts the array slot from a datum handle (the identifier lives in the high half). */
inline constexpr uint32_t k_datum_slot_mask = 0xffff;

/** The 16-bit "none" sentinel used by index/tag fields of records. */
inline constexpr uint16_t k_word_none = 0xffff;

/** The 32-bit "none" sentinel used by datum handles. */
inline constexpr uint32_t k_dword_none = 0xffffffffu;

/** Array slot of a datum handle. */
constexpr uint32_t datum_slot(uint32_t handle) noexcept { return handle & k_datum_slot_mask; }

}  // namespace halo
