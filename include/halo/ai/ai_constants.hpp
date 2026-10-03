#pragma once

#include <cstdint>

namespace halo::ai {

/** The 'iter' tag the data_iterator signature is XORed with: signature = (uintptr_t)data_array ^ k_iterator_signature_key. */
inline constexpr uint32_t k_iterator_signature_key = 0x69746572;

}  // namespace halo::ai
